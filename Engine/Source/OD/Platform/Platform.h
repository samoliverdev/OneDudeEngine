#pragma once
#include "OD/Defines.h"
#include <string>

namespace OD{

enum class CursorState{Normal, Disabled, Hidden};

class OD_API Platform{
    friend class Application;
public:
    static bool PumpMessages();
    static void SwapBuffers();

    static float GetTime();
    static void Sleep(double ms);

    static void SetVSync(bool enabled);
	static bool IsVSync();

    static void SetWindowSize(int width, int height);

    static CursorState GetCursorState();
    static void SetCursorState(CursorState state);

    static void ShowWindow(bool show);
    
    static void* GetInternalData();

    static void* LoadDynamicLibrary(const char* dll);
    static void* LoadDynamicFunction(void* dll, const char* funcName);
    static bool FreeDynimicLibrary(void* dll);

    static std::string OpenFolder();
    static std::string OpenFile(const char* filter = "");
    static std::string SaveFile(const char* filter = "");

    static void SetTaskbarProgress(unsigned long long, unsigned long long);
    static void ClearTaskbarProgress();

    static void ShowPopupProgress();
    static void UpdatePopupProgress(unsigned int);
    static void HidePopupProgress();

private:
    static bool SystemStartup(const char* applicationName, int x, int y, int width, int height);
    static void SystemShutdown(void* plat_state);

    static void PreUpdate();
    static void LateUpdate();

    static void ImguiBegin();
    static void ImguiEnd();

    /*static void BeginOffscreenContextCurrent();
    static void EndOffscreenContextCurrent();*/
};

}