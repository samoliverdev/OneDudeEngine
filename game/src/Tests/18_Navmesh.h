#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>

using namespace OD;

struct Navmesh_18: OD::Module {
    Entity camera;
    Navmesh navmesh;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        Application::Vsync(true);
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 15, 15));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        cam.farClipPlane = 1000;

        Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/plane.glb");
        Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/Cube.glb");

        /*Entity floorEntity = scene->AddEntity("Floor");
        ModelRendererComponent& floorRenderer = floorEntity.AddComponent<ModelRendererComponent>();
        floorRenderer.SetModel(floorModel);
        floorRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
        RigidbodyComponent& floorEntityP = floorEntity.AddComponent<RigidbodyComponent>();
        floorEntityP.SetShape(CollisionShape::BoxShape({25,0.1f,25}));
        floorEntityP.Mass(0);
        floorEntityP.SetType(RigidbodyComponent::Type::Static);
        floorEntityP.NeverSleep(true);
        //floorEntityP->entity()->transform().localEulerAngles({0,0,-25});

        Entity e2 = scene->AddEntity("Cube");
        e2.GetComponent<TransformComponent>().Position(Vector3(8, 0, 4));
        e2.GetComponent<TransformComponent>().LocalScale(Vector3(4*1, 4*1, 4*1));
        ModelRendererComponent& _meshRenderer2 = e2.AddComponent<ModelRendererComponent>();
        Assert(cubeModel != nullptr); 
        _meshRenderer2.SetModel(cubeModel);
        Assert(_meshRenderer2.GetMaterialsOverride().size() > 0);
        _meshRenderer2.GetMaterialsOverride()[0] = LoadFloorMaterial();*/
        
        Ref<Model> enviromentModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/NavmeshEnviromentTest.blend");
        for(int i = 0; i < enviromentModel->materials.size(); i++){
            enviromentModel->materials[i] = LoadFloorMaterial();
        }
        Entity root = scene->Instantiate(enviromentModel);
        TransformComponent& rootTransform = root.GetComponent<TransformComponent>();
        rootTransform.LocalEulerAngles(Vector3(-90, 0, 0));

        Entity navmeshEntity = scene->AddEntity("Navmesh");
        NavmeshComponent& navmeshComp = navmeshEntity.AddComponent<NavmeshComponent>();
        navmeshComp.navmesh = CreateRef<Navmesh>();
        navmeshComp.navmesh->buildSettings.cellSize = 0.3f;
        navmeshComp.navmesh->buildSettings.cellHeight = 0.2f;
        navmeshComp.navmesh->buildSettings.agentHeight = 2.0f;
        navmeshComp.navmesh->buildSettings.agentRadius = 0.6f;
        navmeshComp.navmesh->buildSettings.agentMaxClimb = 0.9f;
        navmeshComp.navmesh->buildSettings.agentMaxSlope = 45.0f;
        navmeshComp.navmesh->buildSettings.regionMinSize = 8;
        navmeshComp.navmesh->buildSettings.regionMergeSize = 20;
        navmeshComp.navmesh->buildSettings.edgeMaxLen = 12.0f;
        navmeshComp.navmesh->buildSettings.edgeMaxError = 1.3f;
        navmeshComp.navmesh->buildSettings.vertsPerPoly = 6.0f;
        navmeshComp.navmesh->buildSettings.detailSampleDist = 6.0f;
        navmeshComp.navmesh->buildSettings.detailSampleMaxError = 1.0f;
        navmeshComp.navmesh->buildSettings.partitionType = SAMPLE_PARTITION_WATERSHED;
        navmeshComp.navmesh->Bake(scene, AABB(Vector3(0, 0, 0), 100, 100, 100));

        Entity navmeshAgent = scene->AddEntity("NavmeshAgent");
        NavmeshAgentComponent& agent = navmeshAgent.AddComponent<NavmeshAgentComponent>();
        agent.SetDestination(Vector3(-8, 0, -11));

        /*NavMeshPath path;
        bool result = navmeshComp.navmesh->FindPath(Vector3(0, 0, 0), Vector3(-8, 0, -11), path);
        LogWarning("Path Corneis: %zd", path.corners.size());*/

        /*navmesh.buildSettings.cellSize = 0.3f;
        navmesh.buildSettings.cellHeight = 0.2f;
        navmesh.buildSettings.agentHeight = 2.0f;
        navmesh.buildSettings.agentRadius = 0.6f;
        navmesh.buildSettings.agentMaxClimb = 0.9f;
        navmesh.buildSettings.agentMaxSlope = 45.0f;
        navmesh.buildSettings.regionMinSize = 8;
        navmesh.buildSettings.regionMergeSize = 20;
        navmesh.buildSettings.edgeMaxLen = 12.0f;
        navmesh.buildSettings.edgeMaxError = 1.3f;
        navmesh.buildSettings.vertsPerPoly = 6.0f;
        navmesh.buildSettings.detailSampleDist = 6.0f;
        navmesh.buildSettings.detailSampleMaxError = 1.0f;
        navmesh.buildSettings.partitionType = SAMPLE_PARTITION_WATERSHED;
        navmesh.Bake(scene, AABB(Vector3Zero, Vector3One * 200.0f));*/
        
        //scene->Start();
        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override {}   
    void OnRender(float deltaTime) override {}
    void OnGUI() override {}
    void OnResize(int width, int height) override {}
    void OnExit() override {}
};