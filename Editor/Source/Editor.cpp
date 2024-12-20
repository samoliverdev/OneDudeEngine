#include "OD/CoreModulesStartup.h"
#include "OD/Core/Module.h"
#include "OD/Core/Application.h"
#include "OD/Core/Project.h"
#include <OD/Editor/Editor.h>
#include <OD/Scene/SceneManager.h>
#include <OD/Platform/Platform.h>
#include <OD/Core/Input.h>
#include <OD/Core/Action.h>
#include <fstream>
#include <stdio.h>
#include <cstdio>
#include <stdlib.h>

bool useLauncher = true;
bool openLauncher = true;
std::string defaultProjectPath;
std::string projectPath = "";
OD::Editor* editor = nullptr;
void* currentDll = nullptr;
OD::Module* currentModule = nullptr;

class Test{
    typedef void(*Func)(void* data);
    static inline void func(void* data){}

    Func funcPtr = &func; 
};

auto GoBackToLauncher = [&](){
    OD::Application::Quit();
    openLauncher = true;
};

auto LoadProject = [&](){
    std::string p = OD::Platform::OpenFolder();// Platform::OpenFile("*.proj");
    if(p.empty() == false){
        OD::ProjectManager::LoadProject((p+"/").c_str());
    }
};

void EditorOnExit(){
    if(currentDll != nullptr) OD::Platform::FreeDynimicLibrary(currentDll);
    if(currentModule != nullptr) delete currentModule;
    delete editor;
}

inline bool FileExists(const std::string& name){
    std::ifstream f(name);
    return f.good();
}

class CoutRedirect{
public:
    CoutRedirect(){
        old = std::cout.rdbuf(buffer.rdbuf()); // redirect cout to buffer stream
    }

    std::string getString(){
        return buffer.str(); // get string
    }

    ~CoutRedirect( ){
        std::cout.rdbuf(old); // reverse redirect
    }

private:
    std::stringstream buffer;
    std::streambuf* old;
};

class Launcher: public OD::Module{
    void OpenProject(){
        using namespace OD;
        std::string p = Platform::OpenFolder();// Platform::OpenFile("*.proj");
        if(p.empty() == false){
            projectPath = p+"/";
            Application::Quit();
        }
    }

    void NewProject(){
        using namespace OD;
        std::string p = Platform::OpenFolder();// Platform::OpenFile("*.proj");
        if(p.empty() == false){
            std::string projectName = "NewProject";

            std::string templat = defaultProjectPath+"Content/ProjectTemplate";
            std::string _projectPath = p + "/" + projectName;
            std::filesystem::copy(
                templat, 
                _projectPath, 
                std::filesystem::copy_options::update_existing 
                | std::filesystem::copy_options::recursive
                //| std::filesystem::copy_options::directories_only
            );

            //system("cmake -S ProjectTemplate -B ProjectTemplate/build -DENGINE_PATH:STRING=\"C:/Users/sam/Desktop/cpp/OneDudeEngine\"");
        }
    }

    void OnInit() override {
        using namespace OD;
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

        //OD::Application::AddModule<OD::Editor>();

        /*auto editor = new OD::Editor();
        editor->AddMenuCommand("File2/OpenProject", [](){ LogWarning("Ai meu cu!!!1"); });
        editor->AddMenuCommand("File2/New", [](){ LogWarning("Ai meu cu!!!1"); });
        editor->AddMenuCommand("File2/Save As", [](){ LogWarning("Ai meu cu!!!1"); });
        editor->AddMenuCommand("File2/Exit", [](){ LogWarning("Ai meu cu!!!1"); });
        OD::Application::AddModule(editor);*/

        //system("C:/Users/sam/Desktop/cpp/OneDudeEngine/Bin/Sandbox.exe");
        //_popen("C:/Users/sam/Desktop/cpp/OneDudeEngine/Bin/Sandbox.exe", "rt");
    }

    void OnExit() override {}

    void OnUpdate(float deltaTime) override {}
    void OnRender(float deltaTime) override {}
    
    void OnGUI() override {
        using namespace OD;

        ImGui::Begin("Launcher");
        if(ImGui::Button("Open Project")) OpenProject();
        if(ImGui::Button("New Project")) NewProject();
        ImGui::End();

        /*ImGui::Begin("Splitter test");

        static float w = 200.0f;
        static float h = 300.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
        ImGui::BeginChild("child1", ImVec2(w, h), true);
        ImGui::EndChild();
        
        ImGui::SameLine();
        ImGui::InvisibleButton("vsplitter", ImVec2(8.0f,h)); 
        if(ImGui::IsItemActive()) w += ImGui::GetIO().MouseDelta.x;
        ImGui::SameLine();
        
        ImGui::BeginChild("child2", ImVec2(0, h), true);
        ImGui::EndChild();
        
        ImGui::InvisibleButton("hsplitter", ImVec2(-1,8.0f));
        if(ImGui::IsItemActive()) h += ImGui::GetIO().MouseDelta.y;
        
        ImGui::BeginChild("child3", ImVec2(0,0), true);
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::End();*/

        bool t = true;
        ImGui::ShowDemoWindow(&t);
    }
    
    void OnResize(int width, int height) override {}
};

class Editor: public OD::Module{
    OD::FuncModule dllModule;
    bool toReload = false;

    void LoadProjectDLL(){
        using namespace OD;

        std::string modulePath = ProjectManager::GetActiveProject()->scriptModulePath; //Application::GetProjectSettings().scriptModulePath;
        currentModule = nullptr;
        currentDll = nullptr;
        LogInfo("DLL Path: %s", modulePath.c_str());

        if(FileExists(modulePath) == false){
            LogError("Load Dynamic Module");
            return;
        }

        typedef Module* (*CreateInstanceFunc)();
        currentDll = OD::Platform::LoadDynamicLibrary(modulePath.c_str());
        
        auto dllModule = new FuncModule();
        dllModule->onInit = (OD::_OnInit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnInit");
        dllModule->onExit = (OD::_OnExit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnExit");
        dllModule->onUpdate = (OD::_OnUpdate)OD::Platform::LoadDynamicFunction(currentDll, "GameOnUpdate");
        currentModule = dllModule;

        OD::Application::AddModule(currentModule);
    }

    void ReloadProjectDLL(){
        using namespace OD;

        SceneManager::Get().GetActiveScene()->Save("tempHotReload.scene");
        //SceneManager::Get().DestroyActiveScene();
        SceneManager::Get().NewScene();

        ((OD::FuncModule*)currentModule)->onInit = nullptr;
        ((OD::FuncModule*)currentModule)->onExit = nullptr;
        ((OD::FuncModule*)currentModule)->onUpdate = nullptr;

        Application::RemoveModule(currentModule);
        Platform::FreeDynimicLibrary(currentDll);
        //delete currentModule; //Fixme memory leak
        currentModule = nullptr;
        currentDll = nullptr;

        system("cmake --build ../build --config Release");

        typedef Module* (*CreateInstanceFunc)();
        currentDll = Platform::LoadDynamicLibrary(ProjectManager::GetActiveProject()->scriptModulePath.c_str());

        auto c = new OD::FuncModule();
        c->onInit = (OD::_OnInit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnInit");
        c->onExit = (OD::_OnExit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnExit");
        c->onUpdate = (OD::_OnUpdate)OD::Platform::LoadDynamicFunction(currentDll, "GameOnUpdate");
        currentModule = c;

        toReload = true;
        //OD::Application::AddModule(currentModule);
        //SceneManager::Get().NewScene()->Load("tempHotReload.scene");
    }

    void ReloadModuleScene(){
        if(toReload == false) return;
        toReload = false;

        OD::Application::AddModule(currentModule);
        OD::SceneManager::Get().NewScene()->Load("tempHotReload.scene");
    }

    void AddODEditor(){
        if(OD::Application::GetModuleByType<OD::Editor>() != nullptr) return;
        
        //OD::Application::AddModule<OD::Editor>();
        editor = new OD::Editor();
        editor->AddMenuCommand("lolo/comands/AiMeuCu", [](){ LogWarning("Ai meu cu!!!1"); });
        editor->AddMenuCommand("CurrentProject/ReloadDllModule", [this](){ ReloadProjectDLL(); });

        if(useLauncher){
            editor->AddMenuCommand("File/GoBackToLauncher", GoBackToLauncher);
        } else {
            editor->AddMenuCommand("File/LoadProject", LoadProject);
        }

        OD::Application::AddModule(editor);
    }

    void AddDefaultScene(){
        using namespace OD;

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

    void OnInit() override {
        //LogWarning("Cur Path: %s", std::filesystem::current_path().string().c_str());
        //LogWarning("Cur Path2: %s", Application::GetArgs()[0].c_str());
        LogInfo("Editor Init %s", "\033[0;32m", "\033[0m");
        
        LoadProjectDLL();
        AddDefaultScene();
        AddODEditor();

        OD::Application::GetOnFrameEnd().Add([&](){ ReloadModuleScene(); });
    }

    void OnExit() override {}

    void OnUpdate(float deltaTime) override {
        return;
        using namespace OD;

        std::string modulePath = ProjectManager::GetActiveProject()->scriptModulePath;

        if(Input::IsKeyDown(KeyCode::R) && currentModule != nullptr){
            //SceneManager::Get().GetActiveScene()->Save("tempHotReload.scene");
            //Clean Old Refs
            Application::RemoveModule(currentModule);
            Platform::FreeDynimicLibrary(currentDll);
            delete currentModule;
            currentModule = nullptr;
            currentDll = nullptr;
            //CoFreeUnusedLibraries();

            //system("cmake -S . -B build");
            //#ifdef NDEBUG
            //system("cmake -S ProjectTemplate -B ProjectTemplate/build -DENGINE_PATH:STRING=\"C:/Users/sam/Desktop/cpp/OneDudeEngine\"");
            //std::remove("C:\\Users\\sam\\Desktop\\cpp\\OneDudeEngine\\ProjectTemplate\\Bin\\Game.dll");
            //system("del C:\\Users\\sam\\Desktop\\cpp\\OneDudeEngine\\ProjectTemplate\\Bin\\Game.dll");
            //system("echo olo");
            system("cmake --build ../build --config Release");
            //system("echo olo");
            //#else 
            //system("cmake --build build --config Debug");
            //#endif

            //modulePath = PingPongCopyFile(modulePath);

            typedef Module* (*CreateInstanceFunc)();
            currentDll = Platform::LoadDynamicLibrary(modulePath.c_str());
            //CreateInstanceFunc func = (CreateInstanceFunc)Platform::LoadDynamicFunction(currentDll, "CreateInstance");
            //currentModule = func();
            auto c = new OD::FuncModule();
            c->onInit = (OD::_OnInit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnInit");
            c->onExit = (OD::_OnExit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnExit");
            c->onUpdate = (OD::_OnUpdate)OD::Platform::LoadDynamicFunction(currentDll, "GameOnUpdate");
            currentModule = c;
            
            OD::Application::AddModule(currentModule);
            //SceneManager::Get().NewScene()->Load("tempHotReload.scene");
        }
    }

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

int main(int argc, char *argv[]){
    for(int i = 0; i < argc; i++) OD::Application::GetArgs().push_back(std::string(argv[i]));

    OD::Action<void()> action;
    action.Add([](){ LogInfo("tesds"); });
    action.Invoke();

    Launcher* launcer = new Launcher();
    Editor* editor = new Editor();
    //OD::Editor* odEditor = new OD::Editor();
    /*std::string*/ defaultProjectPath = RESOURCES_PATH "";
    std::string targetProjectPath = argc > 1 ? argv[1] : "";
    
    if(useLauncher){
        while(openLauncher == true){
            if(openLauncher == false) break;

            if(targetProjectPath.empty() == false){
                OD::CoreModulesStartup();
                OD::Application::Create(editor, GetStartAppConfig(), targetProjectPath.c_str());   
                OD::Application::Run();
                EditorOnExit();
                continue;
            }

            OD::CoreModulesStartup();
            OD::Application::Create(launcer, GetStartAppConfig(), defaultProjectPath.c_str());
            OD::Application::Run();
            
            //OD::Application::RemoveModule(launcer);
            openLauncher = false;
            if(projectPath.empty() == true) break;

            OD::CoreModulesStartup();
            OD::Application::Create(editor, GetStartAppConfig(), projectPath.c_str());   
            OD::Application::Run();
            EditorOnExit();
            projectPath = "";
        }
    } else {
        OD::CoreModulesStartup();
        OD::Application::Create(
            editor, 
            GetStartAppConfig(), 
            targetProjectPath.empty() ? defaultProjectPath.c_str() : targetProjectPath.c_str()
        );   
        OD::Application::Run();
        EditorOnExit();
    }

    delete launcer;
    delete editor;
    //delete odEditor;

    return 0;

    /*OD::CoreModulesStartup();

    Launcher* launcer = new Launcher();
    Editor* editor = new Editor();

    std::cout.sync_with_stdio(true);
    CoutRedirect coutRedirect;

    char buf[1024]; sprintf(buf, "%d score and %d years ago", 4, 7);
    std::cout << buf << std::endl;
    
    std::cout << "Blafdffffffffffffffff" << std::endl;

    for(int i = 0; i < argc; i++){
        OD::Application::GetArgs().push_back(std::string(argv[i]));
    }

    OD::Application::Create(launcer, GetStartAppConfig(), argc > 1 ? argv[1] : RESOURCES_PATH "");
    OD::Application::Run();

    if(projectPath.empty() == true) return 0;

    OD::Application::RemoveModule(launcer);
    OD::Application::Create(editor, GetStartAppConfig(), (projectPath + "/").c_str());
    OD::Application::Run();

    delete launcer;
    delete editor;

    return 0;*/
}