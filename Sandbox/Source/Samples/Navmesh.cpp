#include "OD/pch.h"
#include "Navmesh.h"
#include "Ultis/Ultis.h"
#include <OD/Scene/SceneManager.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/Graphics/Model.h>
#include <OD/Navmesh/Navmesh.h>
#include <OD/Core/Application.h>
#include <OD/Editor/Editor.h>
#include <assert.h>
//#include <entt/entt.hpp>

void NavmeshSample::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    Application::Vsync(false);
    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Ref<Scene> scene = SceneManager::Get().NewScene();

    Entity env = scene->AddEntity("Env");
    scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    cam.farClipPlane = 1000;
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 15, 15));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    
    Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/plane.glb");
    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.glb");

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
    
    Ref<Model> enviromentModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/NavmeshEnviromentTest.blend");
    for(int i = 0; i < enviromentModel->materials.size(); i++){
        enviromentModel->materials[i] = LoadFloorMaterial();
    }
    Entity root = scene->Instantiate(enviromentModel);
    TransformComponent& rootTransform = scene->GetComponent<TransformComponent>(root);
    rootTransform.LocalEulerAngles(Vector3(-90, 0, 0));
    rootTransform.LocalScale(Vector3(20, 20, 5));

        
    Entity navmeshEntity = scene->AddEntity("Navmesh");
    NavmeshComponent& navmeshComp = scene->AddComponent<NavmeshComponent>(navmeshEntity);
    navmeshComp.navmesh = CreateRef<Navmesh>();
    navmeshComp.buildSettings.useTile = true;
    navmeshComp.buildSettings.cellSize = 0.3f;
    navmeshComp.buildSettings.tileSize = 256;
    navmeshComp.buildSettings.cellHeight = 0.2f;
    navmeshComp.buildSettings.agentHeight = 2.0f;
    navmeshComp.buildSettings.agentRadius = 0.6f;
    navmeshComp.buildSettings.agentMaxClimb = 0.9f;
    navmeshComp.buildSettings.agentMaxSlope = 45.0f;
    navmeshComp.buildSettings.regionMinSize = 8;
    navmeshComp.buildSettings.regionMergeSize = 20;
    navmeshComp.buildSettings.edgeMaxLen = 12.0f;
    navmeshComp.buildSettings.edgeMaxError = 1.3f;
    navmeshComp.buildSettings.vertsPerPoly = 6.0f;
    navmeshComp.buildSettings.detailSampleDist = 6.0f;
    navmeshComp.buildSettings.detailSampleMaxError = 1.0f;
    navmeshComp.buildSettings.partitionType = SAMPLE_PARTITION_WATERSHED;

    AABB navmeshBounds = AABB(Vector3(0, 0, 0), 1000, 1000, 1000);
    navmeshComp.navmesh->Bake(scene.get(), navmeshBounds, navmeshComp.buildSettings);
    
    Entity navmeshAgent = scene->AddEntity("NavmeshAgent");
    NavmeshAgentComponent& agent = scene->AddComponent<NavmeshAgentComponent>(navmeshAgent);
    agent.SetDestination(Vector3(-8, 0, -11));
    navmeshAgentEntity = navmeshAgent;

    Entity e2 = scene->AddEntity("Cube");
    scene->GetComponent<TransformComponent>(e2).Position(Vector3(0, 1, 0));
    scene->GetComponent<TransformComponent>(e2).LocalScale(Vector3(0.5f, 2, 0.5f));
    ModelRendererComponent& _meshRenderer2 = scene->AddComponent<ModelRendererComponent>(e2);
    Assert(cubeModel != nullptr); 
    _meshRenderer2.SetModel(cubeModel);
    Assert(_meshRenderer2.GetMaterialsOverride().size() > 0);
    _meshRenderer2.GetMaterialsOverride()[0] = LoadRockMaterial();
    scene->SetParent(navmeshAgent, e2);

    Entity target = scene->AddEntity("target");
    scene->GetComponent<TransformComponent>(target).Position(Vector3(-8, 0, -11));
    targetPosEntity = target;
    
    //RenderContext::GetSettings().enableGizmosRuntime = true;
    //scene->Start();
    Application::AddModule<Editor>();
}

void NavmeshSample::OnUpdate(float deltaTime){
    if(SceneManager::Get().GetActiveScene()->Running() == false) return;
    auto& scene = *SceneManager::Get().GetActiveScene();

    //Entity target(targetPosEntity, SceneManager::Get().GetActiveScene());
    //Entity navmeshAgent(navmeshAgentEntity, SceneManager::Get().GetActiveScene());

    NavmeshAgentComponent& navmeshAgentComp = scene.GetComponent<NavmeshAgentComponent>(navmeshAgentEntity);
    navmeshAgentComp.SetDestination(scene.GetComponent<TransformComponent>(targetPosEntity).Position());
}   

void NavmeshSample::OnRender(float deltaTime){}
void NavmeshSample::OnGUI(){}
void NavmeshSample::OnResize(int width, int height){}
void NavmeshSample::OnExit(){}