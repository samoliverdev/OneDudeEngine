#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include <string>

namespace OD{

enum class CursorState{Normal, Disabled, Hidden};

class OD_API Platform{
    friend class Application;
    friend class MultithreadRendererContext;
public:
    static bool PumpMessages();
    
    static void PollEvents();
    static void SwapBuffers();

    static float GetTime();
    static void Sleep(double ms);

    static void SetVSync(bool enabled);
	static bool IsVSync();

    static void SetFullscreen(bool enabled);
    static bool IsFullscreen();
    static void SetWindowSize(int width, int height);
    static IVector2 GetWindowSize();
    static std::vector<IVector2> GetSupportedResolutions();

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

    static void CreateVulkanSurface(void* instance, void* surface);

private:
    static bool SystemStartup(const struct ApplicationConfig& config);

    static void StopCurrentContext();
    static void MakeMultiThreadContext();
    
    static void SystemShutdown(void* plat_state);

    static void PreUpdate();
    static void LateUpdate();

    static void ImguiBegin();
    static void ImguiEnd();

    /*static void BeginOffscreenContextCurrent();
    static void EndOffscreenContextCurrent();*/
};

}