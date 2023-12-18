#include "StandRenderPipeline2.h"
#include "OD/Core/Application.h"

namespace OD{

struct Light{
	Vector3 color;
	Vector3 direction;
};

void Lighting::Setup(RenderContext* inContext){
    context = inContext;
    SetupDirectionalLight();
}

void Lighting::SetupDirectionalLight(){
    auto lightView = context->GetScene()->GetRegistry().view<LightComponent, TransformComponent>();
    
    int index = 0;
    for(auto entity: lightView){
        LightComponent& light = lightView.get<LightComponent>(entity);
        TransformComponent& trans = lightView.get<TransformComponent>(entity);

        if(light.type == LightComponent::Type::Directional){
            dirLightColors[index] = Mathf::ToVector4(light.color * light.intensity);
            dirLightDirections[index] = Mathf::ToVector4(-trans.Forward());
        }

        index += 1;
        if(index >= maxDirLightCount) break;
    }

    //Assert(index == 2);

    for(auto i: context->globalShaders){
        Shader::Bind(*i);
        i->SetInt(dirLightCountId, index);
        i->SetVector4(dirLightColorsId, dirLightColors, maxDirLightCount);
        i->SetVector4(dirLightDirectionsId, dirLightDirections, maxDirLightCount);
    }
}

void CameraRenderer::Render(Camera inCam, RenderContext* inRenderContext){
    camera = inCam;
    context = inRenderContext;

    Setup();
    lighting.Setup(context);
    DrawVisibleGeometry();
    Submit();
}

void CameraRenderer::Setup(){
    //----------Opaque Settings-----------
    drawTargets[0].settings.enableIntancing = true;
    drawTargets[0].settings.renderQueueRange = RenderQueueRange::Opaue;
    drawTargets[0].settings.sortType = SortType::CommonOpaque;

    //----------Transparent Settings-----------
    drawTargets[1].settings.enableIntancing = false;
    drawTargets[1].settings.renderQueueRange = RenderQueueRange::Transparent;
    drawTargets[1].settings.sortType = SortType::CommonTransparent;

    context->SetupCameraProperties(camera);
    context->SetupRenderers(drawTargets, 2);
    //context->Clean();
}

void CameraRenderer::DrawVisibleGeometry(){
    context->RenderSkybox();
    context->DrawRenderers(drawTargets, 2);
    //context->DrawGizmos();
}

void CameraRenderer::Submit(){
    context->Submit();
}

StandRenderPipeline2::StandRenderPipeline2(Scene* inScene):BaseRenderPipeline(inScene){
    renderContext = new RenderContext(scene);
}

StandRenderPipeline2::~StandRenderPipeline2(){
    
}

void StandRenderPipeline2::SetOverrideFrameBuffer(Framebuffer* out){
    renderContext->overrideFramebuffer = out; 
}

void StandRenderPipeline2::SetOverrideCamera(Camera* cam, Transform trans){
    overrideCamera = cam; 
    overrideCameraTrans = trans; 
}

Framebuffer* StandRenderPipeline2::FinalColor(){
    return renderContext->GetFinalColor();
}

void StandRenderPipeline2::Update(){
    OD_PROFILE_SCOPE("StandRenderPipeline2::Update");

    //----------Setup Envroment Settings-------------
    ///*
    environmentSettings = EnvironmentSettings();
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
    renderContext->Clean();

    renderContext->skyMaterial = environmentSettings.sky;

    if(overrideCamera != nullptr){
        cameraRenderer.Render(*overrideCamera, renderContext);
    } else {
        auto camView = GetScene()->GetRegistry().view<CameraComponent, TransformComponent, InfoComponent>();
        for(auto entity: camView){
            CameraComponent& cam = camView.get<CameraComponent>(entity);
            TransformComponent& trans = camView.get<TransformComponent>(entity);
            InfoComponent& info = camView.get<InfoComponent>(entity);

            cam.UpdateCameraData(trans, Application::ScreenWidth(), Application::ScreenHeight());
            cameraRenderer.Render(cam.GetCamera(), renderContext);
            //LogInfo("Camera Name: %s", info.name.c_str());
        }
    }

    renderContext->End();
}

}