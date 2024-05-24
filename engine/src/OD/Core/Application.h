#pragma once

#include "OD/Defines.h"
#include "Module.h"
#include <string>
#include <vector>

namespace OD {
    
struct ApplicationConfig {
    int startPosX;
    int startPosY;
    int startWidth;
    int startHeight;
    std::string name;
};

class OD_API Application {
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

    static void RemoveModule(Module* module);
    static void AddModule(Module* module);
    template<typename T> static void AddModule(){ AddModule(new T()); }

    static std::vector<std::string>& GetArgs();

private:
    static void OnExit();
};

}