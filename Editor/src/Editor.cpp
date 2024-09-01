#include <OD/Entry.h>
#include <OD/Editor/Editor.h>
#include <OD/Scene/SceneManager.h>
#include <OD/Platform/Platform.h>
#include <fstream>
#include <stdio.h>

inline bool FileExists(const std::string& name){
    std::ifstream f(name);
    return f.good();
}

class Editor: public OD::Module{
    void OnInit() override {
        using namespace OD;

        LogInfo("Editor Init %s", "\033[0;32m", "\033[0m");
        std::string modulePath = Application::GetProjectSettings().scriptModulePath;
        Module* currentModule = nullptr;
        void* currentDll = nullptr;

        if(FileExists(modulePath)){
            typedef Module* (*CreateInstanceFunc)();
            currentDll = OD::Platform::LoadDynamicLibrary(modulePath.c_str());
            CreateInstanceFunc func = (CreateInstanceFunc)OD::Platform::LoadDynamicFunction(currentDll, "CreateInstance");
            currentModule = func();
            OD::Application::AddModule(currentModule);
        } else {
            LogError("Load Dynamic Module");
        }

        if(OD::SceneManager::Get().GetActiveScene() == nullptr){
            Scene* scene = OD::SceneManager::Get().NewScene();

            Entity env = scene->AddEntity("Env");
            env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

            Entity light = scene->AddEntity("Light");
            LightComponent& lightComponent = light.AddComponent<LightComponent>();
            lightComponent.color = {1,1,1};
            light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
            light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
            lightComponent.renderShadow = false;

            Entity camera = scene->AddEntity("Camera");
            CameraComponent& cam = camera.AddComponent<CameraComponent>();
            camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 15, 15));
            camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
            cam.farClipPlane = 1000;
        }

        if(OD::Application::GetModuleByType<OD::Editor>() == nullptr){
            OD::Application::AddModule<OD::Editor>();
        }
    }

    void OnExit() override {}
    void OnUpdate(float deltaTime) override {}
    void OnRender(float deltaTime) override {}
    void OnGUI() override {}
    void OnResize(int width, int height) override {}
};

OD::ApplicationConfig GetStartAppConfig(){
    return OD::ApplicationConfig{
        0, 0,
        800, 600,
        "Editor"
    };
}

OD::Module* CreateMainModule(){
    return new Editor();
}