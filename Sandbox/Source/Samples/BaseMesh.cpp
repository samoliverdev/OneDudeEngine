#include "BaseMesh.h"
#include "ECSTest.h"

void BaseMeshSample::OnInit(){
    using CharacterType = OD::GroupOfComps<OD::TransformComponent, OD::MeshRendererComponent>;

    OD::Registry gg;
    auto character1 = CharacterType::Create(
        gg,
        OD::TransformComponent{},
        OD::MeshRendererComponent{}
    );
    auto character2 = CharacterType::Create(
        gg,
        [](auto& trans, auto& renderer){

        }
    );


    struct Position { float x, y; };
    struct Velocity { float dx, dy; };
    struct Name {std::string name; };

    registry reg;
    
    entity e1 = reg.create();
    reg.add<Position>(e1, 1.0f, 2.0f);
    reg.add<Velocity>(e1, 0.5f, 1.5f);
    //reg.add<Name>(e1, "e1");
    
    entity e2 = reg.create();
    reg.add<Position>(e2, 3.0f, 4.0f);
    reg.add<Velocity>(e2, 0.5f, 1.5f);
    reg.add<Name>(e2, "e2");
    
    // Iterate through all entities with both Position and Velocity
    auto moving = reg.View<Position, Velocity, Name>();
    for (entity e : moving) {
        auto& pos = reg.get<Position>(e);
        auto& vel = reg.get<Velocity>(e);
        auto& name = reg.get<Name>(e);
        pos.x += vel.dx;
        pos.y += vel.dy;
        LogInfo("------Name: %s", name.name.c_str());
    }
    
    LogInfo("Game Init");
    //Assert(false);
    //OD::Application::Vsync(false);

    mesh.vertices.push_back(OD::Vector3(0.5f, 0.5f, 0));
    mesh.vertices.push_back(OD::Vector3(0.5f, -0.5f, 0));
    mesh.vertices.push_back(OD::Vector3(-0.5f, -0.5f, 0));
    mesh.vertices.push_back(OD::Vector3(-0.5f, 0.5f, 0));
    mesh.uv.push_back(OD::Vector3(1, 1, 0));
    mesh.uv.push_back(OD::Vector3(1, 0, 0));
    mesh.uv.push_back(OD::Vector3(0, 0, 0));
    mesh.uv.push_back(OD::Vector3(0, 1, 0));
    mesh.indices.reserve(10);
    mesh.indices.push_back(0);
    mesh.indices.push_back(1);
    mesh.indices.push_back(3);
    mesh.indices.push_back(1);
    mesh.indices.push_back(2);
    mesh.indices.push_back(3);
    mesh.Submit();

    meshMat = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Sandbox/Shaders/test.glsl"));

    auto lit = OD::Shader::CreateFromFile("Engine/Shaders/Lit.glsl");
}

void BaseMeshSample::OnUpdate(float deltaTime){
    if(OD::Input::IsKey(OD::KeyCode::D)){ 
        LogInfo("Pressing key: D"); 
    }

    if(OD::Input::IsKeyDown(OD::KeyCode::R)){
        LogInfo("Reloading Shader");
        //meshShader->Reload();
    }
}   

void BaseMeshSample::OnRender(float deltaTime){
    OD:: Graphics::Begin();

    OD::Camera cam = {OD::Matrix4Identity, OD::Matrix4Identity};
    OD::Graphics::SetCamera(cam);
    OD::Graphics::Clean(0.1f, 0.1f, 0.1f, 1);

    //Graphics::DrawMesh(mesh, *meshMat, Matrix4Identity);
    OD::Graphics::BeginRenderToScreen();
    OD::Graphics::DrawMesh(mesh, *meshMat, OD::math::translate(OD::Vector3(0.5f, 0, 0)));
    OD::Graphics::DrawMesh(mesh, *meshMat, OD::math::translate(OD::Vector3(-0.5f, 0, 0)));
    OD::Graphics::EndRenderToScreen();
    
    OD::Graphics::End();
}

void BaseMeshSample::OnGUI(){
    static bool show;
    ImGui::ShowDemoWindow(&show);
}

void BaseMeshSample::OnResize(int width, int height){}
void BaseMeshSample::OnExit(){}