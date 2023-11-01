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

    static int screenWidth();
    static int screenHeight();
    static float deltaTime();
    static void vsync(bool enabled);
	static bool vsync();

    static void _OnResize(int width, int height);

    template<typename T>
    inline static void AddModule(){
        _modules.push_back(new T());
        _modules.back()->OnInit();
    }

    inline static void AddModule(Module* module){
        _modules.push_back(module);
        _modules.back()->OnInit();
    }

    static std::vector<std::string> args;

private:

    static std::vector<Module*> _modules;
};

}