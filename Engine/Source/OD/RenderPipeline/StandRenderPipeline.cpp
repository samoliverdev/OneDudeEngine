#include "OD/pch.h"
#include "StandRenderPipeline.h"
#include "OD/Core/Application.h"
#include "OD/Core/Lua.h"
#include "OD/Core/Hash.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Graphics/Common.h"
#include "OD/Graphics/Geometry.h"
#include "OD/Graphics/Font.h"
#include "OD/Graphics/UniformBuffer.h"
#include "OD/Graphics/ComputeShader.h"
#include "OD/Graphics/Gizmos.h"
#include "OD/Graphics/Ultis.h"
#include "OD/RenderPipeline/SkinnedBoneSocket.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/RenderPipeline/TextRendererComponent.h"
#include "OD/RenderPipeline/UIComponents.h"
#include "OD/RenderPipeline/Text3DRendererComponent.h"
#include "OD/RenderPipeline/StaticRendererClusterComponent.h"
#include "OD/RenderPipeline/DecalRendererComponent.h"
#include "OD/RenderPipeline/EnvironmentProbeComponent.h"
#include "OD/Animation/Animator.h"
#include "TextRendererComponent.h"
#include "MeshRendererComponent.h"
#include "ModelRendererComponent.h"
#include "SpriteRendererComponent.h"
#include <taskflow/taskflow.hpp>
#include <cstring>
#include <stb/stb_image_write.h>

namespace OD{

//#define UseExperimentalRunComputeRenderList

ShadowTextureSize ShadowQualityToShadowTextureSizeLookup[] = {
    ShadowTextureSize::_256, // VeryLow
    ShadowTextureSize::_512, // Low
    ShadowTextureSize::_1024, // Mediam
    ShadowTextureSize::_2048, // High
    ShadowTextureSize::_4096, // VeryHigh
    ShadowTextureSize::_8192  // Ultra
};

class RendererFeatureTest: public RendererFeatureBase<RendererFeatureTest>, RenderPass{
public:
    template <class Archive>
    void serialize(Archive & ar){
        
    }

    void AddRenderPasses(IRenderer& renderer, RenderContext& context) override {
        renderer.AddPass(this);
    }

    void OnGui() override {

    }

    void Setup(Scene& scene, RenderContext& context) override {

    }

    void Execute(Scene& scene, RenderContext& context, RenderFrameData& data) override {

    }
};

void StandRenderPipelineModuleInit(){
    SceneManager::Get().RegisterCoreComponent<EnvironmentComponent>("EnvironmentComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<CameraComponent>("CameraComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<LightComponent>("LightComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<StaticRendererComponent>("StaticRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<MeshRendererComponent>("MeshRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<SkinnedMeshRendererComponent>("SkinnedMeshRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<SkinnedBoneSocket>("SkinnedBoneSocket", "Renderer");
    SceneManager::Get().RegisterCoreComponent<ModelRendererComponent>("ModelRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<SkinnedModelRendererComponent>("SkinnedModelRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<DecalRendererComponent>("DecalRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<EnvironmentProbeComponent>("EnvironmentProbeComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<TextRendererComponent>("TextRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<SpriteRendererComponent>("SpriteRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<StaticRendererClusterComponent>("StaticRendererClusterComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<CanvasComponent>("CanvasComponent", "UI");
    SceneManager::Get().RegisterCoreComponent<RectTransformComponent>("RectTransformComponent", "UI");
    SceneManager::Get().RegisterCoreComponent<UIImageComponent>("UIImageComponent", "UI");
    SceneManager::Get().RegisterCoreComponent<UITextComponent>("UITextComponent", "UI");
    SceneManager::Get().RegisterCoreComponent<Text3DRendererComponent>("Text3DRendererComponent", "Renderer");
    SceneManager::Get().RegisterCoreComponent<GizmosDrawComponent>("GizmosDrawComponent", "Renderer");
    SceneManager::Get().RegisterSystem<StandRenderPipeline>("StandRenderPipeline");
    //SceneManager::Get().AddGlobalSystem<StandRenderPipeline>();

    SceneManager::Get().AddGlobalSystem<StandRenderPipelineGlobalData>();

    LuaBindsDB::Get().RegisterLuaBind<CameraComponent>();
    LuaBindsDB::Get().RegisterLuaBind<LightComponent>();

    RendererFeatureGlobal::Get().RegisterRendererFeature<RendererFeatureTest>("RendererFeatureTest");
}

#pragma region Shadows
Shadows::Shadows(){
    /*FrameBufferSpecification specification = {};
    specification.width = 1024 * 1;
    specification.height = 1024 * 1;
    specification.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT16};

    specification.sample = Shadows::maxShadowedDirectionalLightCount * Shadows::maxCascades;
    directionalShadowAtlas = ResourceManager::Get().Create<Framebuffer>(specification);
    directionalShadowAtlas->name = "directionalShadowAtlas";

    specification.sample = Shadows::maxShadowedOtherLightCount;
    otherShadowAtlas = ResourceManager::Get().Create<Framebuffer>(specification);
    otherShadowAtlas->name = "otherShadowAtlas";*/


    /*directionalShadowAtlas = ResourceManager::Get().Create<Framebuffer>(FramebufferType::Shadowmap, 1024 * 1, 1024 * 1, Shadows::maxShadowedDirectionalLightCount * Shadows::maxCascades);
    directionalShadowAtlas->name = "directionalShadowAtlas";
    otherShadowAtlas = ResourceManager::Get().Create<Framebuffer>(FramebufferType::Shadowmap, 1024 * 1, 1024 * 1, Shadows::maxShadowedOtherLightCount);
    otherShadowAtlas->name = "otherShadowAtlas";*/

    shadowPass = ResourceManager::Get().Create<Material>("DefaultShadowMap");
    shadowPass->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/ShadowMap.glsl"));
}

Shadows::~Shadows(){
}

void Shadows::Setup(RenderContext* inContext, ShadowSettings inSettings, Camera inCam){
    OD_PROFILE_SCOPE("Shadows::Setup");
    context = inContext;
    settings = inSettings;
    cam = inCam;

    shadowCascadeLevels.clear();
    shadowCascadeLevels.push_back(inSettings.maxDistance * inSettings.directional.cascadeRatio1);
    shadowCascadeLevels.push_back(inSettings.maxDistance * inSettings.directional.cascadeRatio2);
    shadowCascadeLevels.push_back(inSettings.maxDistance * inSettings.directional.cascadeRatio3);
    shadowCascadeLevels.push_back(inSettings.maxDistance * inSettings.directional.cascadeRatio4);
    
    shadowedDirectionalLightCount = 0; 
    shadowedOtherLightCount = 0;
    context->GetDirectionalShadowAtlas()->Resize((int)settings.directional.altasSize, (int)settings.directional.altasSize);
    context->GetOtherShadowAtlas()->Resize((int)settings.other.altasSize, (int)settings.other.altasSize);

    for(auto& i: shadowDirectionalLightsBuffers){
        i.Clean();
    }
    for(auto& i: shadowOtherLightsBuffers){
        i.Clean();
    }
}

void Shadows::AddRunComputeRenderList(){
    ShadowDrawingSettings s;

    //if(data.customShadowPass == nullptr) data.customShadowPass = shadowPass.get();

    int index = -1;
    for(int i = 0; i < shadowedDirectionalLightCount; i++){
        for(int j = 0; j < settings.directional.cascadeCount; j++){
            index += 1;

            scene->GetTaskflow().emplace([&, index](){
                context->RunComputeRenderListShadow(
                    {shadowDirectionalLightsSplits[index].frustum, true}, 
                    s,
                    shadowDirectionalLightsBuffers[index],
                    shadowPass.get()
                );
            });
        }
    } 

    index = -1;
    for(int i = 0; i < shadowedOtherLightCount; i++){
        index += 1;

        scene->GetTaskflow().emplace([&, index](){
            context->RunComputeRenderListShadow(
                {shadowOtherLightsSplits[index].frustum, true}, 
                s,
                shadowOtherLightsBuffers[index],
                shadowPass.get()
            );
        });
    } 
}

inline bool HasNaN(const Plane& p){
    return glm::any(glm::isnan(p.n));
}

inline bool HasNaN(const Frustum& f){
    return HasNaN(f.topFace)    ||
           HasNaN(f.bottomFace) ||
           HasNaN(f.rightFace)  ||
           HasNaN(f.leftFace)   ||
           HasNaN(f.farFace)    ||
           HasNaN(f.nearFace);
}

inline bool HasNaN(const AABB& f){
    return glm::any(glm::isnan(f.center)) || glm::any(glm::isnan(f.extents));
}

void Shadows::AddRenderData(RenderData& data){
    if(data.HasFlag(RenderData::Flag::RenderShadow) == false) return;
    //TODO: Check Split data Culling

    ShadowDrawingSettings s = {};
    s.renderQueueRange = RenderQueueRange::All;
    s.sortType = SortType::None;

    if(data.customShadowPass == nullptr) data.customShadowPass = shadowPass.get();

    int index = -1;
    for(int i = 0; i < shadowedDirectionalLightCount; i++){
        for(int j = 0; j < settings.directional.cascadeCount; j++){
            index += 1;

            //Assert(index >= 0 && index < (maxShadowedDirectionalLightCount * maxCascades));
            //Assert(HasNaN(shadowDirectionalLightsSplits[index].frustum) == false);
            //Assert(HasNaN(data.aabb) == false);

            if(data.aabb.isOnFrustum(shadowDirectionalLightsSplits[index].frustum) == false && data.HasFlag(RenderData::Flag::AlwaysDraw) == false) continue;
            context->AddDrawShadow(data, s, shadowDirectionalLightsBuffers[index]);
        }
    } 

    index = -1;
    for(int i = 0; i < shadowedOtherLightCount; i++){
        index += 1;
        if(data.aabb.isOnFrustum(shadowOtherLightsSplits[index].frustum) == false && data.HasFlag(RenderData::Flag::AlwaysDraw) == false) continue;
        context->AddDrawShadow(data, s, shadowOtherLightsBuffers[index]);
    } 
}

void Shadows::Render(){
    OD_PROFILE_SCOPE("Shadows::Render");

    //if(shadowedDirectionalLightCount > 0) 
        RenderDirectionalShadows();
    //if(shadowedOtherLightCount > 0) 
        RenderOtherShadows();

    Material::SetGlobalFloat("_ShadowBias", settings.directional.shadowBias);
    context->pipelineData._ShadowDistance = settings.maxDistance;

    float f = 1.0f - settings.directional.cascadeFade;
    context->pipelineData._ShadowDistanceFade = Vector4(
        1.0f / settings.maxDistance, 
        1.0f / settings.distanceFade, 
        0, //1.0f / (1.0f - f * f), 
        1.0f
    );
    
    float altlasSize = (int)settings.directional.altasSize;
    Vector4 altasSizes = Vector4Zero;
    altasSizes.x = altlasSize;
    altasSizes.y = 1.0f / altlasSize;
    context->pipelineData._ShadowAtlasSize =  altasSizes;
}

void Shadows::RenderDirectionalShadows(){
    Assert(settings.directional.cascadeCount == 4);

    cascadeCullingSpheres[0] = shadowCascadeLevels[0];
    cascadeCullingSpheres[1] = shadowCascadeLevels[1];
    cascadeCullingSpheres[2] = shadowCascadeLevels[2];
    cascadeCullingSpheres[3] = shadowCascadeLevels[3];

    int index = 0;
    for(int i = 0; i < shadowedDirectionalLightCount; i++){
        for(int j = 0; j < settings.directional.cascadeCount; j++){
            dirShadowMatrices[index] = shadowDirectionalLightsSplits[index].projViewMatrix;
            //cascadeCullingSpheres[index] = shadowDirectionalLightsSplits[index].splitDistance;
            index += 1;
        }
    }

    index = 0;
    for(int i = 0; i < shadowedDirectionalLightCount; i++){
        for(int j = 0; j < settings.directional.cascadeCount; j++){
            context->BeginDrawShadow(*context->GetDirectionalShadowAtlas(), index);
            context->DrawShadows(shadowDirectionalLightsBuffers[index], shadowDirectionalLightsSplits[index], shadowPass);
            context->EndDrawShadow();

            index += 1;
        }
    }

    //Material::SetGlobalInt(cascadeCountId, settings.directional.cascadeCount);
    //Material::SetGlobalMatrix4(dirShadowMatricesId, dirShadowMatrices, maxShadowedDirectionalLightCount * maxCascades); //FIXME: Revise this 8 propety calculate shadowData size
    //Material::SetGlobalFloat(cascadeCullingSpheresId, cascadeCullingSpheres, settings.directional.cascadeCount); //FIXME: Revise this 8 propety calculate shadowData size
    Material::SetGlobalTexture(dirShadowAtlasId, context->GetDirectionalShadowAtlas(), -1);
    context->pipelineData._CascadeCount = settings.directional.cascadeCount;
    std::memcpy(context->pipelineData._DirectionalShadowMatrices, dirShadowMatrices, (MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT * MAX_CASCADE_COUNT) * sizeof(Matrix4)); //FIXME: Revise this 8 propety calculate shadowData size
    for(int i = 0; i < settings.directional.cascadeCount; i++){
        context->pipelineData._CascadeCullingSpheres[i] = Vector4(cascadeCullingSpheres[i]);//FIXME: Revise this 8 propety calculate shadowData size
    };

    Assert(sizeof(float) == sizeof(int));
    Assert(sizeof(Vector3) == 16);
    Assert(sizeof(Vector4) == 16);
    static_assert(alignof(Transform) == 16, "Transform is not 16-byte aligned");
    static_assert(sizeof(Transform) % 16 == 0, "Transform size is not a multiple of 16");
}

void Shadows::RenderOtherShadows(){
    int index = 0;
    for(int i = 0; i < shadowedOtherLightCount; i++){
        otherShadowMatrices[index] = shadowOtherLightsSplits[index].projViewMatrix;
        index += 1;
    }

    for(index = 0; index < shadowedOtherLightCount;){
        if(shadowedOtherLights[index].isPoint){
            RenderPointShadows(index, index, index);
            index += 6;
        } else {
            RenderSpotShadows(index, index, index);
            index += 1;
        }
    }

    Material::SetGlobalTexture(otherShadowAltasId, context->GetOtherShadowAtlas(), -1);
    std::memcpy(context->pipelineData._OtherShadowMatrices, otherShadowMatrices, MAX_SHADOWED_OTHER_LIGHT_COUNT * sizeof(Matrix4));
}

void Shadows::RenderSpotShadows(int index, int split, int tileSize){
    ShadowedOtherLight light = shadowedOtherLights[index];
    context->BeginDrawShadow(*context->GetOtherShadowAtlas(), index);
    context->DrawShadows(shadowOtherLightsBuffers[index], shadowOtherLightsSplits[index], shadowPass);
    context->EndDrawShadow();
}

void Shadows::RenderPointShadows(int index, int split, int tileSize){
    ShadowedOtherLight light = shadowedOtherLights[index];
    
    for(int i = 0; i < 6; i++){
        context->BeginDrawShadow(*context->GetOtherShadowAtlas(), index+i);
        context->DrawShadows(shadowOtherLightsBuffers[index+i], shadowOtherLightsSplits[index+i], shadowPass);
        context->EndDrawShadow();
    }
}

Vector2 Shadows::ReserveDirectionalShadows(LightComponent light, Transform trans){
    if(shadowedDirectionalLightCount < MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT && light.renderShadow){
        ShadowSplitData::SetupCascade(
            &shadowDirectionalLightsSplits[shadowedDirectionalLightCount*settings.directional.cascadeCount],
            settings.directional.cascadeCount, 
            cam, 
            trans,
            shadowCascadeLevels
        );

        shadowedDirectionalLightCount += 1;

        return Vector2(1, (shadowedDirectionalLightCount-1) * settings.directional.cascadeCount);
    }

    return Vector2Zero;
}

Vector4 Shadows::ReserveOtherShadows(LightComponent light, Transform trans){
    bool isPoint = light.type == LightComponent::Type::Point;
    int newLightCount = shadowedOtherLightCount + (isPoint ? 6 : 1);
    
    if(newLightCount > MAX_SHADOWED_OTHER_LIGHT_COUNT || light.renderShadow == false){
        return Vector4Zero; 
        //return Vector4(-1, 0, 0, 0);
    }

    if(isPoint){
        ShadowSplitData::ComputePointShadowData(&shadowOtherLightsSplits[shadowedOtherLightCount], light, trans);
    } else {
        ShadowSplitData::ComputeSpotShadowData(&shadowOtherLightsSplits[shadowedOtherLightCount], light, trans);
    }

    shadowedOtherLights[shadowedOtherLightCount] = {
        0, 
        light.shadowBias, 
        light.shadowNormalBias, 
        isPoint
    };

    Vector4 data = Vector4(
        light.shadowStrength, 
        shadowedOtherLightCount, 
        isPoint ? 1 : 0, 
        1
    );

    shadowedOtherLightCount = newLightCount;
    return data;
}

void Shadows::DrawCascadeFrustums(){
    Vector3 colors[4] = {
        Vector3(1,0,0),
        Vector3(0,1,0),
        Vector3(0,0,1),
        Vector3(0,0,0)
    };

    float scales[4] = {
        1, 1.1f, 1.2f, 1.3f
    };

    for(int i = 0; i < 4; i++){
        Matrix4 scale = math::scale(Matrix4Identity, Vector3One * scales[i]);

        Gizmos::DrawFrustum(
            shadowDirectionalLightsSplits[i].frustum, 
            Matrix4Identity, 
            colors[i]
        );
    }
}

void Shadows::Clear(){
    shadowedDirectionalLightCount = 0;
    shadowedOtherLightCount = 0;
}

#pragma endregion

#pragma region Lighting
struct Light{
	Vector3 color;
	Vector3 direction;
};

void Lighting::Setup(RenderContext* inContext, Shadows* inShadows, ShadowSettings shadowSettings, EnvironmentSettings inEnvironmentSettings){
    OD_PROFILE_SCOPE("Lighting::Setup");
    context = inContext;
    shadows = inShadows;
    environmentSettings = inEnvironmentSettings;
    SetupDirectionalLight();
}

void Lighting::SetupDirectionalLight(){
    auto lightView = scene->GetRegistry().view<LightComponent, TransformComponent>();
    
    curDirLightsCount = 0;
    curOtherLightsCount = 0;

    for(auto entity: lightView){
        LightComponent& light = lightView.get<LightComponent>(entity);
        TransformComponent& trans = lightView.get<TransformComponent>(entity);

        if(light.type == LightComponent::Type::Directional){
            if(curDirLightsCount >= maxDirLightCount) continue;

            dirLightColors[curDirLightsCount] = ToLinear((Vector4)light.color) * light.intensity; //Mathf::ToVector4(light.color * light.intensity);
            dirLightDirections[curDirLightsCount] = Mathf::ToVector4(-trans.Forward());
            Vector2 v = shadows->ReserveDirectionalShadows(light, trans);
            dirLightShadowData[curDirLightsCount] = Vector4(v.x, v.y, 0, 1);

            curDirLightsCount += 1;
            mainDirectionalLightDir = Mathf::ToVector4(-trans.Forward());
        }  

        if(light.type == LightComponent::Type::Point){
            if(curOtherLightsCount >= maxOtherLightCount) continue;

            otherLightColors[curOtherLightsCount] = ToLinear((Vector4)light.color) * light.intensity; //Mathf::ToVector4(light.color * light.intensity);
            Vector4 position = Mathf::ToVector4(trans.Position());
            position.w = 1.0f / math::max(light.radius*light.radius, 0.00001f);
            otherLightPositions[curOtherLightsCount] = position;
            otherLightDirections[curOtherLightsCount] = Vector4Zero;
            otherLightSpotAngles[curOtherLightsCount] = Vector4(0, 1, 0, 0);
            otherLightShadowData[curOtherLightsCount] = shadows->ReserveOtherShadows(light, trans);

            curOtherLightsCount += 1;
        }

        if(light.type == LightComponent::Type::Spot){
            if(curOtherLightsCount >= maxOtherLightCount) continue;

            otherLightColors[curOtherLightsCount] = ToLinear((Vector4)light.color) * light.intensity;
            Vector4 position = Mathf::ToVector4(trans.Position());
            position.w = 1.0f / math::max(light.radius*light.radius, 0.00001f);
            otherLightPositions[curOtherLightsCount] = position;
            otherLightDirections[curOtherLightsCount] = Mathf::ToVector4(-trans.Forward());
            
            float innerCos = math::cos(math::radians(0.5f * light.coneAngleInner));
            float outerCos = math::cos(math::radians(0.5f * light.coneAngleOuter));
            float angleRangeInv = 1.0f / math::max(innerCos - outerCos, 0.001f);
            otherLightSpotAngles[curOtherLightsCount] = Vector4(angleRangeInv, -outerCos * angleRangeInv, 0, 0);
            otherLightShadowData[curOtherLightsCount] = shadows->ReserveOtherShadows(light, trans);

            curOtherLightsCount += 1;
        }
    }


    //Assert(index == 2);
}

void Lighting::UpdateGlobalShaders(){
    OD_PROFILE_SCOPE("Lighting::UpdateGlobalShaders");
    context->pipelineData._DirectionalLightCount = curDirLightsCount;
    if(curDirLightsCount > 0){
        std::memcpy(context->pipelineData._DirectionalLightColors, dirLightColors, curDirLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._DirectionalLightDirections, dirLightDirections, curDirLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._DirectionalLightShadowData, dirLightShadowData, curDirLightsCount * sizeof(Vector4));
    }

    context->pipelineData._OtherLightCount = curOtherLightsCount;
    if(curOtherLightsCount > 0){
        std::memcpy(context->pipelineData._OtherLightColors, otherLightColors, curOtherLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._OtherLightPositions, otherLightPositions, curOtherLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._OtherLightDirections, otherLightDirections, curOtherLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._OtherLightSpotAngles, otherLightSpotAngles, curOtherLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._OtherLightShadowData, otherLightShadowData, curOtherLightsCount * sizeof(Vector4));
    }
}
#pragma endregion

#pragma region CameraRenderer

CameraRenderer::CameraRenderer(){
    //postFXTest = new PostFXTest(2);
    cubemapSkyMaterial = ResourceManager::Get().Create<Material>();
    cubemapSkyMaterial->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/SkyboxCubemap.glsl"));

    Ref<Texture2D> _brdfLUT = ResourceManager::Get().LoadByPath<Texture2D>("brdfLUT");
    if(_brdfLUT == nullptr){
        _brdfLUT = Texture2D::CreateBrdfLUTTexture2D();
        ResourceManager::Get().AddByPath<Texture2D>("brdfLUT", _brdfLUT);
    }

    brdfLUT = Texture2D::CreateBrdfLUTTexture2D(); //_brdfLUT; 
    
    //spriteMesh = Mesh::CenterQuad(false);
    spriteMesh = CreateRef<Mesh>();
    spriteMesh->vertices = {
        {-0.5f, -0.5f, 0},
        {-0.5f,  0.5f, 0},
        { 0.5f, -0.5f, 0},
        { 0.5f,  0.5f, 0},
    };
    /*spriteMesh->vertices = {
        {0.0f, 0.0f, 0},   // bottom-left
        {0.0f, 1.0f, 0},   // top-left
        {1.0f, 0.0f, 0},   // bottom-right
        {1.0f, 1.0f, 0},   // top-right
    };*/
    spriteMesh->uv = {
        {0, 0, 0},
        {0, 1, 0},
        {1, 0, 0},
        {1, 1, 0},
    };
    spriteMesh->drawMode = MeshDrawMode::TRIANGLES_STRIP;
    spriteMesh->Submit();

    spriteMaterial = ResourceManager::Get().Create<Material>();
    spriteMaterial->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Sprite.glsl"));

    spriteMaterial->SetVector4("color", Vector4(1));
    spriteMaterial->SetTexture("mainTex", ResourceManager::Get().LoadByPath<Texture2D>("Engine/Textures/White.jpg"));

    font = ResourceManager::Get().LoadByPath<Font>("Engine/Fonts/OpenSans/static/OpenSans-Regular.ttf", FontSettings{8*3, FontType::MSDF});// Asset::CreateFromFile<Font>("Engine/Fonts/OpenSans/static/OpenSans_Condensed-MediumItalic.ttf");//  OD::Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans_Condensed-MediumItalic.ttf");
    fontMaterial = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Engine/Shaders/FontMSDF.glsl"));
    fontMaterial->SetPass(1);
    fontMaterial->SetFloat("pxRange", font->MsdfPxRange());

    gamaCorrectionPP = new GamaCorrectionPP();

    cubeMesh = CreateRef<Mesh>();
    cubeMesh->vertices = {
        // +X
        {1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, -1.0f},
        // -X
        {-1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, 1.0f},
        // +Y
        {-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f},
        // -Y
        {-1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f},
        // +Z
        {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f},
        // -Z
        {1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}
    };
    cubeMesh->indices = {
        0, 1, 2, 2, 3, 0,       // +X
        4, 5, 6, 6, 7, 4,       // -X
        8, 9,10,10,11, 8,       // +Y
       12,13,14,14,15,12,       // -Y
       16,17,18,18,19,16,       // +Z
       20,21,22,22,23,20        // -Z
    };
    cubeMesh->Submit();

    blitPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
}

CameraRenderer::~CameraRenderer(){
    //delete postFXTest;
    delete gamaCorrectionPP;
}

void CameraRenderer::RenderPassNew(CameraRenderPass& inpass, RenderContext* renderContext, ShadowSettings shadowSettings, EnvironmentSettings& environmentSettings){
    OD_PROFILE_SCOPE("CameraRenderer::Render");

    inpass.camera.width = inpass.camera.viewportRect.z * inpass.camera.width;
    inpass.camera.height = inpass.camera.viewportRect.w * inpass.camera.height; 

    if(inpass.target == nullptr){
        inpass.target = ResourceManager::Get().Create<Framebuffer>(renderContext->GetFinalColor()->Specification());
        inpass.target->name = "CameraRenderPass";
    }

    // ----------- Setup ----------- 
    pass = inpass;
    pass.collectSettings.BuildMask();
    camera = pass.camera;
    renderingPath = pass.renderingPath;
    context = renderContext;
    context->isDeferred = renderingPath == RenderingPath::Deferred;

    passCtx->ClearRenderPasses();
    passCtx->FeaturesRunAddRenderPasses(*context);
    if(inpass.settings.drawPostProcessing == false) passCtx->GetRenderPasses()[(int)RenderPassEvent::PostProcessBeforeForward].clear();
    if(inpass.settings.drawPostProcessing == false) passCtx->GetRenderPasses()[(int)RenderPassEvent::PostProcess].clear();
    if(inpass.settings.drawUI == false) passCtx->GetRenderPasses()[(int)RenderPassEvent::UI].clear();
    passCtx->GetRenderPasses()[(int)RenderPassEvent::PostProcess].push_back(&gamaCorrectionPass);

    shadows.Setup(context, shadowSettings, camera);
    lighting.Setup(context, &shadows, shadowSettings, environmentSettings);
    
    if(pass.settings.drawShadow == false){
        shadows.Clear();   
    }

    //RunRenderDataLoop();
    {
        OD_PROFILE_SCOPE("CameraRenderer::RunRenderDataLoop");

        opaqueDrawTarget.Clean();
        opaqueForwardOnlyDrawTarget.Clean();
        blendDrawTarget.Clean();
        decalDrawTarget.Clean();
        entityIdDrawTarget.Clean();

        entityIdDrawSettings.enableIntancing = false;
        entityIdDrawSettings.renderQueueRange = RenderQueueRange::All;
        entityIdDrawSettings.sortType = SortType::None;
        entityIdDrawTarget.sortType = RendererList::SortType::None;// RendererList::SortType::CommonOpaque;

        //----------Opaque Settings-----------
        opaqueDrawSettings.enableIntancing = true;
        opaqueDrawSettings.renderQueueRange = RenderQueueRange::Opaue;
        opaqueDrawSettings.sortType = SortType::CommonOpaque;
        opaqueDrawSettings.excludedTags.push_back(Hash::StringToHash("ForwardOnly"));
        opaqueDrawTarget.sortType = RendererList::SortType::None; //RendererList::SortType::CommonOpaque;
        
        opaqueForwardOnlyDrawSettings.enableIntancing = true;
        opaqueForwardOnlyDrawSettings.renderQueueRange = RenderQueueRange::Opaue;
        opaqueForwardOnlyDrawSettings.sortType = SortType::CommonOpaque;
        opaqueForwardOnlyDrawSettings.requiredTags.push_back(Hash::StringToHash("ForwardOnly"));
        opaqueForwardOnlyDrawTarget.sortType = RendererList::SortType::None; //RendererList::SortType::CommonOpaque;

        //----------Transparent Settings-----------
        blendDrawSettings.enableIntancing = true; //true; //false;
        blendDrawSettings.renderQueueRange = RenderQueueRange::Transparent;
        blendDrawSettings.sortType = SortType::CommonTransparent;
        blendDrawTarget.sortType = RendererList::SortType::CommonTransparent;

        decalDrawSettings.enableIntancing = true;
        decalDrawSettings.renderQueueRange = RenderQueueRange::All;
        decalDrawSettings.sortType = SortType::None;
        decalDrawSettings.decalTarget = true;
        decalDrawTarget.sortType = RendererList::SortType::None;

        context->RenderDataLoopNew2([&](RenderData& data){
            /*if(pass.collectSettings.collectStatic == false && data.HasFlag(RenderData::Flag::IsStatic) == true) return; 
            if(pass.collectSettings.collectDynamic == false && data.HasFlag(RenderData::Flag::IsStatic) == false) return; 
            
            if(pass.collectSettings.collectMesh == false && data.HasFlag(RenderData::Flag::FromMesh) == true) return;
            if(pass.collectSettings.collectModel == false && data.HasFlag(RenderData::Flag::FromModel) == true) return; 
            if(pass.collectSettings.collectSkinnedMesh == false && data.HasFlag(RenderData::Flag::FromSkinnedMesh) == true) return;
            if(pass.collectSettings.collectSkinnedModel == false && data.HasFlag(RenderData::Flag::FromSkinnedModel) == true) return; 

            if(pass.collectSettings.collectCluster == false && data.HasFlag(RenderData::Flag::FromCluster) == true) return; 
            if(pass.collectSettings.collectParticle == false && data.HasFlag(RenderData::Flag::IsParticle) == true) return; 
            if(pass.collectSettings.collectDecal == false && data.HasFlag(RenderData::Flag::IsDecal) == true) return;*/ 

            if((pass.cullingMask & LayerToMask(data.layer)) == 0) return;

            const uint32_t f = data.flags;
            // static/dynamic special case
            const bool isStatic = (f & RenderData::Flag::IsStatic) != 0;
            if(!pass.collectSettings.collectStatic  && isStatic)  return;
            if(!pass.collectSettings.collectDynamic && !isStatic) return;
            // all other filters in one test
            if((f & pass.collectSettings.rejectIfAny) != 0) return;

            AddRenderData(data); 
            if(pass.settings.drawShadow){
                shadows.AddRenderData(data);
            } 
        });
    }

    shadows.Render();
    lighting.UpdateGlobalShaders();
    renderContext->SetCustomFinalColor(pass.target, pass.targetFace);
    RenderVisibleGeometryNew(environmentSettings);
}

void CameraRenderer::Render(Camera inCam, RenderContext* inRenderContext, ShadowSettings shadowSettings, EnvironmentSettings& environmentSettings, RenderingPath inRenderingPath){
    OD_PROFILE_SCOPE("CameraRenderer::Render");
    // ----------- Setup ----------- 
    camera = inCam;
    renderingPath = inRenderingPath;
    context = inRenderContext;
    context->isDeferred = renderingPath == RenderingPath::Deferred;
    shadows.Setup(context, shadowSettings, camera);
    lighting.Setup(context, &shadows, shadowSettings, environmentSettings);
    /*std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
    for(auto i: postFXs) i->OnSetup();*/
    
    // ----------- Build Render Datas Loop ----------- 
    // Get All RenderData and Building CommandsBuffer to Post Renderer
    // Building the Render Graph
    RunRenderDataLoop();
    
    // ----------- Rendering ------------
    shadows.Render();
    lighting.UpdateGlobalShaders();
    RenderVisibleGeometry(environmentSettings);
}

void CameraRenderer::RunRenderDataLoop(){
    OD_PROFILE_SCOPE("CameraRenderer::RunRenderDataLoop");

    opaqueDrawTarget.Clean();
    opaqueForwardOnlyDrawTarget.Clean();
    blendDrawTarget.Clean();
    decalDrawTarget.Clean();
    entityIdDrawTarget.Clean();

    entityIdDrawSettings.enableIntancing = false;
    entityIdDrawSettings.renderQueueRange = RenderQueueRange::All;
    entityIdDrawSettings.sortType = SortType::None;
    entityIdDrawTarget.sortType = RendererList::SortType::None;// RendererList::SortType::CommonOpaque;

    //----------Opaque Settings-----------
    opaqueDrawSettings.enableIntancing = true;
    opaqueDrawSettings.renderQueueRange = RenderQueueRange::Opaue;
    opaqueDrawSettings.sortType = SortType::CommonOpaque;
    opaqueDrawSettings.excludedTags.push_back(Hash::StringToHash("ForwardOnly"));
    opaqueDrawTarget.sortType = RendererList::SortType::None; //RendererList::SortType::CommonOpaque;
    
    opaqueForwardOnlyDrawSettings.enableIntancing = true;
    opaqueForwardOnlyDrawSettings.renderQueueRange = RenderQueueRange::Opaue;
    opaqueForwardOnlyDrawSettings.sortType = SortType::CommonOpaque;
    opaqueForwardOnlyDrawSettings.requiredTags.push_back(Hash::StringToHash("ForwardOnly"));
    opaqueForwardOnlyDrawTarget.sortType = RendererList::SortType::None; //RendererList::SortType::CommonOpaque;

    //----------Transparent Settings-----------
    blendDrawSettings.enableIntancing = true; //true; //false;
    blendDrawSettings.renderQueueRange = RenderQueueRange::Transparent;
    blendDrawSettings.sortType = SortType::CommonTransparent;
    blendDrawTarget.sortType = RendererList::SortType::CommonTransparent;

    decalDrawSettings.enableIntancing = true;
    decalDrawSettings.renderQueueRange = RenderQueueRange::All;
    decalDrawSettings.sortType = SortType::None;
    decalDrawSettings.decalTarget = true;
    decalDrawTarget.sortType = RendererList::SortType::None;

    #ifdef UseExperimentalRunComputeRenderList

    auto meshRenderView = context->GetScene()->GetRegistry().view<TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable>
    );
    for(auto [entity, trans, info]: meshRenderView.each()){
        context->GetScene()->GetTaskflow().emplace([&](){
            trans.UpdateGlobalTransformCacheIfNeeded();//INFO: I think this is not thread safe!!!
        });
    }
    context->GetScene()->GetExecutor().run(context->GetScene()->GetTaskflow()).wait(); 
    context->GetScene()->GetTaskflow().clear();

    context->GetScene()->GetTaskflow().emplace([&](){
        context->RunComputeRenderList(
            {camera.frustum, true}, 
            opaqueDrawSettings,
            opaqueDrawTarget
        );
    });
    context->GetScene()->GetTaskflow().emplace([&](){
        context->RunComputeRenderList(
            {camera.frustum, true}, 
            blendDrawSettings,
            blendDrawTarget
        );
    });
    context->GetScene()->GetTaskflow().emplace([&](){
        context->RunComputeRenderList(
            {camera.frustum, true}, 
            entityIdDrawSettings,
            entityIdDrawTarget
        );
    });
    shadows.AddRunComputeRenderList();
    context->GetScene()->GetExecutor().run(context->GetScene()->GetTaskflow()).wait(); 
    context->GetScene()->GetTaskflow().clear();

    #else
    
    /*context->RenderDataLoop([&](RenderData& data){
        AddRenderData(data); 
        if(data.HasFlag(RenderData::Flag::RenderShadow) == true) shadows.AddRenderData(data); 
    });*/

    context->UpdateRenderData(*scene, *passCtx);
    context->RenderDataLoopNew([&](RenderData& data){
        AddRenderData(data); 
        //if(data.HasFlag(RenderData::Flag::RenderShadow) == true){
            shadows.AddRenderData(data);
        //} 
    });

    /*context->GetScene()->GetTaskflow().emplace([&](){
        context->RunComputeRenderList(
            {camera.frustum, true}, 
            opaqueDrawSettings,
            opaqueDrawTarget
        );
    });
    context->GetScene()->GetTaskflow().emplace([&](){
        context->RunComputeRenderList(
            {camera.frustum, true}, 
            blendDrawSettings,
            blendDrawTarget
        );
    });
    shadows.AddRunComputeRenderList();
    context->GetScene()->GetExecutor().run(context->GetScene()->GetTaskflow()).wait(); 
    context->GetScene()->GetTaskflow().clear();*/

    #endif
}

void CameraRenderer::AddRenderData(RenderData& data){
    //Assert(Mathf::HasNaN(camera.view) == false);
    //Assert(HasNaN(camera.frustum) == false);

    if(data.aabb.isOnFrustum(camera.frustum) == false && data.HasFlag(RenderData::Flag::AlwaysDraw) == false) return;
    context->AddDrawRenderers(data, opaqueDrawSettings, opaqueDrawTarget);
    context->AddDrawRenderers(data, opaqueForwardOnlyDrawSettings, opaqueForwardOnlyDrawTarget);
    context->AddDrawRenderers(data, blendDrawSettings, blendDrawTarget);
    context->AddDrawRenderers(data, decalDrawSettings, decalDrawTarget);
    context->AddDrawRenderers(data, entityIdDrawSettings, entityIdDrawTarget);
}

glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

glm::mat4 captureViews[] = {
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

/*struct alignas(16) DispatchParametersGPU{
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
};*/

void CameraRenderer::RenderVisibleGeometryNew(EnvironmentSettings& environmentSettings){
    OD_PROFILE_SCOPE("CameraRenderer::RenderVisibleGeometry");
    //OD_GPU_PROFILE_SCOPE("CameraRenderer::RenderVisibleGeometry");

    if(environmentSettings.skyCubemap != nullptr && environmentSettings.skyIrradianceMap == nullptr){
        environmentSettings.skyIrradianceMap = Cubemap::CreateIrradianceMapFromCubeMap(environmentSettings.skyCubemap);
    }
    if(environmentSettings.skyCubemap != nullptr && environmentSettings.skyPrefilterMap == nullptr){
        environmentSettings.skyPrefilterMap = Cubemap::CreatePrefilterMapFromCubeMap(environmentSettings.skyCubemap);
    }

    context->SetupCameraProperties(camera);
    context->BeginDrawToScreenNew();

    Ref<Material> targetSkyMaterial = nullptr;
    if(environmentSettings.environmentSky == EnvironmentSky::Cubemap){
        cubemapSkyMaterial->SetCubemap("mainTex", environmentSettings.skyCubemap);
        targetSkyMaterial = cubemapSkyMaterial;
    }
    if(environmentSettings.environmentSky == EnvironmentSky::CustomMaterial){
        targetSkyMaterial = environmentSettings.skyCustomMaterial;
    }
    context->skyMaterial = targetSkyMaterial;

    context->pipelineData._AmbientLight = ToLinear((Vector4)environmentSettings.ambient);

    if(environmentSettings.environmentLight == EnvironmentLight::Color){
        Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
        context->pipelineData._AmbientLight = ToLinear((Vector4)environmentSettings.ambient);
        context->pipelineData._SkyLightIntensity = 0;
        context->pipelineData._IrradianceMapScale = Vector4Zero;
    }
    if(environmentSettings.environmentLight == EnvironmentLight::SkyCubemap){
        Material::SetGlobalCubemap("_EnvironmentMap", environmentSettings.skyCubemap);
        Material::SetGlobalCubemap("_IrradianceMap", environmentSettings.skyIrradianceMap);
        Material::SetGlobalCubemap("_PrefilterMap", environmentSettings.skyPrefilterMap);
        if(environmentSettings.skyPrefilterMapF != nullptr) Material::SetGlobalTexture("_PrefilterMap", environmentSettings.skyPrefilterMapF, 0);
        Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
        context->pipelineData._AmbientLight = Vector4Zero;
        context->pipelineData._SkyLightIntensity = environmentSettings.skyLightIntensity;
    }

    context->pipelineDataBuffer->SetData(&context->pipelineData, sizeof(PipelineData), 0);
    Material::SetGlobalUniformBuffer("PipelineData", context->pipelineDataBuffer, 0);

    if(renderingPath == RenderingPath::Forward){
        context->CleanSSS();
        context->BeginForwardPass();
        
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        context->DrawRenderersBuffer(opaqueDrawTarget, true);//TODO: Maybe remove sort on here
        context->DrawRenderersBuffer(opaqueForwardOnlyDrawTarget, true);//TODO: Maybe remove sort on here
        
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        //context->DrawRenderersBuffer(blendDrawTarget, true);
        if(environmentSettings.environmentSky != EnvironmentSky::None) context->RenderSkyboxLater(*scene);
        context->DrawRenderersBuffer(blendDrawTarget, true);
        context->DrawGizmos(*scene);  
        
        context->EndForwardPass();
    } else {
        context->BeginDeferredPass();
        //if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        context->DrawRenderersBuffer(opaqueDrawTarget, true, true);//TODO: Maybe remove sort on here
        //if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        context->EndDeferredPass();

        context->CopyDeffered();
        context->BeginDeferredPass(false);
        context->DrawRenderersBuffer(decalDrawTarget, false, true, true);

        //context->EndDeferredPass();
        #if 0
        context->EndDeferredPassAndCopyToForwardPass();
        #else
        context->EndDeferredPass();

        Ref<Framebuffer> deferred = context->GetDeferredFramebuffer();

        context->CleanSSS();
        if(environmentSettings.enableSSS && camera.type != Camera::Type::Preview && camera.type != Camera::Type::Reflection){
            SSS_Settings settings = {
                environmentSettings.sssSurfaceThickness, 
                environmentSettings.sssBilinearThreshold, 
                environmentSettings.sssShadowContrast
            };
            context->DrawSSS(lighting.mainDirectionalLightDir, settings);
        }
 
        context->BeginForwardPass();

        if(lighting.curDirLightsCount <= 0){
            context->DrawDeferredLight(-1);
        } else {
            for(int i = 0; i < lighting.curDirLightsCount; i++){
                context->DrawDeferredLight(i, i == 0);
            }
        }
        
        for(int i = 0; i < lighting.curOtherLightsCount; i++){
            float radiusRecovered = glm::inversesqrt(glm::max(lighting.otherLightPositions[i].w, 1e-6f));
            context->DrawDeferredLightOther(
                i, 
                lighting.otherLightPositions[i], 
                lighting.otherLightDirections[i], 
                radiusRecovered,
                lighting.otherLightDirections[i] != Vector4Zero
            );
        }

        //TODO: Make this work later, current the blit or post shader depth write/test setting is bug something 
        context->EndForwardPass();
        RenderFrameData data;
        context->DrawPostFXs(*scene, *passCtx, data, nullptr, RenderPassEvent::PostProcessBeforeForward);
        
        context->DeferredCopyToForwardPass();

        #endif
        context->BeginForwardPass(false);

        context->DrawRenderersBuffer(opaqueForwardOnlyDrawTarget, true);
        if(environmentSettings.environmentSky != EnvironmentSky::None) context->RenderSkyboxLater(*scene);

        context->DrawRenderersBuffer(blendDrawTarget, true);
        Draw3DText();
        if(pass.settings.drawGizmos) context->DrawGizmos(*scene); 
        context->EndForwardPass();
    }

    /*std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
    if(pass.settings.drawPostProcessing == false){
        postFXs.clear();
        postFXs.push_back(gamaCorrectionPP);
    }
    context->DrawPostFXs(postFXs);*/
    
    RenderFrameData data;
    context->DrawPostFXs(*scene, *passCtx, data);

    context->BeginUIPass();
    if(pass.settings.drawUI){
        RenderUI();
        /*for(auto& i: renderStagePasses->renderPass[(int)RenderStage::UI]){
            i->OnRender(*context->scene, camera);
        }*/
        RenderFrameData data;
        for(auto& i: passCtx->GetRenderPasses()[(int)RenderPassEvent::UI]){
            i->Execute(*scene, *context, data);
        }
    }
    context->EndUIPass();

    context->EndDrawToScreenNew();
}

void CameraRenderer::RenderVisibleGeometry(EnvironmentSettings& environmentSettings){
    OD_PROFILE_SCOPE("CameraRenderer::RenderVisibleGeometry");
    //OD_GPU_PROFILE_SCOPE("CameraRenderer::RenderVisibleGeometry");

    if(environmentSettings.skyCubemap != nullptr && environmentSettings.skyIrradianceMap == nullptr){
        /*FrameBufferSpecification specification;
        specification.width = 32*4;
        specification.height = 32*4;
        specification.type = FramebufferAttachmentType::CUBEMAP;
        specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
        specification.colorAttachments = {{FramebufferTextureFormat::RGB16F}};
        environmentSettings.skyIrradianceMapF = ResourceManager::Get().Create<Framebuffer>(specification);
        //environmentSettings.skyIrradianceMapF->Invalidate();

        Ref<Material> irradianceMat = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/IrradianceConvolution.glsl"));
        irradianceMat->SetCubemap("environmentMap", environmentSettings.skyCubemap);
        irradianceMat->SetMatrix4("projection2", captureProjection);

        for(unsigned int i = 0; i < 6; ++i){
            irradianceMat->SetMatrix4("view2", captureViews[i]);
            Graphics::BeginFramebuffer(*environmentSettings.skyIrradianceMapF, {0, 0, 0, 1}, i);
            Graphics::SetViewport(0, 0, specification.width, specification.height);
            Graphics::BindMaterial(*irradianceMat);
            Graphics::DrawMesh(*cubeMesh, Matrix4Identity);
            Graphics::EndFramebuffer();
        }*/

        environmentSettings.skyIrradianceMap = Cubemap::CreateIrradianceMapFromCubeMap(environmentSettings.skyCubemap);
    }
    if(environmentSettings.skyCubemap != nullptr && environmentSettings.skyPrefilterMap == nullptr){
        /*FrameBufferSpecification specification;
        specification.width = 32*4;
        specification.height = 32*4;
        specification.type = FramebufferAttachmentType::CUBEMAP;
        specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
        specification.colorAttachments = {{FramebufferTextureFormat::RGB16F, true}};
        environmentSettings.skyPrefilterMapF = ResourceManager::Get().Create<Framebuffer>(specification);
        //environmentSettings.skyIrradianceMapF->Invalidate();

        Ref<Material> irradianceMat = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Prefilter.glsl"));
        irradianceMat->SetCubemap("environmentMap", environmentSettings.skyCubemap);
        irradianceMat->SetMatrix4("projection2", captureProjection);

        unsigned int maxMipLevels = 5;
        for(unsigned int mip = 0; mip < maxMipLevels; ++mip){
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            irradianceMat->SetFloat("roughness", roughness);
            for(unsigned int i = 0; i < 6; ++i){
                irradianceMat->SetMatrix4("view2", captureViews[i]);
                Graphics::BeginFramebuffer(*environmentSettings.skyPrefilterMapF, {0, 0, 0, 1}, i, mip);
                Graphics::SetViewport(0, 0, mipWidth, mipHeight);
                Graphics::BindMaterial(*irradianceMat);
                Graphics::DrawMesh(*cubeMesh, Matrix4Identity);
                Graphics::EndFramebuffer();
            }
        }*/

        environmentSettings.skyPrefilterMap = Cubemap::CreatePrefilterMapFromCubeMap(environmentSettings.skyCubemap);
    }

    context->SetupCameraProperties(camera);
    context->BeginDrawToScreen();

    Ref<Material> targetSkyMaterial = nullptr;
    if(environmentSettings.environmentSky == EnvironmentSky::Cubemap){
        cubemapSkyMaterial->SetCubemap("mainTex", environmentSettings.skyCubemap);
        targetSkyMaterial = cubemapSkyMaterial;
    }
    if(environmentSettings.environmentSky == EnvironmentSky::CustomMaterial){
        targetSkyMaterial = environmentSettings.skyCustomMaterial;
    }
    context->skyMaterial = targetSkyMaterial;

    context->pipelineData._AmbientLight = ToLinear((Vector4)environmentSettings.ambient);

    if(environmentSettings.environmentLight == EnvironmentLight::Color){
        Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
        context->pipelineData._AmbientLight = ToLinear((Vector4)environmentSettings.ambient);
        context->pipelineData._SkyLightIntensity = 0;
        context->pipelineData._IrradianceMapScale = Vector4Zero;
    }
    if(environmentSettings.environmentLight == EnvironmentLight::SkyCubemap){
        Material::SetGlobalCubemap("_IrradianceMap", environmentSettings.skyIrradianceMap);
        Material::SetGlobalCubemap("_PrefilterMap", environmentSettings.skyPrefilterMap);
        if(environmentSettings.skyPrefilterMapF != nullptr) Material::SetGlobalTexture("_PrefilterMap", environmentSettings.skyPrefilterMapF, 0);
        Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
        context->pipelineData._AmbientLight = Vector4Zero;
        context->pipelineData._SkyLightIntensity = environmentSettings.skyLightIntensity;
    }

    context->pipelineDataBuffer->SetData(&context->pipelineData, sizeof(PipelineData), 0);
    Material::SetGlobalUniformBuffer("PipelineData", context->pipelineDataBuffer, 0);

    /*context->BeginDrawEntityIds();
    context->DrawEntityIds(entityIdDrawTarget);
    context->EndDrawEntityIds();*/

    if(renderingPath == RenderingPath::Forward){
        context->CleanSSS();
        context->BeginForwardPass();
        
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        //context->DrawZPreePassRenderersBuffer(opaqueDrawTarget, false, false);
        //context->DrawZPreePassRenderersBuffer(opaqueDrawTarget, false, true);
        //Graphics::SetDepthTest(DepthTest::LESS);

        //Graphics::SetColorMask(0,0,0,0);

        context->DrawRenderersBuffer(opaqueDrawTarget, true);//TODO: Maybe remove sort on here
        context->DrawRenderersBuffer(opaqueForwardOnlyDrawTarget, true);//TODO: Maybe remove sort on here
        
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        //context->DrawRenderersBuffer(blendDrawTarget, true);
        if(environmentSettings.environmentSky != EnvironmentSky::None) context->RenderSkyboxLater(*scene);
        context->DrawRenderersBuffer(blendDrawTarget, true);
        context->DrawGizmos(*scene);  
        
        context->EndForwardPass();
    } else {
        context->BeginDeferredPass();
        //if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        context->DrawRenderersBuffer(opaqueDrawTarget, true, true);//TODO: Maybe remove sort on here
        //if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        context->DrawRenderersBuffer(decalDrawTarget, false, true, true);

        //context->EndDeferredPass();
        #if 0
        context->EndDeferredPassAndCopyToForwardPass();
        #else
        context->EndDeferredPass();

        Ref<Framebuffer> deferred = context->GetDeferredFramebuffer();

        context->CleanSSS();
        if(environmentSettings.enableSSS && camera.type != Camera::Type::Preview && camera.type != Camera::Type::Reflection){
            SSS_Settings settings = {
                environmentSettings.sssSurfaceThickness, 
                environmentSettings.sssBilinearThreshold, 
                environmentSettings.sssShadowContrast
            };
            context->DrawSSS(lighting.mainDirectionalLightDir, settings);
        }
 
        /*Graphics::BeginFramebuffer(*deferred, false);
        context->DrawRenderersBuffer(decalDrawTarget, false, true, true);
        Graphics::EndFramebuffer();*/

        /*delete normal;
        delete albedo;
        delete pos;*/
        
        context->BeginForwardPass();
        Graphics::Clean(0, 0, 0, 1);

        if(lighting.curDirLightsCount <= 0){
            context->DrawDeferredLight(-1);
        } else {
            for(int i = 0; i < lighting.curDirLightsCount; i++){
                context->DrawDeferredLight(i, i == 0);
            }
        }
        
        for(int i = 0; i < lighting.curOtherLightsCount; i++){
            float radiusRecovered = glm::inversesqrt(glm::max(lighting.otherLightPositions[i].w, 1e-6f));
            context->DrawDeferredLightOther(
                i, 
                lighting.otherLightPositions[i], 
                lighting.otherLightDirections[i], 
                radiusRecovered,
                lighting.otherLightDirections[i] != Vector4Zero
            );
        }

        context->DeferredCopyToForwardPass();

        #endif
        //context->BeginForwardPass();

        context->DrawRenderersBuffer(opaqueForwardOnlyDrawTarget, true);

        //context->RenderSkyboxLater();
        if(environmentSettings.environmentSky != EnvironmentSky::None) context->RenderSkyboxLater(*scene);
        context->DrawRenderersBuffer(blendDrawTarget, true);
        Draw3DText();
        context->DrawGizmos(*scene); 
        context->EndForwardPass();
    }

    //----------SSS-----------
    /*using namespace Bend;
    if(camera.type != Camera::Type::Preview && camera.type != Camera::Type::Reflection){
        screenSpaceShadowOutput->Resize(camera.width, camera.height);

        Graphics::BeginFramebuffer(*screenSpaceShadowOutput, true, {0, 0, 0, 0}, 0, 0);
        Graphics::EndFramebuffer();

        //glm::vec3 lightDir = glm::normalize(-glm::vec3(1,-1,0));
        glm::vec3 lightDir = -glm::normalize(lighting.mainDirectionalLightDir);
        lightDir.x = -lightDir.x;
        lightDir.z = -lightDir.z;

        glm::vec4 lightVec(lightDir, 0.0f);
        glm::mat4 viewProj = camera.projection * camera.view;
        glm::vec4 lightProjection = viewProj * lightVec;

        //glm::vec4 lightPos(lightPosition, 1.0f);
        //glm::vec4 lightProjection = viewProjectionMatrix * lightPos;

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

        params.SurfaceThickness = 0.005f;
        params.BilinearThreshold = 0.02f;
        params.ShadowContrast = 4.0f;
        params.NearDepthValue = 1.0f;
        params.FarDepthValue = 0.0f;

        params.near = camera.nearClip;
        params.far = camera.farClip;

        params.InvDepthTextureSize[0] = 1.0f / camera.width;
        params.InvDepthTextureSize[1] = 1.0f / camera.height;

        params.SurfaceThickness = 0.005f; // 0.02f;
        params.BilinearThreshold = 0.02f; //0.001f;
        params.ShadowContrast = 4.0f; //1.0f;
        params.NearDepthValue = 0.0f;
        params.FarDepthValue = 1.0f;

        for(int i = 0; i < dispatchList.DispatchCount; i++){
            auto& d = dispatchList.Dispatch[i];
            params.WaveOffset[0] = d.WaveOffset_Shader[0];
            params.WaveOffset[1] = d.WaveOffset_Shader[1];
            screenSpaceShadowData->SetData(&params, sizeof(DispatchParametersGPU));

            screenSpaceShadow->SetTexture("DepthTexture", context->GetDeferredFramebuffer(), -1);
            screenSpaceShadow->SetTexture("OutputTexture", screenSpaceShadowOutput.get(), 0);
            screenSpaceShadow->SetUniformBuffer("DispatchParams", screenSpaceShadowData, 2);
            screenSpaceShadow->Dispatch(d.WaveCount[0], d.WaveCount[1], d.WaveCount[2]);
        }
    }*/
    //------------------------

    std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
    context->DrawPostFXs(postFXs);
    RenderFrameData data;
    context->DrawPostFXs(*scene, *passCtx, data);

    //context->DrawGizmos();
    //for(System* s: context->GetScene()->GetStandSystems()) s->OnRender();
    //RenderUI();

    context->BeginUIPass();
    RenderUI();
    /*for(auto& i: renderStagePasses->renderPass[(int)RenderStage::UI]){
        i->OnRender(*context->scene, camera);
    }*/
    //RenderFrameData data;
    for(auto& i: passCtx->GetRenderPasses()[(int)RenderPassEvent::UI]){
        i->Execute(*scene, *context, data);
    }
    context->EndUIPass();

    context->EndDrawToScreen();
}

void CameraRenderer::RenderSprites(){
    Assert(false && "Outdate");
    /*

    auto spriteView = context->GetScene()->GetRegistry().view<TransformComponent, SpriteRendererComponent>();
    for(auto entity: spriteView){
        TransformComponent& trans = spriteView.get<TransformComponent>(entity);
        SpriteRendererComponent& sprite = spriteView.get<SpriteRendererComponent>(entity);

        if(sprite.material == nullptr) continue;

        Transform scale;
        scale.LocalScale(Vector3(sprite.sprite->Width() / sprite.pixelUnitSize, sprite.sprite->Height() / sprite.pixelUnitSize, 1));
        Matrix4 m = trans.GlobalModelMatrix() * scale.GetLocalModelMatrix();

        auto _mat = sprite.material;
        Material::SubmitGraphicDatas(*_mat);
        _mat->GetShader()->SetVector4("color", sprite.color);
        _mat->GetShader()->SetTexture2D("mainTex", *sprite.sprite, 0);
        _mat->GetShader()->SetMatrix4("model", m);
        Graphics::DrawMeshRaw(*spriteMesh);
    }*/
}

void CameraRenderer::Draw3DText(){
    for(auto [entity, text, trans]: scene->GetRegistry().view<Text3DRendererComponent, TransformComponent>().each()){
        Ref<Font> _font = text.font ? text.font : font;
        Ref<Material> _mat = text.material ? text.material : fontMaterial;
        Matrix4 textModel = trans.GlobalModelMatrix();

        _mat->SetColor4("color", text.color);
        Graphics::DrawText(*_font, *_mat, text.text, textModel, true, {});
    }
}

void RenderUIRecursive(
    Scene& scene, Entity entity, const Ref<Mesh>& mesh, 
    const Ref<Material>& defaultMaterial, 
    const Ref<Font>& defaultFont,
    const Ref<Material>& defaultFontMaterial
){
    if (!scene.HasComponent<RectTransformComponent>(entity))
        return;

    auto& rect = scene.GetComponent<RectTransformComponent>(entity);

    Matrix4 model = math::translate(Vector3(rect.finalPosition, 0.0f)) *
                    math::scale(Vector3(rect.finalSize, 1.0f));

    if (scene.HasComponent<UIImageComponent>(entity)) {
        auto& img = scene.GetComponent<UIImageComponent>(entity);
        Ref<Material> mat = img.material ? img.material : defaultMaterial;
        mat->SetVector4("color", img.color.Linear());

        Graphics::DrawMesh(*mesh, *mat, model);
    }

    if(scene.HasComponent<UITextComponent>(entity)){
        auto& tex = scene.GetComponent<UITextComponent>(entity);
        
        //Graphics::DrawText(*defaultFont, *defaultFontMaterial, tex.text, model, true, {});

        /*Ref<Font> font = tex.font ? tex.font : defaultFont;
        Ref<Material> mat = tex.material ? tex.material : defaultFontMaterial;

        Vector2 pivotOffset = rect.finalSize * rect.pivot; // Offset to align text center with finalPosition
        Vector2 textPos = rect.finalPosition - pivotOffset;

        // Build model matrix for text
        Matrix4 textModel = math::translate(Vector3(textPos, 0.0f)) *
                            math::scale(Vector3(rect.finalSize, 1.0f)); // Same scale as sprite

        // Set material properties
        mat->SetVector4("color", tex.color.Linear());

        // Draw text
        Graphics::DrawText(*font, *mat, tex.text, textModel, true, {});*/

        /*Ref<Font> font = tex.font ? tex.font : defaultFont;
        Ref<Material> mat = tex.material ? tex.material : defaultFontMaterial;
        // Calculate text size in pixels (using tex.scale for size)
        Vector2 textSize = font->CalculateTextMetrics(tex.text).size * tex.scale;

        // Adjust for text mesh's pivot (center at {0.5, -0.5} in mesh space)
        // Use unscaled text metrics to compute pivot offset
        Vector2 unscaledTextSize = font->CalculateTextMetrics(tex.text).size;
        //Vector2 textMeshCenter = {0.5f, -0.5f}; // Text mesh pivot point
        Vector2 pivotOffset = unscaledTextSize * (rect.pivot) * tex.scale;
        Vector2 textPos = rect.finalPosition - pivotOffset;
        //textPos = rect.finalPosition;

        // Build model matrix for text
        Matrix4 textModel = math::translate(Vector3(textPos, 0.0f)) *
                            math::scale(Vector3(tex.scale)); // Scale by tex.scale only

        Graphics::DrawText(*font, *mat, tex.text, textModel, true, {});*/

        float canvasScale = 1.0f;
        CanvasComponent* canvas = scene.TryGetComponentInParent<CanvasComponent>(entity);
        if(canvas != nullptr){
            canvasScale = canvas->scaleFactor;
        }

        bool alignWithTop = true;
    
        Ref<Font> font = tex.font ? tex.font : defaultFont;
        Ref<Material> mat = tex.material ? tex.material : defaultFontMaterial;

        // Calculate text size in pixels, including canvas scale
        Vector2 unscaledTextSize = font->CalculateTextMetrics(tex.text).size;
        Vector2 textSize = unscaledTextSize * tex.scale * canvasScale;

        // Adjust for text mesh's pivot based on alignWithTop
        Vector2 textMeshCenter = alignWithTop ? Vector2{0.5f, -0.5f} : Vector2{0.5f, 0.5f};

        // Compute pivot offset to align text center with rect.finalPosition
        Vector2 pivotOffset = (textMeshCenter) * textSize;// Vector2 pivotOffset = (rect.pivot - textMeshCenter) * textSize;
        Vector2 textPos = rect.finalPosition - pivotOffset;

        Matrix4 textModel = math::translate(Vector3(textPos, 0.0f)) * math::scale(Vector3(tex.scale * canvasScale));

        mat->SetVector4("color", tex.color.Linear());
        Graphics::DrawText(*font, *mat, tex.text, textModel, alignWithTop, {});
    }

    // Renderiza filhos recursivamente
    auto& transform = scene.GetComponent<TransformComponent>(entity);
    for (Entity child : transform.Children()) {
        RenderUIRecursive(scene, child, mesh, defaultMaterial, defaultFont, defaultFontMaterial);
    }
}

//Fixme: Shadow Bug
void CameraRenderer::RenderUI(){
    auto uiCamera = Camera{
        OD::Matrix4Identity, 
        OD::math::ortho(0.0f, (float)camera.width, 0.0f, (float)camera.height, -10.0f, 10.0f)
    };
    uiCamera.width = camera.width;
    uiCamera.height = camera.height;
    Graphics::SetCamera(uiCamera);

    RecalculateUI(*scene, uiCamera);

    /*auto view = context->GetScene()->GetRegistry().view<RectTransformComponent, UIImageComponent, TransformComponent>();
    for(auto entity : view){
        const auto& ui = context->GetScene()->GetComponent<RectTransformComponent>(entity);
        auto& uiImage = context->GetScene()->GetComponent<UIImageComponent>(entity);

        Matrix4 model = math::translate(Vector3(ui.finalPosition, 0)) * math::scale(Vector3(ui.finalSize, 1));
        spriteMaterial->SetVector4("color", uiImage.color.Linear());
        Graphics::DrawMesh(*spriteMesh, *spriteMaterial, model);
    }*/

    auto canvasView = scene->GetRegistry().view<CanvasComponent, TransformComponent>(entt::exclude<SelfDisable>);
    for(auto canvasEntity : canvasView){
        auto& transform = scene->GetComponent<TransformComponent>(canvasEntity);
        for(Entity child : transform.Children()){
            RenderUIRecursive(*scene, child, spriteMesh, spriteMaterial, font, fontMaterial);
        }
    }

    /*
    Graphics::SetBlend(true);
    Graphics::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);
    Graphics::SetDepthMask(false);

    //Camera cam = {Matrix4Identity, math::ortho(0.0f, (float)Application::ScreenWidth(), 0.0f, (float)Application::ScreenHeight(), -10.0f, 10.0f)};
    Camera cam2d = {Matrix4Identity, math::ortho(0.0f, (float)camera.width, 0.0f, (float)camera.height, -10.0f, 10.0f)};
    
    auto textView = context->GetScene()->GetRegistry().view<TransformComponent, TextRendererComponent>();
    for(auto entity: textView){
        TransformComponent& trans = textView.get<TransformComponent>(entity);
        TextRendererComponent& text = textView.get<TextRendererComponent>(entity);

        if(text.font == nullptr) continue;
        if(text.material == nullptr) continue;

        if(text.is3d){
            Graphics::SetCamera(camera);
        } else {
            Graphics::SetCamera(cam2d);
        }

        //Graphics::SetDefaultShaderData(*text.material->GetShader(), trans.GlobalModelMatrix());
        SubShader::Bind(*text.material->GetShader());
        //Graphics::SetProjectionViewMatrix(*text.material->GetShader());
        //Graphics::SetModelMatrix(*text.material->GetShader(), trans.GlobalModelMatrix());
        text.material->GetShader()->SetVector4("color", text.color);
        Graphics::DrawText(
            *text.font, 
            *text.material->GetShader(), 
            text.text, 
            trans.GlobalModelMatrix()
        );
    }

    //CanvasComponent canvas;
    //Vector2 resulutionOffset = {(float)camera.width/canvas.size.x, (float)camera.height/canvas.size.y};
    //Vector2 resulutionScale = {resulutionOffset.x/resulutionOffset.y, resulutionOffset.x/resulutionOffset.y};
    //resulutionScale = {0.5f, 0.5f};
    //resulutionScale = {resulutionOffset.x, resulutionOffset.y};

    auto uiTransView = context->GetScene()->GetRegistry().view<TransformComponent, RectTransformComponet>();
    for(auto entity: uiTransView){
        //Entity e(entity, context->GetScene());
        CanvasComponent* canvas = context->GetScene()->TryGetComponentInParent<CanvasComponent>(entity);
        if(canvas == nullptr) continue;

        //Vector2 resulutionOffset = {(float)camera.width/canvas->size.x, (float)camera.height/canvas->size.y};
        //Vector2 resulutionScale = {resulutionOffset.x/resulutionOffset.y, resulutionOffset.x/resulutionOffset.y};
        Vector2 resulutionScale = canvas->GetResulutionScale(camera.width, camera.height); 

        TransformComponent& trans = uiTransView.get<TransformComponent>(entity);
        RectTransformComponet& rectTrans = uiTransView.get<RectTransformComponet>(entity);

        Vector2 parentSize = {camera.width, camera.height};
        Vector2 anchors = rectTrans.anchors + Vector2(1, 1);

        if(trans.HasParent()){
            Entity parent = trans.Parent(); //(trans.Parent(), context->GetScene());
            if(context->GetScene()->HasComponent<RectTransformComponet>(parent)){
                RectTransformComponet& parentRect = context->GetScene()->GetComponent<RectTransformComponet>(parent);
                parentSize = {parentRect.size.x * resulutionScale.x, parentRect.size.y * resulutionScale.y};
                anchors = rectTrans.anchors;
            }
        }

        trans.LocalPosition(Vector3(rectTrans.pos.x, rectTrans.pos.y, 0) * Vector3(resulutionScale.x, resulutionScale.y, 0));
        trans.LocalPosition(
            trans.LocalPosition() +
            Vector3(
                anchors.x * (parentSize.x/2), 
                anchors.y * (parentSize.y/2), 
                1
            )
        );
        //trans.LocalPosition(trans.LocalPosition() * Vector3(resulutionOffset.x, resulutionOffset.y, 0));
    }

    auto uiImageView = context->GetScene()->GetRegistry().view<TransformComponent, RectTransformComponet, UIImageComponent>();
    for(auto entity: uiImageView){
        //Entity e(entity, context->GetScene());
        CanvasComponent* canvas = context->GetScene()->TryGetComponentInParent<CanvasComponent>(entity);
        if(canvas == nullptr) continue;

        //Vector2 resulutionOffset = {(float)camera.width/canvas->size.x, (float)camera.height/canvas->size.y};
        //Vector2 resulutionScale = {resulutionOffset.x/resulutionOffset.y, resulutionOffset.x/resulutionOffset.y};
        Vector2 resulutionScale = canvas->GetResulutionScale(camera.width, camera.height); 

        Graphics::SetCamera(cam2d);

        TransformComponent& trans = uiImageView.get<TransformComponent>(entity);
        RectTransformComponet& rectTrans = uiImageView.get<RectTransformComponet>(entity);
        UIImageComponent& uiImage = uiImageView.get<UIImageComponent>(entity);
        if(uiImage.material == nullptr) continue;
        if(uiImage.sourceImage == nullptr) continue;
        
        Transform scale;
        scale.LocalScale(Vector3(rectTrans.size.x * resulutionScale.x, rectTrans.size.y * resulutionScale.y, 1));

        Matrix4 m = trans.GlobalModelMatrix() * scale.GetLocalModelMatrix();

        auto _mat = uiImage.material;
        Material::SubmitGraphicDatas(*_mat);
        _mat->GetShader()->SetVector4("color", uiImage.color);
        _mat->GetShader()->SetTexture2D("mainTex", *uiImage.sourceImage, 0);
        Graphics::DrawMesh(*spriteMesh, *_mat->GetShader(), m);
    }

    Graphics::SetBlend(true);
    Graphics::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);
    Graphics::SetDepthMask(false);
    auto uiTextView = context->GetScene()->GetRegistry().view<TransformComponent, RectTransformComponet, UITextComponent>();
    for(auto entity: uiTextView){
        //Entity e(entity, context->GetScene());
        CanvasComponent* canvas = context->GetScene()->TryGetComponentInParent<CanvasComponent>(entity);
        if(canvas == nullptr) continue;

        //Vector2 resulutionOffset = {(float)camera.width/canvas->size.x, (float)camera.height/canvas->size.y};
        //Vector2 resulutionScale = {resulutionOffset.x/resulutionOffset.y, resulutionOffset.x/resulutionOffset.y};
        Vector2 resulutionScale = canvas->GetResulutionScale(camera.width, camera.height); 

        Graphics::SetCamera(cam2d);

        TransformComponent& trans = uiTextView.get<TransformComponent>(entity);
        RectTransformComponet& rectTrans = uiTextView.get<RectTransformComponet>(entity);
        UITextComponent& uiText = uiTextView.get<UITextComponent>(entity);
        if(uiText.font == nullptr) continue;
        if(uiText.material == nullptr) continue;
        
        Transform scale;
        scale.LocalScale(
            uiText.scale *
            Vector3(rectTrans.size.x * resulutionScale.x, rectTrans.size.y * resulutionScale.y, 1)
        );

        Matrix4 m = trans.GlobalModelMatrix() * scale.GetLocalModelMatrix();

        auto _mat = uiText.material;
        SubShader::Bind(*_mat->GetShader());
        //Material::SubmitGraphicDatas(*_mat);

        _mat->GetShader()->SetVector4("color", uiText.color);
        Graphics::DrawText(
            *uiText.font, 
            *_mat->GetShader(), 
            uiText.text, 
            m
        );
    }
    Graphics::SetDepthMask(true);// Temp Fix Shadow Bug
    */
}

std::vector<PostFX*> CameraRenderer::GetPostFXs(EnvironmentSettings& environmentSettings){
    std::vector<PostFX*> out;

    //
    //for(auto& i: environmentSettings.customPostPrecessings) out.push_back(i.get());
    //if( environmentSettings.ssaoPostFX != nullptr) out.push_back(environmentSettings.ssaoPostFX.get());
    //if(environmentSettings.ssgiPostFX != nullptr) out.push_back(environmentSettings.ssgiPostFX.get());
    //if(environmentSettings.bloomPostFX != nullptr) out.push_back(environmentSettings.bloomPostFX.get());
    //if(environmentSettings.toneMappingPostFX != nullptr) out.push_back(environmentSettings.toneMappingPostFX.get());
    //if(environmentSettings.colorGradingPostFX != nullptr) out.push_back(environmentSettings.colorGradingPostFX.get());
    //
    out.push_back(gamaCorrectionPP);

    return out;
}

void CameraRenderer::RenderEntityIds(Camera cam, RenderContext* renderContext){
    context->SetupCameraProperties(cam);
    context->BeginDrawToScreen();

    context->BeginDrawEntityIds();
    context->DrawEntityIds(entityIdDrawTarget);
    context->EndDrawEntityIds();
}

#pragma endregion

#pragma region StandRenderPipeline
void StandRenderPipeline::OnInit(Scene& scene){
    auto* globalData = SceneManager::Get().GetGlobalSystem<StandRenderPipelineGlobalData>();

    renderContext = globalData->context;// CreateRef<RenderContext>();

    cameraRenderer.scene = &scene;
    cameraRenderer.passCtx = &passCtx;

    cameraRenderer.GetShadows().scene = &scene;
    cameraRenderer.GetShadows().passCtx = &passCtx;

    cameraRenderer.GetLighting().scene = &scene;
    cameraRenderer.GetLighting().passCtx = &passCtx;
}

void StandRenderPipeline::OnEnd(Scene& scene){
    //delete renderContext;
}

void StandRenderPipeline::SetOverrideFrameBuffer(Ref<Framebuffer> out){
    renderContext->overrideFramebuffer = out; 
}

void StandRenderPipeline::SetOverrideCamera(Camera* cam, Transform trans){
    overrideCamera = cam; 
    overrideCameraTrans = trans; 
}

Ref<Framebuffer> StandRenderPipeline::FinalColor(){
    return renderContext->GetFinalColor();
}

int StandRenderPipeline::ReadEntityId(int x, int y){
    if(overrideCamera != nullptr){
        cameraRenderer.RenderEntityIds(*overrideCamera, renderContext.get());
    }

    return renderContext->ReadPixeIntFromEntityIdsFramebuffer(x, y);
}

void StandRenderPipeline::SaveScreenshot(const std::string& filename){
    Graphics::BeginFramebuffer(*renderContext->GetFinalColor(), false);

    int width = renderContext->GetFinalColor()->Width();
    int height = renderContext->GetFinalColor()->Height();
    int channels = 4;

    // Allocate buffer (RGBA8)
    std::vector<unsigned char> pixels(width * height * channels);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically (OpenGL is upside down)
    for(int y = 0; y < height / 2; y++){
        int opposite = height - y - 1;
        for(int x = 0; x < width * channels; x++){
            std::swap(pixels[y * width * channels + x], pixels[opposite * width * channels + x]);
        }
    }

    // Save PNG
    stbi_write_png(filename.c_str(), width, height, channels, pixels.data(), width * channels);

    Graphics::EndFramebuffer();
}

void StandRenderPipeline::Update(Scene& scene){
    if(scene.Running() == false) return;

    auto viewStaticRendererCluster = scene.GetRegistry().view<TransformComponent, StaticRendererClusterComponent>();
    for(auto [entity, trans, staticRendererCluster]: viewStaticRendererCluster.each()){
        if(staticRendererCluster.autoCollectChildRenderers && staticRendererCluster.started == false){
            staticRendererCluster.started = true;

            auto viewModelRenderer = scene.GetRegistry().view<TransformComponent, ModelRendererComponent>(
                entt::exclude</*StaticRendererComponent,*/ HideInEditor, SelfDisable, SkipDraw>
            );
            for(auto [entity2, t, c]: viewModelRenderer.each()){
                Ref<Model> model = c.GetModel();
                if(model == nullptr) continue;

                Vector3 targetPos = trans.InverseTransformPoint(t.Position());

                if(staticRendererCluster.IsValidPos(targetPos) == false) continue;

                int _i = 0;
                for(auto i: model->renderTargets){
                    if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

                    auto targetMaterial = model->materials[i.materialIndex];
                    auto targetMesh = model->meshs[i.meshIndex];
                    auto targetMatrix = t.GlobalModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
                    auto aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());
                    if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                        targetMaterial = c.GetMaterialsOverride()[i.materialIndex];
                    }

                    staticRendererCluster.AddModel(
                        targetMesh, 
                        targetMaterial, 
                        targetPos, 
                        targetMatrix, 
                        aabb
                    );
                }

                c.draw = false;
                scene.AddTagComponent<SkipDraw>(entity2);
            }
            
            staticRendererCluster.CreateIntancingCommands();
        }
    }
}

int StandRenderPipeline::ExecutionSortPriority(SystemType type){ 
    if(type == SystemType::Late) return 2500;
    return 1; 
}; 

void StandRenderPipeline::LateUpdate(Scene& scene){
    auto viewSkinnedModelSocket = scene.GetRegistry().view<TransformComponent, SkinnedBoneSocket>();
    for(auto [entity, trans, socket]: viewSkinnedModelSocket.each()){
        if(socket.boneIndex < 0) continue;
        if(trans.HasParent() == false) continue;
        if(scene.HasComponent<SkinnedModelRendererComponent>(trans.Parent()) == false) continue;

        SkinnedModelRendererComponent& skinned = scene.GetComponent<SkinnedModelRendererComponent>(trans.Parent());
        if(socket.boneIndex >= skinned.finalPose.Size()) continue;

        //trans.SetLocalModelMatrix(skinned.finalPose.GetGlobalMatrix(socket.boneIndex));

        if(socket.useRootLocalOffsets){
            auto t = skinned.finalPose.GetGlobalTransform(socket.boneIndex);
            trans.LocalPosition(t.TransformPoint(socket.rootLocalPos));
            trans.LocalRotation(t.Rotation() * socket.rootLocalRot);
        } else {
            auto t = skinned.finalPose.GetGlobalTransform(socket.boneIndex);
            trans.LocalPosition(t.Position() + socket.offset);
            trans.LocalRotation(t.Rotation() * Quaternion(math::radians(socket.OffsetEuler)));
        }

        /*auto& pose = skinned.finalPose;
        int parent = pose.GetParent(socket.boneIndex);
        
        if(parent < 0){
            trans.SetLocalModelMatrix(skinned.localTransform.GetLocalModelMatrix() * skinned.skeletonTransform.GetLocalModelMatrix() * pose.GetLocalMatrix(socket.boneIndex));
        } else {
            trans.SetLocalModelMatrix(pose.GetLocalMatrix(socket.boneIndex));
        } */
    }
}

void StandRenderPipeline::SetupFeatures(EnvironmentComponent& env){
    cachedFeatures.clear();

    cachedFeatures.push_back(&env.settings.ssao);
    cachedFeatures.push_back(&env.settings.ssgi);
    cachedFeatures.push_back(&env.settings.bloom);
    cachedFeatures.push_back(&env.settings.toneMapping);
    cachedFeatures.push_back(&env.settings.colorGrading);
    
    env.features.ForEachFeature([&](Ref<RendererFeature> feature){
        cachedFeatures.push_back(feature.get());
    });
    for(auto i: localFeatures){
        cachedFeatures.push_back(i);
    }

    passCtx.SetupFeatures(cachedFeatures);
}

void StandRenderPipeline::RenderNew(Scene& scene){
    //----------Setup Envroment Settings-------------
    ///*
    //environmentSettings = defaultEnvironmentSettings;
    EnvironmentSettings* environmentSettings = &defaultEnvironmentSettings;

    auto enviView = scene.GetRegistry().view<EnvironmentComponent>();
    for(auto entity: enviView){
        EnvironmentComponent& environmentComponent = enviView.get<EnvironmentComponent>(entity);
        environmentSettings = &environmentComponent.settings;

        SetupFeatures(environmentComponent);
        break;
    }

    renderContext->UpdateRenderData(scene, passCtx);

    //----------Scene Render-------------
    renderContext->Begin();
    
    shadow.directional.shadowBias = environmentSettings->shadowBias;
    shadow.maxDistance = environmentSettings->shadowDistance;
    shadow.directional.altasSize = ShadowQualityToShadowTextureSizeLookup[(int)environmentSettings->directionalshadowQuality];
    shadow.other.altasSize = ShadowQualityToShadowTextureSizeLookup[(int)environmentSettings->othershadowQuality];

    shadow.directional.cascadeRatio1 = environmentSettings->directinalShadowCascade[0];
    shadow.directional.cascadeRatio2 = environmentSettings->directinalShadowCascade[1];
    shadow.directional.cascadeRatio3 = environmentSettings->directinalShadowCascade[2];
    shadow.directional.cascadeRatio4 = environmentSettings->directinalShadowCascade[3];

    //cameraRenderer.renderStagePasses = &renderStagePasses;

    camPasses.clear();

    //-----------EnvironmentProbeComponent-----------
    auto envProbeView = scene.GetRegistry().view<EnvironmentProbeComponent, TransformComponent>();
    for(auto entity : envProbeView){
        //if(scene.Running() == false) continue;

        auto& probe = envProbeView.get<EnvironmentProbeComponent>(entity);
        auto& trans = envProbeView.get<TransformComponent>(entity);

        //if(!probe.ShouldUpdate(time)) continue;

        Camera probeCam;
        probeCam.width = probe.resolution;
        probeCam.height = probe.resolution;
        probeCam.nearClip = 0.1f;
        probeCam.farClip = probe.radius * 2.0f;
        probeCam.viewPos = trans.Position();
        probeCam.fov = Mathf::Deg2Rad(90);
        probeCam.projection = glm::perspective(glm::radians(90.0f), 1.0f, probeCam.nearClip, probeCam.farClip);
        //probeCam.view = math::inverse(trans.GlobalModelMatrix());

        bool isDirt = false;
        if(probe.framebuffer != nullptr && probe.genMipmap != probe.framebuffer->Specification().colorAttachments[0].genMip){
            isDirt = true;
        }
  
        if(probe.framebuffer == nullptr || isDirt){
            FrameBufferSpecification framebufferSpecification = {probe.resolution, probe.resolution};
            framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA16F, probe.genMipmap, CalculateMipCount(probe.resolution, probe.resolution)}}; //renderContext->GetFinalColor()->Specification().colorAttachments;
            framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT16}; //renderContext->GetFinalColor()->Specification().depthAttachment;
            framebufferSpecification.type = FramebufferAttachmentType::CUBEMAP; //TEXTURE_2D_MULTISAMPLE
            framebufferSpecification.sample = 1;
            probe.framebuffer = ResourceManager::Get().Create<Framebuffer>(framebufferSpecification);
            probe.framebuffer->name = "Probe";
        }

        if(probe.resolution > 0) probe.framebuffer->Resize(probe.resolution, probe.resolution);

        for(int face = 0; face < 6; ++face){
            //probeCam.view = captureViews[face];  //INFO: this not include transform
            glm::mat4 viewMatrix = captureViews[face];
            viewMatrix = glm::translate(viewMatrix, -probeCam.viewPos);   // or better: inverse(translation * rotation)
            probeCam.view = viewMatrix;
            probeCam.frustum = CreateFrustumFromMatrix(probeCam.projection * probeCam.view);
            
            CameraRenderPass pass = {};
            pass.camera = probeCam;
            pass.target = probe.framebuffer;
            pass.targetFace = face;
            pass.renderingPath = RenderingPath::Forward;   // usually forward for probes
            pass.isReflectionProbePass = true;
            pass.settings = probe.drawSettings;
            pass.renderingPath = probe.renderingPath;
            pass.collectSettings = probe.collectSettings;
            pass.cullingMask = probe.cullingMask.mask;

            //cameraRenderer.RenderPass(pass, renderContext, shadow, *environmentSettings);
            camPasses.push_back(pass);
        }

        //probe.cubemap->GenerateMipmaps();
        //probe.lastUpdateTime = time;
    }

    //-----------CameraComponent-----------
    int width = Application::ScreenWidth();
    int height = Application::ScreenHeight();

    if(overrideCamera != nullptr){
        Entity mainCamera = scene.GetMainCamera();
        auto targetRenderPath = RenderingPath::Forward;
        if(scene.IsValid(mainCamera)){
            auto& cam = scene.GetComponent<CameraComponent>(mainCamera);
            targetRenderPath = cam.renderingPath;
        }

        width = overrideCamera->width;// renderContext->overrideFramebuffer->Width();
        height = overrideCamera->height;// renderContext->overrideFramebuffer->Height();

        CameraRenderPass pass = {};
        pass.camera = *overrideCamera;
        pass.renderingPath = targetRenderPath;
        pass.settings = {true, true, true, true};
        pass.cullingMask = AllLayersMask;
        camPasses.push_back(pass);
    } else {
        auto camView = scene.GetRegistry().view<CameraComponent, TransformComponent, InfoComponent>(entt::exclude<SelfDisable>);
        for(auto entity: camView){
            CameraComponent& cam = camView.get<CameraComponent>(entity);
            TransformComponent& trans = camView.get<TransformComponent>(entity);
            InfoComponent& info = camView.get<InfoComponent>(entity);

            //Assert(Mathf::HasNaN(trans.GlobalModelMatrix()) == false);

            if(renderContext->overrideFramebuffer != nullptr){
                width = renderContext->overrideFramebuffer->Width();
                height = renderContext->overrideFramebuffer->Height();
            }

            if(width > 0 && height > 0) cam.UpdateCameraData(trans, width, height);

            CameraRenderPass pass = {};
            pass.camera = cam.GetCamera();
            pass.renderingPath = cam.renderingPath;
            pass.settings = cam.passRenderSettings;
            pass.collectSettings = cam.collectSettings;
            pass.cullingMask = AllLayersMask;
            camPasses.push_back(pass);
        }
    }

    std::sort(camPasses.begin(), camPasses.end(), [](const auto& a, const auto& b){
        return a.renderOrder < b.renderOrder;
    });

    for(auto& pass: camPasses){
        cameraRenderer.RenderPassNew(pass, renderContext.get(), shadow, *environmentSettings);
    }

    for(auto entity : envProbeView){
        //if(scene.Running() == false) continue;
        auto& probe = envProbeView.get<EnvironmentProbeComponent>(entity);
        if(probe.genMipmap) probe.framebuffer->GenMipmap();
    }

    //cameraRenderer.RenderComposeNew(camPasses);
    renderContext->DrawCompose(camPasses, width, height);

    renderContext->End();
}

void StandRenderPipeline::Render(Scene& scene){
    OD_PROFILE_SCOPE("StandRenderPipeline2::Update");

    RenderNew(scene);
    return;

    //----------Setup Envroment Settings-------------
    ///*
    //environmentSettings = defaultEnvironmentSettings;
    EnvironmentSettings* environmentSettings = &defaultEnvironmentSettings;

    auto enviView = scene.GetRegistry().view<EnvironmentComponent>();
    for(auto entity: enviView){
        EnvironmentComponent& environmentComponent = enviView.get<EnvironmentComponent>(entity);
        environmentSettings = &environmentComponent.settings;

        SetupFeatures(environmentComponent);
        break;
    }

    /*if(environmentSettings->hasInited == false){
        environmentSettings->InitDefault();
        environmentSettings->hasInited = true;
    }*/

    //*/

    /*auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable>
    );
    for(auto e: meshView){
        auto& info = meshView.get<InfoComponent>(e);
        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);

        scene->GetTaskflow().emplace([&](){
            if(info.enable == false) return;
            if(c.mesh == nullptr) return;
            if(c.material == nullptr) return;

            c.renderData.model = t.GlobalModelMatrix();
            c.renderData.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, c.renderData.model);
        });
    }*/
    /*auto modelView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable>
    );
    for(auto e: modelView){
        auto& c = modelView.get<ModelRendererComponent>(e);
        auto& info = modelView.get<InfoComponent>(e);
        auto& t = modelView.get<TransformComponent>(e);

        scene->GetTaskflow().emplace([&](){
            if(info.enable == false || c.model == nullptr){
                c.renderData.clear();
                return;
            }

            c.renderData.resize(c.model->renderTargets.size());
            for(int i = 0; i < c.model->renderTargets.size(); i++){
                c.renderData[i].model = 
                    t.GlobalModelMatrix() 
                    * c.localTransform.GetLocalModelMatrix() 
                    * c.model->skeleton.GetBindPose().GetGlobalMatrix(c.model->renderTargets[i].bindPoseIndex);
                c.renderData[i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());
            }
        });
    }*/
    /*GetScene()->GetExecutor().run(GetScene()->GetTaskflow()).wait(); 
    GetScene()->GetTaskflow().clear();*/

    //----------Scene Render-------------
    renderContext->Begin();

    shadow.directional.shadowBias = environmentSettings->shadowBias;
    shadow.maxDistance = environmentSettings->shadowDistance;
    shadow.directional.altasSize = ShadowQualityToShadowTextureSizeLookup[(int)environmentSettings->directionalshadowQuality];
    shadow.other.altasSize = ShadowQualityToShadowTextureSizeLookup[(int)environmentSettings->othershadowQuality];

    shadow.directional.cascadeRatio1 = environmentSettings->directinalShadowCascade[0];
    shadow.directional.cascadeRatio2 = environmentSettings->directinalShadowCascade[1];
    shadow.directional.cascadeRatio3 = environmentSettings->directinalShadowCascade[2];
    shadow.directional.cascadeRatio4 = environmentSettings->directinalShadowCascade[3];

    //cameraRenderer.renderStagePasses = &renderStagePasses;

    if(overrideCamera != nullptr){
        Entity mainCamera = scene.GetMainCamera();
        auto targetRenderPath = RenderingPath::Forward;
        if(scene.IsValid(mainCamera)){
            auto& cam = scene.GetComponent<CameraComponent>(mainCamera);
            targetRenderPath = cam.renderingPath;
        }

        cameraRenderer.Render(
            *overrideCamera, 
            renderContext.get(), shadow, 
            *environmentSettings,
            targetRenderPath
        );
    } else {
        auto camView = scene.GetRegistry().view<CameraComponent, TransformComponent, InfoComponent>(entt::exclude<SelfDisable>);
        for(auto entity: camView){
            CameraComponent& cam = camView.get<CameraComponent>(entity);
            TransformComponent& trans = camView.get<TransformComponent>(entity);
            InfoComponent& info = camView.get<InfoComponent>(entity);

            int width = Application::ScreenWidth();
            int height = Application::ScreenHeight();
            if(renderContext->overrideFramebuffer != nullptr){
                width = renderContext->overrideFramebuffer->Width();
                height = renderContext->overrideFramebuffer->Height();
            }

            if(width > 0 && height > 0)cam.UpdateCameraData(trans, width, height);
            //LogInfo("Width: %d Height: %d", renderContext->GetFinalColor()->Width(), renderContext->GetFinalColor()->Height());
            cameraRenderer.Render(
                cam.GetCamera(), 
                renderContext.get(), 
                shadow, 
                *environmentSettings, 
                cam.renderingPath
            );
            //LogInfo("Camera Name: %s", info.name.c_str());
            break;
        }
    }

    //-------------Render UI----------
    /*Camera cam = {Matrix4Identity, math::ortho(0.0f, (float)Application::ScreenWidth(), 0.0f, (float)Application::ScreenHeight(), -10.0f, 10.0f)};
    if(overrideCamera != nullptr) cam = {Matrix4Identity, math::ortho(0.0f, (float)overrideCamera->width, 0.0f, (float)overrideCamera->height, -10.0f, 10.0f)};
    Graphics::SetCamera(cam);

    auto textView = GetScene()->GetRegistry().view<TransformComponent, TextRendererComponent>();
    for(auto entity: textView){
        TransformComponent& trans = textView.get<TransformComponent>(entity);
        TextRendererComponent& text = textView.get<TextRendererComponent>(entity);

        if(text.font == nullptr) continue;
        if(text.material == nullptr) continue;

        Graphics::DrawText(
            *text.font, 
            *text.material->GetShader(), 
            text.text, 
            trans.Position(), 
            1.0f, 
            text.color
        );
    }*/

    renderContext->End();
}

void StandRenderPipeline::OnDrawGizmos(Scene& scene, Camera& cm){
    //return;

    /*
    auto meshRenderView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
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
    */

    auto drawGizmosView = scene.GetRegistry().view<GizmosDrawComponent, TransformComponent>();
    for(auto e: drawGizmosView){
        auto& g = drawGizmosView.get<GizmosDrawComponent>(e);
        auto& t = drawGizmosView.get<TransformComponent>(e);
        Graphics::DrawWireCube(
            Transform(t.TransformPoint(g.center), t.Rotation(), g.size).GetModelMatrix(), 
            g.color, 
            1
        );
    }

    //cameraRenderer.GetShadows().DrawCascadeFrustums();
}

void StandRenderPipeline::OnDrawGizmosSelected(Scene& scene, Camera& cm, Entity e){
    //return;
    
    if(scene.HasComponent<MeshRendererComponent>(e)){
        auto& c = scene.GetComponent<MeshRendererComponent>(e);
        auto& t = scene.GetComponent<TransformComponent>(e);
        if(c.mesh != nullptr){
            AABB aabb = c.boundingVolume;
            AABB globalAABB = c.GetGlobalAABB(t);

            Vector3 color = Vector3(0,0,1);
            Transform _t = t.ToTransform();
            if(aabb.isOnFrustum(cm.frustum, _t)) color = Vector3(1, 0, 0);

            Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
        }
    }

    if(scene.HasComponent<ModelRendererComponent>(e)){
        auto& c = scene.GetComponent<ModelRendererComponent>(e);
        auto& t = scene.GetComponent<TransformComponent>(e);
        if(c.GetModel() != nullptr){

            Transform globalTransform = Transform(t.GlobalModelMatrix() /** c.localTransform.GetModelMatrix()*/);
            AABB aabb = c.GetAABB();
            AABB globalAABB = c.GetGlobalAABB(globalTransform);
            globalAABB = transform_aabb_optimized_abs_center_extents(aabb, globalTransform.GetModelMatrix());
            Vector3 color = Vector3(0,0,1);
            if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(1, 0, 0);

            Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);

            /*if(c.finalPose.Size() == c.model->renderTargets.size()){
                for(int i = 0; i < c.model->renderTargets.size(); i++){
                    auto& target = c.model->renderTargets[i];
                    Matrix4 mat = math::simdMul(t.GlobalModelMatrix(), c.finalPose.GetGlobalMatrix(target.bindPoseIndex));

                    AABB aabb = c.GetAABB();
                    AABB globalAABB = transform_aabb_optimized_abs_center_extents(aabb, mat);

                    Vector3 color = Vector3(0,1,0);
                    //if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(0, 1, 0);

                    Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
                }
            }*/
        }
    }

    if(scene.HasComponent<SkinnedModelRendererComponent>(e)){
        auto& c = scene.GetComponent<SkinnedModelRendererComponent>(e);
        auto& t = scene.GetComponent<TransformComponent>(e);
        if(c.GetModel() != nullptr){
            Transform globalTransform = Transform(t.GlobalModelMatrix() * c.localTransform.GetModelMatrix());

            AABB aabb = c.GetAABB();
            AABB globalAABB = c.GetGlobalAABB(globalTransform);

            Vector3 color = Vector3(0,0,1);
            if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(1, 0, 0);

            Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
        }
    }

    if(scene.HasComponent<SkinnedModelRendererComponent>(e) /*&& scene.HasComponent<AnimatorComponent>(e)*/){
        auto& s = scene.GetComponent<SkinnedModelRendererComponent>(e);
        //auto& c = scene.GetComponent<AnimatorComponent>(e);
        auto& t = scene.GetComponent<TransformComponent>(e);
        if(s.GetModel() != nullptr){
            Transform globalTransform = Transform(
                t.GlobalModelMatrix() * s.localTransform.GetModelMatrix() * s.skeletonTransform.GetModelMatrix() * s.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(0)
            );

            Pose pose;
            /*if(scene.Running()){ 
                pose = c.GetLayer(0).controller.GetCurrentPose();
            } else {
                pose = s.GetModel()->skeleton.GetBindPose(); 
            }*/
            pose = s.finalPose;

            for(int i = 0; i < pose.Size(); i++){
                if(pose.GetParent(i) < 0) continue;
                Vector3 p0 = globalTransform.TransformPoint( pose.GetGlobalTransform(i).Position() );
                Vector3 p1 = globalTransform.TransformPoint( pose.GetGlobalTransform(pose.GetParent(i)).Position() );
                Graphics::DrawLine(p0, p1, Vector3(0, 0, 1), 1);

                Graphics::DrawWireCube(Transform(p0, QuaternionIdentity, Vector3(0.05f)).GetModelMatrix(), Vector3(0, 0, 1), 1);
                Graphics::DrawWireCube(Transform(p1, QuaternionIdentity, Vector3(0.025f)).GetModelMatrix(), Vector3(1, 0, 0), 1);
            }
        }
    }

    if(scene.HasComponent<GizmosDrawComponent>(e)){
        auto& g = scene.GetComponent<GizmosDrawComponent>(e);
        auto& t = scene.GetComponent<TransformComponent>(e);
        Graphics::DrawWireCube(
            Transform(t.TransformPoint(g.center), t.Rotation(), g.size).GetModelMatrix(), 
            g.color, 
            1
        );
    }

    if(scene.HasComponent<StaticRendererClusterComponent>(e)){
        auto& staticRenderer = scene.GetComponent<StaticRendererClusterComponent>(e);
        auto& t = scene.GetComponent<TransformComponent>(e);

        auto* subChunk = staticRenderer.GetSubChunkAtPos(staticRenderer.posTest);
        if(subChunk != nullptr){
            Graphics::DrawWireCube(
                Transform(t.TransformPoint(subChunk->bounds.center), t.Rotation(), subChunk->bounds.extents*2.0f).GetModelMatrix(), 
                {1, 0, 0}, 
                1
            );
        }

        for(auto& i: staticRenderer.chunks){
            Graphics::DrawWireCube(
                Transform(t.TransformPoint(i.bounds.center), t.Rotation(), i.bounds.extents*2.0f).GetModelMatrix(), 
                {0, 0, 1}, 
                1
            );

            for(auto& j: i.subchunks){
                Vector3 scale = j.bounds.extents*2.0f;
                scale.y = 0.05f;
                Graphics::DrawWireCube(
                    Transform(t.TransformPoint(j.bounds.center), t.Rotation(), scale).GetModelMatrix(), 
                    {0, 1, 0}, 
                    1
                );
            }
        }
    }
}

#pragma endregion

}