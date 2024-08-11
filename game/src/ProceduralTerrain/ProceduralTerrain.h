#pragma once
#include <OD/OD.h>
#include "../Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include "Noise.h"
#include "MeshGenerator.h"
#include "EndlessTerrain.h"

using namespace OD;

struct ProceduralTerrain: OD::Module {
    //CameraMovement camMove;
    Entity camera;

    Ref<NoiseData> dataTest;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<EndlessTerrain>("EndlessTerrain");

        Scene* scene = SceneManager::Get().NewScene();

        Entity text = scene->AddEntity("Text");
        text.GetComponent<TransformComponent>().LocalPosition(Vector3(25.0f, 25.0f, 0));
        TextRendererComponent& textRenderer = text.AddComponent<TextRendererComponent>();
        textRenderer.text = "ProceduralTerrain";
        textRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
        textRenderer.font = CreateRef<Font>("res/Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
        textRenderer.material = CreateRef<Material>(Shader::CreateFromFile("res/Engine/Shaders/Font.glsl"));

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = false;

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 100, 15));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        cam.farClipPlane = 50000;

        int mapChunkSize = 241;
        int levelOfDetail = 0;

        dataTest = Noise::GenerateNoiseMap(8000, 8000, 50, 1, 4, 1, 1, Vector2(0, 0));

        /*Ref<NoiseData> heightMap = Noise::GenerateNoiseMap(mapChunkSize, mapChunkSize, 50, 1, 4, 1, 1, Vector2(0, 0));
        Ref<MeshData> meshData = MeshGenerator::GenerateTerrainMesh(heightMap, 50, levelOfDetail);
        Ref<Mesh> terrainMesh = meshData->CreateMesh();

        Entity floorEntity = scene->AddEntity("Terrain");
        MeshRendererComponent& floorRenderer = floorEntity.AddComponent<MeshRendererComponent>();
        floorRenderer.mesh = terrainMesh;
        floorRenderer.material = LoadFloorMaterial();
        floorRenderer.UpdateAABB();*/

        Entity endlessTerrain = scene->AddEntity("endlessTerrain");
        EndlessTerrain* endlessTerrainScript = endlessTerrain.AddComponent<ScriptComponent>().AddScript<EndlessTerrain>();
        endlessTerrainScript->viewer = &camera.GetComponent<TransformComponent>();

        //scene->Save("res/scene1.scene");
        scene->Start();
        //RenderContext::GetSettings().enableGizmosRuntime = true;
        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override {
        Scene* scene = SceneManager::Get().GetActiveScene();
        if(scene->Running() == false) return;

        if(Input::IsKeyDown(KeyCode::T)) RenderContext::GetSettings().enableWireframe = !RenderContext::GetSettings().enableWireframe;

        if(Input::IsKeyDown(KeyCode::R)){
            TransformComponent& cam = scene->GetMainCamera2().GetComponent<TransformComponent>();

            Entity e = SceneManager::Get().GetActiveScene()->AddEntity("PhysicsCube");
            e.GetComponent<TransformComponent>().Position(cam.Position() + cam.Back() * 20.0f);
            //e.GetComponent<TransformComponent>().LocalScale(Vector3(2));
            PhysicsCubeS* script = e.AddComponent<ScriptComponent>().AddScript<PhysicsCubeS>();
            //script->timeToDestroy = 1000000;
        }

        /*TransformComponent& camT = camera.GetComponent<TransformComponent>();
        RayResult hit;
        //Throwing a Possible Null Expection Pointer Here
        if(scene->GetSystem<PhysicsSystem>()->Raycast(camT.Position(), camT.Back() * 1000.0f, hit)){
            LogInfo("Hitting: %s", hit.entity.GetComponent<InfoComponent>().name.c_str());
        }*/

    }   

    void OnRender(float deltaTime) override {}
    void OnGUI() override {}

    void OnResize(int width, int height) override {}
    void OnExit() override {}
};