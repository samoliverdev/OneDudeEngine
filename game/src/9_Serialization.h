#pragma once

#include <OD/OD.h>

using namespace OD;

struct ObjectTest{
    std::string a;
    int b;

    void Serialize(ArchiveNode& s){
        s.typeName("ObjectTest");

        s.Add(&a, "a");
        s.Add(&b, "b");
    }
};

struct ComponentTest_01{
    float speed;
    ObjectTest test;
    std::vector<ObjectTest> tests = {ObjectTest(), ObjectTest()};

    void Serialize(ArchiveNode& s){
        s.typeName("ComponentTest_01");

        s.Add(&speed, "speed");
        s.Add(test, "test");
        s.Add(&tests, "tests");
    }
};

struct SphereShape{
    Vector3 center;
    float radius;

    void Serialize(ArchiveNode& s){
        s.typeName("SphereShape");

        s.Add(&center, "center");
        s.Add(&radius, "radius");
    }
};

struct ComponentTest_02{
    float _float;
    int _int;
    std::string _string;
    Vector3 _vector3;
    Vector4 _Vector4;
    Quaternion _quaternion;

    Ref<Material> _material;

    SphereShape _object;
    std::vector<SphereShape> _list = {SphereShape(), SphereShape()};

    void Serialize(ArchiveNode& s){
        s.typeName("ComponentTest_02");

        s.Add(&_float, "_float");
        s.Add(&_int, "_int");
        s.Add(&_string, "_string");
        s.Add(&_vector3, "_vector3");
        s.Add(&_Vector4, "_Vector4");
        s.Add(&_quaternion, "_quaternion");

        s.Add(&_material, "_material");

        s.Add(_object, "_object");
        s.Add(&_list, "_list");
    }
};

struct Serialization_9: public OD::Module{
    OD::Scene* scene;

    void OnInit() override {
        //SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
        //SceneManager::Get().RegisterSystem<StandRendererSystem>("StandRendererSystem");
        //SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<RotateScript>("RotateScript");

        SceneManager::Get().RegisterComponent<ComponentTest_01>("ComponentTest_01");
        SceneManager::Get().RegisterCoreComponentSimple<ComponentTest_02>("ComponentTest_02");

        scene = SceneManager::Get().NewScene();

        Entity e = scene->AddEntity("Test");
        e.AddComponent<ComponentTest_01>();

        Entity e2 = scene->AddEntity("Test2");
        e2.AddComponent<ComponentTest_02>();

        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override{

    }

    void OnRender(float deltaTime) override {

    }

    void OnGUI() override {}
    void OnResize(int width, int height) override{}

};