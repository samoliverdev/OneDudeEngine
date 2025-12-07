#include "Serializer.h"
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include <OD/Core/Application.h>
#include <OD/Core/TarPackage.h>
#include <OD/Scene/SceneManager.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/MeshRendererComponent.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Cubemap.h>
#include <OD/Editor/Editor.h>
#include <fstream>

#include "SerializerStatic.h"
#include "SerializerStatic2.h"

struct _Transform{
    float x = 0;
    float y = 0;

    template<typename Archive>
    void serialize(Archive& ar){
        ar.value("x", x);
        ar.value("y", y);
    }

};

struct Player{
    int health = 100;
    std::string name = "Sam";
    _Transform transform;

    std::vector<_Transform> trans = { {}, {} };

    template<typename Archive>
    void serialize(Archive& ar){
        ar.value("health", health);
        ar.value("name", name);
        ar.object("transform", transform);
        ar.container("trans", trans);
    }
};

struct MyRecord{
    int x, y;
    float z;

    template <class Archive>
    void serialize(Archive & ar){
        //ar( x, y, z );
        ar(cereal::make_nvp("x", x));
        ar(cereal::make_nvp("y", y));
        ar(cereal::make_nvp("z", z));
    }
};

void SerializerSample::OnInit(){
    Player p;
    p.name = "lolo";
    p.trans[0].x = 20;

    {
    std::ofstream os("Sandbox/test.json");
    cereal::JSONOutputArchive archive(os);
    std::string a = "lolo";
    float b = 50;
    MyRecord c = {};
    c.x = 200;
    archive(cereal::make_nvp("a", a));
    archive(cereal::make_nvp("b", b));
    archive(cereal::make_nvp("c", c));
    }

    {
    std::ofstream os("Sandbox/test.bin");
    cereal::BinaryOutputArchive archive(os);
    std::string a = "lolo";
    float b = 50;
    MyRecord c = {};
    c.x = 200;
    archive(cereal::make_nvp("a", a));
    archive(cereal::make_nvp("b", b));
    archive(cereal::make_nvp("c", c));
    }

    {
    std::ofstream os("Sandbox/test.binport");
    cereal::PortableBinaryOutputArchive archive(os);
    std::string a = "lolo";
    float b = 50;
    MyRecord c = {};
    c.x = 200;
    archive(cereal::make_nvp("a", a));
    archive(cereal::make_nvp("b", b));
    archive(cereal::make_nvp("c", c));
    }

    {
    std::ifstream os("Sandbox/test.binport");
    cereal::PortableBinaryInputArchive archive(os);
    std::string a = "";
    float b = 0;
    MyRecord c = {};
    archive(cereal::make_nvp("a", a));
    archive(cereal::make_nvp("b", b));
    archive(cereal::make_nvp("c", c));
    Assert(b == 50);
    }

    {
    std::ifstream os("Sandbox/test.json");
    cereal::JSONInputArchive archive(os);
    MyRecord c = {};
    archive(cereal::make_nvp("c", c));
    Assert(c.x == 200);
    }

    {
        std::string a = "lolo";
        float b = 50;
        MyRecord c = {};
        c.x = 200;

        toml::table root;
        cereal::TomlOutputArchive2 ar(root);

        ar(cereal::make_nvp("a", a));
        ar(cereal::make_nvp("b", b));
        ar(cereal::make_nvp("c", c));

        std::cout << root << std::endl;
    }

    {
    std::ofstream os("Sandbox/player.toml");
    toml::table root;
    Static::TomlOutputArchive ar(root);
    ar.object("player", p);
    os << root;
    }

    {
    std::ofstream os("Sandbox/player.json");
    Static::CerealOutputArchive ar(os);
    ar.object("player", p);
    }

    {
    std::ofstream os("Sandbox/player.bin", std::ios::binary);
    Static::BitseryOutputArchive ar(os);
    ar.object("player", p);
    ar.flush();
    }

    {
        toml::table t = toml::table{{"test", 250}}; 
    }

    /*{
    std::ifstream os("Sandbox/player.bin", std::ios::binary);
    Static::BitseryInputArchive ar(os);
    ar.object("player", p);
    Assert(p.name == "lolo");
    Assert(p.trans[0].x == 20);
    }*/

    LogInfo("Game Init");
    Application::Vsync(false);

    auto& SceneManager = SceneManager::Get();
    SceneManager.RegisterScript<CameraMovementScript>("CameraMovementScript");
    OD::Scene* scene = SceneManager.NewScene();

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
    cam.renderingPath = CameraComponent::RenderingPath::Deferred;
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(7, 2.5, 0));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-8, 90, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 10;
    //camMove.transform = &camera->GetComponent<TransformComponent>()();
    //camMove.moveSpeed = 60;
    cam.farClipPlane = 1000;

    Entity light = scene->AddEntity("Directional Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    lightComponent.intensity = 1.5f;
    lightComponent.renderShadow = true;
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(95, 95, -30));

    Application::AddModule<Editor>();
    //scene->Start();
}

void SerializerSample::OnUpdate(float deltaTime){

}

void SerializerSample::OnRender(float deltaTime){

}

void SerializerSample::OnGUI(){

}

void SerializerSample::OnResize(int width, int height){

}

void SerializerSample::OnExit(){

}