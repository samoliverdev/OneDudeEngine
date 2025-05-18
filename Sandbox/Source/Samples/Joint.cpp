#include "Joint.h"
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"

void JointSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    OD::Scene& scene = *SceneManager::Get().NewScene();

    Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/plane.obj");
    floorModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));

    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.obj");
    cubeModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));

    Entity env = scene.AddEntity("Env");
    scene.AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity camera = scene.AddEntity("Camera");
    CameraComponent& cam = scene.AddComponent<CameraComponent>(camera);
    scene.GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 2, 4));
    scene.GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    scene.AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    //camMove.transform = &camera->GetComponent<TransformComponent>()();
    //camMove.moveSpeed = 60;
    cam.farClipPlane = 1000;

    Entity e = scene.AddEntity("Floor");
    scene.GetComponent<TransformComponent>(e).Position(Vector3(0,-2, 0));
    scene.GetComponent<TransformComponent>(e).LocalScale(Vector3(10, 1, 10));
    ModelRendererComponent& _meshRenderer = scene.AddComponent<ModelRendererComponent>(e);
    _meshRenderer.SetModel(floorModel);
    for(auto& i: _meshRenderer.GetMaterialsOverride()) i = LoadFloorMaterial();
    RigidbodyComponent& rb = scene.AddComponent<RigidbodyComponent>(e);
    rb.SetShape(CollisionShape::BoxShape({10, 0.1f, 10}));
    rb.SetType(RigidbodyComponent::Type::Static);
    rb.Mass(0);

    ///////////////////
    /*Entity e2 = scene.AddEntity("Cube");
    scene.GetComponent<TransformComponent>(e2).Position(Vector3(0, 1, 0));
    scene.GetComponent<TransformComponent>(e2).LocalScale(Vector3(1, 1, 1));
    ModelRendererComponent& _meshRenderer2 = scene.AddComponent<ModelRendererComponent>(e2);
    _meshRenderer2.SetModel(cubeModel);
    for(auto& i: _meshRenderer2.GetMaterialsOverride()) i = LoadRockMaterial();
    RigidbodyComponent& rb2 = scene.AddComponent<RigidbodyComponent>(e2);
    rb2.SetShape(CollisionShape::BoxShape({0.5f, 1, 0.5f}));*/
    /*
    JointComponent& j = scene.AddComponent<JointComponent>(e2);
    j.pivot = {0, -0.5f, 0};
    j.disableSelfCollision = true;
    j.angularLowerLimit = {-45, 0, 0};
    j.angularUpperLimit = {45, 0, 0};
    */
    ///////////////////

    /*Entity e3 = scene.AddEntity("Cube");
    scene.GetComponent<TransformComponent>(e3).Position(Vector3(0, 2.5f, 0));
    scene.GetComponent<TransformComponent>(e3).LocalScale(Vector3(1, 1, 1));
    ModelRendererComponent& _meshRenderer3 = scene.AddComponent<ModelRendererComponent>(e3);
    _meshRenderer3.SetModel(cubeModel);
    for(auto& i: _meshRenderer3.GetMaterialsOverride()) i = LoadRockMaterial();
    RigidbodyComponent& rb3 = scene.AddComponent<RigidbodyComponent>(e3);
    rb3.SetShape(CollisionShape::BoxShape({0.2f, 2, 0.2f}));
    JointComponent& j2 = scene.AddComponent<JointComponent>(e3);
    j2.pivot = {0, -1, 0};
    j2.connectedBody = e2;
    j2.disableSelfCollision = true;
    j2.angularLowerLimit = {-45, -0, -0};
    j2.angularUpperLimit = {45, 0, 0};

    Entity e4 = scene.AddEntity("Cube");
    scene.GetComponent<TransformComponent>(e4).Position(Vector3(0, 4.5f, 0));
    scene.GetComponent<TransformComponent>(e4).LocalScale(Vector3(1, 1, 1));
    ModelRendererComponent& _meshRenderer4 = scene.AddComponent<ModelRendererComponent>(e4);
    _meshRenderer4.SetModel(cubeModel);
    for(auto& i: _meshRenderer4.GetMaterialsOverride()) i = LoadRockMaterial();
    RigidbodyComponent& rb4 = scene.AddComponent<RigidbodyComponent>(e4);
    rb4.SetShape(CollisionShape::BoxShape({0.2f, 2, 0.2f}));
    JointComponent& j3 = scene.AddComponent<JointComponent>(e4);
    j3.pivot = {0, -1, 0};
    j3.connectedBody = e3;
    j3.disableSelfCollision = true;
    j3.angularLowerLimit = {-45, -0, -0};
    j3.angularUpperLimit = {45, 0, 0};*/

    Entity ragdoll = scene.AddEntity("Ragdoll");
    SkinnedModelRendererComponent& skinnedRagdoll = scene.AddComponent<SkinnedModelRendererComponent>(ragdoll);
    skinnedRagdoll.SetModel(AssetManager::Get().LoadAsset<Model>("Sandbox/Models/RagdollTest.glb"));
    for(auto& i: skinnedRagdoll.GetMaterialsOverride()) i = LoadRockMaterial();
    RagdollComponent& ragdollComp = scene.AddComponent<RagdollComponent>(ragdoll);
    ragdollComp.parts.resize(3);
    ragdollComp.parts[0] = { CollisionShape::BoxShape(Vector3(0.2f, 0.666667f, 0.2f), {0, 0.666667f * 0.5f, 0}), -1, 4 };
    ragdollComp.parts[1] = { CollisionShape::BoxShape(Vector3(0.2f, 0.666667f, 0.2f), {0, 0.666667f * 0.5f, 0}), 0, 5 };
    ragdollComp.parts[2] = { CollisionShape::BoxShape(Vector3(0.2f, 0.666667f, 0.2f), {0, 0.666667f * 0.5f, 0}), 1, 6 };

    ragdollComp.parts[1].twistAxis = {1, 0, 0};
    ragdollComp.parts[1].twistAngleMin = -90;
    ragdollComp.parts[1].twistAngleMax = 90;
    ragdollComp.parts[1].normalAngle = 0;
    ragdollComp.parts[1].planeAngle = 0;

    ragdollComp.parts[2].twistAxis = {1, 0, 0};
    ragdollComp.parts[2].twistAngleMin = 0;
    ragdollComp.parts[2].twistAngleMax = 45;
    ragdollComp.parts[2].normalAngle = 0;
    ragdollComp.parts[2].planeAngle = 0;

    /*scene.AddEntityWith<TransformComponent, ModelRendererComponent>
    ("Plane", [&](auto& transform, auto& meshRenderer){});*/

    RenderContext::GetSettings().enableGizmosRuntime = true;
    Application::AddModule<Editor>();
    //scene.Start();
}

void JointSample::OnUpdate(float deltaTime){

}

void JointSample::OnRender(float deltaTime){}
void JointSample::OnGUI(){}
void JointSample::OnResize(int width, int height){}
void JointSample::OnExit(){}