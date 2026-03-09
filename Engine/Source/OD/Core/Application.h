#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "Action.h"
#include <string>
#include <vector>
#include <functional>

namespace sol{ 
    class state; 
}

namespace OD{

class Module;
    
struct ApplicationConfig{
    int startPosX;
    int startPosY;
    int startWidth;
    int startHeight;
    std::string name;
    bool transparentWindows = false;
};

struct ApplicationCallbacks{
    std::function<void()> onInit;
    std::function<void()> onShutdown;
};

class OD_API Application{
    friend class WebGPUGraphicsDevice;
    friend class OpenGLGraphicsDevice;
public:
    static bool Create(Module* mainModule, ApplicationConfig startAppConfig, const char* projectPath = "", ApplicationCallbacks callbacks = {});
    static bool Run();
    
    static void Quit();
    static void Exit();
    static void GetFramebufferSize(int* width, int* height);

    static int ScreenWidth();
    static int ScreenHeight();
    static float DeltaTime();
    static void Vsync(bool enabled);
	static bool Vsync();

    static void _OnResize(int width, int height);

    //static Project& GetProjectSettings();

    static Action<void()>& GetOnFrameEnd();

    static void RemoveModule(Module* module);
    static void AddModule(Module* module);

    static const std::vector<Module*> Modules();

    //template<typename T> static void AddModule(){ _AddModule(new T()); }

    template<typename T, typename... Args>
    static void AddModule(Args&&... args) {
        _AddModule(new T(std::forward<Args>(args)...));
    }
    
    static const std::vector<Module*>& GetAllModules();
    
    template<typename T> static T* GetModuleByType(){
        for(Module* m : GetAllModules()){
            T* result = dynamic_cast<T*>(m);
            if(result != nullptr) return result;
        }
        return nullptr;
    }

    static std::vector<std::string>& GetArgs();

    static void CreateLuaBind(sol::state& lua);

private:
    static void Loop();
    static void _RemoveModule(Module* module);
    static void _AddModule(Module* module);
    static void OnExit();

    static void DrawImGui();
    static void DrawImGui(std::function<void()> func);
};

}