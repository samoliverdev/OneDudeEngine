#pragma once

#include "OD/Defines.h"
#include "Module.h"
#include <string>

namespace OD {
    
struct ApplicationConfig {
    int startPosX;
    int startPosY;
    int startWidth;
    int startHeight;
    std::string name;
};

class Application {
public:
    static bool Create(Module* mainModule, ApplicationConfig startAppConfig);
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

    template<typename T>
    inline static void AddModule(){
        modules.push_back(new T());
        modules.back()->OnInit();
    }

    inline static void AddModule(Module* module){
        modules.push_back(module);
        modules.back()->OnInit();
    }

    static std::vector<std::string> args;

private:

    static std::vector<Module*> modules;
};

}