#pragma once

#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "Module.h"
#include <string>
#include <vector>

namespace OD {
    
struct OD_API ProjectSettings{
    std::string name = "Untitled";
    std::string startScene;
    std::string assetDirectory = "";
    std::string scriptModulePath = "";

    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, name);
        ArchiveDumpNVP(ar, startScene);
        ArchiveDumpNVP(ar, assetDirectory);
        ArchiveDumpNVP(ar, scriptModulePath);
    }
};

struct ApplicationConfig {
    int startPosX;
    int startPosY;
    int startWidth;
    int startHeight;
    std::string name;
};

class OD_API Application {
public:
    static bool Create(Module* mainModule, ApplicationConfig startAppConfig, const char* projectPath = "");
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

    static ProjectSettings& GetProjectSettings();

    static void RemoveModule(Module* module);
    static void AddModule(Module* module);
    template<typename T> static void AddModule(){ AddModule(new T()); }

    static const std::vector<Module*>& GetAllModules();
    
    template<typename T> static T* GetModuleByType(){
        for(Module* m : GetAllModules()){
            T* result = dynamic_cast<T*>(m);
            if(result != nullptr) return result;
        }
        return nullptr;
    }

    static std::vector<std::string>& GetArgs();

private:
    static void OnExit();
};

}