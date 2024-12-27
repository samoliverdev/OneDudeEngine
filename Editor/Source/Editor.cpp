#include "OD/CoreModulesStartup.h"
#include "OD/Core/Module.h"
#include "OD/Core/Application.h"
#include "OD/Core/Project.h"
#include <OD/Serialization/Serialization.h>
#include <OD/Serialization/SerializationFull.h>
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

struct LauncherSettings{
    char defaultProjectPath[128] = "";
    std::vector<std::string> projectsPath; 
    
    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, defaultProjectPath);
        ArchiveDumpNVP(ar, projectsPath);
    }
};

OD::ApplicationConfig GetLauncherConfig(){
    return OD::ApplicationConfig{
        0, 0,
        600, 400,
        "Launcher"
    };
}

class Launcher: public OD::Module{
    LauncherSettings launcherSettings;
    bool inNewProjectTab = false;
    char projectName[128] = "Project";

    void LoadSettings(){
        OD::LoadOrCreateArchive("LauncherSettings.json", launcherSettings);
    }

    void SaveSettings(){
        OD::SaveArchive("LauncherSettings.json", launcherSettings);
    }

    void OpenProject(){
        using namespace OD;
        std::string p = Platform::OpenFolder();// Platform::OpenFile("*.proj");
        if(p.empty() == false){
            projectPath = p+"/";
            Application::Quit();
        }
    }

    void OpenProject(std::string& path){
        if(std::find(launcherSettings.projectsPath.begin(), launcherSettings.projectsPath.end(), path) == launcherSettings.projectsPath.end()){
            launcherSettings.projectsPath.push_back(path);
        }

        projectPath = path + "/";
        OD::Application::Quit();
    }

    void NewProject(){
        using namespace OD;
        std::string p = Platform::OpenFolder();// Platform::OpenFile("*.proj");
        if(p.empty() == false){
            std::replace(p.begin(), p.end(), '\\', '/');

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

            char str[512];
            sprintf(
                str, 
                "cmake -S %s -B %s/build -DENGINE_PATH:STRING=%s",
                _projectPath.c_str(),
                _projectPath.c_str(),
                defaultProjectPath.c_str()
            );

            system(str);
        }
    }

    void NewProject(const char* projectName, const char* path){
        std::string templat = defaultProjectPath + "Content/ProjectTemplate";
        std::string _projectPath = std::string(path) + "/" + projectName;
        std::filesystem::copy(
            templat, 
            _projectPath, 
            std::filesystem::copy_options::update_existing 
            | std::filesystem::copy_options::recursive
            //| std::filesystem::copy_options::directories_only
        );

        char str[512];
        sprintf(
            str, 
            "cmake -S %s -B %s/build -DENGINE_PATH:STRING=%s",
            _projectPath.c_str(),
            _projectPath.c_str(),
            "C:/Users/sam/Desktop/cpp/OneDudeEngine/" //defaultProjectPath.c_str()
        );
        //cmake -S . -B build -DENGINE_PATH:STRING="C:/Users/sam/Desktop/cpp/OneDudeEngine/"

        if(std::find(launcherSettings.projectsPath.begin(), launcherSettings.projectsPath.end(), _projectPath) == launcherSettings.projectsPath.end()){
            launcherSettings.projectsPath.push_back(_projectPath);
        }

        system(str);
    }

    void OnInit() override {
        LoadSettings();

        using namespace OD;

        //Platform::SetWindowSize(600, 400);

        Scene* scene = OD::SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
        lightComponent.color = {1,1,1};
        scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
        scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = false;

        Entity camera = scene->AddEntity("Camera");
        CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
        scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 15, 15));
        scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
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

    void OnExit() override {
        SaveSettings();
    }

    void OnUpdate(float deltaTime) override {}
    void OnRender(float deltaTime) override {}

    void DrawNewProjectTab(){
        ImGui::InputText("Project Name", projectName, 128);

        ImGui::InputText("Location", launcherSettings.defaultProjectPath, 128);
        ImGui::SameLine();
        //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - widthNeeded);
        if(ImGui::SmallButton("...")){
            std::string p = OD::Platform::OpenFolder();// Platform::OpenFile("*.proj");
            if(p.empty() == false){
                std::replace(p.begin(), p.end(), '\\', '/');
                strcpy_s(launcherSettings.defaultProjectPath, p.c_str());
            }
        }

        if(ImGui::Button("Create Project")){
            NewProject(projectName, launcherSettings.defaultProjectPath);
            inNewProjectTab = false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel")) inNewProjectTab = false;
    }

    void DrawProjectSelectionTab(){
        int i = 0;
        static int m_selectedItem = -1;
        for(auto& it: launcherSettings.projectsPath){
            std::string itemid = "##" + std::to_string(i);
            if(ImGui::Selectable(itemid.c_str(), i == m_selectedItem)){
                //m_selectedItem = i;
                OpenProject(it);
            }
            ImGui::SameLine();
            ImGui::Text("Item: ");
            ImGui::SameLine();
            ImGui::Text(it.c_str());
            i++;
        }
    }

    void OnGUI() override {
        using namespace OD;

        const char* openProjectLabel = "Open Project";
        const char* newProjectLabel = "New Project";

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({(float)Application::ScreenWidth(), (float)Application::ScreenHeight()});
        ImGui::Begin("Launcher", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        //if(ImGui::Button("Open Project")) OpenProject();
        //if(ImGui::Button("New Project")) NewProject();

        auto& style = ImGui::GetStyle();
        float buttonWidth1 = ImGui::CalcTextSize(openProjectLabel).x + style.FramePadding.x * 2.f;
        float buttonWidth2 = ImGui::CalcTextSize(newProjectLabel).x + style.FramePadding.x * 2.f;
        float widthNeeded = buttonWidth1 + style.ItemSpacing.x + buttonWidth2;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - widthNeeded);
        if(ImGui::Button(openProjectLabel)) OpenProject();
        ImGui::SameLine();
        if(ImGui::Button(newProjectLabel)) inNewProjectTab = !inNewProjectTab;

        //ImGui::PushStyleColor(ImGuiCol_Separator, ImColor(0, 189, 0).Value);
        ImGui::Separator();
        //ImGui::PopStyleColor();

        if(inNewProjectTab){
            DrawNewProjectTab();
        } else {
            DrawProjectSelectionTab();
        }


        if(ImGui::Button("Delete..")) ImGui::OpenPopup("Delete?");
        if(ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize)){
            ImGui::Text("All those beautiful files will be deleted.\nThis operation cannot be undone!");
            ImGui::Separator();

            //static int unused_i = 0;
            //ImGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");

            static bool dont_ask_me_next_time = false;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
            ImGui::PopStyleVar();

            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        ImGui::End();

        //bool t = true;
        //ImGui::ShowDemoWindow(&t);
    }
    
    void OnResize(int width, int height) override {}
};

OD::ApplicationConfig GetEditorConfig(){
    return OD::ApplicationConfig{
        0, 0,
        800, 600,
        "Editor"
    };
}

class Editor: public OD::Module{
    OD::FuncModule dllModule;
    bool toReload = false;
    FILE* pipe = nullptr;
    std::vector<std::string> console;
    char con[1024*2];

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

        std::string modulePathCopy = modulePath + "_Copy";
        std::filesystem::copy_file(modulePath, modulePathCopy, std::filesystem::copy_options::update_existing);

        typedef Module* (*CreateInstanceFunc)();
        currentDll = OD::Platform::LoadDynamicLibrary(modulePathCopy.c_str());
        auto dllModule = new FuncModule();
        dllModule->onInit = (OD::_OnInit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnInit");
        dllModule->onExit = (OD::_OnExit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnExit");
        dllModule->onUpdate = (OD::_OnUpdate)OD::Platform::LoadDynamicFunction(currentDll, "GameOnUpdate");
        currentModule = dllModule;
        OD::Application::AddModule(currentModule);
    }

    void ReloadProjectDLL(){
        strcpy_s(con, "");
        console.clear();
        ImGui::OpenPopup("HotReload?");
        pipe = _popen("cmake --build ../build --config Release", "r");
        //pipe = _popen("cmake --build ../build --config RelWithDebInfo", "r");
        return;

        using namespace OD;

        SceneManager::Get().GetActiveScene()->Save("tempHotReload.scene");
        //SceneManager::Get().DestroyActiveScene();
        SceneManager::Get().NewScene();

        if(currentModule != nullptr){
            ((OD::FuncModule*)currentModule)->onInit = nullptr;
            ((OD::FuncModule*)currentModule)->onExit = nullptr;
            ((OD::FuncModule*)currentModule)->onUpdate = nullptr;
            Application::RemoveModule(currentModule);
            Platform::FreeDynimicLibrary(currentDll);
        }

        //delete currentModule; //Fixme memory leak
        currentModule = nullptr;
        currentDll = nullptr;

        system("cmake --build ../build --config Release");

        //pipe = _popen("cmake --build ../build --config Release", "r");

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

    void _ReloadProjectDLL(){
        using namespace OD;

        SceneManager::Get().GetActiveScene()->Save("tempHotReload.scene");
        //SceneManager::Get().DestroyActiveScene();
        SceneManager::Get().NewScene();

        if(currentModule != nullptr){
            //((OD::FuncModule*)currentModule)->onInit = nullptr;
            //((OD::FuncModule*)currentModule)->onExit = nullptr;
            //((OD::FuncModule*)currentModule)->onUpdate = nullptr;
            Application::RemoveModule(currentModule);
            Platform::FreeDynimicLibrary(currentDll);
        }

        //delete currentModule; //Fixme memory leak
        currentModule = nullptr;
        currentDll = nullptr;

        std::string modulePathCopy = ProjectManager::GetActiveProject()->scriptModulePath + "_Copy";
        std::filesystem::copy_file(ProjectManager::GetActiveProject()->scriptModulePath, modulePathCopy, std::filesystem::copy_options::update_existing);

        typedef Module* (*CreateInstanceFunc)();
        currentDll = Platform::LoadDynamicLibrary(modulePathCopy.c_str());

        auto c = new OD::FuncModule();
        c->onInit = (OD::_OnInit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnInit");
        c->onExit = (OD::_OnExit)OD::Platform::LoadDynamicFunction(currentDll, "GameOnExit");
        c->onUpdate = (OD::_OnUpdate)OD::Platform::LoadDynamicFunction(currentDll, "GameOnUpdate");
        currentModule = c;

        OD::Editor::Get()->UnselectAll();

        //toReload = true;

        OD::Application::AddModule(currentModule);
        SceneManager::Get().NewScene()->Load("tempHotReload.scene");
    }

    void ReloadModuleScene(){
        if(toReload == false) return;
        toReload = false;

        _ReloadProjectDLL();
        return;

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
        scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
        lightComponent.color = {1,1,1};
        scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
        scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = false;

        Entity camera = scene->AddEntity("Camera");
        CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
        scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 15, 15));
        scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
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
    void OnGUI() override {
        if(pipe == nullptr) return;

        //if(ImGui::Button("Delete..")) ImGui::OpenPopup("HotReload?");

        ImGui::OpenPopup("HotReload?");

        auto& io = ImGui::GetIO();
        //ImGui::SetNextWindowPos({io.DisplaySize.x/2-150, io.DisplaySize.y/2-100});
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
        //ImGui::SetNextWindowSize({250, 250});
        if(ImGui::BeginPopupModal("HotReload?", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)){
            ImGui::Text("All those beautiful files will be deleted.\nThis operation cannot be undone!");
            ImGui::Separator();

            ImGui::Text("Compilling");

            char buffer[512];
            if(fgets(buffer, sizeof(buffer), pipe) != NULL){
                strcat_s(con, buffer);
                LogWarning("%s", buffer);
            }
            ImGui::InputTextMultiline("", con, sizeof(con), {600, 250});

            //static int unused_i = 0;
            //ImGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");
            if(feof(pipe)){
                int result = _pclose(pipe);
                if(result == 0){
                    /*if (project.state == Project::GENERATING)
                        project.state = Project::GENERATED;
                    else if (project.state == Project::BUILDING)
                        project.state = Project::READY;*/
                    
                    LogWarning("HotReload Success");
                    toReload = true;
                    //_ReloadProjectDLL();
                } else {
                    /*if (project.state == Project::GENERATING)
                        project.state = Project::FAIL_GENERATE;
                    else if (project.state == Project::BUILDING)
                        project.state = Project::FAIL_BUILD;*/

                    LogWarning("HotReload Fails");
                }

                /*exitCode = result;
                project.externalCommandPipe = nullptr;
                return true;*/

                ImGui::CloseCurrentPopup();
                pipe = nullptr;
            }
            //return false;

            /*static bool dont_ask_me_next_time = false;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
            ImGui::PopStyleVar();

            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();*/
        }
    }
    void OnResize(int width, int height) override {}
};

int main(int argc, char *argv[]){
    for(int i = 0; i < argc; i++) OD::Application::GetArgs().push_back(std::string(argv[i]));

    OD::Action<void()> action;
    action.Add([](){ LogInfo("tesds"); });
    action.Invoke();

    Launcher* launcer = new Launcher();
    Editor* editor = new Editor();

    defaultProjectPath = RESOURCES_PATH "";
    std::string targetProjectPath = argc > 1 ? argv[1] : "";
    
    if(useLauncher){
        while(openLauncher == true){
            if(openLauncher == false) break;

            if(targetProjectPath.empty() == false){
                OD::CoreModulesStartup();
                OD::Application::Create(editor, GetEditorConfig(), targetProjectPath.c_str());   
                OD::Application::Run();
                EditorOnExit();
                continue;
            }

            OD::CoreModulesStartup();
            OD::Application::Create(launcer, GetLauncherConfig(), defaultProjectPath.c_str());
            OD::Application::Run();
            
            //OD::Application::RemoveModule(launcer);
            openLauncher = false;
            if(projectPath.empty() == true) break;

            OD::CoreModulesStartup();
            OD::Application::Create(editor, GetEditorConfig(), projectPath.c_str());   
            OD::Application::Run();
            EditorOnExit();
            projectPath = "";
        }
    } else {
        OD::CoreModulesStartup();
        OD::Application::Create(
            editor, 
            GetEditorConfig(), 
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