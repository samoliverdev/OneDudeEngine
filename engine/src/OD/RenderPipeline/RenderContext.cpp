#include "RenderContext.h"
#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "OD/Core/Application.h"
#include "OD/Core/AssetManager.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Defines.h"
#include "OD/Physics/PhysicsSystem.h"

namespace OD{

RenderContext::RenderContext(Scene* inScene){
    scene = inScene;

    FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA16F}, {FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
    framebufferSpecification.sample = 2;
    outColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA8}};
    framebufferSpecification.sample = 1;
    finalColor = new Framebuffer(framebufferSpecification);

    blitShader = AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Blit.glsl");

    skyboxMesh = Mesh::SkyboxCube();
}

RenderContext::~RenderContext(){
    delete outColor;
    delete finalColor;
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

    outColor->Resize(width, height);
    finalColor->Resize(width, height);

    Framebuffer::Bind(*outColor);
}

void RenderContext::EndDrawToScreen(){
    Graphics::BlitFramebuffer(outColor, finalColor);
    //Renderer::BlitQuadPostProcessing(finalColor, nullptr, *blitShader);

    if(overrideFramebuffer != nullptr){
        Graphics::BlitQuadPostProcessing(finalColor, overrideFramebuffer, *blitShader);
    } else {
        Graphics::BlitQuadPostProcessing(finalColor, nullptr, *blitShader);
    }

    Framebuffer::Unbind();

    /*finalColor->Unbind();
    outColor->Unbind();
    if(overrideFramebuffer != nullptr) overrideFramebuffer->Unbind();
    */
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
}

void RenderContext::ScreenClean(){
    //Renderer::Clean(cam.cleanColor.x, cam.cleanColor.y, cam.cleanColor.z, 1);
    Graphics::Clean(0, 0, 1, 1);
}

void RenderContext::SetupLoop(std::function<void(RenderData&)> onReciveRenderData){
    //globalUniformBuffer->CleanData();

    auto meshRenderView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<MeshRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;
        //if(c.GetAABB().isOnFrustum(cam.frustum, t) == false) continue;

        for(auto i: c.GetModel()->renderTargets){
            RenderData data;
            data.transform = t;
            data.distance = math::distance(cam.viewPos, t.Position());
            data.targetMaterial = c.GetModel()->materials[i.materialIndex];
            data.targetMesh = c.GetModel()->meshs[i.meshIndex];
            data.targetMatrix =  t.GlobalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.posePalette = nullptr;
            data.aabb = c.GetAABB();
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex];
            }

            onReciveRenderData(data);
        }
    }

    auto skinnedView = GetScene()->GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent>();
    for(auto e: skinnedView){
        SkinnedMeshRendererComponent& c = skinnedView.get<SkinnedMeshRendererComponent>(e);
        TransformComponent& t = skinnedView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;
        //if(c.GetAABB().isOnFrustum(cam.frustum, t) == false) continue;

        for(auto i: c.GetModel()->renderTargets){
            RenderData data;
            data.transform = t;
            data.distance = math::distance(cam.viewPos, t.Position());
            data.targetMaterial = c.GetModel()->materials[i.materialIndex];
            data.targetMesh = c.GetModel()->meshs[i.meshIndex];
            data.targetMatrix =  t.GlobalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.posePalette = &c.posePalette;
            data.aabb = c.GetAABB();
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex];
            }

            onReciveRenderData(data);
        }
    }
}

void RenderContext::AddDrawRenderers(RenderData& data, DrawingSettings& settings, CommandBuffer& target){
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
        target.AddSkinnedDrawCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix,
            data.posePalette
        }, data.distance);
        return;
    }

    if(isInstancing){
        target.AddDrawInstancingCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix
        });

    } else {
        target.AddDrawCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix,
            data.distance
        }, data.distance);
    } 
}

void RenderContext::SetStandUniforms(Camera& cam, Shader& shader){
    //Shader::Bind(shader);
    shader.SetMatrix4("view", cam.view);
    shader.SetMatrix4("projection", cam.projection);
    shader.SetVector3("viewPos", cam.viewPos);
}

void RenderContext::RenderSkybox(){
    OD_PROFILE_SCOPE("RenderContext::RenderSkybox"); 
    
    if(skyMaterial != nullptr){
        Assert(skyMaterial->GetShader() != nullptr);

        //skyMaterial->UpdateDatas();
        Material::SubmitGraphicDatas(*skyMaterial);

        Graphics::SetCullFace(CullFace::BACK);
        Graphics::SetDepthMask(false);
        Graphics::SetBlend(false);
        Shader::Bind(*skyMaterial->GetShader());
        
        //environmentSettings.sky->shader()->SetCubemap("mainTex", *_skyboxCubemap, 0);
        skyMaterial->GetShader()->SetMatrix4("projection", cam.projection);
        Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
        skyMaterial->GetShader()->SetMatrix4("view", skyboxView);
        Graphics::DrawMesh(skyboxMesh);

        Graphics::SetDepthMask(true);
    }
}

void RenderContext::DrawRenderersBuffer(CommandBuffer& commandBuffer){
    commandBuffer.Sort();
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        if(material.GetShader() == nullptr) return;
        SetStandUniforms(cam, *material.GetShader()); 
    };
    commandBuffer.Submit();
    commandBuffer.onUpdateMaterial = nullptr;
}

void _DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color);

void RenderContext::DrawGizmos(){
    //Renderer::SetCamera(cam);
    Graphics::SetDepthTest(DepthTest::LESS);
    Graphics::SetCullFace(CullFace::BACK);
    Graphics::SetBlend(false);

    scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    Camera cm = cam;
    _DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,0,0));

    auto cameraView = scene->GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto e: cameraView){
        auto& c = cameraView.get<CameraComponent>(e);
        auto& t = cameraView.get<TransformComponent>(e);

        c.UpdateCameraData(t, finalColor->Width(), finalColor->Height());
        Camera cm = c.GetCamera();
        _DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,1,1));
    }

    auto meshRenderView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<MeshRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        AABB aabb = c.GetGlobalAABB(t);
        Graphics::DrawWireCube(Mathf::TRS(t.Position(), QuaternionIdentity, aabb.extents), Vector3(0,0,1), 1);
        //Renderer::DrawWireCube(Matrix4Identity, Vector3(0,1,0), 1);
    }

    auto skinnedMeshRenderView = scene->GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent>();
    for(auto e: skinnedMeshRenderView){
        auto& c = skinnedMeshRenderView.get<SkinnedMeshRendererComponent>(e);
        auto& t = skinnedMeshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        //LogInfo("Ifdfdfd22222222");

        AABB aabb = c.GetGlobalAABB(t);
        Graphics::DrawWireCube(Mathf::TRS(t.Position(), QuaternionIdentity, aabb.extents), Vector3(0,1,0), 1);
        //Renderer::DrawWireCube(Mathf::TRS(t.Position(), QuaternionIdentity, Vector3One), Vector3(0,1,0), 1);
    }
}

void RenderContext::BeginDrawShadow(Framebuffer* shadowMap, int layer){
    Assert(shadowMap != nullptr);

    Framebuffer::Bind(*shadowMap, layer);

    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());
    Graphics::Clean(1, 1, 1, 1);
}

void RenderContext::EndDrawShadow(){
    Framebuffer::Unbind();
}

void RenderContext::AddDrawShadow(RenderData& data, ShadowDrawingSettings& settings, CommandBuffer& commandBuffer){
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
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix,
            data.posePalette
        }, data.distance);
        return;
    }

    if(isInstancing){
        commandBuffer.AddDrawInstancingCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix
        });

    } else {
        commandBuffer.AddDrawCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix,
            data.distance
        }, data.distance);
    } 
}

void RenderContext::DrawShadows(CommandBuffer& commandBuffer, ShadowSplitData& splitData, Ref<Material>& shadowPass){
    commandBuffer.Sort();

    commandBuffer.SetOverrideMaterial(shadowPass);

    Material::SetGlobalMatrix4("lightSpaceMatrix", splitData.projViewMatrix);
 
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        //Shader::SetMatrix4("lightSpaceMatrix", splitData.projViewMatrix);
    };
    commandBuffer.Submit();
    commandBuffer.onUpdateMaterial = nullptr;
    commandBuffer.SetOverrideMaterial(nullptr);
}

Vector3 _Plane3Intersect(Plane p1, Plane p2, Plane p3){ //get the intersection point of 3 planes
    return ( ( p1.distance * math::cross( p2.normal, p3.normal ) ) +
            ( p2.distance * math::cross( p3.normal, p1.normal ) ) +
            ( p3.distance * math::cross( p1.normal, p2.normal ) ) ) /
        ( math::dot( p1.normal, math::cross( p2.normal, p3.normal ) ) );
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

std::vector<Vector4> getFrustumCornersWorldSpace2(const Matrix4& proj, const Matrix4& view){
    const auto inv = math::inverse(proj * view);
    
    std::vector<Vector4> frustumCorners;
    for(unsigned int x = 0; x < 2; ++x){
        for(unsigned int y = 0; y < 2; ++y){
            for(unsigned int z = 0; z < 2; ++z){
                const Vector4 pt = inv * Vector4(
                    2.0f * x - 1.0f, 
                    2.0f * y - 1.0f, 
                    2.0f * z - 1.0f, 
                    1.0f
                );
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
        Matrix4 viewProj = lightView * proj;
        *outFrustom = CreateFrustumFromMatrix2(viewProj);
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
    constexpr float zMult = 10.0f;
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
    return lightProjection * lightView;
}

void ShadowSplitData::SetupCascade(ShadowSplitData* splitData, int count, Camera& cam, Transform& light){
    float shadowDistance = cam.farClip;
    shadowDistance = 500;

    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f, cam.farClip };
    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f };
    std::vector<float> shadowCascadeLevels{ shadowDistance*0.1f, shadowDistance*0.25f, shadowDistance*0.5f, shadowDistance*1.0f };

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
        splitData[i].splitDistance = shadowCascadeLevels[i];
        splitData[i].frustum = frustums[i];
    }
}

}