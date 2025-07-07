#include "StandRenderPipeline.h"
#include "OD/Core/Application.h"
#include "OD/Core/Lua.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Graphics/Geometry.h"
#include "OD/Graphics/Font.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/RenderPipeline/TextRendererComponent.h"
#include "OD/RenderPipeline/UIComponents.h"
#include "OD/Animation/Animator.h"
#include "TextRendererComponent.h"
#include "MeshRendererComponent.h"
#include "ModelRendererComponent.h"
#include "SpriteRendererComponent.h"
#include <taskflow/taskflow.hpp>
#include <cstring>

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

void StandRenderPipelineModuleInit(){
    SceneManager::Get().RegisterCoreComponent<EnvironmentComponent>("EnvironmentComponent");
    SceneManager::Get().RegisterCoreComponent<CameraComponent>("CameraComponent");
    SceneManager::Get().RegisterCoreComponent<LightComponent>("LightComponent");
    SceneManager::Get().RegisterCoreComponent<StaticRendererComponent>("StaticRendererComponent");
    SceneManager::Get().RegisterCoreComponent<MeshRendererComponent>("MeshRendererComponent");
    SceneManager::Get().RegisterCoreComponent<SkinnedMeshRendererComponent>("SkinnedMeshRendererComponent");
    SceneManager::Get().RegisterCoreComponent<ModelRendererComponent>("ModelRendererComponent");
    SceneManager::Get().RegisterCoreComponent<SkinnedModelRendererComponent>("SkinnedModelRendererComponent");
    SceneManager::Get().RegisterCoreComponent<TextRendererComponent>("TextRendererComponent");
    SceneManager::Get().RegisterCoreComponent<SpriteRendererComponent>("SpriteRendererComponent");
    SceneManager::Get().RegisterCoreComponent<RectTransformComponent>("RectTransformComponent");
    SceneManager::Get().RegisterCoreComponent<CanvasComponent>("CanvasComponent");
    SceneManager::Get().RegisterCoreComponent<UIImageComponent>("UIImageComponent");
    SceneManager::Get().RegisterCoreComponent<UITextComponent>("UITextComponent");
    SceneManager::Get().RegisterCoreComponent<GizmosDrawComponent>("GizmosDrawComponent");
    SceneManager::Get().RegisterSystem<StandRenderPipeline>("StandRenderPipeline");

    LuaBindsDB::Get().RegisterLuaBind<CameraComponent>();
    LuaBindsDB::Get().RegisterLuaBind<LightComponent>();
}

#pragma region Shadows
Shadows::Shadows(){
    /*FrameBufferSpecification specification;
    specification.width = 1024 * 1;
    specification.height = 1024 * 1;
    specification.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    specification.sample = Shadows::maxShadowedDirectionalLightCount * Shadows::maxCascades;
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
    directionalShadowAtlas = new Framebuffer(specification);

    specification.sample = Shadows::maxShadowedOtherLightCount;
    otherShadowAtlas = new Framebuffer(specification);*/

    directionalShadowAtlas = new Framebuffer(FramebufferType::Shadowmap, 1024 * 1, 1024 * 1, Shadows::maxShadowedDirectionalLightCount * Shadows::maxCascades);
    otherShadowAtlas = new Framebuffer(FramebufferType::Shadowmap, 1024 * 1, 1024 * 1, Shadows::maxShadowedOtherLightCount);

    shadowPass = CreateRef<Material>();
    shadowPass->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/ShadowMap.glsl"));
}

Shadows::~Shadows(){
    delete directionalShadowAtlas;
    delete otherShadowAtlas;
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
    directionalShadowAtlas->Resize((int)settings.directional.altasSize, (int)settings.directional.altasSize);
    otherShadowAtlas->Resize((int)settings.other.altasSize, (int)settings.other.altasSize);

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

            context->GetScene()->GetTaskflow().emplace([&, index](){
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

        context->GetScene()->GetTaskflow().emplace([&, index](){
            context->RunComputeRenderListShadow(
                {shadowOtherLightsSplits[index].frustum, true}, 
                s,
                shadowOtherLightsBuffers[index],
                shadowPass.get()
            );
        });
    } 
}

void Shadows::AddRenderData(RenderData& data){
    //TODO: Check Split data Culling

    ShadowDrawingSettings s;

    if(data.customShadowPass == nullptr) data.customShadowPass = shadowPass.get();

    int index = -1;
    for(int i = 0; i < shadowedDirectionalLightCount; i++){
        for(int j = 0; j < settings.directional.cascadeCount; j++){
            index += 1;
            if(data.aabb.isOnFrustum(shadowDirectionalLightsSplits[index].frustum) == false) continue;
            context->AddDrawShadow(data, s, shadowDirectionalLightsBuffers[index]);
        }
    } 

    index = -1;
    for(int i = 0; i < shadowedOtherLightCount; i++){
        index += 1;
        if(data.aabb.isOnFrustum(shadowOtherLightsSplits[index].frustum) == false) continue;
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
            context->BeginDrawShadow(directionalShadowAtlas, index);
            context->DrawShadows(shadowDirectionalLightsBuffers[index], shadowDirectionalLightsSplits[index], shadowPass);
            context->EndDrawShadow();

            index += 1;
        }
    }

    //Material::SetGlobalInt(cascadeCountId, settings.directional.cascadeCount);
    //Material::SetGlobalMatrix4(dirShadowMatricesId, dirShadowMatrices, maxShadowedDirectionalLightCount * maxCascades); //FIXME: Revise this 8 propety calculate shadowData size
    //Material::SetGlobalFloat(cascadeCullingSpheresId, cascadeCullingSpheres, settings.directional.cascadeCount); //FIXME: Revise this 8 propety calculate shadowData size
    Material::SetGlobalTexture(dirShadowAtlasId, directionalShadowAtlas, -1);
    context->pipelineData._CascadeCount = settings.directional.cascadeCount;
    std::memcpy(context->pipelineData._DirectionalShadowMatrices, dirShadowMatrices, (maxShadowedDirectionalLightCount * maxCascades) * sizeof(Matrix4)); //FIXME: Revise this 8 propety calculate shadowData size
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

    Material::SetGlobalTexture(otherShadowAltasId, otherShadowAtlas, -1);
    std::memcpy(context->pipelineData._OtherShadowMatrices, otherShadowMatrices, Shadows::maxShadowedOtherLightCount * sizeof(Matrix4));
}

void Shadows::RenderSpotShadows(int index, int split, int tileSize){
    ShadowedOtherLight light = shadowedOtherLights[index];
    context->BeginDrawShadow(otherShadowAtlas, index);
    context->DrawShadows(shadowOtherLightsBuffers[index], shadowOtherLightsSplits[index], shadowPass);
    context->EndDrawShadow();
}

void Shadows::RenderPointShadows(int index, int split, int tileSize){
    ShadowedOtherLight light = shadowedOtherLights[index];
    
    for(int i = 0; i < 6; i++){
        context->BeginDrawShadow(otherShadowAtlas, index+i);
        context->DrawShadows(shadowOtherLightsBuffers[index+i], shadowOtherLightsSplits[index+i], shadowPass);
        context->EndDrawShadow();
    }
}

Vector2 Shadows::ReserveDirectionalShadows(LightComponent light, Transform trans){
    if(shadowedDirectionalLightCount < maxShadowedDirectionalLightCount && light.renderShadow){
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
    
    if(newLightCount > maxShadowedOtherLightCount || light.renderShadow == false){
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
    auto lightView = context->GetScene()->GetRegistry().view<LightComponent, TransformComponent>();
    
    curDirLightsCount = 0;
    curOtherLightsCount = 0;

    for(auto entity: lightView){
        LightComponent& light = lightView.get<LightComponent>(entity);
        TransformComponent& trans = lightView.get<TransformComponent>(entity);

        if(light.type == LightComponent::Type::Directional){
            if(curDirLightsCount >= maxDirLightCount) continue;

            dirLightColors[curDirLightsCount] = light.color * light.intensity; //Mathf::ToVector4(light.color * light.intensity);
            dirLightDirections[curDirLightsCount] = Mathf::ToVector4(-trans.Forward());
            Vector2 v = shadows->ReserveDirectionalShadows(light, trans);
            dirLightShadowData[curDirLightsCount] = Vector4(v.x, v.y, 0, 1);

            curDirLightsCount += 1;
        }  

        if(light.type == LightComponent::Type::Point){
            if(curOtherLightsCount >= maxOtherLightCount) continue;

            otherLightColors[curOtherLightsCount] = light.color * light.intensity; //Mathf::ToVector4(light.color * light.intensity);
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

            otherLightColors[curOtherLightsCount] = light.color * light.intensity;
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
        std::memcpy(context->pipelineData._OtherLightPositions, otherLightDirections, curOtherLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._OtherLightDirections, otherLightDirections, curOtherLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._OtherLightSpotAngles, otherLightSpotAngles, curOtherLightsCount * sizeof(Vector4));
        std::memcpy(context->pipelineData._OtherLightShadowData, otherLightShadowData, curOtherLightsCount * sizeof(Vector4));
    }
}
#pragma endregion

#pragma region CameraRenderer
CameraRenderer::CameraRenderer(){
    //postFXTest = new PostFXTest(2);
    cubemapSkyMaterial = CreateRef<Material>();
    cubemapSkyMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/SkyboxCubemap.glsl"));

    Ref<Texture2D> _brdfLUT = AssetManager::Get().LoadAsset<Texture2D>("brdfLUT");
    if(_brdfLUT == nullptr){
        _brdfLUT = Texture2D::CreateBrdfLUTTexture2D();
        AssetManager::Get().AddAsset<Texture2D>("brdfLUT", _brdfLUT);
    }

    brdfLUT = _brdfLUT; 
    
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

    spriteMaterial = CreateRef<Material>();
    spriteMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Sprite.glsl"));

    spriteMaterial->SetVector4("color", Vector4(1));
    spriteMaterial->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Engine/Textures/White.jpg"));

    font = OD::Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans_Condensed-MediumItalic.ttf");
    fontMaterial = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Engine/Shaders/Font.glsl"));

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
}

CameraRenderer::~CameraRenderer(){
    //delete postFXTest;
    delete gamaCorrectionPP;
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
    blendDrawTarget.Clean();
    entityIdDrawTarget.Clean();

    entityIdDrawSettings.enableIntancing = true;
    entityIdDrawSettings.renderQueueRange = RenderQueueRange::All;
    entityIdDrawSettings.sortType = SortType::None;
    entityIdDrawTarget.sortType = RendererList::SortType::None;// RendererList::SortType::CommonOpaque;

    //----------Opaque Settings-----------
    opaqueDrawSettings.enableIntancing = true;
    opaqueDrawSettings.renderQueueRange = RenderQueueRange::Opaue;
    opaqueDrawSettings.sortType = SortType::CommonOpaque;
    opaqueDrawTarget.sortType = RendererList::SortType::None;// RendererList::SortType::CommonOpaque;

    //----------Transparent Settings-----------
    blendDrawSettings.enableIntancing = false;
    blendDrawSettings.renderQueueRange = RenderQueueRange::Transparent;
    blendDrawSettings.sortType = SortType::CommonTransparent;
    blendDrawTarget.sortType = RendererList::SortType::CommonTransparent;

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

    context->RenderDataLoop([&](RenderData& data){
        AddRenderData(data); 
        shadows.AddRenderData(data); 
    });
    #endif
}

void CameraRenderer::AddRenderData(RenderData& data){
    if(data.aabb.isOnFrustum(camera.frustum) == false) return;

    context->AddDrawRenderers(data, opaqueDrawSettings, opaqueDrawTarget);
    context->AddDrawRenderers(data, blendDrawSettings, blendDrawTarget);
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

void CameraRenderer::RenderVisibleGeometry(EnvironmentSettings& environmentSettings){
    OD_PROFILE_SCOPE("CameraRenderer::RenderVisibleGeometry");

    if(environmentSettings.skyCubemap != nullptr && environmentSettings.skyIrradianceMap == nullptr){
        /*FrameBufferSpecification specification;
        specification.width = 32*4;
        specification.height = 32*4;
        specification.type = FramebufferAttachmentType::CUBEMAP;
        specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
        specification.colorAttachments = {{FramebufferTextureFormat::RGB16F}};
        environmentSettings.skyIrradianceMapF = CreateRef<Framebuffer>(specification);
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
        environmentSettings.skyPrefilterMapF = CreateRef<Framebuffer>(specification);
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

    context->pipelineData._AmbientLight = environmentSettings.ambient;

    if(environmentSettings.environmentLight == EnvironmentLight::Color){
        Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
        context->pipelineData._AmbientLight = environmentSettings.ambient;
        context->pipelineData._SkyLightIntensity = 0;
        context->pipelineData._IrradianceMapScale = Vector4Zero;
    }
    if(environmentSettings.environmentLight == EnvironmentLight::SkyCubemap){
        Material::SetGlobalCubemap("_IrradianceMap", environmentSettings.skyIrradianceMap);
        Material::SetGlobalCubemap("_PrefilterMap", environmentSettings.skyPrefilterMap);
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
        context->BeginForwardPass();
        
        //if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        //context->DrawZPreePassRenderersBuffer(opaqueDrawTarget, false, false);
        //context->DrawZPreePassRenderersBuffer(opaqueDrawTarget, false, true);
        //Graphics::SetDepthTest(DepthTest::LESS);

        //Graphics::SetColorMask(0,0,0,0);
        context->DrawRenderersBuffer(opaqueDrawTarget, true);
        
        //if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        context->DrawRenderersBuffer(blendDrawTarget, true);
        context->RenderSkyboxLater();
        context->DrawGizmos();  
        
        context->EndForwardPass();
    } else {
        context->BeginDeferredPass();
        //if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        context->DrawRenderersBuffer(opaqueDrawTarget, true, true);
        //if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        //context->EndDeferredPass();
        context->EndDeferredPassAndCopyToForwardPass();
        //context->BeginForwardPass();

        //context->RenderSkyboxLater();
        context->DrawRenderersBuffer(blendDrawTarget, true);
        context->RenderSkyboxLater();
        context->DrawGizmos(); 
        context->EndForwardPass();
    }

    std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
    context->DrawPostFXs(postFXs);
    //context->DrawGizmos();
    //for(System* s: context->GetScene()->GetStandSystems()) s->OnRender();
    //RenderUI();

    context->BeginUIPass();
    RenderUI();
    for(auto& i: renderStagePasses->renderPass[(int)RenderStage::UI]){
        i->OnRender(camera);
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
        /*// Get relative text size (in font normalized space) and convert to pixels by * size
        Vector2 textSize = font->CalculateTextMetrics(text).size * size;

        // Apply anchor
        p.x -= textSize.x * anchorX;
        p.y += textSize.y * anchorY;

        auto metrics = font->CalculateTextMetrics(text);
        Vector2 textSize = metrics.size * size;

        // Apply anchor
        p.x -= textSize.x * anchorX;

        // Fix Y: Text metrics size includes ascender and descender, 
        // but the draw baseline is aligned to ascender by default
        float baseline = metrics.ascenderY * size;
        p.y -= baseline; // Shift text baseline to top (like panel)

        // Apply anchorY from top
        p.y -= textSize.y * anchorY;

        // Build model matrix with translation and scale (uniform scale = font height in pixels)
        Matrix4 model = math::translate(Vector3(p, 0.0f)) * math::scale(Vector3(size));*/

        Graphics::DrawText(*defaultFont, *defaultFontMaterial, tex.text, model, true, {});
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

    RecalculateUI(*context->GetScene(), uiCamera);

    /*auto view = context->GetScene()->GetRegistry().view<RectTransformComponent, UIImageComponent, TransformComponent>();
    for(auto entity : view){
        const auto& ui = context->GetScene()->GetComponent<RectTransformComponent>(entity);
        auto& uiImage = context->GetScene()->GetComponent<UIImageComponent>(entity);

        Matrix4 model = math::translate(Vector3(ui.finalPosition, 0)) * math::scale(Vector3(ui.finalSize, 1));
        spriteMaterial->SetVector4("color", uiImage.color.Linear());
        Graphics::DrawMesh(*spriteMesh, *spriteMaterial, model);
    }*/

    auto canvasView = context->GetScene()->GetRegistry().view<CanvasComponent, TransformComponent>();
    for (auto canvasEntity : canvasView) {
        auto& transform = context->GetScene()->GetComponent<TransformComponent>(canvasEntity);
        for (Entity child : transform.Children()) {
            RenderUIRecursive(*context->GetScene(), child, spriteMesh, spriteMaterial, font, fontMaterial);
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

    for(auto& i: environmentSettings.customPostPrecessings) out.push_back(i.get());
    if(environmentSettings.ssaoPostFX != nullptr) out.push_back(environmentSettings.ssaoPostFX.get());
    if(environmentSettings.ssgiPostFX != nullptr) out.push_back(environmentSettings.ssgiPostFX.get());
    if(environmentSettings.bloomPostFX != nullptr) out.push_back(environmentSettings.bloomPostFX.get());
    if(environmentSettings.toneMappingPostFX != nullptr) out.push_back(environmentSettings.toneMappingPostFX.get());
    if(environmentSettings.colorGradingPostFX != nullptr) out.push_back(environmentSettings.colorGradingPostFX.get());
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
StandRenderPipeline::StandRenderPipeline(Scene* inScene):BaseRenderPipeline(inScene){
    renderContext = new RenderContext(scene);
}

StandRenderPipeline::~StandRenderPipeline(){
    delete renderContext;
}

void StandRenderPipeline::SetOverrideFrameBuffer(Framebuffer* out){
    renderContext->overrideFramebuffer = out; 
}

void StandRenderPipeline::SetOverrideCamera(Camera* cam, Transform trans){
    overrideCamera = cam; 
    overrideCameraTrans = trans; 
}

Framebuffer* StandRenderPipeline::FinalColor(){
    return renderContext->GetFinalColor();
}

int StandRenderPipeline::ReadEntityId(int x, int y){
    if(overrideCamera != nullptr){
        cameraRenderer.RenderEntityIds(*overrideCamera, renderContext);
    }

    return renderContext->ReadPixeIntFromEntityIdsFramebuffer(x, y);
}

void StandRenderPipeline::Render(){
    OD_PROFILE_SCOPE("StandRenderPipeline2::Update");

    //----------Setup Envroment Settings-------------
    ///*
    //environmentSettings = defaultEnvironmentSettings;
    EnvironmentSettings* environmentSettings = &defaultEnvironmentSettings;

    auto enviView = GetScene()->GetRegistry().view<EnvironmentComponent>();
    for(auto entity: enviView){
        EnvironmentComponent& environmentComponent = enviView.get<EnvironmentComponent>(entity);
        environmentSettings = &environmentComponent.settings;
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

    cameraRenderer.renderStagePasses = &renderStagePasses;

    if(overrideCamera != nullptr){
        Entity mainCamera = GetScene()->GetMainCamera();
        auto targetRenderPath = CameraRenderer::RenderingPath::Forward;
        if(GetScene()->IsValid(mainCamera)){
            auto& cam = GetScene()->GetComponent<CameraComponent>(mainCamera);
            targetRenderPath = cam.renderingPath == CameraComponent::RenderingPath::Deferred ? CameraRenderer::RenderingPath::Deferred : CameraRenderer::RenderingPath::Forward;
        }

        cameraRenderer.Render(
            *overrideCamera, 
            renderContext, shadow, 
            *environmentSettings,
            targetRenderPath
        );
    } else {
        auto camView = GetScene()->GetRegistry().view<CameraComponent, TransformComponent, InfoComponent>();
        for(auto entity: camView){
            CameraComponent& cam = camView.get<CameraComponent>(entity);
            TransformComponent& trans = camView.get<TransformComponent>(entity);
            InfoComponent& info = camView.get<InfoComponent>(entity);

            cam.UpdateCameraData(trans, renderContext->GetFinalColor()->Width(), renderContext->GetFinalColor()->Height());
            //LogInfo("Width: %d Height: %d", renderContext->GetFinalColor()->Width(), renderContext->GetFinalColor()->Height());
            cameraRenderer.Render(
                cam.GetCamera(), 
                renderContext, 
                shadow, 
                *environmentSettings, 
                cam.renderingPath == CameraComponent::RenderingPath::Deferred ? CameraRenderer::RenderingPath::Deferred : CameraRenderer::RenderingPath::Forward
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

void StandRenderPipeline::OnDrawGizmos(Camera& cm){
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

    auto drawGizmosView = scene->GetRegistry().view<GizmosDrawComponent, TransformComponent>();
    for(auto e: drawGizmosView){
        auto& g = drawGizmosView.get<GizmosDrawComponent>(e);
        auto& t = drawGizmosView.get<TransformComponent>(e);
        Graphics::DrawWireCube(
            Transform(t.TransformPoint(g.center), t.Rotation(), g.size).GetLocalModelMatrix(), 
            g.color, 
            1
        );
    }
}

void StandRenderPipeline::OnDrawGizmosSelected(Camera& cm, Entity e){
    //return;
    
    if(scene->HasComponent<MeshRendererComponent>(e)){
        auto& c = scene->GetComponent<MeshRendererComponent>(e);
        auto& t = scene->GetComponent<TransformComponent>(e);
        if(c.mesh != nullptr){
            AABB aabb = c.boundingVolume;
            AABB globalAABB = c.GetGlobalAABB(t);

            Vector3 color = Vector3(0,0,1);
            Transform _t = t.ToTransform();
            if(aabb.isOnFrustum(cm.frustum, _t)) color = Vector3(1, 0, 0);

            Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
        }
    }

    if(scene->HasComponent<ModelRendererComponent>(e)){
        auto& c = scene->GetComponent<ModelRendererComponent>(e);
        auto& t = scene->GetComponent<TransformComponent>(e);
        if(c.GetModel() != nullptr){

            Transform globalTransform = Transform(t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix());

            AABB aabb = c.GetAABB();
            AABB globalAABB = c.GetGlobalAABB(globalTransform);
            globalAABB = transform_aabb_optimized_abs_center_extents(aabb, globalTransform.GetLocalModelMatrix());

            Vector3 color = Vector3(0,0,1);
            if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(1, 0, 0);

            Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
        }
    }

    if(scene->HasComponent<SkinnedModelRendererComponent>(e)){
        auto& c = scene->GetComponent<SkinnedModelRendererComponent>(e);
        auto& t = scene->GetComponent<TransformComponent>(e);
        if(c.GetModel() != nullptr){
            Transform globalTransform = Transform(t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix());

            AABB aabb = c.GetAABB();
            AABB globalAABB = c.GetGlobalAABB(globalTransform);

            Vector3 color = Vector3(0,0,1);
            if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(1, 0, 0);

            Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
        }
    }

    if(scene->HasComponent<SkinnedModelRendererComponent>(e) && scene->HasComponent<AnimatorComponent>(e)){
        auto& s = scene->GetComponent<SkinnedModelRendererComponent>(e);
        auto& c = scene->GetComponent<AnimatorComponent>(e);
        auto& t = scene->GetComponent<TransformComponent>(e);
        if(s.GetModel() != nullptr){
            Transform globalTransform = Transform(
                t.GlobalModelMatrix() * s.localTransform.GetLocalModelMatrix() * s.skeletonTransform.GetLocalModelMatrix() * s.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(0)
            );

            Pose pose;
            if(scene->Running()){ 
                pose = c.GetLayer(0).controller.GetCurrentPose();
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
    }

    if(scene->HasComponent<GizmosDrawComponent>(e)){
        auto& g = scene->GetComponent<GizmosDrawComponent>(e);
        auto& t = scene->GetComponent<TransformComponent>(e);
        Graphics::DrawWireCube(
            Transform(t.TransformPoint(g.center), t.Rotation(), g.size).GetLocalModelMatrix(), 
            g.color, 
            1
        );
    }
}

#pragma endregion

}