#include "OD/pch.h"
#include "Application.h"
#include "Module.h"
#include "Project.h"
#include "ImGui.h"
#include "Input.h"
#include "Time.h"
#include "Log.h"
#include "Instrumentor.h"
#include "JobSystem.h"
#include "Lua.h"
#include "PhysFSPackage.h"
#include "OD/Core/ResourceManager.h"
#include "OD/Core/GlobalSettings.h"
#include "OD/Platform/Platform.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/CoreModulesStartup.h"
#include "OD/Serialization/SerializationFull.h"

#include "OD/GPU/GPU.h"

#include "OD/Graphics/Texture.h"

namespace OD{

std::vector<std::string> args;
std::vector<Module*> modules;
std::vector<Module*> modulesToAdd;
std::vector<Module*> modulesToRemove;
bool appHasInited = false;
bool inUpdate = false;

Action<void()> onFrameEnd;

ApplicationCallbacks callbacks;

struct GPURendererContext{
    bool multithread = false;
    std::atomic<bool> running = false;

    GPURenderFrame frames[2];

    GPURenderFrame* simulationFrame = nullptr; // Owned by main thread.
    GPURenderFrame* renderFrame = nullptr; // Owned by render thread.

    std::thread renderThread;

    std::mutex mutex;
    std::condition_variable condition;

    bool renderRequested = false;
    bool renderFinished = false;
};

GPURendererContext gpuRendererContext;
GPUDevice* gpuDevice = nullptr;

void Application::RenderThreadLoop(){
    Platform::MakeMultiThreadContext();

    gpuDevice->Init();

    while(gpuRendererContext.running){
        
        // Wait for this frame's render work.
        {
            std::unique_lock<std::mutex> lock(gpuRendererContext.mutex);

            gpuRendererContext.condition.wait(
                lock,
                [&]{
                    return gpuRendererContext.renderRequested || !gpuRendererContext.running;
                }
            );

            if(!gpuRendererContext.running) return;

            gpuRendererContext.renderRequested = false;
        }

        // ------------------------------------------------
        // Execute render commands.
        //
        // This happens on the render thread.
        // ------------------------------------------------

        //RunRender(*renderFrame);
        {
        //SimpleTimer s([](float t){ LogInfo("GpuTime: {}", t); });
        gpuDevice->RunRender(*gpuRendererContext.renderFrame);
        gpuRendererContext.renderFrame->Clear();
        Platform::SwapBuffers();
        }

        // ------------------------------------------------
        // Tell main thread that render is complete.
        // ------------------------------------------------

        {
            std::lock_guard<std::mutex> lock(gpuRendererContext.mutex);
            gpuRendererContext.renderFinished = true;
        }

        gpuRendererContext.condition.notify_one();
    }

    gpuDevice->Shut();
}

void StartRender(){
    {
        std::lock_guard<std::mutex> lock(gpuRendererContext.mutex);
        gpuRendererContext.renderRequested = true;
    }

    gpuRendererContext.condition.notify_one();
}

void WaitForRender(){
    std::unique_lock<std::mutex> lock(gpuRendererContext.mutex);

    gpuRendererContext.condition.wait(
        lock,
        [&]
        {
            return gpuRendererContext.renderFinished || !gpuRendererContext.running;
        }
    );

    gpuRendererContext.renderFinished = false;
}

void SwapRenderFrames(){
    std::swap(
        gpuRendererContext.simulationFrame,
        gpuRendererContext.renderFrame
    );
}

GPURenderFrame& Application::GetRenderFrame(){
    return *gpuRendererContext.simulationFrame;
}

//Module* mainModule;
bool running = true;
int width;
int heigth;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; 

extern GraphicsDevice* graphicsDevice;

bool hasLoadGlobalSetting = false;

const std::vector<Module*> Application::Modules(){
    return modules;
}

//INFO: Maybe register a Shutdown callback and maybe a init call back too
bool Application::Create(Module* inMainModule, ApplicationConfig appConfig, const char* projectPath, ApplicationCallbacks incallbacks, Ref<Project> customProj){
    Log::Init();

    LogInfo("Engine Texture2D TypeId: {}", GetType<Texture2D>());
    //LogInfo("Engine ApplicationConfig EnttTypeId: {}", entt::type_id<ApplicationConfig>().index());
    //LogInfo("Engine Texture2D EnttTypeId: {}", entt::type_id<Texture2D>().index());

    Ref<Project> project = nullptr;

    if(customProj != nullptr){
        project = ProjectManager::LoadProject(projectPath, customProj);
    } else {
        project = ProjectManager::LoadProject(projectPath);
    }

    if(project == nullptr){
        Log::Shutdown();
        return false;
    }

    callbacks = incallbacks;

    /*auto exists = [](const char *fname){
        FILE *file;
        if((file = fopen(fname, "r"))){
            fclose(file);
            return 1;
        }
        return 0;
    };
    auto FileExists = [](const std::string& name){
        std::ifstream f(name);
        return f.good();
    };
    LogInfo("File exist: %s %d", "Engine/Fonts/fa-solid-900.ttf", exists("Engine/Fonts/fa-solid-900.ttf"));*/

    width = appConfig.startWidth;
    heigth = appConfig.startHeight;

    gpuRendererContext.multithread = false;

    Graphics::SelectGraphicsDevice();

    if(Platform::SystemStartup(appConfig) == false) return false;

    Graphics::Initialize();

    ////////////////////////////////
    #ifdef TestNewGPU_API
    gpuDevice = dynamic_cast<GPUDevice*>(Graphics::GetGraphicsDevice());

    if(!gpuRendererContext.multithread) gpuDevice->Init();

    gpuRendererContext.running = true;

    gpuRendererContext.simulationFrame = &gpuRendererContext.frames[0];
    gpuRendererContext.renderFrame = &gpuRendererContext.frames[1];

    if(gpuRendererContext.multithread){
        Platform::StopCurrentContext();
        gpuRendererContext.renderThread = std::thread(&Application::RenderThreadLoop);
    }
    #endif
    ////////////////////////////////
    
    //Input::_Initialize(0, 0);
    #ifdef __EMSCRIPTEN__
    #else
    //JobSystem::Initialize();
    #endif
    //AssetTypesDB::_Init();
    //CoreModulesStartup();
    
    //mainModule = inMainModule;
    AddModule(inMainModule); //mainModule);

    /*for(auto i: modules){
        i->OnInit();
    }*/

    running = true;
    appHasInited = true;

    PhysFS::Init();

    auto PathExists = [](const std::string& path){
        return std::filesystem::exists(path);
    };

    //OD::AssetManager::Get().StartHotReload();
    if(project->defaultPackagePath.empty() == false && PathExists(project->defaultPackagePath)){
        PhysFS::Mount(project->defaultPackagePath.c_str());
        ResourceManager::Get().Mount(PhysFS::GetPackage());
    }

    if(callbacks.onInit != nullptr) callbacks.onInit();

    return true;
}

//#include "OD/Platform/OpenGL/GL.h"
//#include <GLFW/glfw3.h>

void Application::DrawImGui(){
    if(graphicsDevice->ImGuiSupport() == false) return;

    OD_PROFILE_SCOPE("Application::Run::OnGUI");
    Platform::ImguiBegin();
    for(auto i: modules) i->OnGUI();
    Platform::ImguiEnd();
}

void Application::DrawImGui(std::function<void()> func){
    if(graphicsDevice->ImGuiSupport() == false) return;

    OD_PROFILE_SCOPE("Application::Run::OnGUI");
    Platform::ImguiBegin();
    func();
    Platform::ImguiEnd();
}

void Application::Loop(){
    if(hasLoadGlobalSetting == false){
        hasLoadGlobalSetting = true;
        GlobalSettings::Get().Load("../GlobalSettings");
    }

    #if OD_PROFILE
    Instrumentor::BeginLoop();
    #endif

    {
    OD_PROFILE_SCOPE("Application::Run");

    float currentFrame = Platform::GetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame; 

    Time::UnscaledDeltaTime(deltaTime);
    Time::Update(deltaTime);

    Platform::PumpMessages();
    //Platform::PreUpdate();
    //Graphics::_Begin();
    Input::Update();

    OD::ResourceManager::Get().ApplyHotReload();

    for(auto i: modulesToAdd) _AddModule(i);
    modulesToAdd.clear();

    inUpdate = true;
    {
        OD_PROFILE_SCOPE("Application::Run::OnUpdate");
        for(auto i: modules) i->OnUpdate(deltaTime);
    }

    //Platform::SwapBuffers();
    Platform::PreUpdate();
    Graphics::_Begin();
    {
        OD_PROFILE_SCOPE("Application::Run::OnRender");
        for(auto i: modules) i->OnRender(deltaTime);
    }
    DrawImGui();
    /*Platform::ImguiBegin();
    {
        if(graphicsDevice->ImGuiSupport()){
            OD_PROFILE_SCOPE("Application::Run::OnGUI");
            for(auto i: modules) i->OnGUI();
        }
        //#endif
    }
    Platform::ImguiEnd();*/
    inUpdate = false;

    {
    OD_PROFILE_SCOPE("Application::Run::3");
    for(auto i: modulesToRemove) _RemoveModule(i);
    modulesToRemove.clear();
    onFrameEnd.Invoke();
    }

    Input::PostUpdate();
    Graphics::_End();
    Platform::LateUpdate();

    #ifndef TestNewGPU_API
    Platform::PollEvents();
    Platform::SwapBuffers();
    #endif
    }

    #if OD_PROFILE
    Instrumentor::EndLoop();
    #endif
}

bool Application::Run(){
    /*while(running){
        #if OD_PROFILE
        Instrumentor::BeginLoop();
        #endif

        {
        OD_PROFILE_SCOPE("Application::Run");

        float currentFrame = Platform::GetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame; 

        Platform::PumpMessages();
        //Platform::PreUpdate();
        //Graphics::_Begin();
        Input::Update();

        for(auto i: modulesToAdd) _AddModule(i);
        modulesToAdd.clear();

        inUpdate = true;
        {
            OD_PROFILE_SCOPE("Application::Run::OnUpdate");
            for(auto i: modules) i->OnUpdate(deltaTime);
        }
        //Platform::SwapBuffers();
        Platform::PreUpdate();
        Graphics::_Begin();
        {
            OD_PROFILE_SCOPE("Application::Run::OnRender");
            for(auto i: modules) i->OnRender(deltaTime);
        }
        {
            OD_PROFILE_SCOPE("Application::Run::OnGUI");
            for(auto i: modules) i->OnGUI();
        }
        inUpdate = false;

        {
        OD_PROFILE_SCOPE("Application::Run::3");
        for(auto i: modulesToRemove) _RemoveModule(i);
        modulesToRemove.clear();
        onFrameEnd.Invoke();
        }

        Graphics::_End();
        Platform::LateUpdate();
        Platform::SwapBuffers();
        }

        #if OD_PROFILE
        Instrumentor::EndLoop();
        #endif
    }*/

#ifdef __EMSCRIPTEN__
    LogInfo("EMSCRIPTEN Loop");
    emscripten_set_main_loop(Loop, 0, true);
#else
    while(running){
        #ifdef TestNewGPU_API
        /*SimpleTimer timer([](float t){
            LogInfo("FrameTime: {}", t);
        });*/
        if(!gpuRendererContext.multithread){
            {
            //SimpleTimer s([](float t){ LogInfo("CpuTime: {}", t); });
            Loop();
            }

            {
            //SimpleTimer s([](float t){ LogInfo("GpuTime: {}", t); });
            gpuDevice->SyncSingleThreadData();
            gpuDevice->RunRender(*gpuRendererContext.simulationFrame);
            gpuRendererContext.simulationFrame->Clear();
            Platform::PollEvents();
            Platform::SwapBuffers();
            }
            continue;
        }

        StartRender();

        {
        //SimpleTimer s([](float t){ LogInfo("CpuTime: {}", t); });
        Loop();
        Platform::PollEvents();
        }
        WaitForRender();
        SwapRenderFrames();
        gpuDevice->SyncSingleThreadData();
        #else
        Loop();
        #endif
    }
#endif

    OnExit();

    return true;
}

void Application::OnExit(){
    ///////////////////////////
    #ifdef TestNewGPU_API
    if(!gpuRendererContext.multithread) gpuDevice->Shut();

    gpuRendererContext.condition.notify_all();

    if(gpuRendererContext.renderThread.joinable())
        gpuRendererContext.renderThread.join();

    #endif
    /////////////////

    if(callbacks.onShutdown != nullptr) callbacks.onShutdown();

    GlobalSettings::Get().Save("../GlobalSettings");

    //LogInfo("Application::OnExit");

    //OD::AssetManager::Get().StopHotReload();

    /*for(auto i: modules){
        i->OnExit();
        if(i->DeleteOnExit()) delete i;
    }*/
    //OnExit by reverse orde
    for(auto it = modules.rbegin(); it != modules.rend(); ++it){
        auto i = *it;
        i->OnExit();
        if(i->DeleteOnExit()) delete i;
    }
    modules.clear();

    onFrameEnd.Clean();

    ResourceTypesDB::Get().assetFuncs.clear();
    ResourceManager::Get().UnloadAll();
    //AssetTypesDB::Get().assetFuncs.clear();

    PhysFS::Shutdown();

    Graphics::Shutdown();
    //Input::_Shutdown(0);
    Platform::SystemShutdown(0);
    Log::Shutdown();
}

void Application::Quit(){
    running = false;
    gpuRendererContext.running = running;
}

void Application::Exit(){
    OnExit();
    exit(0);
}

void Application::GetFramebufferSize(int* width, int* height){}

float Application::DeltaTime(){ return deltaTime * Time::TimeScale(); }
int Application::ScreenWidth(){ return width; }
int Application::ScreenHeight(){ return heigth; }

void Application::Vsync(bool enabled){ Platform::SetVSync(enabled); }
bool Application::Vsync(){ return Platform::IsVSync(); }

void Application::_OnResize(int inWidth, int inHeight){
    width = inWidth;
    heigth = inHeight;

    if(width == 0 || heigth == 0)return; // skip

    //mainModule->OnResize(_width, _height);
    for(auto i: modules) i->OnResize(width, heigth);
}

Action<void()>& Application::GetOnFrameEnd(){
    return onFrameEnd;
}

void Application::RemoveModule(Module* module){
    if(inUpdate == false){
        _RemoveModule(module);
        return;
    }

    if(std::find(modules.begin(), modules.end(), module) == modules.end()){
        LogError("Trying remove module with has not added");
        return;
    }

    modulesToRemove.push_back(module);
}

void Application::AddModule(Module* module){
    if(inUpdate == true || appHasInited == false){
        modulesToAdd.push_back(module);
        return;
    }
    _AddModule(module); 
}

void Application::_RemoveModule(Module* module){
    if(std::find(modules.begin(), modules.end(), module) == modules.end()){
        LogError("Trying remove module with has not added");
        return;
    }

    modules.erase(std::remove(modules.begin(), modules.end(), module), modules.end());
    module->OnExit();
}

void Application::_AddModule(Module* module){
    modules.push_back(module);
    modules.back()->OnInit();

    //auto SortFunc = [](Module* a, Module* b){ return a->ExecutionSortPriority() < b->ExecutionSortPriority(); };
    //std::stable_sort(modules.begin(), modules.end(), SortFunc);
}

const std::vector<Module*>& Application::GetAllModules(){
    return modules;
}

std::vector<std::string>& Application::GetArgs(){
    return args;
}

void Application::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Application>(
        "Application",
        "ScreenWidth", Application::ScreenWidth,
        "ScreenHeight", Application::ScreenHeight,
        "DeltaTime", Application::DeltaTime,
        "Quit", Application::Quit,
        "Exit", Application::Exit,
        "Vsync", sol::overload(
            [](){ return Application::Vsync(); },
            [](bool v){ Application::Vsync(v); }
        )
    );
}

}