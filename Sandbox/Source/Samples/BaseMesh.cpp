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

#include <OD/Serializer/Serializer.h>
#include <magic_enum/magic_enum.hpp>

#include <OD/Utils/JobSystem.h>

using namespace OD;

enum class MovementType{
    Idle,
    Walking,
    Running
};

inline std::string save_minimal(const Serializer&, const MovementType& value){ return std::string(magic_enum::enum_name(value)); } 
inline void load_minimal(const Serializer&, MovementType& value, const std::string& text){ value = magic_enum::enum_cast<MovementType>(text).value(); } 

namespace glm{
    void serialize(Serializer& s, glm::vec3& v){
        s.Field("x", v.x);
        s.Field("y", v.y);
        s.Field("z", v.z);
    }

    void serialize(Serializer& s, glm::vec4& v){
        s.Field("x", v.x);
        s.Field("y", v.y);
        s.Field("z", v.z);
        s.Field("w", v.w);
    }

    void serialize(Serializer& s, glm::quat& q){
        s.Field("x", q.x);
        s.Field("y", q.y);
        s.Field("z", q.z);
        s.Field("w", q.w);
    }

    inline void serialize(Serializer& s, glm::mat4& m){
        glm::vec4 col0 = m[0];
        glm::vec4 col1 = m[1];
        glm::vec4 col2 = m[2];
        glm::vec4 col3 = m[3];

        s.Field("col0", col0);
        s.Field("col1", col1);
        s.Field("col2", col2);
        s.Field("col3", col3);

        if(s.IsReading()){
            m[0] = col0;
            m[1] = col1;
            m[2] = col2;
            m[3] = col3;
        }
    }
}

struct _Transform{
    glm::vec3 position{0.0f};
    glm::quat rotation{1, 0, 0, 0};
    glm::vec3 scale{1.0f};
    glm::mat4 matrix{1.0f};

    MovementType movement = MovementType::Idle;
    std::vector<glm::vec3> waypoints;

    void Serialize(Serializer& s){
        s.Field("position", position);
        s.Field("rotation", rotation);
        s.Field("scale", scale);
        s.Field("matrix", matrix);
        s.Field("movement", movement);
        s.Field("waypoints", waypoints);
    }
};

struct Actor {
    int health = 100;
    std::string name = "Player";
    _Transform transform = {};

    void Serialize(Serializer& s) {
        ArchiveDump2(s, health);
        ArchiveDumpNVP2(s, name);
        ArchiveDumpNamed2(s, "Transform", transform);
    }
};

struct Transform2{
    glm::vec3 position;
    glm::quat rotation;

    void Serialize(Serializer& s) {
        s.Field("position", position);
        s.Field("rotation", rotation);
    }
};

void BaseMeshSample::OnInit(){
    {
        Actor actorIn = {};
        Actor actorOut = {};

        actorOut.name = "Test";
        actorOut.transform.position = glm::vec3(1, 2, 3);
        actorOut.transform.scale = glm::vec3(5, 5, 5);

        Serializer writer(Mode::Write);
        writer.Field("actor", actorOut);

        std::ofstream fileOut("actor.json");
        writer.Save(fileOut, Format::Json);
        fileOut.close();

        std::ifstream fileIn("actor.json");
        Serializer reader(Mode::Read);
        reader.Load(fileIn, Format::Json);
        reader.Field("actor", actorIn);

        Assert(actorIn.name == "Test");
        Assert(actorIn.transform.position == glm::vec3(1, 2, 3));

        Transform2 trans2;

        Serializer writer2(Mode::Write);
        writer2.Field("transform", trans2);
        std::ofstream out("transform.bin", std::ios::binary);
        writer2.Save(out, Format::Binary);
        out.close();

        Serializer writer3(Mode::Write);
        writer3.Field("transform", trans2);
        std::ofstream out2("transform.json");
        writer3.Save(out2, Format::Json);
        out2.close();
    }

    {
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
    }

    {
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
    }

    {
        tf::Executor executor;
        mini_jobs::JobSystem jobs(executor);

        int positionsToken = 0;
        int velocitiesToken = 0;
        mini_jobs::Resource position(&positionsToken);
        mini_jobs::Resource velocity(&velocitiesToken);

        jobs.Schedule(
            [](){ LogInfo("UpdateMovement"); },
            mini_jobs::Access{}.Read(velocity).Write(position)
        );
        jobs.Schedule(
            [](){ LogInfo("UpdateAnimation"); },
            mini_jobs::Access{}.Read(position)
        );
        jobs.Schedule(
            [](){ LogInfo("UpdateAI"); },
            mini_jobs::Access{}.Write(velocity)
        );
        
        jobs.Run();
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

    /*font = Resource::CreateFromFile<Font>("Engine/Fonts/OpenSans/static/OpenSans_Condensed-MediumItalic.ttf", FontSettings{32, FontType::Raster});
    Assert(font != nullptr);
    fontMat = ResourceManager::Get().Create<OD::Material>(Shader::CreateFromFile("Engine/Shaders/Font.glsl"));*/

    font =  ResourceManager::Get().LoadByPath<Font>("Engine/Fonts/OpenSans/static/OpenSans_Condensed-MediumItalic.ttf", FontSettings{8*3, FontType::MSDF});
    fontMat = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Engine/Shaders/FontMSDF.glsl"));
    fontMat->SetFloat("pxRange", font->MsdfPxRange());
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

    tt.Position(Vector3(25*2, 50*2, 0));
    tt.Scale(Vector3(25*2));
    Graphics::DrawText(*font, *fontMat, "(C) LearnOpenGL.com 2", tt.GetModelMatrix(), false, {});
    
    Graphics::EndRenderToScreen();

    Graphics::End();
}

void BaseMeshSample::OnGUI(){}
void BaseMeshSample::OnResize(int width, int height){}
void BaseMeshSample::OnExit(){}