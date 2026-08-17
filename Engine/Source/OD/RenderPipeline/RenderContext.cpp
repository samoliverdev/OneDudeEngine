#include "OD/pch.h"
#include "RenderContext.h"
#include "RendererFeature.h"
#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "ModelRendererComponent.h"
#include "SpriteRendererComponent.h"
#include "StaticRendererClusterComponent.h"
#include "DecalRendererComponent.h"
#include "BendSssCpu.h"
#include "OD/Animation/Animator.h"
#include "OD/Core/Application.h"
#include "OD/Core/Resource.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Defines.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/Navmesh/Navmesh.h"
#include "OD/Graphics/Geometry.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/UniformBuffer.h"
#include "OD/Graphics/InstancingBuffer.h"
#include "OD/Graphics/ComputeShader.h"
#include "OD/Graphics/Gizmos.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Editor/Editor.h"
#include <taskflow/taskflow.hpp> 
#include <glm/simd/matrix.h>

namespace OD{

RenderContextSettings settings;

std::vector<std::function<void(RendererFeatureContext&)>> addRenderFeatures;

std::vector<std::function<void(RendererFeatureContext&)>>& RendererFeatureContext::_AddRenderFeatures(){
    return addRenderFeatures;
}

RenderContextSettings& RenderContext::GetSettings(){
    return settings;
}

RendererFeatureContext::~RendererFeatureContext(){
    for(auto& i: localRenderFeatures) delete i;
    localRenderFeatures.clear();
}

struct alignas(16) DispatchParametersGPU{
    float LightCoordinate[4];

    int WaveOffset[2];
    float near;
    float far;
    //int padding0[2];

    float SurfaceThickness;
    float BilinearThreshold;
    float ShadowContrast;
    float padding1;

    float FarDepthValue;
    float NearDepthValue;
    float InvDepthTextureSize[2];
};

struct alignas(16) SSSParameters2{
    float LightCoordinate[4];

    int WaveOffset[2];
    float DepthBounds[2];

    float InvDepthTextureSize[2];
    float _pad0[2];

    float SurfaceThickness;
    float BilinearThreshold;
    float ShadowContrast;
    float FarDepthValue;

    float NearDepthValue;  
    float _pad1;  
    float _pad2;  
    float _pad3; 
};

struct alignas(16) SSSParameters3 
{
    // Base alignment: 4 bytes each. 4 x 4 = 16 bytes. (Perfect vec4 alignment)
    float SurfaceThickness;
    float BilinearThreshold;
    float ShadowContrast;
    uint32_t IgnoreEdgePixels = 1;

    // Base alignment: 4 bytes each. 4 x 4 = 16 bytes. (Perfect vec4 alignment)
    uint32_t UsePrecisionOffset = 0;
    uint32_t BilinearSamplingOffsetMode = 0;
    uint32_t DebugOutputEdgeMask = 0;
    uint32_t DebugOutputThreadIndex = 0;

    // Base alignment: 4 bytes each. 2 x 4 = 8 bytes.
    uint32_t DebugOutputWaveIndex;
    uint32_t UseEarlyOut;
    // vec2 requires 8-byte alignment. Since we are at offset 8, it fits perfectly.
    glm::vec2 DepthBounds; // Total: 8 + 8 = 16 bytes.

    // vec4 requires 16-byte alignment.
    glm::vec4 LightCoordinate; // 16 bytes.

    // ivec2 requires 8-byte alignment.
    glm::ivec2 WaveOffset; // 8 bytes.
    // float requires 4-byte alignment. 
    float FarDepthValue;   // 4 bytes.
    float NearDepthValue;  // 4 bytes. Total for this chunk: 8 + 4 + 4 = 16 bytes.

    // vec2 requires 8-byte alignment. 
    glm::vec2 InvDepthTextureSize; // 8 bytes.
    
    // PADDING: std140 structures must be padded to a multiple of 16 bytes (size of a vec4).
    // Current total size is 88 bytes. Next multiple of 16 is 96. We need 8 bytes of padding.
    float padding[2]; 
};

inline void SetPerInstanceData(Matrix4& matrix, Vector4& data){
    matrix[0][3] = data.x;
    matrix[1][3] = data.y;
    matrix[2][3] = data.z;
    matrix[3][3] = data.w;
}

RendererFeatureContext::RendererFeatureContext(){
    for(auto& i: addRenderFeatures){
        i(*this);
    }
}

RenderContext::RenderContext(){
    FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};

    framebufferSpecification.colorAttachments = {
        {FramebufferTextureFormat::RED_INTEGER}
    };
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT24};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 1;
    framebufferSpecification.createDepth = false;
    entityIdOutColor = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification);
    entityIdOutColor->name = "entityIdOutColor";

    framebufferSpecification.colorAttachments = {
        {FramebufferTextureFormat::RGBA16F} //{FramebufferTextureFormat::RGB11B10F}, 
        //{FramebufferTextureFormat::RGBA8}//, 
        //{FramebufferTextureFormat::RED_INTEGER}
    };
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT24};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 1;
    framebufferSpecification.createDepth = true;
    forwardOutColor = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification);
    forwardOutColor->name = "forwardOutColor";
    //forwardOutColor = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());

    framebufferSpecification.colorAttachments = {
        //{FramebufferTextureFormat::RGB16F}, // Pos
        {FramebufferTextureFormat::RGB16F}, // Normal(R,G) Other(B)
        {FramebufferTextureFormat::RGBA16F}, // Albedo, Other(A)
        //{FramebufferTextureFormat::RGB}, // Emission
        {FramebufferTextureFormat::RGBA16F},//, // Spec, Metalic, AO, Other //INFO: if is RGBA8 Other used current for layer, bug becose will be clamped to  0-1
        {FramebufferTextureFormat::RGB11B10F} //Emission
        //{FramebufferTextureFormat::RED_INTEGER} // Object ID
    };
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT24};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 1;
    framebufferSpecification.createDepth = true;
    deferredOutColor = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification);
    deferredOutColor->name = "deferredOutColor";
    //deferredOutColor->ColorAttachmentId(3);
    //deferredOutColor = new Framebuffer(FramebufferType::Deffered, Application::ScreenWidth(), Application::ScreenHeight());

    framebufferSpecification.createDepth = false;
    deferredOutColorCopy = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification);
    deferredOutColorCopy->name = "deferredOutColorCopy";

    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.colorAttachments = {
        {FramebufferTextureFormat::RGB11B10F} //{FramebufferTextureFormat::RGB11B10F}
        //{FramebufferTextureFormat::RGBA8}
    };
    framebufferSpecification.createDepth = false;
    framebufferSpecification.sample = 1;
    finalColor = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification);
    finalColor->name = "finalColor";
    postFx1 = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification);
    postFx1->name = "postFx1";
    postFx2 = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification);
    postFx2->name = "postFx2";
    //finalColor = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());
    //postFx1 = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());
    //postFx2 = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());

    entityIdShader = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/EntityId.glsl"));

    blitShader = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Blit.glsl"));
    //deferredGBufferShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredGBuffer.glsl"));
    //deferredLightPassShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredLightPassLit.glsl"));
    deferredLightPass = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/DeferredLightPassLit.glsl"));

    deferredLightDirSinglePass = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/DeferredLightPassSingleLit.glsl"));
    deferredLightDirSingleOtherPass = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/DeferredLightPassSingleOtherLit.glsl"));

    skyboxMesh = Mesh::SkyboxCube();
    spriteMesh = Mesh::CenterQuad(false);
    fullScreenQuad = Mesh::FullScreenQuad();

    sphereMesh = Resource::CreateFromFile<Model>("Engine/Models/Sphere.obj", ModelLoadSettings{nullptr, 1, false}); // Model::CreateFromFile("Engine/Models/Sphere.obj", {nullptr, 1, false});
    coneMesh = Resource::CreateFromFile<Model>("Engine/Models/Cone.obj", ModelLoadSettings{nullptr, 1, false}); // Model::CreateFromFile("Engine/Models/Cone.obj", {nullptr, 1, false});
    decalMesh = Resource::CreateFromFile<Model>("Engine/Models/Cube.obj", ModelLoadSettings{nullptr, 1, false}); // Model::CreateFromFile("Engine/Models/Cube.obj", {nullptr, 1, false});
    
    pipelineDataBuffer = UniformBuffer::Create(sizeof(PipelineData));
    shadowDataBuffer = UniformBuffer::Create(sizeof(ShadowData));

    //meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    //meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>();

    renderData = ChunkedVector<RenderData>(std::thread::hardware_concurrency());// 4);

    //for(auto& i: RendererFeatureGlobal::Get().GetNewRendererFeatureFuncs()){
    //    rendererFeatures.push_back(i());
    //}

    //screenSpaceShadow = AssetManager::Get().LoadAsset<ComputeShader>("Engine/ComputeShader/BendSssGpu.compute");
    screenSpaceShadow = ResourceManager::Get().LoadByPath<ComputeShader>("Engine/ComputeShader/BendSssGpu2.compute");
    screenSpaceShadowData = CreateRef<UniformBuffer>(sizeof(SSSParameters2));

    FrameBufferSpecification framebufferSpecification2 = {Application::ScreenWidth(), Application::ScreenHeight()};
    framebufferSpecification2.colorAttachments = { {FramebufferTextureFormat::RGBA32F} }; //TODO: Optimaze this size
    framebufferSpecification2.createDepth = false;
    framebufferSpecification2.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification2.sample = 1;
    screenSpaceShadowOutput = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification2);
    screenSpaceShadowOutput->name = "screenSpaceShadowOutput";

    screenSpaceShadow2 = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/ScreenSpaceShadow2.glsl"));

    FrameBufferSpecification specification = {};
    specification.width = 1024 * 1;
    specification.height = 1024 * 1;
    specification.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT16};
    specification.createDepth = true;

    specification.sample = MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT * MAX_CASCADE_COUNT;
    directionalShadowAtlas = ResourceManager::Get().Create<Framebuffer>(specification);
    directionalShadowAtlas->name = "directionalShadowAtlas";

    specification.sample = MAX_SHADOWED_OTHER_LIGHT_COUNT;
    otherShadowAtlas = ResourceManager::Get().Create<Framebuffer>(specification);
    otherShadowAtlas->name = "otherShadowAtlas";
}

RenderContext::~RenderContext(){
    

    //for(auto& i: rendererFeatures) delete i;
    //rendererFeatures.clear();
}

void RenderContext::CopyDeffered(){
    deferredOutColorCopy->Resize(deferredOutColor->Width(), deferredOutColor->Height());
    Graphics::BlitFramebuffer(deferredOutColor.get(), deferredOutColorCopy.get(), 0);
    Graphics::BlitFramebuffer(deferredOutColor.get(), deferredOutColorCopy.get(), 1);
    Graphics::BlitFramebuffer(deferredOutColor.get(), deferredOutColorCopy.get(), 2);
}

void RenderContext::Begin(){

}

void RenderContext::End(){
    Material::CleanGlobalUniformsData();
}

void RenderContext::BeginDrawToScreen(){
    int width = Application::ScreenWidth();
    int height = Application::ScreenHeight();

    if(overrideFramebuffer != nullptr){
        width = overrideFramebuffer->Width();
        height = overrideFramebuffer->Height();
    }

    Ref<Framebuffer> curFinalColor = customFinalColor != nullptr ? customFinalColor : finalColor;

    /*if(finalColor->Width() != width){
        LogError("Current: %d Next: %d", finalColor->Width(), width);
    }*/

    if(width <= 0 || height <= 0) return;

    entityIdOutColor->Resize(width, height);
    deferredOutColor->Resize(width, height);
    forwardOutColor->Resize(width, height);
    curFinalColor->Resize(width, height);
    postFx1->Resize(width, height);
    postFx2->Resize(width, height);
}

void RenderContext::BeginDrawToScreenNew(){
    int width = cam.width;
    int height = cam.height;
    if(width <= 0 || height <= 0) return;

    Ref<Framebuffer> curFinalColor = customFinalColor != nullptr ? customFinalColor : finalColor;

    entityIdOutColor->Resize(width, height);
    deferredOutColor->Resize(width, height);
    forwardOutColor->Resize(width, height);
    curFinalColor->Resize(width, height);
    postFx1->Resize(width, height);
    postFx2->Resize(width, height);
}

void RenderContext::BeginDrawEntityIds(){
    Graphics::BeginFramebuffer(*entityIdOutColor);
    ScreenClean();
}

void RenderContext::EndDrawEntityIds(){
    Graphics::EndFramebuffer();
}

void RenderContext::DrawEntityIds(RendererList& commandBuffer){
    OD_PROFILE_SCOPE("RenderContext::DrawRenderersBuffer");

    //if(sort) 
    commandBuffer.Sort();
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        //if(material.GetShader() == nullptr) return;
        //SetStandUniforms(cam, *material.GetShader()); 

        //Graphics::SetDepthTest(DepthTest::EQUAL);

        /*if(deferred){
            material.EnableKeyword("Deferred");
        } else {
            material.EnableKeyword("Forward");
        }*/
    };
    commandBuffer.overrideMaterial = entityIdShader;
    commandBuffer.Submit(false);
    //commandBuffer.onUpdateMaterial = nullptr;
}

int RenderContext::ReadPixeIntFromEntityIdsFramebuffer(int x, int y){
    return entityIdOutColor->ReadPixel(0, x, y);
}

void RenderContext::BeginForwardPass(bool clean){
    Graphics::BeginFramebuffer(*forwardOutColor, clean, cam.cleanColor);
    //ScreenClean();
}

void RenderContext::EndForwardPass(){
    Graphics::EndFramebuffer();
}

void RenderContext::BeginDeferredPass(bool clean){
    //Assert(false);
    //Framebuffer::Bind(*deferredOutColor);
    Graphics::BeginFramebuffer(*deferredOutColor, clean, cam.cleanColor);
    //ScreenClean();
}

void RenderContext::EndDeferredPass(){
    Graphics::EndFramebuffer();
}

void RenderContext::DeferredCopyToForwardPass(){
    Graphics::BlitFramebuffer(deferredOutColor.get(), forwardOutColor.get(), -1);
}

void RenderContext::DrawDeferredLight(int index, bool combinedIndirect){

    if(index < 0){
        /*deferredLightPass->SetTexture("gPosition", deferredOutColor, 0);
        deferredLightPass->SetTexture("gNormal", deferredOutColor, 1);
        deferredLightPass->SetTexture("gAlbedoSpec", deferredOutColor, 2);
        deferredLightPass->SetTexture("gEmission", deferredOutColor, 3);
        deferredLightPass->SetTexture("gOther", deferredOutColor, 4);
        Graphics::BindMaterial(*deferredLightPass);
        Graphics::DrawMesh(*fullScreenQuad, *deferredLightPass, Matrix4Identity);*/

        deferredLightDirSinglePass->EnableKeyword("INDIRECT");
        //deferredLightDirSinglePass->SetTexture("gPosition", deferredOutColor, 0);
        deferredLightDirSinglePass->SetTexture("gNormal", deferredOutColor, 0);
        deferredLightDirSinglePass->SetTexture("gAlbedoSpec", deferredOutColor, 1);
        deferredLightDirSinglePass->SetTexture("gOther", deferredOutColor, 2);
        deferredLightDirSinglePass->SetTexture("gEmission", deferredOutColor, 3);
        deferredLightDirSinglePass->SetTexture("gDepth", deferredOutColor, -1);
        deferredLightDirSinglePass->SetTexture("sss", screenSpaceShadowOutput, 0);
        Graphics::DrawMesh(*fullScreenQuad, *deferredLightDirSinglePass, Matrix4Identity);
    } else {
        if(combinedIndirect){
            deferredLightDirSinglePass->EnableKeyword("INDIRECTPLUSDIRECTIONAL");
        } else {
            deferredLightDirSinglePass->EnableKeyword("DIRECTIONAL");
        }

        //deferredLightDirSinglePass->SetTexture("gPosition", deferredOutColor, 0);
        deferredLightDirSinglePass->SetTexture("gNormal", deferredOutColor, 0);
        deferredLightDirSinglePass->SetTexture("gAlbedoSpec", deferredOutColor, 1);
        deferredLightDirSinglePass->SetTexture("gOther", deferredOutColor, 2);
        deferredLightDirSinglePass->SetTexture("gEmission", deferredOutColor, 3);
        deferredLightDirSinglePass->SetTexture("gDepth", deferredOutColor, -1);
        deferredLightDirSinglePass->SetTexture("sss", screenSpaceShadowOutput, 0);
        deferredLightDirSinglePass->SetInt("lightIndex", index);
        Graphics::DrawMesh(*fullScreenQuad, *deferredLightDirSinglePass, Matrix4Identity);
    }
}

void RenderContext::DrawDeferredLightOther(int index, Vector3 pos, Vector3 dir, float size, bool isCone){
    //deferredLightDirSingleOtherPass->EnableKeyword("OTHER");
    //deferredLightDirSingleOtherPass->SetTexture("gPosition", deferredOutColor, 0);

    deferredLightDirSingleOtherPass->SetTexture("gNormal", deferredOutColor, 0);
    deferredLightDirSingleOtherPass->SetTexture("gAlbedoSpec", deferredOutColor, 1);
    deferredLightDirSingleOtherPass->SetTexture("gEmission", deferredOutColor, 3);
    deferredLightDirSingleOtherPass->SetTexture("gOther", deferredOutColor, 2);
    deferredLightDirSingleOtherPass->SetTexture("gDepth", deferredOutColor, -1);
    deferredLightDirSingleOtherPass->SetTexture("sss", screenSpaceShadowOutput, 0);
    deferredLightDirSingleOtherPass->SetInt("lightIndex", index);
    deferredLightDirSingleOtherPass->SetFloat("screenWidth", cam.width);
    deferredLightDirSingleOtherPass->SetFloat("screenHeight", cam.height);
    glm::mat4 rotation = glm::mat4(1.0f); // identidade como fallback
    if(glm::length(dir) > 1e-4f){
        rotation = glm::toMat4(glm::quatLookAt(dir, Vector3Up));
    }
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(size));
    Matrix4 worldMatrix = translation * rotation * scale;

    Graphics::DrawMesh(isCone ? *coneMesh->meshs[0] : *sphereMesh->meshs[0], *deferredLightDirSingleOtherPass, worldMatrix);
}

glm::vec4 GetInvDeviceZToWorldZTransform(const glm::mat4& projection){
    float A = projection[2][2];
    float B = projection[3][2];

    glm::vec4 result;

    // z / (deviceDepth - w)
    result.z = B * 0.5f;
    result.w = A * 0.5f + 0.5f;

    return result;
}

void RenderContext::CleanSSS(){
    auto camera = GetCamera();
    screenSpaceShadowOutput->Resize(camera.width, camera.height);

    Graphics::BeginFramebuffer(*screenSpaceShadowOutput, true, {1, 1, 1, 1}, 0, 0);
    Graphics::EndFramebuffer();
}

void RenderContext::DrawSSS(Vector3 _lightDir, SSS_Settings settings){
    /*auto cam = GetCamera();
    
    Graphics::BeginFramebuffer(*screenSpaceShadowOutput, true, {1, 1, 1, 1}, 0, 0);
    screenSpaceShadow2->SetTexture("gDepth", GetDeferredFramebuffer(), -1);
    screenSpaceShadow2->SetTexture("gNormal", GetDeferredFramebuffer(), 0);

    screenSpaceShadow2->SetMatrix4("View_WorldToClip", cam.projection * cam.view);
    screenSpaceShadow2->SetVector4("View_InvDeviceZToWorldZTransform", GetInvDeviceZToWorldZTransform(cam.projection));
    screenSpaceShadow2->SetVector3("lightDirection", glm::normalize(_lightDir));
    screenSpaceShadow2->SetVector3("View_WorldCameraOrigin", cam.viewPos);
    screenSpaceShadow2->SetVector2("g_resolution", {cam.width, cam.height});
    screenSpaceShadow2->SetFloat("nearPlane", cam.nearClip);
    screenSpaceShadow2->SetFloat("farPlane", cam.farClip);
    Graphics::DrawFullScreenQuad(*screenSpaceShadow2, Matrix4Identity);
    Graphics::EndFramebuffer();
    return;*/

    #if 1
    auto camera = GetCamera();

    using namespace Bend;
    
    glm::vec3 lightDir = glm::normalize(_lightDir);
    glm::vec4 lightVec(lightDir, 0.0f);
    glm::mat4 viewProj = camera.projection * camera.view;
    glm::vec4 lightProjection = viewProj * lightVec;
    lightProjection.y = -lightProjection.y; // convert GL clip space → D3D style
    
    float inLightProjection[4] = { lightProjection.x, lightProjection.y, lightProjection.z, lightProjection.w };
    //inLightProjection[1] = -inLightProjection[1];
    int viewportSize[2] = { camera.width, camera.height };
    int minBounds[2] = { 0, 0 };
    int maxBounds[2] = { camera.width - 1, camera.height - 1 };
    
    Bend::DispatchList dispatchList = Bend::BuildDispatchList(
        inLightProjection,
        viewportSize,
        minBounds,
        maxBounds,
        false,   // OpenGL uses [0,1] depth
        64       // wave size
    );
    
    #if 0
    DispatchParametersGPU params{};
    #else
    SSSParameters2 params{};
    #endif

    //memcpy(params.LightCoordinate, dispatchList.LightCoordinate_Shader, sizeof(float)*4);
    params.LightCoordinate[0] = dispatchList.LightCoordinate_Shader[0];
    params.LightCoordinate[1] = dispatchList.LightCoordinate_Shader[1];
    params.LightCoordinate[2] = dispatchList.LightCoordinate_Shader[2];
    params.LightCoordinate[3] = dispatchList.LightCoordinate_Shader[3];

    params.InvDepthTextureSize[0] = 1.0f / camera.width;
    params.InvDepthTextureSize[1] = 1.0f / camera.height;
    #if 1
    params.DepthBounds[0] = 0;
    params.DepthBounds[1] = 1; 
    #endif
    params.NearDepthValue = 0.0f;
    params.FarDepthValue = 1.0f;

    #if 1
    params.SurfaceThickness = settings.surfaceThickness;
    params.BilinearThreshold = settings.bilinearThreshold;
    params.ShadowContrast = settings.shadowContrast;
    #else
    params.SurfaceThickness = 0.005f; // 0.02f;
    params.BilinearThreshold = 0.02f; //0.001f;
    params.ShadowContrast = 4; //4.0f; //1.0f;
    #endif
    
    for(int i = 0; i < dispatchList.DispatchCount; i++){
        auto& d = dispatchList.Dispatch[i];
        params.WaveOffset[0] = d.WaveOffset_Shader[0];
        params.WaveOffset[1] = d.WaveOffset_Shader[1];
        screenSpaceShadowData->SetData(&params, sizeof(SSSParameters2));

        screenSpaceShadow->SetTexture("DepthTexture", GetDeferredFramebuffer(), -1);
        screenSpaceShadow->SetTexture("OutputTexture", screenSpaceShadowOutput, 0);
        screenSpaceShadow->SetUniformBuffer("Params", screenSpaceShadowData, 2);
        screenSpaceShadow->Dispatch(d.WaveCount[0], d.WaveCount[1], d.WaveCount[2]);
    }

    #else
    using namespace Bend;
    auto camera = GetCamera();
    
    glm::vec3 lightDir = glm::normalize(_lightDir);
    glm::vec4 lightVec(lightDir, 0.0f);
    glm::mat4 viewProj = camera.projection * camera.view;
    glm::vec4 lightProjection = viewProj * lightVec;
    lightProjection.y = -lightProjection.y; // convert GL clip space → D3D style
    //glm::vec4 lightProjection = glm::vec4(glm::normalize(_lightDir), 0.0f); //viewProj * lightVec;

    float inLightProjection[4] = { lightProjection.x, lightProjection.y, lightProjection.z, lightProjection.w };
    int viewportSize[2] = { camera.width, camera.height };
    int minBounds[2] = { 0, 0 };
    int maxBounds[2] = { camera.width - 1, camera.height - 1 };

    Bend::DispatchList dispatchList = Bend::BuildDispatchList(
        inLightProjection,
        viewportSize,
        minBounds,
        maxBounds,
        false,   // OpenGL uses [0,1] depth
        64       // wave size
    );

    DispatchParametersGPU params{};

    memcpy(params.LightCoordinate, dispatchList.LightCoordinate_Shader, sizeof(float)*4);

    params.near = camera.nearClip;
    params.far = camera.farClip;
    params.InvDepthTextureSize[0] = 1.0f / camera.width;
    params.InvDepthTextureSize[1] = 1.0f / camera.height;
    params.NearDepthValue = 0.0f;
    params.FarDepthValue = 1.0f;

    params.SurfaceThickness = 0.005f; // 0.02f;
    params.BilinearThreshold = 0.02f; //0.001f;
    params.ShadowContrast = 4.0f; //1.0f;
    
    for(int i = 0; i < dispatchList.DispatchCount; i++){
        auto& d = dispatchList.Dispatch[i];
        params.WaveOffset[0] = d.WaveOffset_Shader[0];
        params.WaveOffset[1] = d.WaveOffset_Shader[1];
        screenSpaceShadowData->SetData(&params, sizeof(DispatchParametersGPU));

        screenSpaceShadow->SetTexture("DepthTexture", GetDeferredFramebuffer(), -1);
        screenSpaceShadow->SetTexture("OutputTexture", screenSpaceShadowOutput.get(), 0);
        screenSpaceShadow->SetUniformBuffer("DispatchParams", screenSpaceShadowData, 2);
        screenSpaceShadow->Dispatch(d.WaveCount[0], d.WaveCount[1], d.WaveCount[2]);
    }
    #endif
}

void RenderContext::EndDeferredPassAndCopyToForwardPass(){
    EndDeferredPass();

    //Framebuffer::Bind(*forwardOutColor);
    Graphics::BeginFramebuffer(*forwardOutColor, true, cam.cleanColor);
    //Graphics::Clean(0, 1, 0, 1);

    //deferredLightPass->SetTexture("gPosition", deferredOutColor, 0);
    deferredLightPass->SetTexture("gNormal", deferredOutColor, 0);
    deferredLightPass->SetTexture("gAlbedoSpec", deferredOutColor, 1);
    //deferredLightPass->SetTexture("gEmission", deferredOutColor, 3);
    deferredLightPass->SetTexture("gOther", deferredOutColor, 2);
    deferredLightPass->SetTexture("gDepth", deferredOutColor, -1);

    Graphics::DrawMesh(*fullScreenQuad, *deferredLightPass, Matrix4Identity);
    
    Graphics::BlitFramebuffer(deferredOutColor.get(), forwardOutColor.get(), -1);
    //Framebuffer::Bind(*forwardOutColor);
    
    //Graphics::BeginFramebuffer(*forwardOutColor);

    /*Framebuffer::Bind(*forwardOutColor);
    Graphics::Clean(0, 1, 0, 1);

    Shader::Bind(*deferredLightPassShader);
    deferredLightPassShader->SetInt("lightsCount", 0);
    deferredLightPassShader->SetVector3("viewPos", cam.viewPos);
    deferredLightPassShader->SetFramebuffer("gPosition", *deferredOutColor, 0, 0);
    deferredLightPassShader->SetFramebuffer("gNormal", *deferredOutColor, 1, 1);
    deferredLightPassShader->SetFramebuffer("gAlbedoSpec", *deferredOutColor, 2, 2);
    Material temp;
    Material::SubmitGraphicDatasCustomShader(temp, *deferredLightPassShader);
    Graphics::BlitQuadPostProcessingRaw(forwardOutColor);
    Graphics::SetDepthTest(DepthTest::LESS); 
    
    Graphics::BlitFramebuffer(deferredOutColor, forwardOutColor, -1);
    Framebuffer::Bind(*forwardOutColor);*/
}

void RenderContext::EndDrawToScreen(){
    Ref<Framebuffer> curFinalColor = customFinalColor != nullptr ? customFinalColor : finalColor;

    Graphics::BeginFramebuffer(*curFinalColor, true, cam.cleanColor);
    blitShader->SetTexture("mainTex", forwardOutColor, 0);
    Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();

    if(overrideFramebuffer != nullptr){
        Graphics::BeginFramebuffer(*overrideFramebuffer, true, cam.cleanColor);
        blitShader->SetTexture("mainTex", curFinalColor, 0);
        Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
        Graphics::EndFramebuffer();
        Graphics::BeginRenderToScreen();
        Graphics::EndRenderToScreen();
    } else {
        Graphics::BeginRenderToScreen();
        blitShader->SetTexture("mainTex", curFinalColor, 0);
        Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
        Graphics::EndRenderToScreen();
    }
    return;

    /*Graphics::EndFramebuffer();
    Graphics::BeginRenderToScreen();
    blitShader->SetTexture("mainTex", forwardOutColor, 0);
    Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
    Graphics::EndRenderToScreen();
    return;*/

    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*overrideFramebuffer, true, cam.cleanColor);
    blitShader->SetTexture("mainTex", forwardOutColor, 0);
    Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginRenderToScreen();
    Graphics::EndRenderToScreen();
    return;

    //Graphics::DrawQuadPostProcessing(forwardOutColor, nullptr, *blitShader);
    //Graphics::EndFramebuffer();
    //return;
    
    //Framebuffer::Unbind();
    //Graphics::EndFramebuffer();
    //return;

    Graphics::BlitFramebuffer(forwardOutColor.get(), curFinalColor.get());
    Graphics::DrawQuadPostProcessing(forwardOutColor.get(), curFinalColor.get(), *blitShader);

    if(overrideFramebuffer != nullptr){
        Graphics::DrawQuadPostProcessing(curFinalColor.get(), overrideFramebuffer.get(), *blitShader);
    } else {
        Graphics::DrawQuadPostProcessing(curFinalColor.get(), nullptr, *blitShader);
    }

    //Framebuffer::Unbind(); 
    Graphics::EndFramebuffer();
}

void RenderContext::EndDrawToScreenNew(){
    Ref<Framebuffer> curFinalColor = customFinalColor != nullptr ? customFinalColor : finalColor;
    int curIndex = customFinalColor != nullptr ? customFinalColorIndex : 0;

    Graphics::BeginFramebuffer(*curFinalColor, true, cam.cleanColor, curIndex);
    blitShader->SetTexture("mainTex", forwardOutColor, 0);
    Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();
}

void RenderContext::DrawCompose(std::vector<CameraRenderPass>& passes, int width, int height){
    auto Draw = [&](){
        for(auto& pass: passes){
            if(pass.isReflectionProbePass) continue;

            int viewportX = int(pass.camera.viewportRect.x * width);
            int viewportY = int(pass.camera.viewportRect.y * height);
            int viewportW = int(pass.camera.viewportRect.z * width);
            int viewportH = int(pass.camera.viewportRect.w * height);
            Graphics::SetViewport(
                viewportX,
                viewportY,
                viewportW,
                viewportH
            );
            blitShader->SetTexture("mainTex", pass.target, 0);
            Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
        }
    };

    if(overrideFramebuffer != nullptr){
        overrideFramebuffer->Resize(width, height);

        Graphics::BeginFramebuffer(*overrideFramebuffer, true, cam.cleanColor);
        Draw();
        Graphics::EndFramebuffer();
        Graphics::BeginRenderToScreen();
        Graphics::EndRenderToScreen();
    } else {
        Graphics::BeginRenderToScreen();
        Draw();
        Graphics::EndRenderToScreen();
    }
}

/*void RenderContext::_Renderer::AddPass(RenderPass* pass){
    if(pass->event == RenderPassEvent::PostProcess){
        postFxPasses.push_back(pass);
    }
}*/

void RenderContext::DrawPostFXs(std::vector<PostFX*>& postFXs){
    //Graphics::SetDepthMask(false);

    //TODO: Move this to other place later, this is just for test
    /*for(auto* i: rendererFeatures){
        i->AddRenderPasses(_renderer, *this);
    }*/

    step = false;
    /*Framebuffer**/ finalFramebuffer = postFx1;
    Graphics::BlitFramebuffer(forwardOutColor.get(), postFx1.get());
    //Graphics::BlitQuadPostProcessing(outColor, postFx1, *blitShader);

    for(auto i: postFXs){
        finalFramebuffer = step == false ? postFx2 : postFx1;

        if(i->enable){
            i->OnRenderImage(
                step == false ? postFx1 : postFx2, 
                step == false ? postFx2 : postFx1,
                *this
            );
        } else {
            Graphics::BlitFramebuffer(
                step == false ? postFx1.get() : postFx2.get(),
                step == false ? postFx2.get() : postFx1.get()
            );
            /*Graphics::BlitQuadPostProcessing(
                step == false ? postFx1 : postFx2, 
                step == false ? postFx2 : postFx1,
                *blitShader
            );*/
        }

        step = !step;
    }

    //Graphics::DrawQuadPostProcessing(finalFramebuffer, forwardOutColor, *blitShader);
    /*Graphics::BeginFramebuffer(*forwardOutColor);
    blitShader->SetTexture("mainTex", finalFramebuffer, 0);
    Graphics::DrawFullScreenQuad(*blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();*/
}

void RenderContext::DrawPostFXs(Scene& scene, RendererFeatureContext& passCtx, RenderFrameData& data, RenderPass* last, RenderPassEvent pass){
    std::vector<RenderPass*>& passes = passCtx.renderPasses[(int)pass];
    //Graphics::SetDepthMask(false);

    step = false;
    /*Framebuffer**/ finalFramebuffer = postFx1;
    Graphics::BlitFramebuffer(forwardOutColor.get(), postFx1.get());
    //Graphics::BlitQuadPostProcessing(outColor, postFx1, *blitShader);

    for(auto i: passes){
        finalFramebuffer = step == false ? postFx2 : postFx1;

        if(true /*i->enable*/){
            data.src = step == false ? postFx1 : postFx2;
            data.dst = step == false ? postFx2 : postFx1;
            i->Execute(scene, *this, data);
            /*i->OnRenderImage(
                step == false ? postFx1 : postFx2, 
                step == false ? postFx2 : postFx1,
                this
            );*/
        } else {
            Graphics::BlitFramebuffer(
                step == false ? postFx1.get() : postFx2.get(),
                step == false ? postFx2.get() : postFx1.get()
            );
            /*Graphics::BlitQuadPostProcessing(
                step == false ? postFx1 : postFx2, 
                step == false ? postFx2 : postFx1,
                *blitShader
            );*/
        }

        step = !step;
    }

    if(last != nullptr){
        finalFramebuffer = step == false ? postFx2 : postFx1;
        data.src = step == false ? postFx1 : postFx2;
        data.dst = step == false ? postFx2 : postFx1;
        last->Execute(scene, *this, data);
    }

    Graphics::BlitFramebuffer(finalFramebuffer.get(), forwardOutColor.get());

    /*Graphics::BeginFramebuffer(*forwardOutColor, false);
    blitShader->SetTexture("mainTex", finalFramebuffer, 0);
    Graphics::DrawFullScreenQuad(*blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();*/
}

void RenderContext::BeginUIPass(){
    Graphics::BeginFramebuffer(*forwardOutColor, false, cam.cleanColor);
    //blitShader->SetTexture("mainTex", finalFramebuffer, 0);
    //Graphics::DrawFullScreenQuad(*blitShader, Matrix4Identity);

    /*auto uiCamera = Camera{
        OD::Matrix4Identity, 
        OD::math::ortho(0.0f, (float)cam.width, 0.0f, (float)cam.height, -10.0f, 10.0f)
        //OD::math::ortho(0.0f, (float)cam.width, (float)cam.height, 0.0f, -10.0f, 10.0f)
    };
    Graphics::SetCamera(uiCamera);*/
    //Graphics::SetCamera(cam);
    //Graphics::CleanDepthOnly();
}

void RenderContext::EndUIPass(){
    Graphics::EndFramebuffer();
}

void RenderContext::SetupCameraProperties(Camera inCam){
    cam = inCam;

    Graphics::SetCamera(cam);
    Graphics::SetViewport(
        0, 
        0, 
        cam.width, 
        cam.height
    );

    /*int viewportX = int(cam.viewportRect.x * cam.width);
    int viewportY = int(cam.viewportRect.y * cam.height);
    int viewportW = int(cam.viewportRect.z * cam.width);
    int viewportH = int(cam.viewportRect.w * cam.height);
    Graphics::SetViewport(
        viewportX,
        viewportY,
        viewportW,
        viewportH
    );*/

    Material::SetGlobalMatrix4("view", cam.view);
    Material::SetGlobalMatrix4("projection", cam.projection);
    Material::SetGlobalVector3("viewPos", cam.viewPos); //TODO: this "viewPos" look like is not realtime update in the shader what make pbr bug, so check later if the SetGlobalxxx is bug
}

void RenderContext::ScreenClean(){
    //Renderer::Clean(cam.cleanColor.x, cam.cleanColor.y, cam.cleanColor.z, 1);
    Graphics::Clean(0, 0, 1, 1);
}

void RenderContext::RunComputeRenderListShadow(ComputeRenderListSettings settings, ShadowDrawingSettings drawSettings, RendererList& renderList, Material* shadowPass){
    //RenderDataLoop2([&](auto& data){
    RenderDataLoopNew([&](auto& data){
        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) return;
        if(data.customShadowPass == nullptr) data.customShadowPass = shadowPass;
        AddDrawShadow(data, drawSettings, renderList);
    });
}

void RenderContext::RunComputeRenderList(ComputeRenderListSettings settings, DrawingSettings drawSettings, RendererList& renderList){
    //RenderDataLoop2([&](auto& data){
    RenderDataLoopNew([&](auto& data){
        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) return;
        AddDrawRenderers(data, drawSettings, renderList);
    });

    /*auto staticMeshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: staticMeshView){
        auto& info = staticMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = staticMeshView.get<MeshRendererComponent>(e);
        auto& t = staticMeshView.get<TransformComponent>(e);
        auto& s = staticMeshView.get<StaticRendererComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        if(s.staticDatas.size() != 1) s.staticDatas.resize(1);
        if(s.staticDatas[0].isDirt){
            s.staticDatas[0].isDirt = false;
            s.staticDatas[0].m = t.GlobalModelMatrix();
            s.staticDatas[0].aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, s.staticDatas[0].m);
        }

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  s.staticDatas[0].m;
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = s.staticDatas[0].aabb;
        
        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
        AddDrawRenderers(data, drawSettings, renderList);
    }

    auto meshStaticRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: meshStaticRenderView){
        auto& info = meshStaticRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshStaticRenderView.get<ModelRendererComponent>(e);
        auto& t = meshStaticRenderView.get<TransformComponent>(e);
        auto& s = meshStaticRenderView.get<StaticRendererComponent>(e);
    
        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        if(s.staticDatas.size() != model->renderTargets.size()){
            s.staticDatas.resize(model->renderTargets.size());
            for(auto& i: s.staticDatas) i.isDirt = true;
        }

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            if(s.staticDatas[_i].isDirt){
                s.staticDatas[_i].isDirt = false;
                s.staticDatas[_i].m = t.GlobalModelMatrix();
                s.staticDatas[_i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), s.staticDatas[_i].m);
            }

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  s.staticDatas[_i].m;
            data.aabb = s.staticDatas[_i].aabb;
            data.posePalette = nullptr;
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            _i += 1;

            if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
            AddDrawRenderers(data, drawSettings, renderList);
        }
    }

    auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable>
    );
    for(auto e: meshView){
        auto& info = meshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix = t.GlobalModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
        AddDrawRenderers(data, drawSettings, renderList);
    }

    auto meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable>
    );
    for(auto e: meshRenderView){
        auto& info = meshRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //if(c.renderData.size() != model->renderTargets.size()) continue;

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix = t.GlobalModelMatrix()  * c.localTransform.GetLocalModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.posePalette = nullptr;
            //data.aabb = c.GetGlobalAABB(t);
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix); //Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());
            //data.aabb.Expand2(Vector3(5.5f));
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            _i += 1;

            if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
            AddDrawRenderers(data, drawSettings, renderList);
        }
    }

    auto skinnedMeshView = GetScene()->GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: skinnedMeshView){
        auto& info = skinnedMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        SkinnedMeshRendererComponent& c = skinnedMeshView.get<SkinnedMeshRendererComponent>(e);
        TransformComponent& t = skinnedMeshView.get<TransformComponent>(e);

        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  t.GlobalModelMatrix();;
        //data.transform = Transform(data.targetMatrix); //t.ToTransform();
        
        //INFO: Try optimize
        if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
            c.finalPose.GetMatrixPalette(c.posePalette, c.skeleton.GetInvBindPose());
        } 
        data.posePalette = &c.posePalette;
        
        //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = (int)e;

        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
        AddDrawRenderers(data, drawSettings, renderList);
    }

    auto skinnedView = GetScene()->GetRegistry().view<SkinnedModelRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto [e, c, t, info]: skinnedView.each()){
        //auto& info = skinnedView.get<InfoComponent>(e);
        if(info.enable == false) continue;
        //SkinnedModelRendererComponent& c = skinnedView.get<SkinnedModelRendererComponent>(e);
        //TransformComponent& t = skinnedView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //TODO: Revisar isto, fix temporariamente o model nao esta send renderizando sem chama UpdatePosePalette
        if(c.posePalette.size() == 0) c.UpdatePosePalette();

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;
            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            //data.transform = Transform(data.targetMatrix); //t.ToTransform();
            
            //INFO: Try optimize
            if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
                c.finalPose.GetMatrixPalette(c.posePalette, model->skeleton.GetInvBindPose()); 
            }
            data.posePalette = &c.posePalette;
            
            //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);//Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());

            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = (int)e;
            
            _i += 1;

            if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
            AddDrawRenderers(data, drawSettings, renderList);
        }
    }*/
}

void RenderContext::RenderDataLoop2(Scene& scene, std::function<void(RenderData&)> onReciveRenderData){
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop");

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::0");
    /*auto staticMeshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: staticMeshView){
        auto& info = staticMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = staticMeshView.get<MeshRendererComponent>(e);
        auto& t = staticMeshView.get<TransformComponent>(e);
        auto& s = staticMeshView.get<StaticRendererComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        if(s.staticDatas.size() != 1) s.staticDatas.resize(1);
        if(s.staticDatas[0].isDirt){
            s.staticDatas[0].isDirt = false;
            s.staticDatas[0].m = t.GlobalModelMatrix();
            s.staticDatas[0].aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, s.staticDatas[0].m);
        }

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  s.staticDatas[0].m;
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = s.staticDatas[0].aabb;
        
        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }*/
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::1");
    /*auto meshStaticRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: meshStaticRenderView){
        auto& info = meshStaticRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshStaticRenderView.get<ModelRendererComponent>(e);
        auto& t = meshStaticRenderView.get<TransformComponent>(e);
        auto& s = meshStaticRenderView.get<StaticRendererComponent>(e);
    
        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        if(s.staticDatas.size() != model->renderTargets.size()){
            s.staticDatas.resize(model->renderTargets.size());
            for(auto& i: s.staticDatas) i.isDirt = true;
        }

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            if(s.staticDatas[_i].isDirt){
                s.staticDatas[_i].isDirt = false;
                s.staticDatas[_i].m = t.GlobalModelMatrix();
                s.staticDatas[_i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), s.staticDatas[_i].m);
            }

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  s.staticDatas[_i].m;
            data.aabb = s.staticDatas[_i].aabb;
            data.posePalette = nullptr;
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            onReciveRenderData(data);
            _i += 1;
        }
    }*/
    //}

    //////////////////////////////////////////////////////////

    auto staticRendererClusterView = scene.GetRegistry().view<StaticRendererClusterComponent, TransformComponent, InfoComponent>(
        entt::exclude<HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto [entity, c, t, i]: staticRendererClusterView.each()){
        for(auto& chunk: c.chunks){
            if(chunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

            for(auto& subchunk: chunk.subchunks){
                if(subchunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

                for(auto& renderTarget: subchunk.targets){
                    RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = renderTarget.targetMaterial.get();
                    data.customShadowPass = nullptr;
                    data.targetMesh = renderTarget.targetMesh.get();
                    data.targetMatrix = renderTarget.targetMatrix;
                    data.posePalette = nullptr;
                    data.aabb = renderTarget.aabb;

                    data.perDrawData.Int_0_SetMask(0, true);
                    data.perDrawData.int_0[0] = 0;
                    //data.perDrawData.int_0[1] = 0;

                    #if EnableExperimentalPerDrawCustomData
                    data.useCustomData = c.useCustomData;
                    data.customData = c.customData;
                    #endif

                    onReciveRenderData(data);
                }

                subchunk.drawIntancingCommands.Each([&](DrawInstancingCommand2& cmd){
                    RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = cmd.material;
                    data.customShadowPass = nullptr;
                    data.targetMesh = cmd.meshs;
                    data.targetMatrix = Matrix4Identity;
                    data.posePalette = nullptr;
                    data.aabb = subchunk.renderBounds;
                    //data.perDrawData.int_0_Count = 0;// .clear();
                    data.instancingBuffer = cmd.buffer.get();
                    
                    onReciveRenderData(data);
                });
            }
        }
    }

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::2");
    auto meshView = scene.GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto e: meshView){
        const auto& info = meshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        const auto& c = meshView.get<MeshRendererComponent>(e);
        const auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix = t.GlobalModelMatrixReadSafe();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.Int_0_SetMask(0, true);
        data.perDrawData.Int_0_SetMask(1, true);
        data.perDrawData.int_0[0] = ((int)e) + 1;
        data.perDrawData.int_0[1] = info.layer;

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::3");
    auto meshRenderView = scene.GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto e: meshRenderView){
        const auto& info = meshRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        const auto& t = meshRenderView.get<TransformComponent>(e);

        if(c.draw == false) continue;

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //if(c.renderData.size() != model->renderTargets.size()) continue;

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix = t.GlobalModelMatrixReadSafe() /** c.localTransform.GetLocalModelMatrix()*/ * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.posePalette = nullptr;
            //data.aabb = c.GetGlobalAABB(t);
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix); //Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrixReadSafe());
            //data.aabb.Expand2(Vector3(5.5f));
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.Int_0_SetMask(0, true);
            data.perDrawData.Int_0_SetMask(1, true);
            data.perDrawData.int_0[0] = ((int)e) + 1;
            data.perDrawData.int_0[1] = info.layer;

            onReciveRenderData(data);
            _i += 1;
        }
    }
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::4");
    auto skinnedMeshView = scene.GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: skinnedMeshView){
        const auto& info = skinnedMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        SkinnedMeshRendererComponent& c = skinnedMeshView.get<SkinnedMeshRendererComponent>(e);
        const TransformComponent& t = skinnedMeshView.get<TransformComponent>(e);

        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
        data.targetMaterial = c.material.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  t.GlobalModelMatrixReadSafe();
        //data.transform = Transform(data.targetMatrix); //t.ToTransform();
        
        //INFO: Try optimize
        /*if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
            c.finalPose.GetMatrixPalette(c.posePalette, c.skeleton.GetInvBindPose());
        }*/ 
        data.posePalette = &c.posePalette;
        
        //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.Int_0_SetMask(0, true);
        data.perDrawData.Int_0_SetMask(1, true);
        data.perDrawData.int_0[0] = ((int)e) + 1;
        data.perDrawData.int_0[1] = info.layer;

        onReciveRenderData(data);
    }
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::5");
    auto skinnedView = scene.GetRegistry().view<SkinnedModelRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: skinnedMeshView){
        const auto& info = skinnedView.get<InfoComponent>(e);
        if(info.enable == false) continue;
        SkinnedModelRendererComponent& c = skinnedView.get<SkinnedModelRendererComponent>(e);
        const TransformComponent& t = skinnedView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //TODO: Revisar isto, fix temporariamente o model nao esta send renderizando sem chama UpdatePosePalette
        //if(c.posePalette.size() == 0) c.UpdatePosePalette();

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;
            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  t.GlobalModelMatrixReadSafe() /** c.localTransform.GetLocalModelMatrix()*/ * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            //data.transform = Transform(data.targetMatrix); //t.ToTransform();
            
            //INFO: Try optimize
            /*if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
                c.finalPose.GetMatrixPalette(c.posePalette, model->skeleton.GetInvBindPose()); 
            }*/
            data.posePalette = &c.posePalette;
            
            //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);//Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);

            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.Int_0_SetMask(0, true);
            data.perDrawData.Int_0_SetMask(1, true);
            data.perDrawData.int_0[0] = ((int)e) + 1;
            data.perDrawData.int_0[1] = info.layer;
            if(c.updateWhenOffscreen) data.SetFlag(RenderData::Flag::AlwaysDraw, true); //data.awalsDraw = true;
            
            onReciveRenderData(data);
            _i += 1;
        }
    }
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::6");
    /*auto spriteView = GetScene()->GetRegistry().view<TransformComponent, SpriteRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto entity: spriteView){
        auto& info = spriteView.get<InfoComponent>(entity);
        if(info.enable == false) continue;

        TransformComponent& t = spriteView.get<TransformComponent>(entity);
        SpriteRendererComponent& c = spriteView.get<SpriteRendererComponent>(entity);
        //if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        AABB aabb(Vector3(0), c.sprite->Width() / c.pixelUnitSize, c.sprite->Height() / c.pixelUnitSize, 1);
        Transform scale;
        scale.LocalScale(Vector3(c.sprite->Width() / c.pixelUnitSize, c.sprite->Height() / c.pixelUnitSize, 1));

        c.material->SetVector4("color", c.color);
        c.material->SetTexture("mainTex", c.sprite);

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.targetMesh = spriteMesh.get();
        data.targetMatrix =  t.GlobalModelMatrix() * scale.GetLocalModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(aabb, data.targetMatrix);

        onReciveRenderData(data);
    }*/
    //}
}

//Deprecated
void RenderContext::RenderDataLoop(Scene& scene, std::function<void(RenderData&)> onReciveRenderData){
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop");

    /*{
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::-2");
    std::vector<RenderData> outRenderData;  
    for(auto& i: renderFeatures){
        //i->scene = scene;
        i->OnCollectRenderData(cam, outRenderData);
    }
    for(auto& i: outRenderData){
        onReciveRenderData(i);
    }
    }*/

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::-1");
    auto staticRendererClusterView = scene.GetRegistry().view<StaticRendererClusterComponent, TransformComponent, InfoComponent>(
        entt::exclude<HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto [entity, c, t, i]: staticRendererClusterView.each()){
        for(auto& chunk: c.chunks){
            //if(chunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

            /*
            for(auto* r : receivers){
                uint32_t wants = r->Wants(...);
                if(wants){ 
                    interested.push_back(r);
                }
            }
            */

            for(auto& subchunk: chunk.subchunks){
                //if(subchunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

                /*
                for(auto* r : interested){
                    uint32_t wants = r->Wants(...);
                    if(wants == false){ 
                        interested.remove(r);
                    }
                }
                */

                for(auto& renderTarget: subchunk.targets){
                    RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = renderTarget.targetMaterial.get();
                    data.customShadowPass = nullptr;
                    data.targetMesh = renderTarget.targetMesh.get();
                    data.targetMatrix = renderTarget.targetMatrix;
                    data.posePalette = nullptr;
                    data.aabb = renderTarget.aabb;

                    /*
                    for(auto* r : interested){
                        uint32_t wants = r->Wants(...);
                        if(wants == false){ 
                            interested.remove(r);
                        }
                    }
                    */

                    //data.perDrawData.int_0_Count = 0;//.resize(1);//TODO: Optimaze this, this can be make heap allocation
                    //data.perDrawData.int_0[0] = 0;
                    //data.perDrawData.int_0[1] = 0;//GetLayerIndex(info.layer);

                    #if EnableExperimentalPerDrawCustomData
                    data.useCustomData = c.useCustomData;
                    data.customData = c.customData;
                    #endif

                    onReciveRenderData(data);
                }

                subchunk.drawIntancingCommands.Each([&](DrawInstancingCommand2& cmd){
                    RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = cmd.material;
                    data.customShadowPass = nullptr;
                    data.targetMesh = cmd.meshs;
                    data.targetMatrix = Matrix4Identity;
                    data.posePalette = nullptr;
                    data.aabb = subchunk.renderBounds;
                    //data.perDrawData.int_0_Count = 0; //.clear();
                    data.instancingBuffer = cmd.buffer.get();
                    
                    onReciveRenderData(data);
                });
            }
        }
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::0");
    auto staticMeshView = scene.GetRegistry().view<MeshRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: staticMeshView){
        auto& info = staticMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = staticMeshView.get<MeshRendererComponent>(e);
        auto& t = staticMeshView.get<TransformComponent>(e);
        auto& s = staticMeshView.get<StaticRendererComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        if(s.staticDatas.size() != 1) s.staticDatas.resize(1);
        if(s.staticDatas[0].isDirt){
            s.staticDatas[0].isDirt = false;
            s.staticDatas[0].m = t.GlobalModelMatrix();
            s.staticDatas[0].aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, s.staticDatas[0].m);
        }

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  s.staticDatas[0].m;
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = s.staticDatas[0].aabb;
        
        data.perDrawData.Int_0_SetMask(0, true);
        data.perDrawData.Int_0_SetMask(1, true);
        data.perDrawData.int_0[0] = ((int)e) + 1;
        data.perDrawData.int_0[1] = info.layer;

        if(c.useCustomData){
            data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
            data.perDrawData.vector4_0[0] = c.customData;
            
        }

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::1");
    auto meshStaticRenderView = scene.GetRegistry().view<ModelRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: meshStaticRenderView){
        auto& info = meshStaticRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshStaticRenderView.get<ModelRendererComponent>(e);
        if(c.draw == false) continue;

        auto& t = meshStaticRenderView.get<TransformComponent>(e);
        auto& s = meshStaticRenderView.get<StaticRendererComponent>(e);
    
        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        if(s.staticDatas.size() != model->renderTargets.size()){
            s.staticDatas.resize(model->renderTargets.size());
            for(auto& i: s.staticDatas) i.isDirt = true;
        }

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            if(s.staticDatas[_i].isDirt){
                s.staticDatas[_i].isDirt = false;
                s.staticDatas[_i].m = t.GlobalModelMatrix();
                s.staticDatas[_i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), s.staticDatas[_i].m);
            }

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  s.staticDatas[_i].m;
            data.aabb = s.staticDatas[_i].aabb;
            data.posePalette = nullptr;
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.Int_0_SetMask(0, true);
            data.perDrawData.Int_0_SetMask(1, true);
            data.perDrawData.int_0[0] = ((int)e) + 1;
            data.perDrawData.int_0[1] = info.layer;

            if(c.useCustomData){
                data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
                data.perDrawData.vector4_0[0] = c.customData;
            }

            #if EnableExperimentalPerDrawCustomData
            data.useCustomData = c.useCustomData;
            data.customData = c.customData;
            #endif

            onReciveRenderData(data);
            _i += 1;
        }
    }
    }

    //////////////////////////////////////////////////////////

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::2");
    auto meshView = scene.GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto e: meshView){
        auto& info = meshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix = t.GlobalModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.Int_0_SetMask(0, true);
        data.perDrawData.Int_0_SetMask(1, true);
        data.perDrawData.int_0[0] = ((int)e) + 1;
        data.perDrawData.int_0[1] = info.layer;

        if(c.useCustomData){
            data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
            data.perDrawData.vector4_0[0] = c.customData;
        }

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::3");
    auto meshRenderView = scene.GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto e: meshRenderView){
        auto& info = meshRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        if(c.draw == false) continue;

        auto& t = meshRenderView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //if(c.renderData.size() != model->renderTargets.size()) continue;

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            //data.targetMatrix = t.GlobalModelMatrix() * c.localTransform.GetModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.targetMatrix = math::simdMul(t.GlobalModelMatrix(), model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex));
            data.posePalette = nullptr;
            //data.aabb = c.GetGlobalAABB(t);
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix); //Isto pode esta errado pq o aabb é do model interior, nao por mesh
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());
            //data.aabb.Expand2(Vector3(5.5f));
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.Int_0_SetMask(0, true);
            data.perDrawData.Int_0_SetMask(1, true);
            data.perDrawData.int_0[0] = ((int)e) + 1;
            data.perDrawData.int_0[1] = info.layer;

            if(c.useCustomData){
                data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
                data.perDrawData.vector4_0[0] = c.customData;
            }

            #if EnableExperimentalPerDrawCustomData
            data.useCustomData = c.useCustomData;
            data.customData = c.customData;
            #endif

            onReciveRenderData(data);
            _i += 1;
        }
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::4");
    auto skinnedMeshView = scene.GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: skinnedMeshView){
        auto& info = skinnedMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        SkinnedMeshRendererComponent& c = skinnedMeshView.get<SkinnedMeshRendererComponent>(e);
        TransformComponent& t = skinnedMeshView.get<TransformComponent>(e);

        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  t.GlobalModelMatrix();
        //data.transform = Transform(data.targetMatrix); //t.ToTransform();
        
        //INFO: Try optimize
        if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
            c.finalPose.GetMatrixPalette(c.posePalette, c.skeleton.GetInvBindPose());
        } 
        data.posePalette = &c.posePalette;
        
        //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.Int_0_SetMask(0, true);
        data.perDrawData.Int_0_SetMask(1, true);
        data.perDrawData.int_0[0] = ((int)e) + 1;
        data.perDrawData.int_0[1] = info.layer;

        if(c.useCustomData){
            data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
            data.perDrawData.vector4_0[0] = c.customData;
        }

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::5");
    auto skinnedView = scene.GetRegistry().view<SkinnedModelRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto [e, c, t, info]: skinnedView.each()){
        //auto& info = skinnedView.get<InfoComponent>(e);
        if(info.enable == false) continue;
        if(c.draw == false) continue;
        //SkinnedModelRendererComponent& c = skinnedView.get<SkinnedModelRendererComponent>(e);
        //TransformComponent& t = skinnedView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //TODO: Revisar isto, fix temporariamente o model nao esta send renderizando sem chama UpdatePosePalette
        if(c.posePalette.size() == 0) c.UpdatePosePalette();

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;
            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();

            auto m1 = t.GlobalModelMatrix();
            auto m2 = model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            //data.targetMatrix = m1 * m2;*/
            /*glm_mat4_mul(
			    &m1[0].data,
			    &m2[0].data,
			    &data.targetMatrix[0].data
            );*/
            //LogInfo("%f", _m3[0].a);

            //TODO: Finish this optimization, maybe add option to enable GetGlobalMatrix(i.bindPoseIndex)
            //data.targetMatrix =  t.GlobalModelMatrix()/** c.localTransform.GetModelMatrix()*/ * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            //data.targetMatrix = t.GlobalModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.targetMatrix = math::simdMul(t.GlobalModelMatrix(), model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex));
            //data.transform = Transform(data.targetMatrix); //t.ToTransform();
            
            //INFO: Try optimize
            if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
                c.finalPose.GetMatrixPalette(c.posePalette, model->skeleton.GetInvBindPose()); 
            }
            data.posePalette = &c.posePalette;
            
            //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);//Isto pode esta errado pq o aabb é do model interior, nao por mesh
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());

            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.Int_0_SetMask(0, true);
            data.perDrawData.Int_0_SetMask(1, true);
            data.perDrawData.int_0[0] = ((int)e) + 1;
            data.perDrawData.int_0[1] = info.layer;

            data.customShadowPass = data.targetMaterial->DepthPass() != -1 ? data.targetMaterial : nullptr; 

            //TODO: Refactory perDrawData to avoid memory alocation
            if(c.useCustomData){
                data.perDrawData.Vector4_0_SetMask(0, true);
                data.perDrawData.vector4_0[0] = c.customData;
            }

            #if EnableExperimentalPerDrawCustomData
            data.useCustomData = c.useCustomData;
            data.customData = c.customData;
            #endif

            if(c.updateWhenOffscreen) data.SetFlag(RenderData::Flag::AlwaysDraw, true);//data.awalsDraw = true;
            
            onReciveRenderData(data);
            _i += 1;
        }
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::6");
    auto spriteView = scene.GetRegistry().view<TransformComponent, SpriteRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto entity: spriteView){
        auto& info = spriteView.get<InfoComponent>(entity);
        if(info.enable == false) continue;

        TransformComponent& t = spriteView.get<TransformComponent>(entity);
        SpriteRendererComponent& c = spriteView.get<SpriteRendererComponent>(entity);
        //if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        AABB aabb(Vector3(0), c.sprite->Width() / c.pixelUnitSize, c.sprite->Height() / c.pixelUnitSize, 1);
        Transform scale;
        scale.Scale(Vector3(c.sprite->Width() / c.pixelUnitSize, c.sprite->Height() / c.pixelUnitSize, 1));

        c.material->SetVector4("color", c.color);
        c.material->SetTexture("mainTex", c.sprite);

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.targetMesh = spriteMesh.get();
        data.targetMatrix =  t.GlobalModelMatrix() * scale.GetModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(aabb, data.targetMatrix);

        onReciveRenderData(data);
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::Decal");
    auto decalView = scene.GetRegistry().view<DecalRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto [entity, decal, trans, info]: decalView.each()){
        RenderData data;
        data.distance = math::distance2(cam.viewPos, trans.Position());
        data.targetMaterial = decal.material.get();
        data.customShadowPass = nullptr;
        data.targetMesh = decalMesh->meshs[0].get();

        data.perDrawData.Int_0_SetMask(0, true);
        data.perDrawData.Int_0_SetMask(1, true);
        data.perDrawData.int_0[0] = ((int)entity) + 1;
        data.perDrawData.int_0[1] = decal.customLayerIndex;
        
        if(decal.useCustomOffsetAndSize == false){
            data.targetMatrix = trans.GlobalModelMatrix();
            data.aabb = transform_aabb_optimized_abs_center_extents(
                AABB(Vector3Zero, 0.5f, 0.5f, 0.5f), data.targetMatrix
            );
        } else {
            Transform offsetTrans;
            offsetTrans.Position(decal.offset);
            offsetTrans.Scale(decal.size);
            data.targetMatrix = trans.GlobalModelMatrix() * offsetTrans.GetModelMatrix();
            /*data.aabb = transform_aabb_optimized_abs_center_extents(
                AABB(offsetTrans.Position(), offsetTrans.Scale().x/2, offsetTrans.Scale().y/2, offsetTrans.Scale().z/2), trans.GlobalModelMatrix()
            );*/
            data.aabb = transform_aabb_optimized_abs_center_extents(
                AABB(Vector3Zero, 0.5f, 0.5f, 0.5f), data.targetMatrix
            );
        }

        SetPerInstanceData(data.targetMatrix, decal.perInstanceData);

        data.SetFlag(RenderData::Flag::IsDecal, true);// .isDecal = true;
        data.SetFlag(RenderData::Flag::RenderShadow, false);// .renderShadow = false;

        onReciveRenderData(data);
    }
    }
}

template<typename Iter, typename Func>
void tf_for_each2(tf::Taskflow& taskflow, Iter begin, Iter end, Func func){
    size_t total = std::distance(begin, end);
    if (total == 0) return;

    const int num_threads = std::thread::hardware_concurrency();
    size_t chunk = (total + num_threads - 1) / num_threads;

    // Local helper lambda to move iterators
    auto advance_iter = [](Iter it, size_t offset) {
        if constexpr (std::is_base_of_v<
            std::random_access_iterator_tag,
            typename std::iterator_traits<Iter>::iterator_category
        >) {
            LogInfo("Use Random Acess");
            return it + offset;  // fast for random-access
        } else {
            return std::next(it, offset); // safe for forward iterators
        }
    };

    for (int t = 0; t < num_threads; ++t) {
        auto chunk_begin = advance_iter(begin, t * chunk);
        auto chunk_end   = (t + 1 < num_threads)
                         ? advance_iter(begin, (t + 1) * chunk)
                         : end;

        if (chunk_begin == end) break;

        taskflow.emplace([=] {
            for (auto it = chunk_begin; it != chunk_end; ++it) {
                func(*it);
            }
        });
    }
}

template<typename Iter, typename Func>
void tf_for_each3(tf::Taskflow& taskflow, Iter begin, Iter end, size_t num_tasks = 0, Func func = {}){
    using traits = std::iterator_traits<Iter>;
    using diff_t = typename traits::difference_type;

    diff_t total_diff = std::distance(begin, end);
    if(total_diff <= 0) return;

    const size_t total = static_cast<size_t>(total_diff);

    // determine num_tasks
    if(num_tasks == 0) {
        num_tasks = std::thread::hardware_concurrency();
        if(num_tasks == 0) num_tasks = 1;
    }

    // clamp tasks to not exceed total items
    if(num_tasks > total) num_tasks = total;

    const size_t chunk_size = (total + num_tasks - 1) / num_tasks; // ceil

    // helper: advance iterator by an offset (works for random-access and forward iterators)
    auto advance_iter = [](Iter it, size_t offset) -> Iter {
        if constexpr (
            std::is_same_v<typename traits::iterator_category, std::random_access_iterator_tag> ||
            std::is_base_of_v<std::random_access_iterator_tag, typename traits::iterator_category>
        ) {
            return it + static_cast<diff_t>(offset);
        } else {
            return std::next(it, static_cast<diff_t>(offset));
        }
    };

    // create tasks: each task gets its start/end iterators and its index
    for(size_t t = 0; t < num_tasks; ++t){
        const size_t start_off = t * chunk_size;
        if(start_off >= total) break;

        const size_t end_off = std::min(total, start_off + chunk_size);

        Iter chunk_begin = advance_iter(begin, start_off);
        Iter chunk_end   = advance_iter(begin, end_off);

        // capture chunk iterators and task index by value; capture func by value to avoid dangling
        taskflow.emplace([chunk_begin, chunk_end, t, func]() mutable {
            for (Iter it = chunk_begin; it != chunk_end; ++it) {
                func(*it, t); // <-- user lambda receives (element, taskIndex)
            }
        });
    }
}

//////////////////////////////////////////
inline void FillMeshRenderData(RenderContext& ctx, RenderData& data, MeshRendererComponent& c, TransformComponent& t, InfoComponent& info, entt::entity e){
    data.layer = info.layer;

    data.distance = math::distance2(ctx.GetCamera().viewPos, t.Position());
    data.targetMaterial = c.material.get();
    data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
    data.targetMesh = c.mesh.get();
    data.targetMatrix = t.GlobalModelMatrix();
    data.posePalette = nullptr;
    //data.aabb = c.GetGlobalAABB(t);
    data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

    data.perDrawData.Int_0_SetMask(0, true);
    data.perDrawData.Int_0_SetMask(1, true);
    data.perDrawData.int_0[0] = ((int)e) + 1;
    data.perDrawData.int_0[1] = info.layer;

    if(c.useCustomData){
        data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
        data.perDrawData.vector4_0[0] = c.customData;
    }

    data.SetFlag(RenderData::Flag::IsStatic, false);
    data.SetFlag(RenderData::Flag::FromMesh, true);

    Vector4 perInstanceData = {0, 0, 0, float(info.layer)};
    SetPerInstanceData(data.targetMatrix, perInstanceData);

    #if EnableExperimentalPerDrawCustomData
    data.useCustomData = c.useCustomData;
    data.customData = c.customData;
    #endif
}

inline void FillStaticMeshRenderData(RenderContext& ctx, RenderData& data, StaticRendererComponent& s, MeshRendererComponent& c, TransformComponent& t, InfoComponent& info, entt::entity e){
    data.layer = info.layer;

    data.distance = math::distance2(ctx.GetCamera().viewPos, t.Position());
    data.targetMaterial = c.material.get();
    data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
    data.targetMesh = c.mesh.get();
    data.targetMatrix =  s.staticDatas[0].m;
    data.posePalette = nullptr;
    //data.aabb = c.GetGlobalAABB(t);
    data.aabb = s.staticDatas[0].aabb;
    
    data.perDrawData.Int_0_SetMask(0, true);
    data.perDrawData.Int_0_SetMask(1, true);
    data.perDrawData.int_0[0] = ((int)e) + 1;
    data.perDrawData.int_0[1] = info.layer;

    data.SetFlag(RenderData::Flag::IsStatic, true);
    data.SetFlag(RenderData::Flag::FromMesh, true);

    if(c.useCustomData){
        data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
        data.perDrawData.vector4_0[0] = c.customData;
    }

    #if EnableExperimentalPerDrawCustomData
    data.useCustomData = c.useCustomData;
    data.customData = c.customData;
    #endif
}

inline void FillSkinnedMeshRenderData(RenderContext& ctx, RenderData& data, SkinnedMeshRendererComponent& c, TransformComponent& t, InfoComponent& info, entt::entity e){
    data.layer = info.layer;

    data.distance = math::distance2(ctx.GetCamera().viewPos, t.Position());
    data.targetMaterial = c.material.get();
    data.targetMesh = c.mesh.get();
    data.targetMatrix =  t.GlobalModelMatrix();
    //data.transform = Transform(data.targetMatrix); //t.ToTransform();
    
    //INFO: Try optimize
    if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
        c.finalPose.GetMatrixPalette(c.posePalette, c.skeleton.GetInvBindPose());
    } 
    data.posePalette = &c.posePalette;
    
    //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
    data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

    data.perDrawData.Int_0_SetMask(0, true);
    data.perDrawData.Int_0_SetMask(1, true);
    data.perDrawData.int_0[0] = ((int)e) + 1;
    data.perDrawData.int_0[1] = info.layer;

    data.SetFlag(RenderData::Flag::FromSkinnedModel, true);

    if(c.useCustomData){
        data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
        data.perDrawData.vector4_0[0] = c.customData;
    }

    Vector4 perInstanceData = {0, 0, 0, float(info.layer)};
    SetPerInstanceData(data.targetMatrix, perInstanceData);

    #if EnableExperimentalPerDrawCustomData
    data.useCustomData = c.useCustomData;
    data.customData = c.customData;
    #endif
}

inline void FillModelRenderData(RenderContext& ctx, RenderData& data, ModelRendererComponent& c, TransformComponent& t, InfoComponent& info, entt::entity e, Model* model, const Model::RenderTarget& target){
    data.layer = info.layer;

    data.distance = math::distance2(ctx.GetCamera().viewPos, t.PositionReadSafe());// t.Position());
    data.targetMaterial = model->materials[target.materialIndex].get();
    data.targetMesh = model->meshs[target.meshIndex].get();
    //data.targetMatrix = t.GlobalModelMatrix() * c.localTransform.GetModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
    data.targetMatrix = math::simdMul(t.GlobalModelMatrix(), c.finalPose.GetGlobalMatrix(target.bindPoseIndex));// model->skeleton.GetBindPose().GetGlobalMatrix(target.bindPoseIndex));
    data.posePalette = nullptr;
    //data.aabb = c.GetGlobalAABB(t);
    data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix()); //data.targetMatrix); //Isto pode esta errado pq o aabb é do model interior, nao por mesh
    //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());
    //data.aabb.Expand2(Vector3(5.5f));
    if(target.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[target.materialIndex] != nullptr){
        data.targetMaterial = c.GetMaterialsOverride()[target.materialIndex].get();
    }

    data.perDrawData.Int_0_SetMask(0, true);
    data.perDrawData.Int_0_SetMask(1, true);
    data.perDrawData.int_0[0] = ((int)e) + 1;
    data.perDrawData.int_0[1] = info.layer;

    if(c.useCustomData){
        data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
        data.perDrawData.vector4_0[0] = c.customData;
    }
    
    data.SetFlag(RenderData::Flag::IsStatic, false);
    data.SetFlag(RenderData::Flag::FromModel, true);

    Vector4 perInstanceData = {0, 0, 0, float(info.layer)};
    SetPerInstanceData(data.targetMatrix, perInstanceData);

    data.customShadowPass = c.customShadowPass != nullptr ? c.customShadowPass.get() : (data.targetMaterial->DepthPass() != -1 ? data.targetMaterial : nullptr); 
    
    if(c.castShadow == false) data.SetFlag(RenderData::Flag::RenderShadow, false);

    #if EnableExperimentalPerDrawCustomData
    data.useCustomData = c.useCustomData;
    data.customData = c.customData;
    #endif
}

inline void FillStaticModelRenderData(RenderContext& ctx, RenderData& data, StaticRendererComponent& s, ModelRendererComponent& c, TransformComponent& t, InfoComponent& info, entt::entity e, Model* model, const Model::RenderTarget& target, int index){
    data.layer = info.layer;

    data.distance = math::distance2(ctx.GetCamera().viewPos, t.Position());
    data.targetMaterial = model->materials[target.materialIndex].get();
    data.targetMesh = model->meshs[target.meshIndex].get();
    data.targetMatrix =  s.staticDatas[index].m;
    data.aabb = s.staticDatas[index].aabb;
    data.posePalette = nullptr;
    if(target.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[target.materialIndex] != nullptr){
        data.targetMaterial = c.GetMaterialsOverride()[target.materialIndex].get();
    }

    data.perDrawData.Int_0_SetMask(0, true);
    data.perDrawData.Int_0_SetMask(1, true);
    data.perDrawData.int_0[0] = ((int)e) + 1;
    data.perDrawData.int_0[1] = info.layer;

    data.SetFlag(RenderData::Flag::IsStatic, true);
    data.SetFlag(RenderData::Flag::FromModel, true);

    Vector4 perInstanceData = {0, 0, 0, float(info.layer)};
    SetPerInstanceData(data.targetMatrix, perInstanceData);

    if(c.useCustomData){
        data.perDrawData.Vector4_0_SetMask(0, true);//.resize(1);
        data.perDrawData.vector4_0[0] = c.customData;
    }

    #if EnableExperimentalPerDrawCustomData
    data.useCustomData = c.useCustomData;
    data.customData = c.customData;
    #endif
}

inline void FillSkinnedModelRenderData(RenderContext& ctx, RenderData& data, SkinnedModelRendererComponent& c, TransformComponent& t, InfoComponent& info, entt::entity e, Model* model, const Model::RenderTarget& target){
    data.layer = info.layer;
    
    data.distance = math::distance2(ctx.GetCamera().viewPos, t.Position());
    data.targetMaterial = model->materials[target.materialIndex].get();
    data.targetMesh = model->meshs[target.meshIndex].get();

    auto m1 = t.GlobalModelMatrix();
    auto m2 = model->skeleton.GetBindPose().GetGlobalMatrix(target.bindPoseIndex);

    //TODO: Finish this optimization, maybe add option to enable GetGlobalMatrix(i.bindPoseIndex)
    //data.targetMatrix =  t.GlobalModelMatrix()/** c.localTransform.GetModelMatrix()*/ * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
    //data.targetMatrix = t.GlobalModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
    data.targetMatrix = math::simdMul(t.GlobalModelMatrix(), model->skeleton.GetBindPose().GetGlobalMatrix(target.bindPoseIndex)); //TODO: Maybe use final pose on here
    //data.transform = Transform(data.targetMatrix); //t.ToTransform();
    
    //INFO: Try optimize
    if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
        c.finalPose.GetMatrixPalette(c.posePalette, model->skeleton.GetInvBindPose()); 
    }
    data.posePalette = &c.posePalette;
    data.skinnedBuffer = c.useSkinnedData == false ? nullptr : c.skinnedData.get(); //c.skinnedData == nullptr ? nullptr : (c.useSkinnedData ? c.skinnedData.get() : nullptr);
    
    //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
    data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);//Isto pode esta errado pq o aabb é do model interior, nao por mesh
    //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());

    if(target.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[target.materialIndex] != nullptr){
        data.targetMaterial = c.GetMaterialsOverride()[target.materialIndex].get();
    }

    data.perDrawData.Int_0_SetMask(0, true);
    data.perDrawData.Int_0_SetMask(1, true);
    data.perDrawData.int_0[0] = ((int)e) + 1;
    data.perDrawData.int_0[1] = info.layer;

    data.SetFlag(RenderData::Flag::FromSkinnedModel, true);

    data.customShadowPass = c.customShadowPass != nullptr ? c.customShadowPass.get() : (data.targetMaterial->DepthPass() != -1 ? data.targetMaterial : nullptr); 

    //TODO: Refactory perDrawData to avoid memory alocation
    if(c.useCustomData){
        data.perDrawData.Vector4_0_SetMask(0, true);
        data.perDrawData.vector4_0[0] = c.customData;
    }

    Vector4 perInstanceData = {0, 0, 0, float(info.layer)};
    SetPerInstanceData(data.targetMatrix, perInstanceData);

    #if EnableExperimentalPerDrawCustomData
    data.useCustomData = c.useCustomData;
    data.customData = c.customData;
    #endif

    if(c.updateWhenOffscreen) data.SetFlag(RenderData::Flag::AlwaysDraw, true);// .awalsDraw = true;
    if(c.castShadow == false) data.SetFlag(RenderData::Flag::RenderShadow, false);
}

inline void UpdateSkinnedData(SkinnedModelRendererComponent& skinned){
    if(skinned.skinnedData == nullptr){
        skinned.skinnedData = CreateRef<UniformBuffer>(sizeof(Matrix4) * MAX_BONES); //120);
    }
    //if(skinned.posePalette.size() == 0) continue;
    skinned.skinnedData->SetData(skinned.posePalette.data(), sizeof(Matrix4) * skinned.posePalette.size());
}

inline void FillDecalRenderData(RenderContext& ctx, RenderData& data, DecalRendererComponent& decal, InfoComponent& info, TransformComponent& trans, Model* decalMesh, entt::entity entity){
    data.distance = math::distance2(ctx.GetCamera().viewPos, trans.Position());
    data.targetMaterial = decal.material.get();
    data.customShadowPass = nullptr;
    data.targetMesh = decalMesh->meshs[0].get();

    data.perDrawData.Int_0_SetMask(0, true);
    data.perDrawData.Int_0_SetMask(1, true);
    data.perDrawData.int_0[0] = ((int)entity) + 1;
    data.perDrawData.int_0[1] = decal.customLayerIndex;
    
    if(decal.useCustomOffsetAndSize == false){
        data.targetMatrix = trans.GlobalModelMatrix();
        data.aabb = transform_aabb_optimized_abs_center_extents(
            AABB(Vector3Zero, 0.5f, 0.5f, 0.5f), data.targetMatrix
        );
    } else {
        Transform offsetTrans;
        offsetTrans.Position(decal.offset);
        offsetTrans.Scale(decal.size);
        data.targetMatrix = trans.GlobalModelMatrix() * offsetTrans.GetModelMatrix();
        /*data.aabb = transform_aabb_optimized_abs_center_extents(
            AABB(offsetTrans.Position(), offsetTrans.Scale().x/2, offsetTrans.Scale().y/2, offsetTrans.Scale().z/2), trans.GlobalModelMatrix()
        );*/
        data.aabb = transform_aabb_optimized_abs_center_extents(
            AABB(Vector3Zero, 0.5f, 0.5f, 0.5f), data.targetMatrix
        );
    }

    decal.perInstanceData.w = float(decal.customLayerIndex);
    SetPerInstanceData(data.targetMatrix, decal.perInstanceData);

    data.SetFlag(RenderData::Flag::IsDecal, true);// .isDecal = true;
    data.SetFlag(RenderData::Flag::RenderShadow, false);// .renderShadow = false;
}
/////////////////////////////////////////

void RenderContext::UpdateRenderData(Scene& scene, RendererFeatureContext& passCtx){
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData");

    for(int i = 0; i < renderData.chunk_count(); i++){
        renderData[i].reserve(2000);
        renderData[i].clear();
    }

    auto& taskflow = scene.GetTaskflow();

    {
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::OnCollectRenderData");
    std::vector<RenderData>& outRenderData = renderData[0];  
    for(auto& i: passCtx.cachedRenderFeatures){
        i->OnCollectRenderData(scene, *this, outRenderData);
    }
    }

    /*{
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::OnCollectRenderData");
    for(auto& i: cachedRenderFeatures){
        i->OnCollectRenderData(*this, renderData);
    }
    }*/

    {
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::Mesh");
    auto meshView = scene.GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    tf_for_each3(scene.GetTaskflow(), meshView.begin(), meshView.end(), renderData.chunk_count(), [&](auto e, int taskIndex){
        auto [c, t, info] = meshView.get<MeshRendererComponent,TransformComponent,InfoComponent>(e);
        if(!info.enable || !c.mesh || !c.material) return;

        RenderData& data = renderData.GetNew(taskIndex);
        FillMeshRenderData(*this, data, c, t, info, e);
    });
    scene.RunAllTaskAndSync();
    }

    {
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::Model");
    auto meshRenderView = scene.GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    tf_for_each3(scene.GetTaskflow(), meshRenderView.begin(), meshRenderView.end(), renderData.chunk_count(), [&](auto e, int taskIndex){
        auto [c, t, info] = meshRenderView.get<ModelRendererComponent,TransformComponent,InfoComponent>(e);
        if(info.enable == false || c.draw == false || c.model == nullptr) return;

        Ref<Model> model = c.GetModel();
        if(c.finalPose.Size() != model->skeleton.GetBindPose().Size()) c.finalPose = model->skeleton.GetBindPose();

        Assert(c.GetRenderTargetVisibility().size() == model->renderTargets.size());
        for(int i = 0; i < model->renderTargets.size(); i++){  //for(auto i: model->renderTargets){
            auto& target = model->renderTargets[i];
            if(c.GetRenderTargetVisibility()[i] == false) continue;
            //if(i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[i] == false) continue;

            RenderData& data = renderData.GetNew(taskIndex); 
            FillModelRenderData(*this, data, c, t, info, e, model.get(), target);
        }
    });
    scene.RunAllTaskAndSync();
    }

    {
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::SkinnedMesh");
    auto skinnedMeshView = scene.GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    tf_for_each3(scene.GetTaskflow(), skinnedMeshView.begin(), skinnedMeshView.end(), renderData.chunk_count(), [&](auto e, int taskIndex){
        auto [c, t, info] = skinnedMeshView.get<SkinnedMeshRendererComponent, TransformComponent, InfoComponent>(e);
        if(info.enable == false || c.mesh == nullptr || c.material == nullptr) return; //continue;

        RenderData& data = renderData.GetNew(taskIndex);
        FillSkinnedMeshRenderData(*this, data, c, t, info, e);
    });
    scene.RunAllTaskAndSync();
    }

    {
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::SkinnedModel");
    auto skinnedView = scene.GetRegistry().view<SkinnedModelRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    tf_for_each3(scene.GetTaskflow(), skinnedView.begin(), skinnedView.end(), renderData.chunk_count(), [&](auto e, int taskIndex){
        auto [c, t, info] = skinnedView.get<SkinnedModelRendererComponent,TransformComponent,InfoComponent>(e);
        if(info.enable == false || c.draw == false || c.model == nullptr) return; //continue;

        Ref<Model> model = c.GetModel();
        if(model == nullptr) return; //continue;

        //TODO: Revisar isto, fix temporariamente o model nao esta send renderizando sem chama UpdatePosePalette
        if(c.posePalette.size() == 0) c.UpdatePosePalette();

        for(int i = 0; i < model->renderTargets.size(); i++){  //for(auto i: model->renderTargets){
            auto& target = model->renderTargets[i];
            if(c.GetRenderTargetVisibility()[i] == false) continue;
            //if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;
            
            RenderData& data = renderData.GetNew(taskIndex);
            FillSkinnedModelRenderData(*this, data, c, t, info, e, model.get(), target);
        }
    });
    scene.RunAllTaskAndSync();

    auto skinnedView2 = scene.GetRegistry().view<SkinnedModelRendererComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto [entity, skinned]: skinnedView2.each()){
        if(skinned.useSkinnedData == false) continue;
        UpdateSkinnedData(skinned);
        
        /*if(skinned.skinnedData == nullptr){
            skinned.skinnedData = CreateRef<UniformBuffer>(sizeof(Matrix4) * MAX_BONES); //120);
        }
        //if(skinned.posePalette.size() == 0) continue;
        skinned.skinnedData->SetData(skinned.posePalette.data(), sizeof(Matrix4) * skinned.posePalette.size());*/
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::Decal");
    auto decalView = scene.GetRegistry().view<DecalRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<HideInEditor, SelfDisable, SkipDraw>
    );
    tf_for_each3(scene.GetTaskflow(), decalView.begin(), decalView.end(), renderData.chunk_count(), [&](auto entity, int taskIndex){
        auto [decal, trans, info] = decalView.get<DecalRendererComponent, TransformComponent, InfoComponent>(entity);

        RenderData& data = renderData.GetNew(taskIndex);
        FillDecalRenderData(*this, data, decal, info, trans, decalMesh.get(), entity);
    });
    scene.RunAllTaskAndSync();
    }

    ///////////////////////////////////////////////

    {
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::StaticMesh");
    auto staticMeshView = scene.GetRegistry().view<MeshRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    tf_for_each3(scene.GetTaskflow(), staticMeshView.begin(), staticMeshView.end(), renderData.chunk_count(), [&](auto e, int taskIndex){
        auto [c, t, s, info] = staticMeshView.get<MeshRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(e);
        if(info.enable == false || c.mesh == nullptr || c.material == nullptr) return; //continue;

        if(s.staticDatas.size() != 1) s.staticDatas.resize(1);
        if(s.staticDatas[0].isDirt){
            s.staticDatas[0].isDirt = false;
            s.staticDatas[0].m = t.GlobalModelMatrix();
            s.staticDatas[0].aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, s.staticDatas[0].m);
        }

        RenderData& data = renderData.GetNew(taskIndex);
        FillStaticMeshRenderData(*this, data, s, c, t, info, e);
    });
    scene.RunAllTaskAndSync();
    }

    {
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::StaticModel");
    auto meshStaticRenderView = scene.GetRegistry().view<ModelRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    tf_for_each3(scene.GetTaskflow(), meshStaticRenderView.begin(), meshStaticRenderView.end(), renderData.chunk_count(), [&](auto e, int taskIndex){
        auto [c, t, s, info] = meshStaticRenderView.get<ModelRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(e);
        if(info.enable == false || c.draw == false || c.model == nullptr) return; //continue;

        Ref<Model> model = c.GetModel();

        if(s.staticDatas.size() != model->renderTargets.size()){
            s.staticDatas.resize(model->renderTargets.size());
            for(auto& i: s.staticDatas) i.isDirt = true;
        }

        int _i = 0;
        for(int i = 0; i < model->renderTargets.size(); i++){  //for(auto i: model->renderTargets){
            auto& target = model->renderTargets[i];
            if(c.GetRenderTargetVisibility()[i] == false) continue;

            if(s.staticDatas[_i].isDirt){
                s.staticDatas[_i].isDirt = false;
                s.staticDatas[_i].m = math::simdMul(t.GlobalModelMatrix(), c.finalPose.GetGlobalMatrix(target.bindPoseIndex)); //t.GlobalModelMatrix();
                s.staticDatas[_i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), s.staticDatas[_i].m);
            }

            RenderData& data = renderData.GetNew(taskIndex);
            FillStaticModelRenderData(*this, data, s, c, t, info, e, model.get(), target, _i);
            _i += 1;
        }
    });
    scene.RunAllTaskAndSync();
    }

    /////////////////////////////////////////////

    {
    //TODO: Update this to use FillxxxRenderData function
    OD_PROFILE_SCOPE("RenderContext::UpdateRenderData::StaticRendererCluster");
    auto staticRendererClusterView = scene.GetRegistry().view<StaticRendererClusterComponent, TransformComponent, InfoComponent>(
        entt::exclude<HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto [entity, c, t, i]: staticRendererClusterView.each()){
        for(auto& chunk: c.chunks){
            //if(chunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

            /*
            for(auto* r : receivers){
                uint32_t wants = r->Wants(...);
                if(wants){ 
                    interested.push_back(r);
                }
            }
            */

            for(auto& subchunk: chunk.subchunks){
                //if(subchunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

                /*
                for(auto* r : interested){
                    uint32_t wants = r->Wants(...);
                    if(wants == false){ 
                        interested.remove(r);
                    }
                }
                */

                for(auto& renderTarget: subchunk.targets){
                    RenderData& data = renderData.GetNew(0); //RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = renderTarget.targetMaterial.get();
                    data.customShadowPass = nullptr;
                    data.targetMesh = renderTarget.targetMesh.get();
                    data.targetMatrix = renderTarget.targetMatrix;
                    data.posePalette = nullptr;
                    data.aabb = renderTarget.aabb;

                    /*
                    for(auto* r : interested){
                        uint32_t wants = r->Wants(...);
                        if(wants == false){ 
                            interested.remove(r);
                        }
                    }
                    */

                    //data.perDrawData.int_0_Count = 0;//.resize(1);//TODO: Optimaze this, this can be make heap allocation
                    //data.perDrawData.int_0[0] = 0;
                    //data.perDrawData.int_0[1] = 0;//GetLayerIndex(info.layer);

                    #if EnableExperimentalPerDrawCustomData
                    data.useCustomData = c.useCustomData;
                    data.customData = c.customData;
                    #endif
                }

                subchunk.drawIntancingCommands.Each([&](DrawInstancingCommand2& cmd){
                    RenderData& data = renderData.GetNew(0); //RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = cmd.material;
                    data.customShadowPass = nullptr;
                    data.targetMesh = cmd.meshs;
                    data.targetMatrix = Matrix4Identity;
                    data.posePalette = nullptr;
                    data.aabb = subchunk.renderBounds;
                    //data.perDrawData.int_0_Count = 0; //.clear();
                    data.instancingBuffer = cmd.buffer.get();

                    data.SetFlag(RenderData::Flag::IsStatic, true);
                    data.SetFlag(RenderData::Flag::FromCluster, true);
                });
            }
        }
    }
    }
}

void RenderContext::RenderDataLoopNew(std::function<void(RenderData&)> onReciveRenderData){
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoopNew");
    for(int i = 0; i < renderData.chunk_count(); i++){
        for(auto& renderData: renderData[i]){
            onReciveRenderData(renderData);
        }
    }
}

void RendererFeatureContext::SetupFeatures(std::vector<RendererFeature*>& features){
    cachedRenderFeatures.clear();
    
    for(auto i: localRenderFeatures){
        if(i->enable == false) continue;
        cachedRenderFeatures.push_back(i);
    }
    for(auto i: features){
        if(i->enable == false) continue;
        cachedRenderFeatures.push_back(i);
    }
}

void RendererFeatureContext::FeaturesRunAddRenderPasses(RenderContext& context){    
    for(auto i: cachedRenderFeatures){
        i->AddRenderPasses(*this, context);
    }
    for(auto& i: renderPasses){
        std::stable_sort(i.begin(), i.end(), [](RenderPass* a, RenderPass* b){
            return a->priority < b->priority;
        });
    }
}

inline bool HasAllTags(const std::vector<uint32_t>& required, const std::vector<uint32_t>& tags){
    //if(required.size() > tags.size()) return false;
    if(tags.size() <= 0) return false;
    for(uint32_t r : required){
        bool found = false;
        for(uint32_t t : tags){
            if(t == r){
                found = true;
                break;
            }
        }
        if(!found) return false;
    }
    return true;
}

//Maybe more fast
inline bool HasAllTags2(const std::vector<uint32_t>& required, const std::vector<uint32_t>& tags){
    //if(required.size() > tags.size()) return false;
    if(tags.size() <= 0) return false;
    for(uint32_t r : required){
        bool found = false;
        for(uint32_t t : tags){
            found |= (t == r);
        }
        if(!found) return false;
    }
    return true;
}

inline bool HasAnyTags(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b){
    if(a.empty() || b.empty()) return false;
    for(uint32_t x : a){
        for(uint32_t y : b){
            if(x == y) return true;
        }
    }
    return false;
}

void RenderContext::AddDrawRenderers(RenderData& data, DrawingSettings& settings, RendererList& target){
    Assert(data.targetMaterial != nullptr);

    if(data.HasFlag(RenderData::Flag::IsDecal) && settings.decalTarget == false) return;
    if(settings.decalTarget && data.HasFlag(RenderData::Flag::IsDecal) == false) return;

    //TODO: Experimental, Messure the performace Later
    if(settings.requiredTags.size() > 0){
        if(HasAllTags2(settings.requiredTags, data.targetMaterial->TagsHash()) == false) return;
    }

    //TODO: Experimental, Messure the performace Later
    if(settings.excludedTags.size() > 0){
        if(HasAnyTags(settings.excludedTags, data.targetMaterial->TagsHash()) == true) return;
    }

    bool isBlend = data.targetMaterial->IsBlend();
    bool isInstancing = data.targetMaterial->EnableInstancingValid();
    if(settings.enableIntancing == false){
        isInstancing = false;
    }
    if(settings.renderQueueRange == RenderQueueRange::Transparent){
        if(isBlend == false) return;
        //isInstancing = false;
    }
    if(settings.renderQueueRange == RenderQueueRange::Opaue){
        if(isBlend == true) return;
    }

    /*if(data.HasFlag(RenderData::Flag::IsDecal) != settings.decalTarget) return;// --- Early very cheap branch ---
    
    Material* mat = data.targetMaterial;   
    const bool isBlend = mat->IsBlend();
    const bool wantBlend = settings.renderQueueRange == RenderQueueRange::Transparent;
    if(isBlend != wantBlend) return;// FAST render-queue check

    bool isInstancing = mat->EnableInstancingValid() && settings.enableIntancing;*/

    if(data.posePalette != nullptr){
        target.AddSkinnedDrawCommand({
            data.targetMatrix,
            data.perDrawData,
            data.targetMaterial->CurrentShader().drawTypes[1].get(),
            data.targetMaterial,
            data.targetMesh,
            data.posePalette,
            data.skinnedBuffer
        }, data.distance);
        return;
    }

    if(isInstancing){
        if(data.instancingBuffer != nullptr){
            target.AddDrawInstancingCommand({
                data.instancingBuffer, data.targetMaterial->CurrentShader().drawTypes[2].get(), data.targetMaterial, data.targetMesh, data.distance
            });
        } else{
            target.AddDrawInstancingCommand({
                data.targetMatrix,
                data.targetMaterial->CurrentShader().drawTypes[2].get(),
                data.targetMaterial,
                data.targetMesh,
            });
        }
    } else {
        target.AddDrawCommand({
            data.targetMatrix,
            data.perDrawData,
            data.targetMaterial->CurrentShader().drawTypes[0].get(),
            data.targetMaterial,
            data.targetMesh,
            data.distance
            /*#if EnableExperimentalPerDrawCustomData
            data.useCustomData,
            data.customData,
            #endif*/
        }, data.distance);
    } 
}

void RenderContext::RenderSkyboxLater(Scene& scene){
    OD_PROFILE_SCOPE("RenderContext::RenderSkybox"); 
    if(skyMaterial == nullptr) return;
    
    Assert(skyMaterial->GetShader() != nullptr);

    //skyMaterial->UpdateDatas();
    //Material::SubmitGraphicDatas(*skyMaterial);

    /*Graphics::SetCullFace(CullFace::BACK);
    //Graphics::SetDepthMask(false);
    Graphics::SetDepthTest(DepthTest::LESS_EQUAL);
    Graphics::SetBlend(false);
    Shader::Bind(*skyMaterial->GetShader());*/
    
    //environmentSettings.sky->shader()->SetCubemap("mainTex", *_skyboxCubemap, 0);
    /*skyMaterial->GetShader()->SetMatrix4("projection", cam.projection);
    Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
    skyMaterial->GetShader()->SetMatrix4("view", skyboxView);
    Graphics::DrawMeshRaw(*skyboxMesh);*/

    Vector3 lightDir = {1, 1, 1};
    Vector4 lightColor = {1, 1, 1, 1};

    auto view = scene.GetRegistry().view<TransformComponent, LightComponent>();
    for(auto [entity, trans, light]: view.each()){
        if(light.type != LightComponent::Type::Directional) continue;
        lightDir = trans.Forward();
        lightColor = light.color;
        break;
    }

    //skyMaterial->SetMatrix4("projection", cam.projection);
    Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
    skyMaterial->SetMatrix4("skyboxView", skyboxView);
    skyMaterial->SetVector3("lightDir", -lightDir);
    skyMaterial->SetColor3("lightColor", lightColor);
    Graphics::DrawMesh(*skyboxMesh, *skyMaterial, Matrix4Identity);

    //Graphics::SetDepthTest(DepthTest::LESS);
    //Graphics::SetDepthMask(true);
}

void RenderContext::DrawRenderersBuffer(RendererList& commandBuffer, bool sort, bool deferred, bool isDecal){
    OD_PROFILE_SCOPE("RenderContext::DrawRenderersBuffer");

    //if(sort) 
    commandBuffer.Sort();
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        //if(material.GetShader() == nullptr) return;
        //SetStandUniforms(cam, *material.GetShader()); 

        //Graphics::SetDepthTest(DepthTest::EQUAL);

        if(material.MainPass() != -1) material.SetPass(material.MainPass());

        if(deferred){
            material.EnableKeyword("Deferred");
        } else {
            material.EnableKeyword("Forward");
        }

        if(isDecal){
            Ref<Framebuffer> deferred = deferredOutColor;
            Ref<Framebuffer> deferredCopy = deferredOutColorCopy;
            //decal.material->SetMatrix4("decalWorldToLocal", math::inverse(trans.GlobalModelMatrix()));
            //material.SetTexture("gPosition", deferred, 0);
            material.SetTexture("gNormal", deferredCopy, 0);
            material.SetTexture("gAlbedoSpec", deferredCopy, 1);
            material.SetTexture("gOther", deferredCopy, 2);
            material.SetTexture("gDepth", deferred, -1);
        }

    };
    commandBuffer.Submit();
    //commandBuffer.onUpdateMaterial = nullptr;
}

void RenderContext::DrawZPreePassRenderersBuffer(RendererList& commandBuffer, bool sort, bool post){
    OD_PROFILE_SCOPE("RenderContext::DrawZPreePassRenderersBuffer");

    //if(sort) 
    commandBuffer.Sort();
    commandBuffer.Submit();
}

//void _DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color);

void RenderContext::DrawGizmos(Scene& scene){
    OD_PROFILE_SCOPE("RenderContext::DrawGizmos"); 

    if(SceneManager::Get().GetActiveScene() == nullptr) return;
    if(SceneManager::Get().GetActiveScene()->Running() && settings.enableGizmosRuntime == false) return;
    if(SceneManager::Get().GetActiveScene()->Running() == false && settings.enableGizmos == false) return;

    //Renderer::SetCamera(cam);
    
    Camera cm = cam;
    Ref<Framebuffer> curFinalColor = customFinalColor != nullptr ? customFinalColor : finalColor;
    
    //scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    //FIXME: This could be call OnDrawGizmos Twice
    /*for(System* s: scene->GetPhysicsSystems()) s->OnDrawGizmos(cm);
    for(System* s: scene->GetStandSystems()) s->OnDrawGizmos(cm);
    for(System* s: scene->GetLateSystems()) s->OnDrawGizmos(cm);
    for(System* s: scene->GetRendererSystems()) s->OnDrawGizmos(cm);*/
    for(auto& s: scene.GetSystems()) s.second->OnDrawGizmos(scene, cm);

    Editor* editor = Application::GetModuleByType<Editor>();
    if(editor != nullptr){
        //if(scene->IsValid(editor->GetSelectionEntity())){
            /*for(System* s: scene->GetPhysicsSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
            for(System* s: scene->GetStandSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
            for(System* s: scene->GetLateSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
            for(System* s: scene->GetRendererSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());*/
        //    for(auto& s: scene->GetSystems()) s.second->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
        //}

        for(auto& i: editor->GetSelectedEntities()){
            if(scene.IsValid(i) == false) continue;
            for(auto& s: scene.GetSystems()) s.second->OnDrawGizmosSelected(scene, cm, i);
        }
    }

    //_DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,0,0));

    auto cameraView = scene.GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto e: cameraView){
        auto& c = cameraView.get<CameraComponent>(e);
        auto& t = cameraView.get<TransformComponent>(e);

        c.UpdateCameraData(t, curFinalColor->Width(), curFinalColor->Height());
        cm = c.GetCamera(); //Camera cm = c.GetCamera();
        //_DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,1,1));
        Gizmos::DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,1,1));
    }

    /*auto meshRenderView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<MeshRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;

        AABB aabb = c.boundingVolume;
        AABB globalAABB = c.GetGlobalAABB(t);

        Vector3 color = Vector3(0,0,1);
        Transform _t = t.ToTransform();
        if(aabb.isOnFrustum(cm.frustum, _t)) color = Vector3(1, 0, 0);

        Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
    }

    auto modelRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>();
    for(auto e: modelRenderView){
        auto& c = modelRenderView.get<ModelRendererComponent>(e);
        auto& t = modelRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        Transform globalTransform = Transform(t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix());

        AABB aabb = c.GetAABB();
        AABB globalAABB = c.GetGlobalAABB(globalTransform);

        Vector3 color = Vector3(0,0,1);
        if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(1, 0, 0);

        Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
    }

    auto skinnedModelRenderView = scene->GetRegistry().view<SkinnedModelRendererComponent, TransformComponent>();
    for(auto e: skinnedModelRenderView){
        auto& c = skinnedModelRenderView.get<SkinnedModelRendererComponent>(e);
        auto& t = skinnedModelRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        Transform globalTransform = Transform(t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix());

        AABB aabb = c.GetAABB();
        AABB globalAABB = c.GetGlobalAABB(globalTransform);

        Vector3 color = Vector3(0,0,1);
        if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(1, 0, 0);

        Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
    }

    auto animView = scene->GetRegistry().view<AnimatorComponent, SkinnedModelRendererComponent, TransformComponent>();
    for(auto e: animView){
        auto& s = animView.get<SkinnedModelRendererComponent>(e);
        auto& c = animView.get<AnimatorComponent>(e);
        auto& t = animView.get<TransformComponent>(e);
        if(s.GetModel() == nullptr) continue;

        Transform globalTransform = Transform(
            t.GlobalModelMatrix() * s.localTransform.GetLocalModelMatrix() * s.skeletonTransform.GetLocalModelMatrix() * s.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(0)
        );

        Pose pose;
        if(scene->Running()){ 
            pose = c.controller.GetCurrentPose();
        } else {
            pose = s.GetModel()->skeleton.GetBindPose(); 
        }

        for(int i = 0; i < pose.Size(); i++){
            if(pose.GetParent(i) < 0) continue;
            Vector3 p0 = globalTransform.TransformPoint( pose.GetGlobalTransform(i).LocalPosition() );
            Vector3 p1 = globalTransform.TransformPoint( pose.GetGlobalTransform(pose.GetParent(i)).LocalPosition() );
            Graphics::DrawLine(p0, p1, Vector3(0, 0, 1), 1);

            Graphics::DrawWireCube(Transform(p0, QuaternionIdentity, Vector3(0.05f)).GetLocalModelMatrix(), Vector3(0, 0, 1), 1);
            Graphics::DrawWireCube(Transform(p1, QuaternionIdentity, Vector3(0.025f)).GetLocalModelMatrix(), Vector3(1, 0, 0), 1);
        }
    }

    auto drawGizmosView = scene->GetRegistry().view<GizmosDrawComponent, TransformComponent>();
    for(auto e: drawGizmosView){
        auto& g = drawGizmosView.get<GizmosDrawComponent>(e);
        auto& t = drawGizmosView.get<TransformComponent>(e);
        Graphics::DrawWireCube(Transform(t.Position(), t.Rotation(), g.globalScale).GetLocalModelMatrix(), Vector3(0, 1, 0), 1);
    }
    */

    /*auto navmeshView = scene->GetRegistry().view<NavmeshComponent>();
    for(auto e: navmeshView){
        auto& n = navmeshView.get<NavmeshComponent>(e);
        if(n.navmesh == nullptr) continue;
        n.navmesh->DrawDebug();
    }*/
}

void RenderContext::CleanShadow(Framebuffer& shadowMap, int layer){
    //Framebuffer::Bind(*shadowMap, layer);
    Graphics::BeginFramebuffer(shadowMap, true, Vector4(0, 0, 0, 1), layer);
    Graphics::SetViewport(0, 0, shadowMap.Width(), shadowMap.Height());
    Graphics::Clean(1, 1, 1, 1);
    //Framebuffer::Unbind();
    Graphics::EndFramebuffer();
}

void RenderContext::BeginDrawShadow(Framebuffer& shadowMap, int layer){
    //Framebuffer::Bind(*shadowMap, layer);
    Graphics::BeginFramebuffer(shadowMap, true, Vector4(0, 0, 0, 1), layer);
    Graphics::SetViewport(0, 0, shadowMap.Width(), shadowMap.Height());
    Graphics::Clean(1, 1, 1, 1);
}

void RenderContext::EndDrawShadow(){
    //Framebuffer::Unbind();
    Graphics::EndFramebuffer();
}

void RenderContext::AddDrawShadow(RenderData& data, ShadowDrawingSettings& settings, RendererList& commandBuffer){
    bool isBlend = data.targetMaterial->IsBlend();
    bool isInstancing = data.targetMaterial->EnableInstancingValid();
    if(settings.enableIntancing == false){
        isInstancing = false;
    }

    if(settings.renderQueueRange == RenderQueueRange::Transparent){
        if(isBlend == false) return;
        isInstancing = false;
    }
    if(settings.renderQueueRange == RenderQueueRange::Opaue){
        if(isBlend == true) return;
    }

    if(data.posePalette != nullptr){
        commandBuffer.AddSkinnedDrawCommand({
            data.targetMatrix,
            data.perDrawData,
            data.customShadowPass->CurrentShader().drawTypes[1].get(),
            data.customShadowPass, 
            //data.targetMaterial,
            data.targetMesh,
            data.posePalette,
            data.skinnedBuffer
        }, data.distance);
        return;
    }

    if(isInstancing){
        if(data.instancingBuffer != nullptr){
            commandBuffer.AddDrawInstancingCommand({
                data.instancingBuffer, 
                data.customShadowPass->CurrentShader().drawTypes[2].get(),
                data.customShadowPass, //data.targetMaterial, 
                data.targetMesh,
                data.distance
            });
        } else{
            commandBuffer.AddDrawInstancingCommand({
                data.targetMatrix,
                data.customShadowPass->CurrentShader().drawTypes[2].get(),
                data.customShadowPass, 
                //data.targetMaterial,
                data.targetMesh
            });
        }

    } else {
        /*commandBuffer.AddDrawCommand({
            data.targetMatrix,
            data.customShadowPass, 
            //data.targetMaterial,
            data.targetMesh,
            data.distance
        }, data.distance);*/

        commandBuffer.AddDrawCommand({
            data.targetMatrix,
            data.perDrawData,
            data.customShadowPass->CurrentShader().drawTypes[0].get(),
            data.customShadowPass,
            data.targetMesh,
            data.distance
            /*#if EnableExperimentalPerDrawCustomData
            data.useCustomData,
            data.customData,
            #endif*/
        }, data.distance);
    } 
}

void RenderContext::DrawShadows(RendererList& commandBuffer, ShadowSplitData& splitData, Ref<Material>& shadowPass){
    OD_PROFILE_SCOPE("RenderContext::DrawShadows");
    commandBuffer.Sort();
    //commandBuffer.SetOverrideMaterial(shadowPass);

    //Material::SetGlobalMatrix4("lightSpaceMatrix", splitData.projViewMatrix);

    shadowData.lightSpaceMatrix = splitData.projViewMatrix;
    pipelineDataBuffer->SetData(&shadowData, sizeof(ShadowData), 0);
    Material::SetGlobalUniformBuffer("ShadowData", pipelineDataBuffer, 0);
 
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        //Shader::SetMatrix4("lightSpaceMatrix", splitData.projViewMatrix);
        if(material.DepthPass() != -1){
            material.SetPass(material.DepthPass());
            //LogInfo("Set Shadow Pass of: %s", material.GetShader()->Path().c_str());
        }
    };
    commandBuffer.Submit();
    commandBuffer.onUpdateMaterial = nullptr;
    commandBuffer.SetOverrideMaterial(nullptr);
}

/*
Vector3 _Plane3Intersect(Plane p1, Plane p2, Plane p3){ //get the intersection point of 3 planes
    return ( ( -p1.n.w * math::cross( Vector3(p2.n), Vector3(p3.n) ) ) +
            ( -p2.n.w * math::cross( Vector3(p3.n), Vector3(p1.n) ) ) +
            ( -p3.n.w * math::cross( Vector3(p1.n), Vector3(p2.n) ) ) ) /
        ( math::dot( Vector3(p1.n), math::cross( Vector3(p2.n), Vector3(p3.n) ) ) );
}

// Source: https://forum.unity.com/threads/drawfrustum-is-drawing-incorrectly.208081/
void _DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color = Vector3(1,1,1)){
    Vector3 nearCorners[4]; //Approx'd nearplane corners
    Vector3 farCorners[4]; //Approx'd farplane corners
    Plane camPlanes[6];
    camPlanes[0] = frustum.leftFace;
    camPlanes[1] = frustum.rightFace;
    camPlanes[2] = frustum.bottomFace;
    camPlanes[3] = frustum.topFace;
    camPlanes[4] = frustum.nearFace;
    camPlanes[5] = frustum.farFace;

    Plane temp = camPlanes[1]; camPlanes[1] = camPlanes[2]; camPlanes[2] = temp; //swap [1] and [2] so the order is better for the loop

    for(int i = 0; i < 4; i++){
        nearCorners[i] = _Plane3Intersect(camPlanes[4], camPlanes[i], camPlanes[(i + 1) % 4]); //near corners on the created projection matrix
        farCorners[i] = _Plane3Intersect(camPlanes[5], camPlanes[i], camPlanes[(i + 1) % 4]); //far corners on the created projection matrix
    }

    for(int i = 0; i < 4; i++){
        Graphics::DrawLine(model, nearCorners[i], nearCorners[( i + 1 ) % 4], color, 1); //near corners on the created projection matrix
        Graphics::DrawLine(model, farCorners[i], farCorners[( i + 1 ) % 4], color, 1); //far corners on the created projection matrix
        Graphics::DrawLine(model, nearCorners[i], farCorners[i], color, 1); //sides of the created projection matrix
    }
}
*/

std::vector<Vector4> getFrustumCornersWorldSpace2(const Matrix4& proj, const Matrix4& view){
    const auto inv = math::inverse(math::simdMul(proj, view));
    
    std::vector<Vector4> frustumCorners;
    for(unsigned int x = 0; x < 2; ++x){
        for(unsigned int y = 0; y < 2; ++y){
            for(unsigned int z = 0; z < 2; ++z){
                const Vector4 pt = inv * Vector4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                //const Vector4 pt = math::simdMul(inv, Vector4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f));
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    
    return frustumCorners;
}

glm::mat4 getLightSpaceMatrix2(Camera& cam, Vector3 lightDir, const float nearPlane, const float farPlane, Frustum* outFrustom = nullptr){
    const auto proj = glm::perspective(cam.fov, (float)cam.width / (float)cam.height, nearPlane, farPlane);
    const auto corners = getFrustumCornersWorldSpace2(proj, cam.view);

    glm::vec3 center = glm::vec3(0, 0, 0);
    for(const auto& v : corners){
        center += glm::vec3(v);
    }
    center /= corners.size();

    const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

    if(outFrustom != nullptr){
        //*outFrustom = CreateFrustumFromMatrix(lightView,proj);
        Matrix4 viewProj = math::simdMul(lightView, proj);
        //*outFrustom = CreateFrustumFromMatrix2(math::transpose( math::simdMul(proj, lightView) ));
        *outFrustom = CreateFrustumFromMatrix(math::simdMul(proj, lightView));
    }

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for(const auto& v : corners){
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    // Tune this parameter according to the scene
    constexpr float zMult = 20; //10; //10.0f;
    if(minZ < 0){
        minZ *= zMult;
    } else {
        minZ /= zMult;
    }
    if(maxZ < 0){
        maxZ /= zMult;
    } else {
        maxZ *= zMult;
    }

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return math::simdMul(lightProjection, lightView);
}

void ShadowSplitData::SetupCascade(ShadowSplitData* splitData, int count, Camera& cam, Transform& light, std::vector<float>& shadowCascadeLevels){
    //float shadowDistance = cam.farClip;
    //shadowDistance = 500;

    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f, cam.farClip };
    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f };
    //std::vector<float> shadowCascadeLevels{ shadowDistance*0.1f, shadowDistance*0.25f, shadowDistance*0.5f, shadowDistance*1.0f };

    Assert(shadowCascadeLevels.size() == count);
    
    Vector3 lightDir = -light.Forward();
    float cameraNearPlane = cam.nearClip;
    float cameraFarPlane = cam.farClip;

    std::vector<glm::mat4> lightMatrixs;
    std::vector<Frustum> frustums;

    for(size_t i = 0; i < shadowCascadeLevels.size(); ++i){
        Frustum frustum;

        if(i == 0){
            lightMatrixs.push_back(getLightSpaceMatrix2(cam, lightDir, cameraNearPlane, shadowCascadeLevels[i], &frustum));
            frustums.push_back(frustum);
        }else if (i < shadowCascadeLevels.size()){
            lightMatrixs.push_back(getLightSpaceMatrix2(cam, lightDir, shadowCascadeLevels[i - 1], shadowCascadeLevels[i], &frustum));
            frustums.push_back(frustum);
        }
    }

    Assert(lightMatrixs.size() == count);

    for(int i = 0; i < count; i++){
        splitData[i].projViewMatrix = lightMatrixs[i];
        //splitData[i].splitDistance = shadowCascadeLevels[i];
        //splitData[i].frustum = frustums[i];
        //splitData[i].frustum = CreateFrustumFromMatrix2(math::transpose(lightMatrixs[i]));
        splitData[i].frustum = CreateFrustumFromMatrix(lightMatrixs[i]);
    }
}

void ShadowSplitData::ComputeSpotShadowData(ShadowSplitData* splitData, LightComponent& light, Transform& transform){
    auto lightProjection = glm::perspective(Mathf::Deg2Rad(light.coneAngleOuter*2), 1.0f, 0.1f, light.radius);
    auto lightView = glm::lookAt(transform.Position(), transform.Position() - (-transform.Forward()), Vector3Up);

    splitData->projViewMatrix = lightProjection * lightView;
    //splitData->frustum = CreateFrustumFromMatrix2(math::transpose(splitData->projViewMatrix));
    splitData->frustum = CreateFrustumFromMatrix(splitData->projViewMatrix);
}

void ShadowSplitData::ComputePointShadowData(ShadowSplitData* splitData, LightComponent& light, Transform& transform){
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light.radius); 
    Vector3 lightPos = transform.Position();

    std::vector<glm::mat4> shadowMats;
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

    for(int i = 0; i < 6; i++){
        splitData[i].projViewMatrix = shadowMats[i];
        //splitData[i].frustum = CreateFrustumFromMatrix2(math::transpose(shadowMats[i]));
        splitData[i].frustum = CreateFrustumFromMatrix(shadowMats[i]);
    }
}

}