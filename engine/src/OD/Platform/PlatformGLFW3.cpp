#include "Platform.h"

#include <string.h>

#include "OD/Core/ImGui.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <ImGuizmo/ImGuizmo.h>

#include "GL.h"
#include <GLFW/glfw3.h>

#include "OD/Core/Input.h"
#include "OD/Core/Application.h"

#define OPENGL_DEBUG 1
#define OpenglMajorVer 4
#define OpenglMinorVer 6

namespace OD{

GLFWwindow* window;
bool vSync;

void UpdateFpsCounter(GLFWwindow* window) {
    static double previous_seconds;
    static int frame_count;
    double current_seconds = glfwGetTime();
    double elapsed_seconds = current_seconds - previous_seconds;
    if ( elapsed_seconds > 0.25 ) {
        previous_seconds = current_seconds;
        double fps       = (double)frame_count / elapsed_seconds;
        char tmp[128];
        sprintf( tmp, "opengl @ fps: %.2f", fps );
        glfwSetWindowTitle( window, tmp );
        frame_count = 0;
    }
    frame_count++;
}

void imguiOnInit(GLFWwindow* window){
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

    io.FontDefault = io.Fonts->AddFontFromFileTTF("res/Builtins/Fonts/OpenSans/static/OpenSans-Regular.ttf", 16.0f);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable){
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGuiLayer::SetDarkTheme();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void imguiOnPreUpdate(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void imguiOnUpdate(GLFWwindow* window){
    ImVec4 _clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    if(ImGuiLayer::GetCleanAll() == true){
        glClearColor(_clear_color.x * _clear_color.w, _clear_color.y * _clear_color.w, _clear_color.z * _clear_color.w, _clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable){
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void imguiOnDestroy(){
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
    Application::_OnResize(width, height);
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos){
    //Input::ProcessMouseMove(xpos, ypos);
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    //Input::ProcessMouseWheel(xoffset);
}

void DebugCallback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, const void* param) {
	
	std::string sourceStr;
	switch(source) {
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		sourceStr = "WindowSys";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		sourceStr = "App";
		break;
	case GL_DEBUG_SOURCE_API:
		sourceStr = "OpenGL";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		sourceStr = "ShaderCompiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		sourceStr = "3rdParty";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		sourceStr = "Other";
		break;
	default:
		sourceStr = "Unknown";
	}
	
	std::string typeStr;
	switch(type) {
	case GL_DEBUG_TYPE_ERROR:
		typeStr = "Error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		typeStr = "Deprecated";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		typeStr = "Undefined";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		typeStr = "Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		typeStr = "Performance";
		break;
	case GL_DEBUG_TYPE_MARKER:
		typeStr = "Marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		typeStr = "PushGrp";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		typeStr = "PopGrp";
		break;
	case GL_DEBUG_TYPE_OTHER:
		typeStr = "Other";
		break;
	default:
		typeStr = "Unknown";
	}
	
	std::string sevStr;
	switch(severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		sevStr = "HIGH";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		sevStr = "MED";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		sevStr = "LOW";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		sevStr = "NOTIFY";
		break;
	default:
		sevStr = "UNK";
	}

    //printf("%s:%s[%s](%d): %s\n", sourceStr, typeStr, sevStr, id, message);
    LogError("%s:%s[%s](%d): %s\n", sourceStr.c_str(), typeStr.c_str(), sevStr.c_str(), id, message);
}

bool Platform::SystemStartup(const char* applicationName, int x, int y, int width, int height){
    if (!glfwInit()) {
        LogError("Glfw Erro to init");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OpenglMajorVer);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OpenglMinorVer);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef OPENGL_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    #endif

    //glfwWindowHint(GLFW_MAXIMIZED , GL_TRUE);

    LogInfo("Glfw creationg windows: %s %d %d", applicationName, width, height);
    window = glfwCreateWindow(width, height, applicationName, NULL, NULL);

    if (!window){
        glfwTerminate();
        LogError("Glfw Erro on window creation");
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); //vsync on
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    #ifdef OPENGL_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(DebugCallback, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    #endif

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback); 
    glfwSetScrollCallback(window, ScrollCallback); 

    glViewport(0, 0, width, height);
    imguiOnInit(window);

    LogInfo("Opengl Version: %s", glGetString(GL_VERSION));

    return true;
}

void Platform::PreUpdate(){
    UpdateFpsCounter(window);
    imguiOnPreUpdate();
}

void Platform::LateUpdate(){
    imguiOnUpdate(window);
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
    if(glfwWindowShouldClose(window)){
        Application::Quit();
        return false;
    }
    return true; 
}

void Platform::SwapBuffers(){
    glfwSwapBuffers(window);
    glfwPollEvents();
}

float Platform::GetTime(){ return glfwGetTime(); }
void Platform::Sleep(double ms){}

void Platform::SetVSync(bool enabled){
    if (enabled)
        glfwSwapInterval(1);
    else
        glfwSwapInterval(0);

    vSync = enabled;
}

bool Platform::IsVSync(){ return vSync; }

void* Platform::GetInternalData(){
    return window;
}

}