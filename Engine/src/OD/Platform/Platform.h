#pragma once
#include "OD/Defines.h"

namespace OD{

enum class CursorState{Normal, Disabled, Hidden};

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

    static void SetCursorState(CursorState state);

    static void ShowWindow(bool show);
    
    static void* GetInternalData();

    static void* LoadDynamicLibrary(const char* dll);
    static void* LoadDynamicFunction(void* dll, const char* funcName);
    static bool FreeDynimicLibrary(void* dll);

    /*static void BeginOffscreenContextCurrent();
    static void EndOffscreenContextCurrent();*/
};

}