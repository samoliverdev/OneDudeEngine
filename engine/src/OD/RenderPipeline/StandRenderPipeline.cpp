#include "StandRenderPipeline.h"
#include "OD/Core/Application.h"

namespace OD{

#pragma region Shadows
Shadows::Shadows(){
    FrameBufferSpecification specification;
    specification.width = 1024 * 1;
    specification.height = 1024 * 1;
    specification.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    specification.sample = Shadows::maxShadowedDirectionalLightCount * Shadows::maxCascades;
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
    directionalShadowAtlas = new Framebuffer(specification);

    shadowPass = CreateRef<Material>();
    shadowPass->SetShader(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/ShadowMap.glsl"));
}

void Shadows::Setup(RenderContext* inContext, ShadowSettings inSettings, Camera inCam){
    context = inContext;
    settings = inSettings;
    cam = inCam;

    shadowCascadeLevels.clear();
    shadowCascadeLevels.push_back(inSettings.maxDistance * inSettings.directional.cascadeRatio1);
    shadowCascadeLevels.push_back(inSettings.maxDistance * inSettings.directional.cascadeRatio2);
    shadowCascadeLevels.push_back(inSettings.maxDistance * inSettings.directional.cascadeRatio3);
    shadowCascadeLevels.push_back(inSettings.maxDistance * inSettings.directional.cascadeRatio4);
    
    shadowedDirectionalLightCount = 0; 
    directionalShadowAtlas->Resize((int)settings.directional.altasSize, (int)settings.directional.altasSize);

    for(auto& i: shadowDirectionalLightsBuffers){
        i.Clean();
    }
}

void Shadows::OnSetupLoop(RenderData& data){
    //TODO: Check Split data Culling

    ShadowDrawingSettings s;

    int index = -1;
    for(int i = 0; i < shadowedDirectionalLightCount; i++){
        for(int j = 0; j < settings.directional.cascadeCount; j++){
            index += 1;

            if(data.aabb.isOnFrustum(shadowDirectionalLightsSplits[index].frustum, data.transform) == false) continue;
            context->AddDrawShadow(data, s, shadowDirectionalLightsBuffers[index]);
        }
    } 
}

void Shadows::Render(){
    RenderDirectionalShadows();
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
#pragma endregion

#pragma region Lighting
struct Light{
	Vector3 color;
	Vector3 direction;
};

void Lighting::Setup(RenderContext* inContext, Shadows* inShadows, ShadowSettings shadowSettings, EnvironmentSettings inEnvironmentSettings){
    context = inContext;
    shadows = inShadows;
    environmentSettings = inEnvironmentSettings;
    SetupDirectionalLight();
}

void Lighting::SetupDirectionalLight(){
    auto lightView = context->GetScene()->GetRegistry().view<LightComponent, TransformComponent>();
    
    int index = 0;
    for(auto entity: lightView){
        LightComponent& light = lightView.get<LightComponent>(entity);
        TransformComponent& trans = lightView.get<TransformComponent>(entity);

        if(light.type == LightComponent::Type::Directional){
            dirLightColors[index] = light.color * light.intensity; //Mathf::ToVector4(light.color * light.intensity);
            dirLightDirections[index] = Mathf::ToVector4(-trans.Forward());
            Vector2 v = shadows->ReserveDirectionalShadows(light, trans);
            dirLightShadowData[index] = Vector4(v.x, v.y, 0, 1);
        }

        index += 1;
        if(index >= maxDirLightCount) break;
    }

    currentLightsCount = index;

    //Assert(index == 2);
}

void Lighting::UpdateGlobalShaders(){
    //return;

    Material::SetGlobalVector3(ambientLightId, environmentSettings.ambient);

    Material::SetGlobalInt(dirLightCountId, currentLightsCount);
    Material::SetGlobalVector4(dirLightColorsId, dirLightColors, maxDirLightCount);
    Material::SetGlobalVector4(dirLightDirectionsId, dirLightDirections, maxDirLightCount);
    Material::SetGlobalVector4(dirLightShadowDataId, dirLightShadowData, maxDirLightCount);

    Material::SetGlobalInt(Shadows::cascadeCountId, shadows->settings.directional.cascadeCount);
    Material::SetGlobalMatrix4(Shadows::dirShadowMatricesId, shadows->dirShadowMatrices, 16); //FIXME: Revise this 8 propety calculate shadowData size
    Material::SetGlobalFloat(Shadows::cascadeCullingSpheresId, shadows->cascadeCullingSpheres, shadows->settings.directional.cascadeCount); //FIXME: Revise this 8 propety calculate shadowData size
    Material::SetGlobalTexture(Shadows::dirShadowAtlasId, shadows->directionalShadowAtlas, 12, -1);

    //i->SetVector4(Shadows::cascadeDataId, shadows->cascadeData, 8); //FIXME: Revise this 8 propety calculate shadowData size

    Material::SetGlobalFloat("_ShadowBias", shadows->settings.directional.shadowBias);

    Material::SetGlobalFloat(Shadows::shadowDistanceId, shadows->settings.maxDistance);

    float f = 1.0f - shadows->settings.directional.cascadeFade;
    Material::SetGlobalVector4(
        Shadows::shadowDistanceFadeId,
        Vector4(
            1.0f / shadows->settings.maxDistance, 
            1.0f / shadows->settings.distanceFade, 
            0, //1.0f / (1.0f - f * f), 
            1.0f
        )
    );

    float altlasSize = (int)shadows->settings.directional.altasSize;
    Material::SetGlobalVector4(
        Shadows::shadowAtlasSizeId, 
        Vector4(altlasSize, 1.0f / altlasSize, 0, 0)
    );
}
#pragma endregion

#pragma region CameraRenderer
CameraRenderer::CameraRenderer(){
    postFXTest = new PostFXTest(2);
}

void CameraRenderer::Render(Camera inCam, RenderContext* inRenderContext, ShadowSettings shadowSettings, EnvironmentSettings environmentSettings){
    // ----------- Setup ----------- 
    camera = inCam;
    context = inRenderContext;
    shadows.Setup(context, shadowSettings, camera);
    lighting.Setup(context, &shadows, shadowSettings, environmentSettings);
    std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
    for(auto i: postFXs) i->OnSetup();
    
    // ----------- Build Render Datas Loop ----------- 
    // Get All RenderData and Building CommandsBuffer to Post Renderer
    // Building the Render Graph
    RunSetupLoop();
    
    // ----------- Rendering ------------
    shadows.Render();
    lighting.UpdateGlobalShaders();
    RenderVisibleGeometry(environmentSettings);
}

void CameraRenderer::RunSetupLoop(){
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

    context->SetupLoop([&](RenderData& data){
        OnSetupLoop(data);
        shadows.OnSetupLoop(data);
    });
}

void CameraRenderer::OnSetupLoop(RenderData& data){
    //TODO: Check Culling
    if(data.aabb.isOnFrustum(camera.frustum, data.transform) == false) return;

    context->AddDrawRenderers(data, opaqueDrawSettings, opaqueDrawTarget);
    context->AddDrawRenderers(data, blendDrawSettings, blendDrawTarget);
}

void CameraRenderer::RenderVisibleGeometry(EnvironmentSettings& environmentSettings){
    context->BeginDrawToScreen();
    context->ScreenClean();

    context->SetupCameraProperties(camera);
    context->RenderSkybox(environmentSettings.skyboxCubemap);
    context->DrawRenderersBuffer(opaqueDrawTarget);
    context->DrawRenderersBuffer(blendDrawTarget);
    
    //context->DrawGizmos();
    
    std::vector<PostFX*> postFXs = GetPostFXs(environmentSettings);
    context->DrawPostFXs(postFXs);

    context->DrawGizmos();

    context->EndDrawToScreen();
}

std::vector<PostFX*> CameraRenderer::GetPostFXs(EnvironmentSettings& environmentSettings){
    std::vector<PostFX*> out{ 
        environmentSettings.bloomPostFX.get(),
        environmentSettings.colorGradingPostFX.get(),
        environmentSettings.toneMappingPostFX.get()
    };
    return out;
}

#pragma endregion

#pragma region StandRenderPipeline
StandRenderPipeline::StandRenderPipeline(Scene* inScene):BaseRenderPipeline(inScene){
    renderContext = new RenderContext(scene);
}

StandRenderPipeline::~StandRenderPipeline(){
    
}

System* StandRenderPipeline::Clone(Scene* inScene) const { 
    //LogInfo("Clone StandRenderPipeline2");
    return new StandRenderPipeline(inScene); 
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
    environmentSettings = defaultEnvironmentSettings;
    auto enviView = GetScene()->GetRegistry().view<EnvironmentComponent>();
    for(auto entity: enviView){
        EnvironmentComponent& environmentComponent = enviView.get<EnvironmentComponent>(entity);
        if(environmentComponent.inited == false) environmentComponent.Init();

        environmentSettings = environmentComponent.settings;
        break;
    }
    //*/

    //----------Scene Render-------------
    renderContext->Begin();

    renderContext->skyMaterial = environmentSettings.sky;
    shadow.directional.shadowBias = environmentSettings.shadowBias;

    if(overrideCamera != nullptr){
        cameraRenderer.Render(*overrideCamera, renderContext, shadow, environmentSettings);
    } else {
        auto camView = GetScene()->GetRegistry().view<CameraComponent, TransformComponent, InfoComponent>();
        for(auto entity: camView){
            CameraComponent& cam = camView.get<CameraComponent>(entity);
            TransformComponent& trans = camView.get<TransformComponent>(entity);
            InfoComponent& info = camView.get<InfoComponent>(entity);

            cam.UpdateCameraData(trans, renderContext->GetFinalColor()->Width(), renderContext->GetFinalColor()->Height());
            //LogInfo("Width: %d Height: %d", renderContext->GetFinalColor()->Width(), renderContext->GetFinalColor()->Height());
            cameraRenderer.Render(cam.GetCamera(), renderContext, shadow, environmentSettings);
            //LogInfo("Camera Name: %s", info.name.c_str());
            break;
        }
    }

    renderContext->End();
}
#pragma endregion

}