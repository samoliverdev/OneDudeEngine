#include "Navmesh.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>

void NavmeshSample::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    Application::Vsync(false);
    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Scene* scene = SceneManager::Get().NewScene();

    Entity env = scene->AddEntity("Env");
    env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

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
    TransformComponent& rootTransform = root.GetComponent<TransformComponent>();
    rootTransform.LocalEulerAngles(Vector3(-90, 0, 0));
    rootTransform.LocalScale(Vector3(20, 20, 5));

        
    Entity navmeshEntity = scene->AddEntity("Navmesh");
    NavmeshComponent& navmeshComp = navmeshEntity.AddComponent<NavmeshComponent>();
    navmeshComp.navmesh = CreateRef<Navmesh>();
    navmeshComp.navmesh->buildSettings.useTile = true;
    navmeshComp.navmesh->buildSettings.cellSize = 0.3f;
    navmeshComp.navmesh->buildSettings.tileSize = 256;
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
    
    float tileSize = navmeshComp.navmesh->buildSettings.tileSize * navmeshComp.navmesh->buildSettings.cellSize;
    float halfTileSize = tileSize/2;

    AABB navmeshBounds = AABB(Vector3(0, 0, 0), 1000, 1000, 1000);
    Vector3 boundsSize(tileSize);

    navmeshComp.navmesh->Bake(scene, navmeshBounds);
    
    //if(navmeshComp.navmesh->useTile == false){
        //navmeshComp.navmesh->Bake(scene, navmeshBounds);
    //} else {
        //navmeshComp.navmesh->TileInit(scene, navmeshBounds);
        //navmeshComp.navmesh->BakeAllTiles(scene, navmeshBounds);
        /*navmeshComp.navmesh->BakeTile(
            scene, 
            navmeshBounds, 
            Vector3(0,0,0)
        );
        navmeshComp.navmesh->BakeTile(
            scene, 
            navmeshBounds, 
            Vector3(tileSize*1, 0, 0)
        );
        navmeshComp.navmesh->BakeTile(
            scene, 
            navmeshBounds,
            Vector3(tileSize*-1, 0, 0)
        );
        navmeshComp.navmesh->BakeTile(
            scene, 
            navmeshBounds, 
            Vector3(0,0,tileSize*-1)
        );*/
    //}

    Entity navmeshAgent = scene->AddEntity("NavmeshAgent");
    NavmeshAgentComponent& agent = navmeshAgent.AddComponent<NavmeshAgentComponent>();
    agent.SetDestination(Vector3(-8, 0, -11));
    navmeshAgentEntity = navmeshAgent.Id();

    Entity e2 = scene->AddEntity("Cube");
    e2.GetComponent<TransformComponent>().Position(Vector3(0, 1, 0));
    e2.GetComponent<TransformComponent>().LocalScale(Vector3(0.5f, 2, 0.5f));
    ModelRendererComponent& _meshRenderer2 = e2.AddComponent<ModelRendererComponent>();
    Assert(cubeModel != nullptr); 
    _meshRenderer2.SetModel(cubeModel);
    Assert(_meshRenderer2.GetMaterialsOverride().size() > 0);
    _meshRenderer2.GetMaterialsOverride()[0] = LoadRockMaterial();
    scene->SetParent(navmeshAgent.Id(), e2.Id());

    Entity target = scene->AddEntity("target");
    target.GetComponent<TransformComponent>().Position(Vector3(-8, 0, -11));
    targetPosEntity = target.Id();

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
    
    //RenderContext::GetSettings().enableGizmosRuntime = true;
    //scene->Start();
    Application::AddModule<Editor>();
}

void NavmeshSample::OnUpdate(float deltaTime){
    if(SceneManager::Get().GetActiveScene()->Running() == false) return;

    Entity target(targetPosEntity, SceneManager::Get().GetActiveScene());
    Entity navmeshAgent(navmeshAgentEntity, SceneManager::Get().GetActiveScene());

    NavmeshAgentComponent& navmeshAgentComp = navmeshAgent.GetComponent<NavmeshAgentComponent>();
    navmeshAgentComp.SetDestination(target.GetComponent<TransformComponent>().Position());
}   

void NavmeshSample::OnRender(float deltaTime){}
void NavmeshSample::OnGUI(){}
void NavmeshSample::OnResize(int width, int height){}
void NavmeshSample::OnExit(){}