#pragma once
#include "OD/Defines.h"

namespace OD{

class OD_API Platform{
public:
    static bool SystemStartup(const char* applicationName, int x, int y, int width, int height);
    static void SystemShutdown(void* plat_state);

    static void PreUpdate();
    static void LateUpdate();

    static bool PumpMessages();
    static void SwapBuffers();

    static float GetTime();
    static void Sleep(double ms);

    static void SetVSync(bool enabled);
	static bool IsVSync();

    static void ShowWindow(bool show);
    
    static void* GetInternalData();

    static void* LoadDynamicLibrary(char* dll);
    static void* LoadDynamicFunction(void* dll, char* funcName);
    static bool FreeDynimicLibrary(void* dll);
};

}