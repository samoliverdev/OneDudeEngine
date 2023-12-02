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


/*template <class Archive,
        cereal::traits::EnableIf<cereal::traits::is_text_archive<Archive>::value>
        = cereal::traits::sfinae, class T>
std::enable_if_t<std::is_enum_v<T>, std::string> save_minimal(Archive &, const T& h){
    return std::string(magic_enum::enum_name(h));
}

template <class Archive, cereal::traits::EnableIf<cereal::traits::is_text_archive<Archive>::value>
        = cereal::traits::sfinae, class T> std::enable_if_t<std::is_enum_v<T>, void> load_minimal( Archive const &, T& enumType, std::string const& str){
    enumType = magic_enum::enum_cast<T>(str).value();
}
*/

using namespace OD;

struct ObjectTest{
    std::string a;
    int b;

    void Serialize(ArchiveNode& s){
        s.typeName("ObjectTest");

        s.Add(&a, "a");
        s.Add(&b, "b");
    }

    template <class Archive>
    void serialize(Archive & ar){
        ar(
            CEREAL_NVP(a), 
            CEREAL_NVP(b)
        );
    }
};

template<typename T>
struct TestRegisterComponent{
    TestRegisterComponent(const char* name){
        SceneManager::Get().RegisterComponent<T>(name);
    }
};

struct ComponentTest_01{
    float speed;
    ObjectTest test;
    std::vector<ObjectTest> tests = {ObjectTest(), ObjectTest()};

    inline static TestRegisterComponent<ComponentTest_01> registerTest = TestRegisterComponent<ComponentTest_01>("ComponentTest_01");

    void Serialize(ArchiveNode& s){
        s.typeName("ComponentTest_01");

        s.Add(&speed, "speed");
        s.Add(test, "test");
        s.Add(&tests, "tests");
    }

    template <class Archive>
    void serialize(Archive & ar){
        ar(
            CEREAL_NVP(speed), 
            CEREAL_NVP(test),
            CEREAL_NVP(tests)
        );
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

    template <class Archive>
    void serialize(Archive & ar){
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

    template <class Archive>
    void serialize(Archive & ar){
        ar(
            CEREAL_NVP(_float), 
            CEREAL_NVP(_int),
            CEREAL_NVP(_string),
            CEREAL_NVP(_vector3),
            CEREAL_NVP(_Vector4),
            CEREAL_NVP(_quaternion),
            CEREAL_NVP(_object),
            CEREAL_NVP(_list)
        );
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
    

struct Serialization_9: public OD::Module{
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

        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override{

    }

    void OnRender(float deltaTime) override {

    }

    void OnGUI() override {}
    void OnResize(int width, int height) override{}

};