#include "SynthCity.h"
//#include <OD/RenderPipeline/StandRenderPipeline2.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"

void SynthCitySample::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    Application::Vsync(false);

    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Scene* scene = SceneManager::Get().NewScene();
    //scene->RemoveSystem<StandRenderPipeline>();
    //scene->AddSystem<StandRenderPipeline2>();

    Entity env = scene->AddEntity("Env");
    env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = light.AddComponent<LightComponent>();
    lightComponent.color = {1,1,1};
    light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
    light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
    lightComponent.renderShadow = true;

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = camera.AddComponent<CameraComponent>();
    camera.GetComponent<TransformComponent>().LocalPosition(Vector3(-37.4206, -34.8714, 38.0931));
    camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-21.3550, -30.5680, 0));
    camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 160;
    cam.farClipPlane = 10000;
    cam.fieldOfView = 60;

    Ref<Model> cityModel = AssetManager::Get().LoadAsset<Model>(
        "Sandbox/Models/PolygonCity/City.fbx"
    );
    cityModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    for(auto& i: cityModel->materials) i->SetEnableInstancing(true);

    /*Entity floorEntity = scene->AddEntity("City");
    ModelRendererComponent& floorRenderer = floorEntity.AddComponent<ModelRendererComponent>();
    floorRenderer.SetModel(cityModel);
    TransformComponent& cityTransform = floorEntity.GetComponent<TransformComponent>();
    cityTransform.LocalScale(Vector3(0.01f, 0.01f, 0.01f));*/

    Entity city = scene->Instantiate(cityModel);
    city.GetComponent<TransformComponent>().LocalScale(Vector3(0.01f));

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