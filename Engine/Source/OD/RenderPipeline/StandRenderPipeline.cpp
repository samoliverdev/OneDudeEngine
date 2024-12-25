#include "StandRenderPipeline.h"
#include "OD/Core/Application.h"
#include "OD/Core/Lua.h"
#include "TextRendererComponent.h"
#include "OD/Scene/SceneManager.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/RenderPipeline/TextRendererComponent.h"
#include "OD/RenderPipeline/UIComponents.h"
#include "OD/Platform/GL.h"
#include "MeshRendererComponent.h"
#include "ModelRendererComponent.h"
#include "SpriteRendererComponent.h"
#include <taskflow/taskflow.hpp>

namespace OD{

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
    SceneManager::Get().RegisterCoreComponent<MeshRendererComponent>("MeshRendererComponent");
    SceneManager::Get().RegisterCoreComponent<ModelRendererComponent>("ModelRendererComponent");
    SceneManager::Get().RegisterCoreComponent<SkinnedModelRendererComponent>("SkinnedModelRendererComponent");
    SceneManager::Get().RegisterCoreComponent<TextRendererComponent>("TextRendererComponent");
    SceneManager::Get().RegisterCoreComponent<SpriteRendererComponent>("SpriteRendererComponent");
    SceneManager::Get().RegisterCoreComponent<RectTransformComponet>("RectTransformComponet");
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
    FrameBufferSpecification specification;
    specification.width = 1024 * 1;
    specification.height = 1024 * 1;
    specification.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    specification.sample = Shadows::maxShadowedDirectionalLightCount * Shadows::maxCascades;
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
    directionalShadowAtlas = new Framebuffer(specification);

    specification.sample = Shadows::maxShadowedOtherLightCount;
    otherShadowAtlas = new Framebuffer(specification);

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

    if(shadowedDirectionalLightCount > 0) 
        RenderDirectionalShadows();
    if(shadowedOtherLightCount > 0) 
        RenderOtherShadows();

    Material::SetGlobalFloat("_ShadowBias", settings.directional.shadowBias);
    Material::SetGlobalFloat(shadowDistanceId, settings.maxDistance);

    float f = 1.0f - settings.directional.cascadeFade;
    Material::SetGlobalVector4(
        Shadows::shadowDistanceFadeId,
        Vector4(
            1.0f / settings.maxDistance, 
            1.0f / settings.distanceFade, 
            0, //1.0f / (1.0f - f * f), 
            1.0f
        )
    );
    
    float altlasSize = (int)settings.directional.altasSize;
    Vector4 altasSizes = Vector4Zero;
    altasSizes.x = altlasSize;
    altasSizes.y = 1.0f / altlasSize;
    Material::SetGlobalVector4(shadowAtlasSizeId, altasSizes);
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

    Material::SetGlobalInt(cascadeCountId, settings.directional.cascadeCount);
    Material::SetGlobalMatrix4(dirShadowMatricesId, dirShadowMatrices, maxShadowedDirectionalLightCount * maxCascades); //FIXME: Revise this 8 propety calculate shadowData size
    Material::SetGlobalFloat(cascadeCullingSpheresId, cascadeCullingSpheres, settings.directional.cascadeCount); //FIXME: Revise this 8 propety calculate shadowData size
    Material::SetGlobalTexture(dirShadowAtlasId, directionalShadowAtlas, -1);
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
    Material::SetGlobalMatrix4(otherShadowMatricesId, otherShadowMatrices, Shadows::maxShadowedOtherLightCount);
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
    Material::SetGlobalInt(dirLightCountId, curDirLightsCount);
    if(curDirLightsCount > 0){
        Material::SetGlobalVector4(dirLightColorsId, dirLightColors, curDirLightsCount);
        Material::SetGlobalVector4(dirLightDirectionsId, dirLightDirections, curDirLightsCount);
        Material::SetGlobalVector4(dirLightShadowDataId, dirLightShadowData, curDirLightsCount);
    }

    Material::SetGlobalInt(otherLightCountId, curOtherLightsCount);
    if(curOtherLightsCount > 0){
        Material::SetGlobalVector4(otherLightColorsId, otherLightColors, curOtherLightsCount);
        Material::SetGlobalVector4(otherLightPositionsId, otherLightPositions, curOtherLightsCount);
        Material::SetGlobalVector4(otherLightDirectionId, otherLightDirections, curOtherLightsCount);
        Material::SetGlobalVector4(otherLightSpotAnglesId, otherLightSpotAngles, curOtherLightsCount);
        Material::SetGlobalVector4(otherLightShadowDataId, otherLightShadowData, curOtherLightsCount);
    }
}
#pragma endregion

#pragma region CameraRenderer
CameraRenderer::CameraRenderer(){
    postFXTest = new PostFXTest(2);
    cubemapSkyMaterial = CreateRef<Material>();
    cubemapSkyMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/SkyboxCubemap.glsl"));
    brdfLUT = Texture2D::CreateBrdfLUTTexture2D();
    spriteMesh = Mesh::CenterQuad(false);
    gamaCorrectionPP = new GamaCorrectionPP();
}

CameraRenderer::~CameraRenderer(){
    delete postFXTest;
    delete gamaCorrectionPP;
}

void CameraRenderer::Render(Camera inCam, RenderContext* inRenderContext, ShadowSettings shadowSettings, EnvironmentSettings environmentSettings, RenderingPath inRenderingPath){
    OD_PROFILE_SCOPE("CameraRenderer::Render");
    // ----------- Setup ----------- 
    camera = inCam;
    renderingPath = inRenderingPath;
    context = inRenderContext;
    shadows.Setup(context, shadowSettings, camera);
    lighting.Setup(context, &shadows, shadowSettings, environmentSettings);
    std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
    for(auto i: postFXs) i->OnSetup();
    
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

    //----------Opaque Settings-----------
    opaqueDrawSettings.enableIntancing = true;
    opaqueDrawSettings.renderQueueRange = RenderQueueRange::Opaue;
    opaqueDrawSettings.sortType = SortType::CommonOpaque;
    opaqueDrawTarget.sortType = CommandBuffer::SortType::CommonOpaque;

    //----------Transparent Settings-----------
    blendDrawSettings.enableIntancing = false;
    blendDrawSettings.renderQueueRange = RenderQueueRange::Transparent;
    blendDrawSettings.sortType = SortType::CommonTransparent;
    blendDrawTarget.sortType = CommandBuffer::SortType::CommonTransparent;

    //tf::Taskflow taskflow;
    //tf::Executor executor;

    context->RenderDataLoop([&](RenderData& data){
        AddRenderData(data); 
        shadows.AddRenderData(data); 
    });

    //executor.run(taskflow).wait();
}

void CameraRenderer::AddRenderData(RenderData& data){
    if(data.aabb.isOnFrustum(camera.frustum) == false) return;

    context->AddDrawRenderers(data, opaqueDrawSettings, opaqueDrawTarget);
    context->AddDrawRenderers(data, blendDrawSettings, blendDrawTarget);
}

void CameraRenderer::RenderVisibleGeometry(EnvironmentSettings& environmentSettings){
    OD_PROFILE_SCOPE("CameraRenderer::RenderVisibleGeometry");

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

    if(environmentSettings.environmentLight == EnvironmentLight::Color){
        Material::SetGlobalVector3("_AmbientLight", environmentSettings.ambient);
        Material::SetGlobalFloat("_SkyLightIntensity", 0);
        //Material::SetGlobalVector3("_IrradianceMapScale", Vector3Zero);
        //Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
    }
    if(environmentSettings.environmentLight == EnvironmentLight::SkyCubemap){
        Material::SetGlobalVector3("_AmbientLight", Vector3Zero);
        Material::SetGlobalCubemap("_IrradianceMap", environmentSettings.skyIrradianceMap);
        Material::SetGlobalCubemap("_PrefilterMap", environmentSettings.skyPrefilterMap);
        Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
        Material::SetGlobalFloat("_SkyLightIntensity", environmentSettings.skyLightIntensity);
    }

    if(renderingPath == RenderingPath::Forward){
        context->BeginForwardPass();
        
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        context->DrawRenderersBuffer(opaqueDrawTarget, false);
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        context->RenderSkyboxLater();
        context->DrawRenderersBuffer(blendDrawTarget, true);
        //Graphics::SetDepthMask(true);
        //RenderSprites();
    } else {
        context->BeginDeferredPass();
        
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        context->DrawRenderersBuffer(opaqueDrawTarget, false, true);
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        context->EndDeferredPassAndCopyToForwardPass();
        context->RenderSkyboxLater();
        context->DrawRenderersBuffer(blendDrawTarget, true);
        //Graphics::SetDepthMask(true);
        //RenderSprites();
    }

    //context->RenderSkyboxLater();
    //context->DrawGizmos();
    
    std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
    context->DrawPostFXs(postFXs);
    context->DrawGizmos();
    for(System* s: context->GetScene()->GetStandSystems()) s->OnRender();
    RenderUI();

    context->EndDrawToScreen();

    /*if(renderingPath == RenderingPath::Forward){
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

        if(environmentSettings.environmentLight == EnvironmentLight::Color){
            Material::SetGlobalVector3("_AmbientLight", environmentSettings.ambient);
            Material::SetGlobalFloat("_SkyLightIntensity", 0);
            //Material::SetGlobalVector3("_IrradianceMapScale", Vector3Zero);
        }
        if(environmentSettings.environmentLight == EnvironmentLight::SkyCubemap){
            Material::SetGlobalVector3("_AmbientLight", Vector3Zero);
            Material::SetGlobalCubemap("_IrradianceMap", environmentSettings.skyIrradianceMap);
            Material::SetGlobalCubemap("_PrefilterMap", environmentSettings.skyPrefilterMap);
            Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
            Material::SetGlobalFloat("_SkyLightIntensity", environmentSettings.skyLightIntensity);
        }

        context->BeginForwardPass();
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        context->DrawRenderersBuffer(opaqueDrawTarget, false);
        context->DrawRenderersBuffer(blendDrawTarget, true);
        RenderSprites();
        if(context->GetSettings().enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if(targetSkyMaterial != nullptr){
            context->skyMaterial = targetSkyMaterial;
            context->RenderSkyboxLater();
        }
        
        std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
        context->DrawPostFXs(postFXs);

        context->DrawGizmos();

        for(System* s: context->GetScene()->GetStandSystems()){
            s->OnRender();
        }

        RenderUI();

        context->EndDrawToScreen();
    } else {
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

        if(environmentSettings.environmentLight == EnvironmentLight::Color){
            Material::SetGlobalVector3("_AmbientLight", environmentSettings.ambient);
            Material::SetGlobalFloat("_SkyLightIntensity", 0);
            //Material::SetGlobalVector3("_IrradianceMapScale", Vector3Zero);
        }
        if(environmentSettings.environmentLight == EnvironmentLight::SkyCubemap){
            Material::SetGlobalVector3("_AmbientLight", Vector3Zero);
            Material::SetGlobalCubemap("_IrradianceMap", environmentSettings.skyIrradianceMap);
            Material::SetGlobalCubemap("_PrefilterMap", environmentSettings.skyPrefilterMap);
            Material::SetGlobalTexture("_BrdfLUT", brdfLUT);
            Material::SetGlobalFloat("_SkyLightIntensity", environmentSettings.skyLightIntensity);
        }

        context->BeginDeferredPass();
        context->DrawRenderersBuffer(opaqueDrawTarget, false, true);
        context->EndDeferredPassAndCopyToForwardPass();
        context->DrawRenderersBuffer(blendDrawTarget, true);

        if(targetSkyMaterial != nullptr){
            context->skyMaterial = targetSkyMaterial;
            context->RenderSkyboxLater();
        }

        std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
        context->DrawPostFXs(postFXs);

        context->DrawGizmos();

        for(System* s: context->GetScene()->GetStandSystems()) s->OnRender();
        
        RenderUI();

        context->EndDrawToScreen();
    }*/
}

void CameraRenderer::RenderSprites(){
    Assert(false && "Outdate");

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
    }
}

//Fixme: Shadow Bug
void CameraRenderer::RenderUI(){
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

        if(/*camera.isDebug*/ text.is3d){
            Graphics::SetCamera(camera);
        } else {
            Graphics::SetCamera(cam2d);
        }

        //Graphics::SetDefaultShaderData(*text.material->GetShader(), trans.GlobalModelMatrix());
        Shader::Bind(*text.material->GetShader());
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
        Entity e(entity, context->GetScene());
        CanvasComponent* canvas = e.TryGetComponentInParent<CanvasComponent>();
        if(canvas == nullptr) continue;

        //Vector2 resulutionOffset = {(float)camera.width/canvas->size.x, (float)camera.height/canvas->size.y};
        //Vector2 resulutionScale = {resulutionOffset.x/resulutionOffset.y, resulutionOffset.x/resulutionOffset.y};
        Vector2 resulutionScale = canvas->GetResulutionScale(camera.width, camera.height); 

        TransformComponent& trans = uiTransView.get<TransformComponent>(entity);
        RectTransformComponet& rectTrans = uiTransView.get<RectTransformComponet>(entity);

        Vector2 parentSize = {camera.width, camera.height};
        Vector2 anchors = rectTrans.anchors + Vector2(1, 1);

        if(trans.HasParent()){
            Entity parent(trans.Parent(), context->GetScene());
            if(parent.HasComponent<RectTransformComponet>()){
                RectTransformComponet& parentRect = parent.GetComponent<RectTransformComponet>();
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
        Entity e(entity, context->GetScene());
        CanvasComponent* canvas = e.TryGetComponentInParent<CanvasComponent>();
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
        Entity e(entity, context->GetScene());
        CanvasComponent* canvas = e.TryGetComponentInParent<CanvasComponent>();
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
        Shader::Bind(*_mat->GetShader());
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
}

std::vector<PostFX*> CameraRenderer::GetPostFXs(EnvironmentSettings& environmentSettings){
    std::vector<PostFX*> out;

    if(environmentSettings.bloomPostFX != nullptr) out.push_back(environmentSettings.bloomPostFX.get());
    if(environmentSettings.toneMappingPostFX != nullptr) out.push_back(environmentSettings.toneMappingPostFX.get());
    out.push_back(gamaCorrectionPP);
    if(environmentSettings.colorGradingPostFX != nullptr) out.push_back(environmentSettings.colorGradingPostFX.get());

    return out;
}

#pragma endregion

#pragma region StandRenderPipeline
StandRenderPipeline::StandRenderPipeline(Scene* inScene):BaseRenderPipeline(inScene){
    renderContext = new RenderContext(scene);
}

StandRenderPipeline::~StandRenderPipeline(){
    
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

void StandRenderPipeline::Update(){
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

    //----------Scene Render-------------
    renderContext->Begin();

    shadow.directional.shadowBias = environmentSettings->shadowBias;
    shadow.maxDistance = environmentSettings->shadowDistance;
    shadow.directional.altasSize = ShadowQualityToShadowTextureSizeLookup[(int)environmentSettings->directionalshadowQuality];
    shadow.other.altasSize = ShadowQualityToShadowTextureSizeLookup[(int)environmentSettings->othershadowQuality];

    
    if(overrideCamera != nullptr){
        Entity mainCamera = GetScene()->GetMainCamera();
        auto targetRenderPath = CameraRenderer::RenderingPath::Forward;
        if(mainCamera.IsValid()){
            auto& cam = mainCamera.GetComponent<CameraComponent>();
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
#pragma endregion

}