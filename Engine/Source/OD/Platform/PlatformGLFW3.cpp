#include "OD/pch.h"
#include "Platform.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/Input.h"
#include "OD/Core/Application.h"
#include "OD/Core/Project.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Scene/SceneManager.h"
#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>
#include <string.h>

#ifdef EMSCRIPTEN
#include<emscripten/emscripten.h>
#define GLFW_INCLUDE_ES3
#endif

#include <imgui/backends/imgui_impl_glfw.h>

#define GLFW_INCLUDE_NONE
//#include "OpenGL/GL.h"
#include <GLFW/glfw3.h>

#include <fstream>

/*#define OPENGL_DEBUG 1
#define OpenglMajorVer 4
#define OpenglMinorVer 6*/

#define OPENGL_DEBUG //need enable in OpenglGraphicDevice.cpp too

namespace OD{

extern GraphicsDevice* graphicsDevice;

GLFWwindow* window;
//GLFWwindow* offscreenWindow;
int windowPosX, windowPosY;
bool vSync = false;
bool fullscreen = false;
bool hidden = false;

CursorState cursorState;

void UpdateFpsCounter(GLFWwindow* window){
    static double previous_seconds;
    static int frame_count;
    double current_seconds = glfwGetTime();
    double elapsed_seconds = current_seconds - previous_seconds;
    if ( elapsed_seconds > 0.25 ) {
        previous_seconds = current_seconds;
        double fps       = (double)frame_count / elapsed_seconds;
        char tmp[128];
        sprintf( tmp, "opengl @ fps: %.2f", fps );
        #if !defined(__EMSCRIPTEN__)
        glfwSetWindowTitle( window, tmp );
        #endif
        frame_count = 0;
    }
    frame_count++;
}

void UpdateWindowTitle(GLFWwindow* window){
    static double previous_seconds;
    static int frame_count;
    double current_seconds = glfwGetTime();
    double elapsed_seconds = current_seconds - previous_seconds;
    if(elapsed_seconds > 0.25){
        previous_seconds = current_seconds;
        double fps = (double)frame_count / elapsed_seconds;
        frame_count = 0;

        char tmp[128*4];
        sprintf(
            tmp, 
            "%s - %s - Opengl - Fps: %.2f", 
            ProjectManager::GetActiveProject() != nullptr ? ProjectManager::GetActiveProject()->name.c_str() : "None Project", 
            SceneManager::Get().GetActiveScene() != nullptr ? SceneManager::Get().GetActiveScene()->Path().c_str() : "None Scene",
            fps
        );
        #if !defined(__EMSCRIPTEN__)
        glfwSetWindowTitle( window, tmp );
        #endif
    }
    frame_count++;
}

void imguiOnInit(GLFWwindow* window){
    if(graphicsDevice->ImGuiSupport() == false) return;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //io = ImGui::GetIO(); (void)io;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    auto FileExists = [](const std::string& name){
        std::ifstream f(name);
        return f.good();
    };

    if(FileExists("Engine/Fonts/OpenSans/static/OpenSans-Regular.ttf")){
        io.FontDefault = io.Fonts->AddFontFromFileTTF("Engine/Fonts/OpenSans/static/OpenSans-Regular.ttf", 16.5f);
    }

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable){
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGuiLayer::SetDarkTheme();

    //ImGui::GetStyle().Alpha = 0.9f;   // 50% opacity

    auto graphicsDeviceInfo = graphicsDevice->GetInfo();

    if(graphicsDeviceInfo.apiName == "OpenGL"){
        ImGui_ImplGlfw_InitForOpenGL(window, true);
    } else {
        ImGui_ImplGlfw_InitForOther(window, true);
    }

    // Setup Platform/Renderer backends
    //ImGui_ImplGlfw_InitForOpenGL(window, true);
    //ImGui_ImplOpenGL3_Init("#version 150");
    //graphicsDevice->ImGuiInit();
}

void Platform::ImguiBegin(){
    if(graphicsDevice->ImGuiSupport() == false) return;

    /*#if defined(__EMSCRIPTEN__)
    return;
    #endif*/

    graphicsDevice->ImGuiNewFrame();
    //ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void Platform::ImguiEnd(){
    if(graphicsDevice->ImGuiSupport() == false) return;

    ImVec4 _clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Rendering
    ImGui::Render();
    
    /*int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    if(ImGuiLayer::GetCleanAll() == true){
        glClearColor(_clear_color.x * _clear_color.w, _clear_color.y * _clear_color.w, _clear_color.z * _clear_color.w, _clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());*/

    /*int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    graphicsDevice->ImGuiRenderDrawData(0, 0, display_w, display_h);*/
    graphicsDevice->ImGuiRenderDrawData(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    return;

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    ImGuiIO& io = ImGui::GetIO();
    if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable){
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void imguiOnDestroy(){
    if(graphicsDevice->ImGuiSupport() == false) return;
    //if(graphicsDevice->ImGuiSupport() == false) return;
    
    // Cleanup
    //ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height){
    //glViewport(0, 0, width, height);
    Application::_OnResize(width, height);
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos){
    //LogInfo("MouseCallback");
    //Input::ProcessMouseMove(xpos, ypos);
}

extern Vector2 mouseWheelOffsets;

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    //Input::ProcessMouseWheel(xoffset);
    mouseWheelOffsets.x += xoffset;
    mouseWheelOffsets.y += yoffset;
}

bool Platform::SystemStartup(const ApplicationConfig& config){
    if(!glfwInit()){
        LogError("Glfw Erro to init");
        return false;
    }

    auto graphicsDeviceInfo = graphicsDevice->GetInfo();

    #if !defined(__EMSCRIPTEN__)
    if(graphicsDeviceInfo.apiName == "OpenGL"){
        if(graphicsDeviceInfo.version == 4){
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 0);
            #ifdef OPENGL_DEBUG
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            #endif
        } else if(graphicsDeviceInfo.version == 3){
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
        } else {
            Assert(false && "OpenGL Version not suppoted");
        }
    } else{
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
	    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }
    
    glfwWindowHint(GLFW_VISIBLE, hidden == false ? GLFW_TRUE : GLFW_FALSE);
    #else   
    
    #endif


    //glfwWindowHint(GLFW_MAXIMIZED , GL_TRUE);

    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    //offscreenWindow = glfwCreateWindow(640, 480, "", NULL, NULL);

    //glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    //INFO: Experimental
    if(config.transparentWindows){
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
        glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE); //INFO: to this work, i modied: imgui_impl_glfw see in the: "INFO: Disable to...."
    }

    LogInfo("Glfw creationg windows: {} {} {}", config.name, config.startWidth, config.startHeight);
    window = glfwCreateWindow(config.startWidth, config.startHeight, config.name.c_str(), NULL, NULL);

    /*const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    width = mode->width;
    height = mode->height;
    window = glfwCreateWindow(width, height, applicationName, glfwGetPrimaryMonitor(), nullptr);*/

    if(!window){
        glfwTerminate();
        LogError("Glfw Erro on window creation");
        return false;
    }

    #if !defined(__EMSCRIPTEN__)
    glfwSwapInterval(0); //vsync on
    #endif

    if(graphicsDeviceInfo.apiName == "OpenGL"){
        //#if !defined(__EMSCRIPTEN__)
        glfwMakeContextCurrent(window);
        graphicsDevice->LoadContext((void*)glfwGetProcAddress);
        //#endif
        //gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        /*if(graphicsDeviceInfo.version == 4){
            #if OPENGL_DEBUG
            glEnable(GL_DEBUG_OUTPUT);
            glDebugMessageCallback(DebugCallback, NULL);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
            #endif
        }*/
    }

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback); 
    glfwSetScrollCallback(window, ScrollCallback); 

    //glViewport(0, 0, width, height);
    imguiOnInit(window);

    if(config.transparentWindows){
        glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE); //INFO: to this work, i modied: imgui_impl_glfw see in the: "INFO: Disable to...."
    }

    /*LogInfo("Opengl Version: %s", glGetString(GL_VERSION));
    LogInfo("GL_VENDOR: %s", glGetString(GL_VENDOR));
    LogInfo("GL_RENDERER: %s", glGetString(GL_RENDERER));
    LogInfo("GL_SHADING_LANGUAGE_VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));*/

    //glEnable(GL_POLYGON_SMOOTH);

    return true;
}

void Platform::PreUpdate(){
    OD_PROFILE_SCOPE("Platform::PreUpdate");
    //UpdateFpsCounter(window);
    UpdateWindowTitle(window);
    //imguiOnPreUpdate();
}

void Platform::LateUpdate(){
    //OD_PROFILE_SCOPE("Platform::LateUpdate");
}

void Platform::SystemShutdown(void* plat_state){
    imguiOnDestroy();
    glfwDestroyWindow(window);
    glfwTerminate();

    LogInfo("Glfw shutdown");
}

bool Input::IsKey(KeyCode key){
    auto state = glfwGetKey(window, (int)key);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButton(MouseButton button){
    auto state = glfwGetMouseButton(window, (int)button);
    return state == GLFW_PRESS;
}

void Input::GetMousePosition(double* x, double* y){
    glfwGetCursorPos(window, x, y);
}

bool Platform::PumpMessages(){ 
    OD_PROFILE_SCOPE("Platform::PumpMessages");
    
    if(glfwWindowShouldClose(window)){
        Application::Quit();
        return false;
    }
    return true; 
}

void Platform::SwapBuffers(){
    {
    /*OD_PROFILE_SCOPE("Platform::glFlush");
    glFinish();
    Sleep(1);*/
    }

    {
    OD_PROFILE_SCOPE("Platform::SwapBuffers");
    glfwSwapBuffers(window);
    }

    {
    OD_PROFILE_SCOPE("Platform::glfwPollEvents");
    glfwPollEvents();
    //glfwWaitEvents();
    }
}

float Platform::GetTime(){ return glfwGetTime(); }
void Platform::Sleep(double ms){}

void Platform::SetVSync(bool enabled){
    if(enabled){
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }

    vSync = enabled;
}

bool Platform::IsVSync(){ return vSync; }

void Platform::SetFullscreen(bool enabled){
    fullscreen = enabled;

    if(fullscreen){
        int windowWidth;
        int windowHeight;

        // Save windowed position & size
        glfwGetWindowPos(window, &windowPosX, &windowPosY);
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        // Get primary monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // Switch to fullscreen
        glfwSetWindowMonitor(
            window,
            monitor,
            0, 0,
            windowWidth, //mode->width,
            windowHeight, //mode->height,
            mode->refreshRate
        );
    } else {
        int windowWidth;
        int windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        glfwSetWindowMonitor(
            window,
            nullptr,
            windowPosX,
            windowPosY,
            windowWidth,
            windowHeight,
            0
        );
    }
}

bool Platform::IsFullscreen(){
    return fullscreen;
}

void Platform::SetWindowSize(int width, int height){
    glfwSetWindowSize(window, width, height);
}

IVector2 Platform::GetWindowSize(){
    int windowWidth;
    int windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    return {windowWidth, windowHeight};
}

std::vector<IVector2> Platform::GetSupportedResolutions(){
    std::vector<IVector2> result;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if(!monitor) return result;

    int count;
    const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);

    for(int i = 0; i < count; ++i){
        IVector2 res{ modes[i].width, modes[i].height };

        // Avoid duplicates (many modes differ only by refresh rate)
        bool exists = false;
        for(const auto& r : result){
            if (r.x == res.x && r.y == res.y){
                exists = true;
                break;
            }
        }

        if(!exists) result.push_back(res);
    }

    return result;
}

CursorState Platform::GetCursorState(){
    return cursorState;
}

void Platform::SetCursorState(CursorState state){
    cursorState = state;
    if(cursorState == CursorState::Normal) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if(cursorState == CursorState::Hidden) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    if(cursorState == CursorState::Disabled) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Platform::ShowWindow(bool show){
    hidden = show == false;

    if(window == nullptr) return;

    if(show){
        glfwShowWindow(window);
    } else {
        glfwHideWindow(window);
    }
}

void* Platform::GetInternalData(){
    return window;
}

/*void Platform::BeginOffscreenContextCurrent(){
    glfwMakeContextCurrent(offscreenWindow);
}

void Platform::EndOffscreenContextCurrent(){
    glfwMakeContextCurrent(nullptr);
}*/

}

#ifdef _WIN32

#include <windows.h>
#include <shobjidl.h> // For ITaskbarList3
#include <objbase.h>  
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h> // For glfwGetWin32Window
#include <commctrl.h> // For progress bar
#include <atomic>
#include <thread>

// Globals for taskbar progress
static ITaskbarList3* g_Taskbar = nullptr;
static HWND g_TaskbarHwnd = nullptr;

// Globals for popup progress window
static HWND g_ProgressWnd = nullptr;
static HWND g_ProgressBar = nullptr;
static std::atomic<bool> g_Running{ false };
static std::thread g_ProgressThread;

// Forward declarations
LRESULT CALLBACK ProgressWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void CreateProgressWindow() {
    /*InitCommonControls();

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = ProgressWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"ODProgressWindow";

    RegisterClassW(&wc);

    const int winWidth = 320;
    const int winHeight = 100;

    // Calcula o centro da tela
    int screenWidth = OD::Application::ScreenWidth(); //GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = OD::Application::ScreenHeight(); //GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - winWidth) / 2;
    int posY = (screenHeight - winHeight) / 2;

    g_ProgressWnd = CreateWindowExW(
        0, wc.lpszClassName, L"Processing Progress",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        posX, posY, winWidth, winHeight,
        NULL, NULL, wc.hInstance, NULL
    );

    g_ProgressBar = CreateWindowEx(
        0, PROGRESS_CLASS, NULL,
        WS_CHILD | WS_VISIBLE,
        20, 40, 280, 20,
        g_ProgressWnd, NULL, wc.hInstance, NULL
    );

    SendMessage(g_ProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(g_ProgressBar, PBM_SETSTEP, (WPARAM)1, 0);

    ShowWindow(g_ProgressWnd, SW_SHOW);
    UpdateWindow(g_ProgressWnd);*/

    InitCommonControls();

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = ProgressWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Ou crie um pincel escuro customizado
    wc.lpszClassName = L"ODProgressWindow";

    RegisterClassW(&wc);

    const int winWidth = 320;
    const int winHeight = 100;

    // Centraliza a janela
    int screenWidth = OD::Application::ScreenWidth();
    int screenHeight = OD::Application::ScreenHeight();
    int posX = (screenWidth - winWidth) / 2;
    int posY = (screenHeight - winHeight) / 2;

    g_ProgressWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW, // Layered permite transparência e WS_EX_TOOLWINDOW remove da barra de tarefas
        wc.lpszClassName, L"",
        WS_POPUP | WS_VISIBLE, // WS_POPUP remove a moldura
        posX, posY, winWidth, winHeight,
        NULL, NULL, wc.hInstance, NULL
    );

    // Escurecer fundo da janela
    SetLayeredWindowAttributes(g_ProgressWnd, 0, 255, LWA_ALPHA);
    HBRUSH darkBrush = CreateSolidBrush(RGB(30, 30, 30)); // fundo escuro
    SetClassLongPtrW(g_ProgressWnd, GCLP_HBRBACKGROUND, (LONG_PTR)darkBrush);

    // Barra de progresso estilo moderno
    g_ProgressBar = CreateWindowEx(
        0, PROGRESS_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        20, 40, 280, 20,
        g_ProgressWnd, NULL, wc.hInstance, NULL
    );

    SendMessage(g_ProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(g_ProgressBar, PBM_SETSTEP, (WPARAM)1, 0);

    ShowWindow(g_ProgressWnd, SW_SHOW);
    UpdateWindow(g_ProgressWnd);
}

void UpdateProgressWindow(int percent) {
    if (g_ProgressBar) {
        SendMessage(g_ProgressBar, PBM_SETPOS, (WPARAM)percent, 0);
        UpdateWindow(g_ProgressBar);
    }
}

void CloseProgressWindow() {
    if (g_ProgressWnd) {
        DestroyWindow(g_ProgressWnd);
        g_ProgressWnd = nullptr;
        g_ProgressBar = nullptr;
    }
}

LRESULT CALLBACK ProgressWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    /*switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);*/

    switch (msg) {
    case WM_DESTROY:
        // NÃO CHAME PostQuitMessage AQUI!
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Run a message loop on a thread to keep the progress window responsive
void ProgressWindowThread() {
    MSG msg;
    while (g_Running.load()) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void OD::Platform::SetTaskbarProgress(unsigned long long current, unsigned long long total) {
    if (!g_Taskbar) {
        HRESULT hr = ::CoCreateInstance(
            CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
            __uuidof(ITaskbarList3),
            reinterpret_cast<void**>(&g_Taskbar)
        );
        if (FAILED(hr) || !OD::window) return;

        g_Taskbar->HrInit();
        g_TaskbarHwnd = glfwGetWin32Window(window);
        if (!g_TaskbarHwnd) return;

        g_Taskbar->SetProgressState(g_TaskbarHwnd, TBPF_NORMAL);
    }

    if (g_Taskbar) {
        g_Taskbar->SetProgressValue(g_TaskbarHwnd, current, total);
    }
}

void OD::Platform::ClearTaskbarProgress() {
    if (g_Taskbar && g_TaskbarHwnd) {
        g_Taskbar->SetProgressState(g_TaskbarHwnd, TBPF_NOPROGRESS);
        g_Taskbar->Release();
        g_Taskbar = nullptr;
        g_TaskbarHwnd = nullptr;
    }
}

// NEW functions to control popup progress window:
void OD::Platform::ShowPopupProgress() {
    if (g_Running.load()) return; // already running

    g_Running.store(true);
    CreateProgressWindow();
    g_ProgressThread = std::thread(ProgressWindowThread);
}

void OD::Platform::UpdatePopupProgress(unsigned int percent) {
    UpdateProgressWindow(percent);
}

void OD::Platform::HidePopupProgress() {
    g_Running.store(false);
    if (g_ProgressThread.joinable())
        g_ProgressThread.join();
    CloseProgressWindow();
}

#else

// Dummy stubs for non-Windows platforms

void OD::Platform::SetTaskbarProgress(unsigned long long, unsigned long long) {}
void OD::Platform::ClearTaskbarProgress() {}

void OD::Platform::ShowPopupProgress() {}
void OD::Platform::UpdatePopupProgress(unsigned int) {}
void OD::Platform::HidePopupProgress() {}


#endif