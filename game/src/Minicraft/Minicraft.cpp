#include "Minicraft.h"

void Minicraft::OnInit(){
    LogInfo("Game Init");

    Application::Vsync(false);
    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
    SceneManager::Get().RegisterScript<RotateScript>("RotateScript");
    SceneManager::Get().RegisterComponent<ChunkComponent>("ChunkComponent");
    OD::Scene* scene = SceneManager::Get().NewScene();

    scene->AddSystem<WorldManagerSystem>();
    
    Entity env = scene->AddEntity("Env");
    env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);
    env.GetComponent<EnvironmentComponent>().settings.shadowQuality = ShadowQuality::Low;

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = camera.AddComponent<CameraComponent>();
    camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 2, 4));
    camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
    camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 20;
    cam.farClipPlane = 1000;

    Entity light = scene->AddEntity("Directional Light");
    LightComponent& lightComponent = light.AddComponent<LightComponent>();
    lightComponent.color = Vector3(1,1,1);
    lightComponent.intensity = 1;
    lightComponent.renderShadow = false;
    light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
    light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));

    /*Entity e2 = scene->AddEntity("Cube");
    e2.GetComponent<TransformComponent>().Position(Vector3(0, 0, 0));
    e2.GetComponent<TransformComponent>().LocalScale(Vector3(1, 1, 1));
    ModelRendererComponent& _meshRenderer2 = e2.AddComponent<ModelRendererComponent>();

    Ref<Model> cubeModel = AssetManager::Get().LoadModel(
        "res/Engine/Models/Cube.obj",
        AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl")
    );
    _meshRenderer2.SetModel(cubeModel);*/

    /*Entity _chunk = scene->AddEntity("Chunk");
    ChunkComponent& chunk = _chunk.AddComponent<ChunkComponent>();
    MeshRendererComponent& mesh = _chunk.AddComponent<MeshRendererComponent>();
    chunk.data.SetSize(32, 32*24);*/

    /*for(int z = 0; z < chunk.data.GetSize(); z++){
        for(int x = 0; x < chunk.data.GetSize(); x++){
            int targetY = random(1, chunk.data.GetHeight());
            for(int y = 0; y < chunk.data.GetHeight(); y++){
                //chunk.data.SetVoxel(x, y, z, y <= targetY ? Voxel{1} : Voxel{0});
                chunk.data.SetVoxel(x, y, z, Voxel{1});
            }
        }
    }*/

    /*for(int z = 0; z < chunk.data.GetSize(); z++){
        for(int x = 0; x < chunk.data.GetSize(); x++){
            int targetY = random(1, chunk.data.GetSize());
            for(int y = 0; y < chunk.data.GetSize(); y++){
                if(chunk.data.GetVoxel(x, y, z).id == 0) continue;

                std::string name = "Voxel(" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z) + ")";
                Entity e = scene->AddEntity(name);

                MeshRendererComponent& mr = e.AddComponent<MeshRendererComponent>();
                mr.mesh = cubeModel->meshs[0];
                mr.material = cubeModel->materials[0];
            
                e.GetComponent<TransformComponent>().LocalPosition(Vector3(x, y, z));

                scene->SetParent(_chunk.Id(), e.Id());
            }
        }
    }*/

    //Application::AddModule<Editor>();
    scene->Start();
}

void Minicraft::OnUpdate(float deltaTime){

}   

void Minicraft::OnRender(float deltaTime){

}

void Minicraft::OnGUI(){

}

void Minicraft::OnResize(int width, int height){}
void Minicraft::OnExit(){}