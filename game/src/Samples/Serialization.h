#pragma once

#include <OD/OD.h>

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <fstream>
using namespace OD;

struct ObjectTest{
    std::string a;
    int b;

    template<class Archive>
    void serialize(Archive& ar){
        ar(
            CEREAL_NVP(a), 
            CEREAL_NVP(b)
        );
    }
};

template<typename T>
struct TestRegisterComponent{
    TestRegisterComponent(const char* name){
        //SceneManager::Get().RegisterComponent<T>(name);
        SceneManager::Get().RegisterCoreComponentSimple<T>(name);
    }
};

#define REGISTER_COMPONENT(component_name) inline static TestRegisterComponent<component_name> component_name_Reg = TestRegisterComponent<component_name>(#component_name)

struct ComponentTest_01{
    float speed;
    ObjectTest test;
    std::vector<ObjectTest> tests = {ObjectTest(), ObjectTest()};

    //inline static TestRegisterComponent<ComponentTest_01> registerTest = TestRegisterComponent<ComponentTest_01>("ComponentTest_01");
    REGISTER_COMPONENT(ComponentTest_01);

    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(speed));
        ArchiveDump(ar, CEREAL_NVP(test));
        ArchiveDump(ar, CEREAL_NVP(tests));
    }
};

struct SphereShape{
    Vector3 center;
    float radius;

    template<class Archive>
    void serialize(Archive& ar){
        ar(
            CEREAL_NVP(center), 
            CEREAL_NVP(radius)
        );
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

    void OnCreate(Entity& e){
        LogWarning("ComponentTest_02 OnCreate %s", e.GetComponent<InfoComponent>().name.c_str());
    }
    
    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(_float));
        ArchiveDump(ar, CEREAL_NVP(_int));
        ArchiveDump(ar, CEREAL_NVP(_string));
        ArchiveDump(ar, CEREAL_NVP(_vector3));
        ArchiveDump(ar, CEREAL_NVP(_Vector4));
        ArchiveDump(ar, CEREAL_NVP(_quaternion));
        ArchiveDump(ar, CEREAL_NVP(_object));
        ArchiveDump(ar, CEREAL_NVP(_list));
    }
};

struct MyClass{
    enum class Type{A, B};

    int x, y, z;
    Type type;
    OD::Vector3 vec;

    template <class Archive>
    void serialize(Archive& ar){
        ar(x, y, z, type, vec);
    }
};
    

struct SerializationSample: public OD::Module{
    OD::Scene* scene;

    void OnInit() override {
        {
            std::ofstream os("out.cereal");
            cereal::JSONOutputArchive archive(os);

            MyClass myData = {0, 5, 10, MyClass::Type::B};
            int someInt = 996;
            double d = 3.13546546;
            archive(myData, someInt, d);
        }

        {
            std::ifstream is("out.cereal");
            cereal::JSONInputArchive archive(is);
            
            MyClass m1;
            int someInt;
            double d;

            archive(m1); // NVPs not strictly necessary when loading

            Assert(m1.type == MyClass::Type::B);
        }

        //SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
        //SceneManager::Get().RegisterSystem<StandRendererSystem>("StandRendererSystem");
        //SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<RotateScript>("RotateScript");

        //SceneManager::Get().RegisterComponent<ComponentTest_01>("ComponentTest_01");
        SceneManager::Get().RegisterCoreComponentSimple<ComponentTest_02>("ComponentTest_02");

        scene = SceneManager::Get().NewScene();

        Entity e = scene->AddEntity("Test");
        e.AddComponent<ComponentTest_01>();

        Entity e2 = scene->AddEntity("Test2");
        e2.AddComponent<ComponentTest_02>();
        e2.AddComponent<ScriptComponent>().AddScript<RotateScript>();

        Application::AddModule<Editor>();

        /*typedef Module* (*CreateInstanceFunc)();
        void* module = Platform::LoadDynamicLibrary("build/lib/Release/dynamic_module.dll");
        CreateInstanceFunc func = (CreateInstanceFunc)Platform::LoadDynamicFunction(module, "CreateInstance");
        Application::AddModule(func());*/
    }

    void OnUpdate(float deltaTime) override{

    }

    void OnRender(float deltaTime) override {

    }

    void OnGUI() override {}
    void OnResize(int width, int height) override{}
    void OnExit() override {}

};