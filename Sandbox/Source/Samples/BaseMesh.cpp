#include "OD/pch.h"
#include "BaseMesh.h"
#include "ECSTest.h"
#include <OD/Graphics/Font.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Graphics/CommandBuffer.h>
#include <OD/Scene/Scene.h>
#include <OD/Scene/GroupOfComps.h>
#include <OD/Core/Input.h>
#include <OD/Core/Application.h>
#include <OD/Core/ImGui.h>
#include <OD/RenderPipeline/MeshRendererComponent.h>

using namespace OD;

void BaseMeshSample::OnInit(){
    using CharacterType = OD::GroupOfComps<OD::TransformComponent, OD::MeshRendererComponent>;

    Registry gg;
    auto character1 = CharacterType::Create(
        gg,
        TransformComponent{},
        MeshRendererComponent{}
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
        LogInfo("------Name: {}", name.name.c_str());
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
    mesh.indices.push_back(3);
    mesh.indices.push_back(1);
    mesh.indices.push_back(1);
    mesh.indices.push_back(3);
    mesh.indices.push_back(2);
    mesh.Submit();

    meshMat = ResourceManager::Get().Create<Material>(Resource::CreateFromFile<Shader>("Sandbox/Shaders/test.glsl")); //CreateRef<Material>(Shader::CreateFromFile("Sandbox/Shaders/test.glsl"));

    font = Resource::CreateFromFile<Font>("Engine/Fonts/OpenSans/static/OpenSans_Condensed-MediumItalic.ttf", FontSettings{32, FontType::Raster});
    Assert(font != nullptr);
    fontMat = ResourceManager::Get().Create<OD::Material>(Shader::CreateFromFile("Engine/Shaders/Font.glsl"));
}

void BaseMeshSample::OnUpdate(float deltaTime){
    if(Input::IsKey(KeyCode::D)){ 
        LogInfo("Pressing key: D"); 
    }

    if(Input::IsKeyDown(KeyCode::R)){
        LogInfo("Reloading Shader");
        //meshShader->Reload();
    }
}   

#undef DrawText

void BaseMeshSample::OnRender(float deltaTime){
    CommandBuffer cmd;
    cmd.DrawMesh(mesh, *meshMat, math::translate(Vector3(0.5f, 0, 0)));
    cmd.DrawMesh(mesh, *meshMat, math::translate(Vector3(-0.5f, 0, 0)));

    Graphics::Begin();

    //OD::Graphics::BeginRenderToScreen({0.5f, 0.1f, 0.1f, 1.0f});
    Graphics::BeginRenderToScreen({0.0f, 0.0f, 1.0f, 0.0f});
    Graphics::SetViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());

    Camera cam = {Matrix4Identity, Matrix4Identity};
    Graphics::SetCamera(cam);

    //Graphics::DrawMesh(mesh, *meshMat, math::translate(Vector3(0.5f, 0, 0)));
    //Graphics::DrawMesh(mesh, *meshMat, math::translate(Vector3(-0.5f, 0, 0)));
    cmd.Execute();
    
    cam = {Matrix4Identity,math::ortho(0.0f, (float)Application::ScreenWidth(), 0.0f, (float)Application::ScreenHeight(), -10.0f, 10.0f)};
    Graphics::SetCamera(cam);
    Transform tt;
    tt.Position(Vector3(25*2, 25*2, 0));
    tt.Scale(Vector3(25*2));
    Graphics::DrawText(*font, *fontMat, "(C) LearnOpenGL.com", tt.GetModelMatrix(), false, {});
    
    Graphics::EndRenderToScreen();

    Graphics::End();
}

void BaseMeshSample::OnGUI(){}
void BaseMeshSample::OnResize(int width, int height){}
void BaseMeshSample::OnExit(){}