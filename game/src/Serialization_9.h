#pragma once

#include <OD/OD.h>

using namespace OD;

struct ObjectTest{
    std::string a;
    int b;

    void Serialize(ArchiveNode& s){
        s.name = "ObjectTest";
        s.Add(&a, "a");
        s.Add(&b, "b");
    }
};

struct ComponentTest_01{
    float speed;
    ObjectTest test;
    std::vector<ObjectTest> tests = {ObjectTest(), ObjectTest()};

    void Serialize(ArchiveNode& s){
        s.name = "ComponentTest_01";
        s.Add(&speed, "speed");
        s.Add(test, "test");
        s.Add(tests, "tests");
    }
};

struct Serialization_9: public OD::Module{
    OD::Scene* scene;

    void OnInit() override {
        SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
        SceneManager::Get().RegisterSystem<StandRendererSystem>("StandRendererSystem");
        SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");
        SceneManager::Get().RegisterSystem<AnimatorSystem>("AnimatorSystem");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<RotateScript>("RotateScript");
        SceneManager::Get().RegisterComponent<ComponentTest_01>("ComponentTest_01");

        scene = SceneManager::Get().NewScene();

        Entity e = scene->AddEntity("Test");
        e.AddComponent<ComponentTest_01>();

        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override{

    }

    void OnRender(float deltaTime) override {

    }

    void OnGUI() override {}
    void OnResize(int width, int height) override{}

};