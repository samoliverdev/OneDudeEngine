#include "SynthCity.h"
//#include <OD/RenderPipeline/StandRenderPipeline2.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"

void SynthCitySample::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    //Application::Vsync(true);

    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Scene* scene = SceneManager::Get().NewScene();
    //scene->RemoveSystem<StandRenderPipeline>();
    //scene->AddSystem<StandRenderPipeline2>();

    Entity env = scene->AddEntity("Env");
    EnvironmentComponent& envComp = scene->AddComponent<EnvironmentComponent>(env);
    envComp.settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};
    envComp.settings.environmentLight = EnvironmentLight::SkyCubemap;
    envComp.settings.toneMappingPostFX->enable = true;
    envComp.settings.toneMappingPostFX->mode = ToneMappingPostFX::Mode::Neutral;
    envComp.settings.colorGradingPostFX->enable = true;
    envComp.settings.colorGradingPostFX->contrast = 18;
    envComp.settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};
    envComp.settings.skyCubemap = Cubemap::CreateFromFileHDR("Sandbox/HDRIs/industrial_sunset_puresky_2k.hdr");
    envComp.settings.skyIrradianceMap = Cubemap::CreateIrradianceMapFromCubeMap(envComp.settings.skyCubemap);
    envComp.settings.skyPrefilterMap = Cubemap::CreatePrefilterMapFromCubeMap(envComp.settings.skyCubemap);
    

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
    lightComponent.renderShadow = true;

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0.934, 0.476, 0.548));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-15.355, 50.432, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 160;
    cam.farClipPlane = 10000;
    cam.fieldOfView = 60;

    //Ref<Model> cityModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/PolygonCity/City.fbx");

    Ref<Model> cityModel = CreateRef<Model>();
    Model::CreateFromFile(*cityModel, "C:/Users/sam/Desktop/Apocalipse.fbx", {nullptr, 1, false});

    cityModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit2.glsl"));
    for(auto& i: cityModel->materials) i->SetEnableInstancing(true);

    /*Entity floorEntity = scene->AddEntity("City");
    ModelRendererComponent& floorRenderer = floorEntity.AddComponent<ModelRendererComponent>();
    floorRenderer.SetModel(cityModel);
    TransformComponent& cityTransform = floorEntity.GetComponent<TransformComponent>();
    cityTransform.LocalScale(Vector3(0.01f, 0.01f, 0.01f));*/

    Entity city = scene->Instantiate(cityModel, false);
    scene->GetComponent<TransformComponent>(city).LocalScale(Vector3(0.01f));

    Application::AddModule<Editor>();
    //scene->Start();
}

void SynthCitySample::OnUpdate(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Update();
}   

void SynthCitySample::OnRender(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Draw();
}

void SynthCitySample::OnGUI(){
    
    /*
    if(ImGui::Begin("Profile")){
        for(auto i: Instrumentor::Get().results()){ 
            float durration = (i.end - i.start) * 0.001f;
            ImGui::Text("%s: %.3f.ms", i.name, durration);
        }
        Instrumentor::Get().results().clear();
    }
    ImGui::End();
    */

}

void SynthCitySample::OnResize(int width, int height){}
void SynthCitySample::OnExit(){}