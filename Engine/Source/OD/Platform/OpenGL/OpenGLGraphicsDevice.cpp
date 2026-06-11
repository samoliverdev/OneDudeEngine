#ifdef OPENGL_SUPPORT
#include "OD/pch.h"
#include "OpenGLGraphicsDevice.h"
#include "GL.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Camera.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Font.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Model.h"
#include "OD/Graphics/SubShader.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Cubemap.h"
#include "OD/Graphics/InstancingBuffer.h"
#include "OD/Graphics/UniformBuffer.h"
#include "OD/Graphics/ComputeBuffer.h"
#include "OD/Graphics/ComputeShader.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/SerializationFull.h"
#include "OD/Core/Application.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Log.h"
#include <stb/stb_image.h> //TODO: Remove this from This Graphic device
#include <imgui/backends/imgui_impl_opengl3.h>

#define UseUniformBuffer 1

namespace OD{

#define OPENGL_DEBUG //need enable in PlatformGLFW3.cpp too

constexpr size_t operator"" _KB(unsigned long long v) { return v * 1024ULL; }
constexpr size_t operator"" _MB(unsigned long long v) { return v * 1024ULL * 1024ULL; }

struct VRAMTracker{
    enum class Category{
        Other, Texture, Mesh, Framebuffer, Buffer, Shader
    };

    size_t totalAllocatedBytes = 0;
    size_t totalFreedBytes     = 0;

    // Optional: per-category tracking
    size_t texturesBytes       = 0;
    size_t meshBytes       = 0;
    size_t framebuffersBytes   = 0;
    size_t buffersBytes        = 0;     // VBO, EBO, UBO, instancing
    size_t shadersBytes        = 0;     // very rough

    void Add(size_t bytes, Category category = Category::Other){
        totalAllocatedBytes += bytes;
        if (category == Category::Texture)      texturesBytes     += bytes;
        else if (category == Category::Mesh) meshBytes += bytes;
        else if (category == Category::Framebuffer) framebuffersBytes += bytes;
        else if (category == Category::Buffer)      buffersBytes      += bytes;
        else if (category == Category::Shader)      shadersBytes      += bytes;

        Assert(texturesBytes < 2000_MB);
        Assert(meshBytes < 2000_MB);
    }

    void Free(size_t bytes, Category category = Category::Other){
        totalFreedBytes += bytes;
        if (category == Category::Texture)      texturesBytes     -= bytes;
        else if (category == Category::Mesh) meshBytes -= bytes;
        else if (category == Category::Framebuffer) framebuffersBytes -= bytes;
        else if (category == Category::Buffer)      buffersBytes      -= bytes;
        else if (category == Category::Shader)      shadersBytes      -= bytes;
    }

    size_t CurrentUsage() const {
        return totalAllocatedBytes > totalFreedBytes ? totalAllocatedBytes - totalFreedBytes : 0;
    }

    // For debugging / UI
    /*std::string Report() const {
        char buf[512];
        snprintf(buf, sizeof(buf),
            "VRAM estimate: %zu MiB total\n"
            "  Textures:     %zu MiB\n"
            "  Framebuffers: %zu MiB\n"
            "  Buffers:      %zu MiB\n"
            "  Shaders:      %zu KiB\n",
            CurrentUsage() / (1024*1024),
            texturesBytes     / (1024*1024),
            framebuffersBytes / (1024*1024),
            buffersBytes      / (1024*1024),
            shadersBytes      / 1024
        );
        return buf;
    }*/
};

VRAMTracker vram;   // member of OpenGLGraphicsDevice or global / singleton

//INFO: This is becose GL_POINTS is equal of GL_NONE (0) 
#define INVALID_DRAW_MODE 0xFFFFFFFF

GLenum meshDrawModeLookup[] = {
    GL_TRIANGLES,
    GL_LINES,
    GL_POINTS,
    #ifdef OpenGL46
    GL_QUADS,
    #else
    INVALID_DRAW_MODE, //GL_NONE,
    #endif
    GL_TRIANGLE_STRIP
};  

OpenGLGraphicsDevice::OpenGLGraphicsDevice(){
    info.apiName = "OpenGL";
    info.version = OpenGLVersion;
    info.supportUniformBuffer = true;
}

void OpenGLGraphicsDevice::LoadContext(void* data){
    LogInfo("OpenGLGraphicsDevice::LoadContext");
    #ifdef __EMSCRIPTEN__
    #else
    gladLoadGLLoader((GLADloadproc)data);
    #endif
}

GraphicsStats& OpenGLGraphicsDevice::GetStats(){ 
    return stats; 
}

GraphicsDebug& OpenGLGraphicsDevice::GetGraphicsDebug(){ 
    return debugData; 
}

GPUMemoryStats& OpenGLGraphicsDevice::GetMemoryStats(){
    memoryStats.buffersBytes = vram.buffersBytes;
    memoryStats.framebuffersBytes = vram.framebuffersBytes;
    memoryStats.meshBytes = vram.meshBytes;
    memoryStats.texturesBytes = vram.texturesBytes;
    return memoryStats;
}

GraphicsDeviceInfo OpenGLGraphicsDevice::GetInfo(){
    return info;
}

#ifdef OPENGL_DEBUG
void DebugCallback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, const void* param){
	
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

    //if(source == GL_DEBUG_SOURCE_SHADER_COMPILER && type == GL_DEBUG_TYPE_OTHER) return;

    //printf("%s:%s[%s](%d): %s\n", sourceStr, typeStr, sevStr, id, message);
    LogError("{}:{}[{}]({}): {}\n", sourceStr, typeStr, sevStr, id, message);
    Assert(false);
}
#endif

Ref<Mesh> _cubeMesh = nullptr;
unsigned int globalMat4VBO = 0;

#if 0
void ValidateTextures(){
    GLint maxUnits = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);

    for(int i = 0; i < maxUnits; i++){
        glActiveTexture(GL_TEXTURE0 + i);

        GLint tex = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);

        if(tex == 0) continue; // unused slot is fine

        Assert(glIsTexture(tex));
        if(!glIsTexture(tex)) {
            printf("Invalid texture at unit %d\n", i);
            __debugbreak();
        }

        GLint width = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);

        Assert(width > 0);
        if (width == 0) {
            printf("Texture has NO DATA at unit %d\n", i);
            __debugbreak();
        }
    }
}

void OnDrawAssetsTest(){
    // Check program
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    Assert(program != 0);

    // Check texture unit 0
    /*glActiveTexture(GL_TEXTURE0);

    GLint tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    Assert(tex != 0);
    Assert(glIsTexture(tex));

    // Check texture has data
    GLint width = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    Assert(width > 0);*/

    ValidateTextures();
}
#else
#define OnDrawAssetsTest()
#endif

void OpenGLGraphicsDevice::Initialize(){
    auto CreateLineVAO = [&](unsigned int* vao, unsigned int* vbo, int vertexCount){
        #ifdef USE_VAO
        glGenVertexArrays(1, vao);
        glBindVertexArray(*vao);
        glCheckError();
        #endif
        
        glGenBuffers(1, vbo);
        glBindBuffer(GL_ARRAY_BUFFER, *vbo);
        //glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCount * 3, NULL, GL_STATIC_DRAW);
        glCheckError();
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
        glEnableVertexAttribArray(0);
        glCheckError();
    
        #ifdef USE_VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glCheckError();
        #endif
    };
    
    auto CreateWiredCubeVAO = [&](unsigned int& wiredCubeVAO, unsigned int& wiredCubeEBO, unsigned int& wiredCubeVBO){
        float vertex[] = {
            -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f, 0.5f, -0.5f,   -0.5f, 0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f, 0.5f,  0.5f,   -0.5f, 0.5f,  0.5f
        };
    
        unsigned int indices[] = {
            0, 1, 1, 2, 2, 3, 3, 0, 
            4, 5, 5, 6, 6, 7, 7, 4,
            0, 4, 1, 5, 2, 6, 3, 7
        };
    
        #ifdef USE_VAO
        glGenVertexArrays(1, &wiredCubeVAO);
        glBindVertexArray(wiredCubeVAO);
        glCheckError();
        #endif
        
    
        glGenBuffers(1, &wiredCubeEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wiredCubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glCheckError();
    
        glGenBuffers(1, &wiredCubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, wiredCubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);
        glCheckError();
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
        glEnableVertexAttribArray(0);
        glCheckError();
    
        #ifdef USE_VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glCheckError();
        #endif
    };
    
    auto CreateTextQuadVAO = [&](unsigned int& textQuadVAO, unsigned int& textQuadVBO){
        #ifdef USE_VAO
        glGenVertexArrays(1, &textQuadVAO);
        glBindVertexArray(textQuadVAO);
        glCheckError();
        #endif
    
        glGenBuffers(1, &textQuadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 5, NULL, GL_STATIC_DRAW);
        glCheckError();
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        glCheckError();
    
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glCheckError();
    
        #ifdef USE_VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);   
        glCheckError();
        #endif
    };

    LogInfo("OpenGLGraphicsDevice::Initialize");

    glViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    LogInfo("OpenGLGraphicsDevice::Initialize2");
    //#if !defined(__EMSCRIPTEN__)
    if(ImGuiSupport()){
        ImGui_ImplOpenGL3_Init(
            OpenglHeader 
            //"#version 150"
        );
    }
    //#endif

    #ifdef OPENGL_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(DebugCallback, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    #endif

    LogInfo("Opengl Version: {}", (char*)glGetString(GL_VERSION));
    LogInfo("GL_VENDOR: {}", (char*)glGetString(GL_VENDOR));
    LogInfo("GL_RENDERER: {}", (char*)glGetString(GL_RENDERER));
    LogInfo("GL_SHADING_LANGUAGE_VERSION: {}", (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    GLint maxBindings = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);
    LogInfo("GL_MAX_UNIFORM_BUFFER_BINDINGS: {}", maxBindings);

    glEnable(GL_DEPTH_TEST); 

    #ifndef USE_VAO
    glGenVertexArrays(1, &globalVAO);
	glBindVertexArray(globalVAO);
    glCheckError();
    #endif

    defaultSkybox = Cubemap::CreateFromFile(
        "Engine/Textures/Skybox/right.jpg",
        "Engine/Textures/Skybox/left.jpg",
        "Engine/Textures/Skybox/top.jpg",
        "Engine/Textures/Skybox/bottom.jpg",
        "Engine/Textures/Skybox/front.jpg",
        "Engine/Textures/Skybox/back.jpg"
    );
    AssetManager::Get().AddAsset("DefaultSkyboxCubemap", defaultSkybox);

    fullScreenQuad = Mesh::FullScreenQuad();
   
    /*gismoShader = SubShader::CreateFromFile("Engine/Shaders/Gizmos.glsl");
    Assert(gismoShader != nullptr);*/

    gismoMaterial = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Gizmos.glsl"));

    // TODO: Maybe delete this opengl data
    CreateLineVAO(&lineVAO, &lineVBO, 2);
    CreateLineVAO(&lineCommandsVAO, &lineCommandsVBO, MAX_LINES_VERTEX_DRAWCALL*2);
    CreateWiredCubeVAO(wiredCubeVAO, wiredCubeEBO, wiredCubeVBO); 
    CreateTextQuadVAO(textQuadVAO, textQuadVBO);

    GLint maxLayers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
    LogInfo("MaxArrayTextureLayers: {}", maxLayers);

    //#if OPENGL_DEBUG
    GLint maxVertexUniformComponents;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
    LogInfo("MaxVertexUniformComponents: {}", maxVertexUniformComponents);
    //glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformComponents);
    //LogInfo("MaxVertexUniformComponentVectors: %d", maxVertexUniformComponents);

    GLint maxFragmentUniformComponents;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniformComponents);
    LogInfo("MaxFragmentUniformComponents: {}", maxFragmentUniformComponents);
    //glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformComponents);
    //LogInfo("MaxFragmentUniformComponentVectors: %d", maxFragmentUniformComponents);
    //#endif

    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    LogInfo("MaxTextureSize: {}", maxTextureSize);

    GLint maxTextureUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    LogInfo("MaxTextureUnits: {}", maxTextureUnits);

    #if UseUniformBuffer
    glGenBuffers(1, &cameraDataBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraDataBuffer);
    glCheckError();

    glGenBuffers(1, &globalDataBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, globalDataBuffer);
    glCheckError();

    glGenBuffers(1, &renderPipelineDataBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, renderPipelineDataBuffer);
    glCheckError();
    
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckError();
    #endif

    _cubeMesh = CreateRef<Mesh>();
    _cubeMesh->vertices = {
        // +X
        {1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, -1.0f},
        // -X
        {-1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, 1.0f},
        // +Y
        {-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f},
        // -Y
        {-1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f},
        // +Z
        {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f},
        // -Z
        {1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}
    };
    _cubeMesh->indices = {
        0, 1, 2, 2, 3, 0,       // +X
        4, 5, 6, 6, 7, 4,       // -X
        8, 9,10,10,11, 8,       // +Y
       12,13,14,14,15,12,       // -Y
       16,17,18,18,19,16,       // +Z
       20,21,22,22,23,20        // -Z
    };
    _cubeMesh->Submit();

    irradianceMat = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/IrradianceConvolution.glsl"));
    prefilterMat = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Prefilter.glsl"));
    brdfMat = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/brdf.glsl"));
    equirectangularToCubemapMat = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/EquirectangularToCubemap.glsl"));

    glGenBuffers(1, &globalMat4VBO);
    glBindBuffer(GL_ARRAY_BUFFER, globalMat4VBO);
    Matrix4 m = Matrix4Identity;
    glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4), &m, GL_STATIC_DRAW); //GL_DYNAMIC_DRAW //GL_STREAM_DRAW
}

void OpenGLGraphicsDevice::Shutdown(){
    fullScreenQuad = nullptr;
    _cubeMesh = nullptr;
    irradianceMat = nullptr;
    prefilterMat = nullptr;
    brdfMat = nullptr;
    equirectangularToCubemapMat = nullptr;

    //#if !defined(__EMSCRIPTEN__)
    ImGui_ImplOpenGL3_Shutdown();
    //#endif
}

void OpenGLGraphicsDevice::Begin(){
    //TODO: Add #ifn FINAL_BUILD on debugData and stats updates

    stats.drawCalls = 0;
    stats.vertices = 0;
    stats.tris = 0;
    stats.shaderBinds = 0;
    stats.uniformSet = 0;
    stats.uniformBufferUpdates = 0;
    stats.materialSubmitDatas = 0;
    begin = true;
    lastMat = nullptr;
    lastShader = nullptr;

    curPerInstancingDrawData = 0;

    debugData.datas.clear();

    //TODO: destory if(mat.glData.mainBuffer != 0) glDeleteBuffers(1, &mat.glData.mainBuffer); on Shutdown

    auto CalcPerInstancingDrawDataSize = [&](){
        size_t totalVRAM = 0;
        for (const auto& drawData : perInstancingDrawData){
            totalVRAM += drawData.capacity; // capacity is in bytes
        }

        return totalVRAM;
    };

    glCheckError();

    //LogInfo("CaPerInstancingDrawDataSize: {}MB", CalcPerInstancingDrawDataSize() / (1024*1024));
}

void OpenGLGraphicsDevice::End(){
    begin = false;
    glCheckError();
}

void OpenGLGraphicsDevice::_Begin(){
    return;
    #ifndef USE_VAO
    glBindVertexArray(globalVAO);
    glCheckError();
    #endif
    glCheckError();
}

void OpenGLGraphicsDevice::_End(){
    return;
    #ifndef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
    glCheckError();
}

bool OpenGLGraphicsDevice::HasBegin(){
    return begin;
}

void OpenGLGraphicsDevice::BeginRenderToScreen(Vector4 clearColor){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Clean(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glCheckError();
    debugData.BeginPass(nullptr);
}

void OpenGLGraphicsDevice::EndRenderToScreen(){
    //Application::DrawImGui();
    glCheckError();
    debugData.EndPass();
}

void OpenGLGraphicsDevice::Clean(float r, float g, float b, float a){
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glCheckError();
}

void OpenGLGraphicsDevice::CleanColorOnly(float r, float g, float b, float a){
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT); 
    glCheckError();
}

void OpenGLGraphicsDevice::CleanDepthOnly(){
    glClear(GL_DEPTH_BUFFER_BIT); 
    glCheckError();
}

void OpenGLGraphicsDevice::SetCamera(Camera& inCamera){
    camera = inCamera;

    #if UseUniformBuffer
    struct CameraData{
        Matrix4 projection;
        Matrix4 view;
        Matrix4 invProjection;
        Matrix4 inView;
    };
    CameraData data = {camera.projection, camera.view, math::inverse(camera.projection), math::inverse(camera.view)};

    glBindBuffer(GL_UNIFORM_BUFFER, cameraDataBuffer);
    glCheckError();
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &data, GL_STATIC_DRAW); //GL_DYNAMIC_DRAW
    glCheckError();
    stats.uniformBufferUpdates += 1;
    #endif
}

Camera OpenGLGraphicsDevice::GetCamera(){
    return camera;
}

void OpenGLGraphicsDevice::SetColorMask(Vector4 mask){
    glColorMask(mask.r, mask.g, mask.b, mask.a);
    glCheckError();
}   

void OpenGLGraphicsDevice::SetRenderMode(RenderMode mode){
    if(mode == RenderMode::SHADED) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if(mode == RenderMode::WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glCheckError();
}

void OpenGLGraphicsDevice::SetDepthMask(bool value){
    if(value){
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
    glCheckError();
}

void OpenGLGraphicsDevice::SetDepthTest(DepthTest depthTest){
    switch(depthTest){
      case DepthTest::DISABLE:
        glDisable(GL_DEPTH_TEST);
        break;
      case DepthTest::LESS:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);  
        break;
      case DepthTest::LESS_EQUAL:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);  
        break;
      case DepthTest::EQUAL:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_EQUAL);  
        break;
      case DepthTest::GREATER:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GREATER);  
        break;
      case DepthTest::GREATER_EQUAL:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);  
        break;
      case DepthTest::DIFFERENT:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_NOTEQUAL);  
        break;
      case DepthTest::ALWAYS:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);  
        break;
      case DepthTest::NEVER:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_NEVER);  
        break;
    }
    glCheckError();
}

void OpenGLGraphicsDevice::SetCullFace(CullFace cullFace){
    switch(cullFace){
      case CullFace::BACK:
        glEnable(GL_CULL_FACE); 
        glCullFace(GL_BACK);
        break;

      case CullFace::FRONT:
        glEnable(GL_CULL_FACE); 
        glCullFace(GL_FRONT);
        break;

      case CullFace::FRONT_AND_BACK:
        glEnable(GL_CULL_FACE); 
        glCullFace(GL_FRONT_AND_BACK);
        break;

      case CullFace::NONE:
        glDisable(GL_CULL_FACE);
        break;
    }
    glCheckError();
}

void OpenGLGraphicsDevice::SetBlend(bool b){
    if(b){
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    glCheckError();
}

int OpenGLGraphicsDevice::BlendModeToGL(BlendMode blendMode){
    if(blendMode == BlendMode::ZERO) return GL_ZERO;
    if(blendMode == BlendMode::ONE) return GL_ONE;
    if(blendMode == BlendMode::SRC_COLOR) return GL_SRC_COLOR;
    if(blendMode == BlendMode::ONE_MINUS_SRC_COLOR) return GL_ONE_MINUS_SRC_COLOR;
    if(blendMode == BlendMode::DST_COLOR) return GL_DST_COLOR;
    if(blendMode == BlendMode::ONE_MINUS_DST_COLOR) return GL_ONE_MINUS_DST_COLOR;
    if(blendMode == BlendMode::SRC_ALPHA) return GL_SRC_ALPHA;
    if(blendMode == BlendMode::ONE_MINUS_SRC_ALPHA) return GL_ONE_MINUS_SRC_ALPHA;
    if(blendMode == BlendMode::DST_ALPHA) return GL_DST_ALPHA;
    if(blendMode == BlendMode::ONE_MINUS_DST_ALPHA) return GL_ONE_MINUS_DST_ALPHA;
    if(blendMode == BlendMode::CONSTANT_COLOR) return GL_CONSTANT_COLOR;
    if(blendMode == BlendMode::ONE_MINUS_CONSTANT_COLOR) return GL_ONE_MINUS_CONSTANT_COLOR;
    if(blendMode == BlendMode::CONSTANT_ALPHA) return GL_CONSTANT_ALPHA;
    if(blendMode == BlendMode::ONE_MINUS_CONSTANT_ALPHA) return GL_ONE_MINUS_CONSTANT_ALPHA;

    glCheckError();
    Assert(false);
    return 0;
}

void OpenGLGraphicsDevice::SetBlendFunc(BlendMode sfactor, BlendMode dfactor){
    glBlendFunc(BlendModeToGL(sfactor), BlendModeToGL(dfactor));
    glCheckError();
}

int OpenGLGraphicsDevice::SubShaderGetLocation(SubShader& shader, const char* name){
    //return shader.glData.uniforms[name];
    if(shader.glData.uniforms.count(name)) return shader.glData.uniforms[name];
    
    GLint location = glGetUniformLocation(shader.glData.id, name);
    glCheckError();
    
    shader.glData.uniforms[name] = location;
    return location;
}

void OpenGLGraphicsDevice::SubShaderSetFloat(SubShader& shader, const char* name, float value){
    Graphics::GetStats().uniformSet += 1;
    debugData.UniformSet(name);
    glUniform1f(SubShaderGetLocation(shader, name), value);
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetFloat(SubShader& shader, const char* name, float* value, int count){
    stats.uniformSet += 1;
    debugData.UniformSet(name);
    glUniform1fv(SubShaderGetLocation(shader, name), (GLsizei)count, (GLfloat*)value);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetInt(SubShader& shader, const char* name, int value){
    stats.uniformSet += 1;
    glUniform1i(SubShaderGetLocation(shader, name), value);
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetVector2(SubShader& shader, const char* name, Vector2 value){
    Graphics::GetStats().uniformSet += 1;
    glUniform2f(SubShaderGetLocation(shader, name), value.x, value.y);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetVector3(SubShader& shader, const char* name, Vector3 value){
    Graphics::GetStats().uniformSet += 1;
    debugData.UniformSet(name);
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform3f(SubShaderGetLocation(shader, name), value.x, value.y, value.z);
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetVector4(SubShader& shader, const char* name, Vector4 value){
    Graphics::GetStats().uniformSet += 1;
    debugData.UniformSet(name);
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform4f(SubShaderGetLocation(shader, name), value.x, value.y, value.z, value.w);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetVector4(SubShader& shader, const char* name, Vector4* value, int count){
    Graphics::GetStats().uniformSet += 1;
    debugData.UniformSet(name);
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform4fv(SubShaderGetLocation(shader, name), (GLsizei)count, (GLfloat*)value);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetMatrix4(SubShader& shader, const char* name, Matrix4 value){
    Graphics::GetStats().uniformSet += 1;
    debugData.UniformSet(name);
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniformMatrix4fv(SubShaderGetLocation(shader, name), 1, GL_FALSE, glm::value_ptr(static_cast<glm::mat4>(value)));
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetMatrix4(SubShader& shader, const char* name, std::vector<Matrix4>& value){
    Graphics::GetStats().uniformSet += 1;
    debugData.UniformSet(name);
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniformMatrix4fv(SubShaderGetLocation(shader, name), (GLsizei)value.size(), GL_FALSE, glm::value_ptr(value[0]));
    //Set(GetLocation(name), &value[0], (unsigned int)value.size());
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s Cout: %zd", name, path.c_str(), value.size()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetMatrix4(SubShader& shader, const char* name, Matrix4* value, int count){
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    Assert(shader.glData.id == currentProgram);

    Graphics::GetStats().uniformSet += 1;
    debugData.UniformSet(name);
    //if(curBindShaderRenderId != rendererId) Bind(*this);
    glUniformMatrix4fv(SubShaderGetLocation(shader, name), (GLsizei)count, GL_FALSE, (GLfloat*)value);
    glCheckError();
}

void OpenGLGraphicsDevice::Texture2DBind(Texture2D& tex, int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, tex.glData.id);
    glCheckError();
}

void OpenGLGraphicsDevice::Texture2DArrayBind(Texture2DArray& tex, int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.glData.id);
    glCheckError();
}

void OpenGLGraphicsDevice::CubemapBind(Cubemap& cubemap, int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.glData.id);
    glCheckError();
}

void OpenGLGraphicsDevice::UniformBufferBind(UniformBuffer& buffer, int index){
    glBindBuffer(GL_UNIFORM_BUFFER, buffer.glData.id);
    glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.glData.id);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetTexture2D(SubShader& shader, const char* name, Texture2D& value, int index){
    Graphics::GetStats().uniformSet += 1;
    debugData.UniformSet(name);
    //glActiveTexture(GL_TEXTURE0 + index); glCheckError();
    Texture2DBind(value, index);
    SubShaderSetInt(shader, name, index);
}

void OpenGLGraphicsDevice::SubShaderSetTexture2DArray(SubShader& shader, const char* name, Texture2DArray& value, int index){
    Graphics::GetStats().uniformSet += 1;
    //glActiveTexture(GL_TEXTURE0 + index); glCheckError();
    Texture2DArrayBind(value, index);
    SubShaderSetInt(shader, name, index);
}

void OpenGLGraphicsDevice::SubShaderSetCubemap(SubShader& shader, const char* name, Cubemap& value, int index){
    Graphics::GetStats().uniformSet += 1;
    //glActiveTexture(GL_TEXTURE0 + index); glCheckError();
    CubemapBind(value, index);
    SubShaderSetInt(shader, name, index);
}

void OpenGLGraphicsDevice::SubShaderSetFramebuffer(SubShader& shader, const char* name, Framebuffer& framebuffer, int index, int colorAttachmentIndex){
    stats.uniformSet += 1;
    debugData.UniformSet(name);

    Assert(framebuffer.Specification().type != FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE);
    //Assert(framebuffer.specification().sample <= 1);

    glActiveTexture(GL_TEXTURE0 + index);
    glCheckError();

    unsigned int target = GL_TEXTURE_2D;
    if(framebuffer.Specification().type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
        target = GL_TEXTURE_2D_ARRAY;
    }
    if(framebuffer.Specification().type == FramebufferAttachmentType::CUBEMAP){
        target = GL_TEXTURE_CUBE_MAP;
    }

    if(colorAttachmentIndex == -1){
        //glBindTexture(target, framebuffer.DepthAttachmentId());
        glBindTexture(target, framebuffer.glData.depthAttachment);
        glCheckError();
    } else {
        Assert(framebuffer.glData.colorAttachments.size() > colorAttachmentIndex);
        //glBindTexture(target, framebuffer.ColorAttachmentId(colorAttachmentIndex));
        glBindTexture(target, framebuffer.glData.colorAttachments[colorAttachmentIndex]);
        glCheckError();
    }

    //glCheckError();
    SubShaderSetInt(shader, name, index);
}

bool OpenGLGraphicsDevice::SubShaderSetUniformBuffer(SubShader& shader, const char* name, UniformBuffer& buffer, int index){
    Graphics::GetStats().uniformSet += 1;
    UniformBufferBind(buffer, index);

    GLuint blockIndex = glGetUniformBlockIndex(shader.glData.id, name);
    if(blockIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(shader.glData.id, blockIndex, index);
        glCheckError();
        return true; 
    } 

    return false;
}

/*void SubShaderSetUniforBuffer(GLSubShaderData& shader, const char* name, UniformBuffer& buffer, int index){
    Graphics::GetStats().uniformSet += 1;
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    UniformBuffer::Bind(buffer, index);
    unsigned int bufferIndex = glGetUniformBlockIndex(shader.id, name);   
    glUniformBlockBinding(shader.id, bufferIndex, index);
    glCheckError();
}*/

/*void OpenGLGraphicsDevice::ApplyUniformTo(Material& material, SubShader& shader, std::unordered_map<std::string, MaterialMap>& maps){
    auto ContainUniformName = [&](SubShader shader, const std::string& name){ 
        return std::find(shader.glData._uniforms.begin(), shader.glData._uniforms.end(), name) != shader.glData._uniforms.end(); 
    };

    for(const auto& i: maps){
        const MaterialMap& map = i.second;

        #if UseUniformBuffer
        if(material.glData.mainUniformData != nullptr && material.glData.mainBufferDef.members.count(i.first)){
            UniformBufferDef::Member m = material.glData.mainBufferDef.members[i.first];

            if(map.type == MaterialMap::Type::Int){
                Assert(m.size >= sizeof(int));
                memcpy((char*)material.glData.mainUniformData + m.pos, &map.valueInt, sizeof(int));
            } else if(map.type == MaterialMap::Type::Float){
                Assert(m.size >= sizeof(float));
                memcpy((char*)material.glData.mainUniformData + m.pos, &map.valueFloat, sizeof(float));
            } else if(map.type == MaterialMap::Type::Vector2){
                Assert(m.size >= sizeof(Vector2));
                memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector2));
            } else if(map.type == MaterialMap::Type::Vector3){
                Assert(m.size >= sizeof(Vector3));
                memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector3));
            } else if(map.type == MaterialMap::Type::Vector4){
                Assert(m.size >= sizeof(Vector4));
                memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector4));
            } else if(map.type == MaterialMap::Type::Matrix4){
                Assert(m.size >= sizeof(Matrix4));
                memcpy((char*)material.glData.mainUniformData + m.pos, &map.matrix, sizeof(Matrix4));
            } else if(map.type == MaterialMap::Type::FloatList){
                Assert(map.list != nullptr);
                Assert(map.listCount > 0);

                //Assert(m.size >= sizeof(float) * map.listCount);
                //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<float*>(map.list), sizeof(float) * map.listCount);
                int stride = m.arrayStride > 0 ? m.arrayStride : 16; // fallback seguro
                char* base = (char*)material.glData.mainUniformData + m.pos;
                float* src = static_cast<float*>(map.list);
                for(int j = 0; j < map.listCount; ++j){
                    memcpy(base + j * stride, &src[j], sizeof(float));
                }
            } else if(map.type == MaterialMap::Type::Vector4List){
                Assert(map.list != nullptr);
                Assert(map.listCount > 0);

                //Assert(m.size >= sizeof(Vector4) * map.listCount);
                //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Vector4*>(map.list), sizeof(Vector4) * map.listCount);
                int stride = m.arrayStride > 0 ? m.arrayStride : sizeof(Vector4);
                char* base = (char*)material.glData.mainUniformData + m.pos;
                Vector4* src = static_cast<Vector4*>(map.list);
                for(int j = 0; j < map.listCount; ++j){
                    memcpy(base + j * stride, &src[j], sizeof(Vector4));
                }
            } else if(map.type == MaterialMap::Type::Matrix4List){
                Assert(map.list != nullptr);
                Assert(map.listCount > 0);
                
                //Assert(m.size >= sizeof(Matrix4) * map.listCount);
                //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Matrix4*>(map.list), sizeof(Matrix4) * map.listCount);
                int stride = m.arrayStride > 0 ? m.arrayStride : sizeof(Matrix4); // normalmente 64
                char* base = (char*)material.glData.mainUniformData + m.pos;
                Matrix4* src = static_cast<Matrix4*>(map.list);
                for(int j = 0; j < map.listCount; ++j){
                    memcpy(base + j * stride, &src[j], sizeof(Matrix4));
                }
            } else {
                Assert(false && "Type Not Supported in A UnifomBuffer");
            }
            continue;
        }
        #endif
        
        if(ContainUniformName(shader, i.first) == false) continue;

        if(map.type == MaterialMap::Type::Int){
            SubShaderSetInt(shader, i.first.c_str(), map.valueInt);
        }
        if(map.type == MaterialMap::Type::Float){
            SubShaderSetFloat(shader, i.first.c_str(), map.valueFloat);
        }
        if(map.type == MaterialMap::Type::Vector2){
            SubShaderSetVector2(shader, i.first.c_str(), Vector2(map.vec.vector.x, map.vec.vector.y));
        }
        if(map.type == MaterialMap::Type::Vector3){
            SubShaderSetVector3(shader, i.first.c_str(), Vector3(map.vec.vector.x, map.vec.vector.y, map.vec.vector.z));
        }
        if(map.type == MaterialMap::Type::Vector4){
            SubShaderSetVector4(shader, i.first.c_str(), map.vec.vector);
        }
        if(map.type == MaterialMap::Type::Matrix4){
            SubShaderSetMatrix4(shader, i.first.c_str(), i.second.matrix);
        }
        if(map.type == MaterialMap::Type::Texture){
            if(i.second.texture == nullptr) continue;
            Assert(i.second.texture != nullptr);
            SubShaderSetTexture2D(shader, i.first.c_str(), *i.second.texture, material.currentTextureSlot);
            material.currentTextureSlot += 1;
        }
        if(map.type == MaterialMap::Type::TextureArray){
            SubShaderSetTexture2DArray(shader, i.first.c_str(), *i.second.textureArray, material.currentTextureSlot);
            material.currentTextureSlot += 1;
        }
        if(map.type == MaterialMap::Type::Framebuffer){
            SubShaderSetFramebuffer(shader, i.first.c_str(), *i.second.framebuffer, material.currentTextureSlot, map.framebufferAttachment);
            material.currentTextureSlot += 1;
        }
        if(map.type == MaterialMap::Type::Cubemap){
            SubShaderSetCubemap(shader, i.first.c_str(), *i.second.cubemap, material.currentTextureSlot);
            material.currentTextureSlot += 1;
        }
        if(map.type == MaterialMap::Type::FloatList){
            SubShaderSetFloat(shader, i.first.c_str(), static_cast<float*>(map.list), map.listCount);
        }
        if(map.type == MaterialMap::Type::Vector4List){
            SubShaderSetVector4(shader, i.first.c_str(), static_cast<Vector4*>(map.list), map.listCount);
        }
        if(map.type == MaterialMap::Type::Matrix4List){
            SubShaderSetMatrix4(shader, i.first.c_str(), static_cast<Matrix4*>(map.list), map.listCount);
        }
    }
}
*/
#if 0
void OpenGLGraphicsDevice::BindMaterial(Material& mat, int drawType){
    Assert(drawType >= 0 && drawType <= 3);
    Assert(mat.currentShader.drawTypes[drawType] != nullptr);

    auto ContainUniformName = [&](SubShader& shader, const std::string& name){ 
        return std::find(shader.glData._uniforms.begin(), shader.glData._uniforms.end(), name) != shader.glData._uniforms.end(); 
    };

    auto ApplyUniformTo = [&](Material& material, SubShader& shader, const std::unordered_map<std::string, MaterialMap>& maps){
        for(const auto& i: maps){
            const MaterialMap& map = i.second;

            #if UseUniformBuffer
            if(material.glData.mainUniformData != nullptr && material.glData.mainBufferDef.members.count(i.first)){
                const UniformBufferDef::Member m = material.glData.mainBufferDef.members[i.first];

                if(map.type == MaterialMap::Type::Int){
                    Assert(m.size >= sizeof(int));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.valueInt, sizeof(int));
                } else if(map.type == MaterialMap::Type::Float){
                    Assert(m.size >= sizeof(float));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.valueFloat, sizeof(float));
                } else if(map.type == MaterialMap::Type::Vector2){
                    //#ifdef GLM_FORCE_ALIGNED
                        Assert(m.size >= (sizeof(float) * 2));
                        memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector.x, sizeof(float) * 2);
                    /*#else
                        Assert(m.size >= sizeof(Vector2));
                        memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector2));
                    #endif*/
                } else if(map.type == MaterialMap::Type::Vector3){
                    //#ifdef GLM_FORCE_ALIGNED
                        Assert(m.size >= (sizeof(float) * 3));
                        memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector.x, sizeof(float) * 3);
                    /*#else
                        Assert(m.size >= sizeof(Vector3));
                        memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector3));
                    #endif*/
                } else if(map.type == MaterialMap::Type::Vector4){
                    Assert(m.size >= sizeof(Vector4));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector4));
                } else if(map.type == MaterialMap::Type::Matrix4){
                    Assert(m.size >= sizeof(Matrix4));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.matrix, sizeof(Matrix4));
                } else if(map.type == MaterialMap::Type::FloatList){
                    Assert(map.list != nullptr);
                    Assert(map.listCount > 0);

                    //Assert(m.size >= sizeof(float) * map.listCount);
                    //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<float*>(map.list), sizeof(float) * map.listCount);
                    int stride = m.arrayStride > 0 ? m.arrayStride : 16; // fallback seguro
                    char* base = (char*)material.glData.mainUniformData + m.pos;
                    float* src = static_cast<float*>(map.list);
                    for(int j = 0; j < map.listCount; ++j){
                        memcpy(base + j * stride, &src[j], sizeof(float));
                    }
                } else if(map.type == MaterialMap::Type::Vector4List){
                    Assert(map.list != nullptr);
                    Assert(map.listCount > 0);

                    //Assert(m.size >= sizeof(Vector4) * map.listCount);
                    //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Vector4*>(map.list), sizeof(Vector4) * map.listCount);
                    int stride = m.arrayStride > 0 ? m.arrayStride : sizeof(Vector4);
                    char* base = (char*)material.glData.mainUniformData + m.pos;
                    Vector4* src = static_cast<Vector4*>(map.list);
                    for(int j = 0; j < map.listCount; ++j){
                        memcpy(base + j * stride, &src[j], sizeof(Vector4));
                    }
                } else if(map.type == MaterialMap::Type::Matrix4List){
                    Assert(map.list != nullptr);
                    Assert(map.listCount > 0);
                    
                    //Assert(m.size >= sizeof(Matrix4) * map.listCount);
                    //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Matrix4*>(map.list), sizeof(Matrix4) * map.listCount);
                    int stride = m.arrayStride > 0 ? m.arrayStride : sizeof(Matrix4); // normalmente 64
                    char* base = (char*)material.glData.mainUniformData + m.pos;
                    Matrix4* src = static_cast<Matrix4*>(map.list);
                    for(int j = 0; j < map.listCount; ++j){
                        memcpy(base + j * stride, &src[j], sizeof(Matrix4));
                    }
                } else {
                    Assert(false && "Type Not Supported in A UnifomBuffer");
                }
                continue;
            }
            #endif
            
            if(ContainUniformName(shader, i.first) == false) continue;

            if(map.type == MaterialMap::Type::Int){
                SubShaderSetInt(shader, i.first.c_str(), map.valueInt);
            }
            if(map.type == MaterialMap::Type::Float){
                SubShaderSetFloat(shader, i.first.c_str(), map.valueFloat);
            }
            if(map.type == MaterialMap::Type::Vector2){
                SubShaderSetVector2(shader, i.first.c_str(), Vector2(map.vec.vector.x, map.vec.vector.y));
            }
            if(map.type == MaterialMap::Type::Vector3){
                SubShaderSetVector3(shader, i.first.c_str(), Vector3(map.vec.vector.x, map.vec.vector.y, map.vec.vector.z));
            }
            if(map.type == MaterialMap::Type::Vector4){
                SubShaderSetVector4(shader, i.first.c_str(), map.vec.vector);
            }
            if(map.type == MaterialMap::Type::Matrix4){
                SubShaderSetMatrix4(shader, i.first.c_str(), i.second.matrix);
            }
            if(map.type == MaterialMap::Type::Buffer){
                if(i.second.buffer == nullptr) continue;
                Assert(i.second.buffer != nullptr);
                SubShaderSetUniformBuffer(shader, i.first.c_str(), *i.second.buffer, material.currentBufferSlot);
                material.currentBufferSlot += 1;
            }
            if(map.type == MaterialMap::Type::Texture){
                if(i.second.texture == nullptr) continue;//TODO: Add a default texure by type if is null
                Assert(i.second.texture != nullptr);
                SubShaderSetTexture2D(shader, i.first.c_str(), *i.second.texture, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::TextureArray){
                SubShaderSetTexture2DArray(shader, i.first.c_str(), *i.second.textureArray, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Framebuffer){
                SubShaderSetFramebuffer(shader, i.first.c_str(), *i.second.framebuffer, material.currentTextureSlot, map.framebufferAttachment);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Cubemap){
                SubShaderSetCubemap(shader, i.first.c_str(), *i.second.cubemap, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::FloatList){
                SubShaderSetFloat(shader, i.first.c_str(), static_cast<float*>(map.list), map.listCount);
            }
            if(map.type == MaterialMap::Type::Vector4List){
                SubShaderSetVector4(shader, i.first.c_str(), static_cast<Vector4*>(map.list), map.listCount);
            }
            if(map.type == MaterialMap::Type::Matrix4List){
                SubShaderSetMatrix4(shader, i.first.c_str(), static_cast<Matrix4*>(map.list), map.listCount);
            }
        }
    };

    auto SubmitGraphicDatas = [&](Material& material){
        stats.materialSubmitDatas += 1;
        debugData.BindMaterial(&material);
        material.currentTextureSlot = 0;
        material.currentBufferSlot = 0;
        material.UpdateCurrentShader();

        Assert(material.GetShader() != nullptr);
        if(material.GetShader() == nullptr) return;

        SubShaderBind(*material.currentShader.drawTypes[drawType]); //TODO: Optmize thi by bind and ApplyUniformTo global of lastShader, and add material.currentTextureSlot by subshader instead of material 
        ApplyUniformTo(material, *material.currentShader.drawTypes[drawType], material.maps);
        ApplyUniformTo(material, *material.currentShader.drawTypes[drawType], Material::globalMaps);
        Assert(material.currentTextureSlot < 32);
    };

    Assert(mat.currentShader.drawTypes[drawType] != nullptr && "Shader is not vali!");
    Assert(mat.GetShader()->IsComplete() == true && "Shader is not vali!");

    if(&mat != lastMat || mat.isDirty == true || mat.currentShader.drawTypes[drawType].get() != lastShader){
        SubmitGraphicDatas(mat);
        mat.isDirty = false;
        #if UseUniformBuffer
        if(mat.glData.mainBuffer != 0){
            glBindBuffer(GL_UNIFORM_BUFFER, mat.glData.mainBuffer);
            glCheckError();
            glBufferData(GL_UNIFORM_BUFFER, mat.glData.mainBufferDef.size, mat.glData.mainUniformData, GL_STATIC_DRAW); //GL_DYNAMIC_DRAW
            glCheckError();
        }
        unsigned int index2 = glGetUniformBlockIndex(mat.currentShader.drawTypes[drawType]->glData.id, "Main");  
        if(index2 != GL_INVALID_INDEX){
            glBindBuffer(GL_UNIFORM_BUFFER, mat.glData.mainBuffer);
            glBindBufferBase(GL_UNIFORM_BUFFER, mat.currentBufferSlot, mat.glData.mainBuffer);
            glCheckError(); 
            glUniformBlockBinding(mat.currentShader.drawTypes[drawType]->glData.id, index2, mat.currentBufferSlot); // 1);
            mat.currentBufferSlot += 1;
            glCheckError(); 
        }  
        #endif
    }
    lastMat = &mat;
    
    if(mat.currentShader.drawTypes[drawType].get() != lastShader){
        #if UseUniformBuffer
        unsigned int index = glGetUniformBlockIndex(mat.currentShader.drawTypes[drawType]->glData.id, "CamDraw");   
        if(index != GL_INVALID_INDEX){
            glBindBuffer(GL_UNIFORM_BUFFER, cameraDataBuffer);
            glBindBufferBase(GL_UNIFORM_BUFFER, mat.currentBufferSlot, cameraDataBuffer);
            glCheckError(); 
            glUniformBlockBinding(mat.currentShader.drawTypes[drawType]->glData.id, index, mat.currentBufferSlot); // 0);
            mat.currentBufferSlot += 1;
            glCheckError(); 
        } else {
            SubShaderSetMatrix4(*mat.currentShader.drawTypes[drawType], "projection", camera.projection); //mat.currentShader->SetMatrix4("projection", camera.projection);
            SubShaderSetMatrix4(*mat.currentShader.drawTypes[drawType], "view", camera.view); //mat.currentShader->SetMatrix4("view", camera.view);

            //SubShaderSetMatrix4(*mat.currentShader.drawTypes[drawType], "invProjection", math::inverse(camera.projection)); //mat.currentShader->SetMatrix4("projection", camera.projection);
            //SubShaderSetMatrix4(*mat.currentShader.drawTypes[drawType], "invView", math::inverse(camera.view)); //mat.currentShader->SetMatrix4("view", camera.view);
        }
        /*unsigned int index2 = glGetUniformBlockIndex(mat.currentShader->glData.id, "Main");  
        if(index2 != GL_INVALID_INDEX){
            glBindBuffer(GL_UNIFORM_BUFFER, mat.glData.mainBuffer);
            glBindBufferBase(GL_UNIFORM_BUFFER, mat.currentBufferSlot, mat.glData.mainBuffer);
            glCheckError(); 
            glUniformBlockBinding(mat.currentShader->glData.id, index2, mat.currentBufferSlot); // 1);
            mat.currentBufferSlot += 1;
            glCheckError(); 
        }*/       
        #else
        //SubShaderBind(*mat.currentShader);
        SubShaderSetMatrix4(*mat.currentShader, "projection", camera.projection); //mat.currentShader->SetMatrix4("projection", camera.projection);
        SubShaderSetMatrix4(*mat.currentShader, "view", camera.view); //mat.currentShader->SetMatrix4("view", camera.view);
        #endif
    }
    lastShader = mat.currentShader.drawTypes[drawType].get();
}  
#else
void OpenGLGraphicsDevice::BindMaterial(Material& mat, int drawType){
    Assert(drawType >= 0 && drawType < (int)Shader::DrawType::Count);
    Assert(mat.currentShader.drawTypes[drawType] != nullptr);

    auto ContainUniformName = [&](SubShader& shader, const std::string& name){ 
        return std::find(shader.glData._uniforms.begin(), shader.glData._uniforms.end(), name) != shader.glData._uniforms.end(); 
    };

    auto ApplyUniformTo = [&](Material& material, SubShader& shader, const std::unordered_map<std::string, MaterialMap>& maps){
        for(const auto& i: maps){
            const MaterialMap& map = i.second;

            #if UseUniformBuffer
            if(material.glData.mainUniformData != nullptr && material.glData.mainBufferDef.members.count(i.first)){
                const UniformBufferDef::Member m = material.glData.mainBufferDef.members[i.first];

                if(map.type == MaterialMap::Type::Int){
                    Assert(m.size >= sizeof(int));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.valueInt, sizeof(int));
                } else if(map.type == MaterialMap::Type::Float){
                    Assert(m.size >= sizeof(float));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.valueFloat, sizeof(float));
                } else if(map.type == MaterialMap::Type::Vector2){
                    //#ifdef GLM_FORCE_ALIGNED
                        Assert(m.size >= (sizeof(float) * 2));
                        memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector.x, sizeof(float) * 2);
                    /*#else
                        Assert(m.size >= sizeof(Vector2));
                        memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector2));
                    #endif*/
                } else if(map.type == MaterialMap::Type::Vector3){
                    //#ifdef GLM_FORCE_ALIGNED
                        Assert(m.size >= (sizeof(float) * 3));
                        memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector.x, sizeof(float) * 3);
                    /*#else
                        Assert(m.size >= sizeof(Vector3));
                        memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector3));
                    #endif*/
                } else if(map.type == MaterialMap::Type::Vector4){
                    Assert(m.size >= sizeof(Vector4));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector4));
                } else if(map.type == MaterialMap::Type::Matrix4){
                    Assert(m.size >= sizeof(Matrix4));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.matrix, sizeof(Matrix4));
                } else if(map.type == MaterialMap::Type::FloatList){
                    Assert(map.list != nullptr);
                    Assert(map.listCount > 0);

                    //Assert(m.size >= sizeof(float) * map.listCount);
                    //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<float*>(map.list), sizeof(float) * map.listCount);
                    int stride = m.arrayStride > 0 ? m.arrayStride : 16; // fallback seguro
                    char* base = (char*)material.glData.mainUniformData + m.pos;
                    float* src = static_cast<float*>(map.list);
                    for(int j = 0; j < map.listCount; ++j){
                        memcpy(base + j * stride, &src[j], sizeof(float));
                    }
                } else if(map.type == MaterialMap::Type::Vector4List){
                    Assert(map.list != nullptr);
                    Assert(map.listCount > 0);

                    //Assert(m.size >= sizeof(Vector4) * map.listCount);
                    //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Vector4*>(map.list), sizeof(Vector4) * map.listCount);
                    int stride = m.arrayStride > 0 ? m.arrayStride : sizeof(Vector4);
                    char* base = (char*)material.glData.mainUniformData + m.pos;
                    Vector4* src = static_cast<Vector4*>(map.list);
                    for(int j = 0; j < map.listCount; ++j){
                        memcpy(base + j * stride, &src[j], sizeof(Vector4));
                    }
                } else if(map.type == MaterialMap::Type::Matrix4List){
                    Assert(map.list != nullptr);
                    Assert(map.listCount > 0);
                    
                    //Assert(m.size >= sizeof(Matrix4) * map.listCount);
                    //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Matrix4*>(map.list), sizeof(Matrix4) * map.listCount);
                    int stride = m.arrayStride > 0 ? m.arrayStride : sizeof(Matrix4); // normalmente 64
                    char* base = (char*)material.glData.mainUniformData + m.pos;
                    Matrix4* src = static_cast<Matrix4*>(map.list);
                    for(int j = 0; j < map.listCount; ++j){
                        memcpy(base + j * stride, &src[j], sizeof(Matrix4));
                    }
                } else {
                    Assert(false && "Type Not Supported in A UnifomBuffer");
                }
                continue;
            }
            #endif
            
            if(ContainUniformName(shader, i.first) == false) continue;

            if(map.type == MaterialMap::Type::Int){
                SubShaderSetInt(shader, i.first.c_str(), map.valueInt);
            }
            if(map.type == MaterialMap::Type::Float){
                SubShaderSetFloat(shader, i.first.c_str(), map.valueFloat);
            }
            if(map.type == MaterialMap::Type::Vector2){
                SubShaderSetVector2(shader, i.first.c_str(), Vector2(map.vec.vector.x, map.vec.vector.y));
            }
            if(map.type == MaterialMap::Type::Vector3){
                SubShaderSetVector3(shader, i.first.c_str(), Vector3(map.vec.vector.x, map.vec.vector.y, map.vec.vector.z));
            }
            if(map.type == MaterialMap::Type::Vector4){
                SubShaderSetVector4(shader, i.first.c_str(), map.vec.vector);
            }
            if(map.type == MaterialMap::Type::Matrix4){
                SubShaderSetMatrix4(shader, i.first.c_str(), i.second.matrix);
            }
            if(map.type == MaterialMap::Type::Buffer){
                if(i.second.buffer == nullptr) continue;
                Assert(i.second.buffer != nullptr);
                SubShaderSetUniformBuffer(shader, i.first.c_str(), *i.second.buffer, material.currentBufferSlot);
                material.currentBufferSlot += 1;
            }
            if(map.type == MaterialMap::Type::Texture){
                if(i.second.texture == nullptr) continue;//TODO: Add a default texure by type if is null
                Assert(i.second.texture != nullptr);
                SubShaderSetTexture2D(shader, i.first.c_str(), *i.second.texture, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::TextureArray){
                SubShaderSetTexture2DArray(shader, i.first.c_str(), *i.second.textureArray, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Framebuffer){
                SubShaderSetFramebuffer(shader, i.first.c_str(), *i.second.framebuffer, material.currentTextureSlot, map.framebufferAttachment);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Cubemap){
                SubShaderSetCubemap(shader, i.first.c_str(), *i.second.cubemap, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::FloatList){
                SubShaderSetFloat(shader, i.first.c_str(), static_cast<float*>(map.list), map.listCount);
            }
            if(map.type == MaterialMap::Type::Vector4List){
                SubShaderSetVector4(shader, i.first.c_str(), static_cast<Vector4*>(map.list), map.listCount);
            }
            if(map.type == MaterialMap::Type::Matrix4List){
                SubShaderSetMatrix4(shader, i.first.c_str(), static_cast<Matrix4*>(map.list), map.listCount);
            }
        }
    };

    auto _PreBind = [&](Material& material){
        material.UpdateCurrentShader();
    };

    auto _BindShader = [&](SubShader& shader){
        SubShaderBind(shader); 

        Assert(mat.currentBufferSlot >= 0 && "currentBufferSlot became negative!");
        Assert(mat.currentBufferSlot < 16 && "currentBufferSlot is too high! Only 4-8 binding points are safe on most drivers");

        #if UseUniformBuffer
        unsigned int index = glGetUniformBlockIndex(shader.glData.id, "CamDraw");   
        if(index != GL_INVALID_INDEX){
            glBindBuffer(GL_UNIFORM_BUFFER, cameraDataBuffer);
            glBindBufferBase(GL_UNIFORM_BUFFER, mat.currentBufferSlot, cameraDataBuffer);
            glCheckError(); 
            //TODO: temp hard code becose some strange(only in some pos and angle) bug on here, fix this later
            glUniformBlockBinding(shader.glData.id, index, 0); //mat.currentBufferSlot); // 0);
            mat.currentBufferSlot += 1;
            glCheckError(); 
        } else {
            SubShaderSetMatrix4(shader, "projection", camera.projection);
            SubShaderSetMatrix4(shader, "view", camera.view);
        }  
        #else
        SubShaderSetMatrix4(shader, "projection", camera.projection); //mat.currentShader->SetMatrix4("projection", camera.projection);
        SubShaderSetMatrix4(shader, "view", camera.view); //mat.currentShader->SetMatrix4("view", camera.view);
        #endif
    };

    auto _BindMaterial = [&](Material& material){
        stats.materialSubmitDatas += 1;
        debugData.BindMaterial(&material);
        material.currentTextureSlot = 0;
        material.currentBufferSlot = 1; //INFO: is 1 becose 0 is used for CamDraw

        ApplyUniformTo(material, *material.currentShader.drawTypes[drawType], material.maps);
        ApplyUniformTo(material, *material.currentShader.drawTypes[drawType], Material::globalMaps);
        Assert(material.currentTextureSlot < 32);

        #if UseUniformBuffer
        if(material.glData.mainBuffer != 0 && material.isDirtyUniformData == true){
            glBindBuffer(GL_UNIFORM_BUFFER, material.glData.mainBuffer);
            glCheckError();
            glBufferData(GL_UNIFORM_BUFFER, material.glData.mainBufferDef.size, material.glData.mainUniformData, GL_STATIC_DRAW); //GL_DYNAMIC_DRAW
            glCheckError();
            stats.uniformBufferUpdates += 1;
        }
        unsigned int index2 = glGetUniformBlockIndex(material.currentShader.drawTypes[drawType]->glData.id, "Main");  
        if(index2 != GL_INVALID_INDEX){
            glBindBuffer(GL_UNIFORM_BUFFER, mat.glData.mainBuffer);
            glBindBufferBase(GL_UNIFORM_BUFFER, mat.currentBufferSlot, mat.glData.mainBuffer);
            glCheckError(); 
            glUniformBlockBinding(mat.currentShader.drawTypes[drawType]->glData.id, index2, mat.currentBufferSlot); // 1);
            mat.currentBufferSlot += 1;
            glCheckError(); 
        }  

        #endif
    };

    Assert(mat.GetShader() != nullptr);
    if(mat.GetShader() == nullptr) return;

    Assert(mat.currentShader.drawTypes[drawType] != nullptr && "Shader is not vali!");
    Assert(mat.GetShader()->IsComplete() == true && "Shader is not vali!");

    if(&mat != lastMat || mat.isDirty == true) _PreBind(mat);
    if(mat.currentShader.drawTypes[drawType].get() != lastShader) _BindShader(*mat.currentShader.drawTypes[drawType]);
    if(&mat != lastMat || mat.isDirty == true || mat.isDirtyUniformData == true) _BindMaterial(mat);

    mat.isDirtyUniformData = false;
    mat.isDirty = false;

    lastMat = &mat;
    lastShader = mat.currentShader.drawTypes[drawType].get();
}  
#endif

void OpenGLGraphicsDevice::SendPerDrawData(PerDrawData& perDrawData){
    /*if(perDrawData.int_0_Count > 0) SubShaderSetInt(*lastShader, "perDrawInt_0", perDrawData.int_0[0]);
    if(perDrawData.int_0_Count > 1) SubShaderSetInt(*lastShader, "perDrawInt_1", perDrawData.int_0[1]);
    if(perDrawData.vector4_0_Count > 0) SubShaderSetVector4(*lastShader, "perDrawVector4_0", perDrawData.vector4_0[0]);
    if(perDrawData.vector4_0_Count > 1) SubShaderSetVector4(*lastShader, "perDrawVector4_1", perDrawData.vector4_0[1]);*/

    if(perDrawData.Int_0_HasMask(0)) SubShaderSetInt(*lastShader, "perDrawInt_0", perDrawData.int_0[0]);
    if(perDrawData.Int_0_HasMask(1)) SubShaderSetInt(*lastShader, "perDrawInt_1", perDrawData.int_0[1]);

    if(perDrawData.Vector4_0_HasMask(0)) SubShaderSetVector4(*lastShader, "perDrawVector4_0", perDrawData.vector4_0[0]);
    if(perDrawData.Vector4_0_HasMask(1)) SubShaderSetVector4(*lastShader, "perDrawVector4_1", perDrawData.vector4_0[1]);
}

bool OpenGLGraphicsDevice::InstancingBufferCreate(InstancingBuffer& buffer){
    glGenBuffers(1, &buffer.glData.id);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.glData.id);
    glCheckError();

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckError();
    return true;
}

void OpenGLGraphicsDevice::InstancingBufferDestroy(InstancingBuffer& buffer){
    if(buffer.glData.id != 0) glDeleteBuffers(1, &buffer.glData.id);
    buffer.glData.id = 0;
    glCheckError();
}

bool OpenGLGraphicsDevice::InstancingBufferIsValid(InstancingBuffer& buffer){
    return buffer.glData.id != 0;
}

void OpenGLGraphicsDevice::InstancingBufferSetData(InstancingBuffer& buffer, const Matrix4* data, unsigned int count){
    buffer.isMatrix4x3 = false;
    Assert(InstancingBufferIsValid(buffer) == true);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.glData.id);
    glCheckError();
    glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * count, data, GL_STATIC_DRAW); //GL_STREAM_DRAW
    glCheckError();
}

void OpenGLGraphicsDevice::InstancingBufferSetData(InstancingBuffer& buffer, const Matrix4x3* data, unsigned int count){
    buffer.isMatrix4x3 = true;
    Assert(InstancingBufferIsValid(buffer) == true);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.glData.id);
    glCheckError();
    glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4x3) * count, data, GL_STATIC_DRAW); //GL_STREAM_DRAW
    glCheckError();
}

void OpenGLGraphicsDevice::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix, PerDrawData* perDrawData = nullptr){
    if(mat.currentShader.drawTypes[0] == nullptr) return;
    BindMaterial(mat);
    
    if(perDrawData != nullptr) SendPerDrawData(*perDrawData);

    if(MeshIsValid(mesh) == false){
        #ifdef GRAPHIC_LOG_ERROR
        LogError("DrawMesh::InvalidMesh");
        #endif
        return;
    }

    Assert(MeshIsValid(mesh) && "Mesh is not vali!");

    SubShaderSetMatrix4(*lastShader, "model", modelMatrix);
    
    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount;
    stats.tris += mesh.indiceCount / 3;
    debugData.DrawMesh(&mesh);
    
    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #else
    MeshBind(mesh); //mesh.Bind();
    #endif

    Assert(meshDrawModeLookup[(int)mesh.drawMode] != INVALID_DRAW_MODE && "Dont support the current mesh.drawMode!");

    if(mesh.glData.ebo != 0){
        OnDrawAssetsTest();
        glDrawElements(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0);
        glCheckError();
    } else {
        OnDrawAssetsTest();
        glDrawArrays(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount);
        glCheckError();
    }
}

void OpenGLGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 modelMatrix, Matrix4* animMatrixs, int count, PerDrawData* perDrawData = nullptr){
    //LogInfo("DrawMeshSkinned1");
    if(mat.currentShader.drawTypes[1] == nullptr) return;
    BindMaterial(mat, 1);

    Assert(count <= MAX_BONES);
    
    if(perDrawData != nullptr) SendPerDrawData(*perDrawData);

    if(MeshIsValid(mesh) == false){
        #ifdef GRAPHIC_LOG_ERROR
        LogError("DrawMesh::InvalidMesh");
        #endif
        return;
    }

    Assert(MeshIsValid(mesh) && "Mesh is not vali!");

    //Matrix4 temp[120] = {};
    //memcpy(temp, animMatrixs, count * sizeof(Matrix4));
    //SubShaderSetMatrix4(*lastShader, "animated", temp, 120);

    SubShaderSetMatrix4(*lastShader, "animated", animMatrixs, count);
    SubShaderSetMatrix4(*lastShader, "model", modelMatrix);
    
    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount;
    stats.tris += mesh.indiceCount / 3;
    debugData.DrawMeshSkinned(&mesh);
    
    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #else
    MeshBind(mesh); //mesh.Bind();
    #endif

    Assert(meshDrawModeLookup[(int)mesh.drawMode] != GL_NONE && "Dont support the current mesh.drawMode!");

    if(mesh.glData.ebo != 0){
        OnDrawAssetsTest();
        glDrawElements(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0);
        glCheckError();
    } else {
        OnDrawAssetsTest();
        glDrawArrays(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount);
        glCheckError();
    }
}

void OpenGLGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, UniformBuffer* data, int count, PerDrawData* perDrawData){
    //LogInfo("DrawMeshSkinned2");
    if(mat.currentShader.drawTypes[(int)Shader::DrawType::SkinnedDraw2] == nullptr) return;
    BindMaterial(mat, (int)Shader::DrawType::SkinnedDraw2);
    
    Assert(count <= MAX_BONES);
    
    if(perDrawData != nullptr) SendPerDrawData(*perDrawData);

    if(MeshIsValid(mesh) == false){
        #ifdef GRAPHIC_LOG_ERROR
        LogError("DrawMesh::InvalidMesh");
        #endif
        return;
    }

    Assert(MeshIsValid(mesh) && "Mesh is not vali!");
    Assert(data != nullptr);
    
    //SubShaderSetMatrix4(*lastShader, "animated", animMatrixs, count);
    SubShaderSetMatrix4(*lastShader, "model", model);
    bool r = SubShaderSetUniformBuffer(*lastShader, "PerDrawData", *data, 10); //TODO: Change later this hard code slot index
    //Assert(r == true);
    
    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount;
    stats.tris += mesh.indiceCount / 3;
    debugData.DrawMeshSkinned(&mesh);
    
    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #else
    MeshBind(mesh); //mesh.Bind();
    #endif

    Assert(meshDrawModeLookup[(int)mesh.drawMode] != GL_NONE && "Dont support the current mesh.drawMode!");

    if(mesh.glData.ebo != 0){
        OnDrawAssetsTest();
        glDrawElements(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0);
        glCheckError();
    } else {
        OnDrawAssetsTest();
        glDrawArrays(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount);
        glCheckError();
    }

}

void OpenGLGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* modelMatrixs, int count){
    Assert(count > 0);
    if(mat.currentShader.drawTypes[(int)Shader::DrawType::InstancingDraw] == nullptr) return;
    BindMaterial(mat, (int)Shader::DrawType::InstancingDraw);
    
    Assert(MeshIsValid(mesh) && "Mesh is not vali!");

    mesh.SubmitInstancingCustomModelMatrixs(modelMatrixs, count);

    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount * count;
    stats.tris += (mesh.indiceCount * count) / 3;
    debugData.DrawMeshInstancing(&mesh);

    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    #else
    MeshBind(mesh); //mesh.Bind();
    #endif

    Assert(meshDrawModeLookup[(int)mesh.drawMode] != GL_NONE && "Dont support the current mesh.drawMode!");

    if(mesh.glData.ebo != 0){
        OnDrawAssetsTest();
        glDrawElementsInstanced(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0, count);
        glCheckError();
    } else {
        OnDrawAssetsTest();
        glDrawArraysInstanced(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount, count);
        glCheckError();
    }

    //glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4x3* modelMatrixs, int count){
    if(mat.currentShader.drawTypes[(int)Shader::DrawType::InstancingDraw43] == nullptr) return;
    BindMaterial(mat, (int)Shader::DrawType::InstancingDraw43);
    
    Assert(MeshIsValid(mesh) && "Mesh is not vali!");

    MeshSubmitInstancingCustomModelMatrixs(mesh, modelMatrixs, count);

    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount * count;
    stats.tris += (mesh.indiceCount * count) / 3;
    debugData.DrawMeshInstancing(&mesh);

    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    #else
    MeshBind(mesh); //mesh.Bind();
    #endif

    Assert(meshDrawModeLookup[(int)mesh.drawMode] != GL_NONE && "Dont support the current mesh.drawMode!");

    if(mesh.glData.ebo != 0){
        OnDrawAssetsTest();
        glDrawElementsInstanced(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0, count);
        glCheckError();
    } else {
        OnDrawAssetsTest();
        glDrawArraysInstanced(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount, count);
        glCheckError();
    }

    //glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Material& mat, InstancingBuffer& buffer, int count){
    if(count <= 0) return;
    if(mat.currentShader.drawTypes[buffer.IsMatrix4x3() ? 3 : 2] == nullptr) return;
    BindMaterial(mat, buffer.IsMatrix4x3() ? 3 : 2);

    Assert(MeshIsValid(mesh) && "Mesh is not valid!");
    Assert(count > 0);

    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount * count;
    stats.tris += (mesh.indiceCount * count) / 3;
    debugData.DrawMeshInstancing(&mesh);

    // Bind mesh geometry
    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    #else
    MeshBind(mesh); //mesh.Bind();
    #endif

    // Bind instancing buffer
    glBindBuffer(GL_ARRAY_BUFFER, buffer.glData.id);
    glCheckError();

    if(buffer.isMatrix4x3){
        // Set up instance attributes (mat4 = 4 vec4s)
        std::size_t vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(10);
        glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(0));
        glEnableVertexAttribArray(11);
        glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(1 * vec4Size));
        glEnableVertexAttribArray(12);
        glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(2 * vec4Size));
        glCheckError();

        glVertexAttribDivisor(10, 1);
        glVertexAttribDivisor(11, 1);
        glVertexAttribDivisor(12, 1);
        glCheckError();
    } else {
        // Set up instance attributes (mat4 = 4 vec4s)
        std::size_t vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(10);
        glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(0));
        glEnableVertexAttribArray(11);
        glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(1 * vec4Size));
        glEnableVertexAttribArray(12);
        glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(2 * vec4Size));
        glEnableVertexAttribArray(13);
        glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(3 * vec4Size));
        glCheckError();

        glVertexAttribDivisor(10, 1);
        glVertexAttribDivisor(11, 1);
        glVertexAttribDivisor(12, 1);
        glVertexAttribDivisor(13, 1);
        glCheckError();
    }

    Assert(meshDrawModeLookup[(int)mesh.drawMode] != GL_NONE && "Dont support the current mesh.drawMode!");

    // Draw instanced
    if(mesh.glData.ebo != 0){
        OnDrawAssetsTest();
        glDrawElementsInstanced(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0, count);
        glCheckError();
    } else {
        OnDrawAssetsTest();
        glDrawArraysInstanced(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount, count);
        glCheckError();
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
}

void OpenGLGraphicsDevice::DrawModel(Model& model, Matrix4 modelMatrix){
    int index = 0;
    for(auto i: model.renderTargets){
        Ref<Material> targetMaterial = model.materials[i.materialIndex];
        Ref<Mesh> targetMesh = model.meshs[i.meshIndex];
        Matrix4 targetMatrix =  modelMatrix * model.skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
        DrawMesh(*targetMesh, *targetMaterial, targetMatrix);
    }
}

void OpenGLGraphicsDevice::AddDrawLineCommand(Vector3 start, Vector3 end){
    lineCommandsData.push_back(start.x);
    lineCommandsData.push_back(start.y);
    lineCommandsData.push_back(start.z);

    lineCommandsData.push_back(end.x);
    lineCommandsData.push_back(end.y);
    lineCommandsData.push_back(end.z);
}

void OpenGLGraphicsDevice::DrawLinesComamnd(Vector3 color, int lineWidth){
    if(lineWidth > 1) lineWidth = 1;

    //drawCalls += 1;
    stats.vertices += lineCommandsData.size() / 3;
    stats.tris += 0;

    BindMaterial(*gismoMaterial);
    Assert(lastShader != nullptr);
    SubShaderSetVector3(*lastShader, "color", color);
    SubShaderSetMatrix4(*lastShader, "model", Matrix4Identity);

    /*SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);*/

	glLineWidth(lineWidth);
    glCheckError();

    #ifdef USE_VAO
	glBindVertexArray(lineCommandsVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineCommandsVBO);
    glCheckError();
    #else
    glBindBuffer(GL_ARRAY_BUFFER, lineCommandsVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glCheckError();
    #endif
    
    /*glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * lineCommandsData.size(), &lineCommandsData[0]);
	glCheckError();
    glDrawArrays(GL_LINES, 0, lineCommandsData.size()/3);
    glCheckError();*/

    for(int i = 0; i < lineCommandsData.size(); i += 2 * MAX_LINES_VERTEX_DRAWCALL * 3){
        stats.drawCalls += 1;
        int batchVertexCount = std::min<int>(lineCommandsData.size() - i, 2 * MAX_LINES_VERTEX_DRAWCALL * 3);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * batchVertexCount, &lineCommandsData[i]);
	    glCheckError();
        OnDrawAssetsTest();
        glDrawArrays(GL_LINES, 0, batchVertexCount/3);
        glCheckError();
    }

    lineCommandsData.clear();
}

void OpenGLGraphicsDevice::DrawLine(Vector3 start, Vector3 end, Vector3 color, int width){
    stats.drawCalls += 1;
    stats.vertices += 2;
    stats.tris += 0;

    BindMaterial(*gismoMaterial);
    Assert(lastShader != nullptr);
    SubShaderSetVector3(*lastShader, "color", color);
    SubShaderSetMatrix4(*lastShader, "model", Matrix4Identity);

    /*SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);*/

	glLineWidth(width);

    float line[6] = {start.x, start.y, start.z, end.x, end.y, end.z};

    #ifdef USE_VAO
	glBindVertexArray(lineVAO);
    #else
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glCheckError();
    #endif

	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line), line);
    OnDrawAssetsTest();
	glDrawArrays(GL_LINES, 0, 2);
    glCheckError();

	//glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int width){
    stats.drawCalls += 1;
    stats.vertices += 2;
    stats.tris += 0;

    start = Vector3(model * Vector4(start.x, start.y, start.z, 1));
    end = Vector3(model * Vector4(end.x, end.y, end.z, 1));

    BindMaterial(*gismoMaterial);
    Assert(lastShader != nullptr);
    SubShaderSetVector3(*lastShader, "color", color);
    SubShaderSetMatrix4(*lastShader, "model", Matrix4Identity);

    /*SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity); //gismoShader->SetMatrix4("model", model);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);*/

	glLineWidth(width);

    float line[6] = {start.x, start.y, start.z, end.x, end.y, end.z};

    #ifdef USE_VAO
	glBindVertexArray(lineVAO);
    #else
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glCheckError();
    #endif

	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line), line);
    OnDrawAssetsTest();
	glDrawArrays(GL_LINES, 0, 2);
    glCheckError();
	
    //glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){
    stats.drawCalls += 1;
    stats.vertices += 8;
    stats.tris += 24;

    BindMaterial(*gismoMaterial);
    Assert(lastShader != nullptr);
    SubShaderSetVector3(*lastShader, "color", color);
    SubShaderSetMatrix4(*lastShader, "model", modelMatrix);

    /*SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", modelMatrix);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);*/

    #ifdef USE_VAO
    glBindVertexArray(wiredCubeVAO);
    #else
    glBindBuffer(GL_ARRAY_BUFFER, wiredCubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wiredCubeEBO);
    glCheckError();
    #endif
    
    glLineWidth(lineWidth);
    OnDrawAssetsTest();
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    glCheckError();

    //glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawFullScreenQuad(Material& mat, Matrix4 modelMatrix){
    DrawMesh(*fullScreenQuad, mat, modelMatrix);
}

static uint32_t DecodeUTF8(const char* s, int& advance){
    unsigned char c = (unsigned char)s[0];

    if(c < 0x80){
        advance = 1;
        return c;
    } else if((c >> 5) == 0x6){
        advance = 2;
        return((c & 0x1F) << 6) | (s[1] & 0x3F);
    } else if ((c >> 4) == 0xE){
        advance = 3;
        return ((c & 0x0F) << 12) |
               ((s[1] & 0x3F) << 6) |
               (s[2] & 0x3F);
    } else if ((c >> 3) == 0x1E){
        advance = 4;
        return ((c & 0x07) << 18) |
               ((s[1] & 0x3F) << 12) |
               ((s[2] & 0x3F) << 6) |
               (s[3] & 0x3F);
    }

    advance = 1;
    return '?';
}

//TODO: Move GraphicDevice::DrawText to New High Level Draw API Later, so not need for each graphic device implement this
void OpenGLGraphicsDevice::DrawText(Font& f, Material& s, std::string text, Matrix4 model, bool alignWithTop, const TextParams& textParams){
    const auto& fontGeometry = f.data->fontGeometry;
    const auto& metrics = fontGeometry.getMetrics();
    Ref<Texture2D> fontAtlas = f.fontAtlas;

    s.SetTexture("mainTex", fontAtlas);
    BindMaterial(s);
    Assert(lastShader != nullptr);
    SubShaderSetMatrix4(*lastShader, "model", model);

    #ifdef USE_VAO
    glBindVertexArray(textQuadVAO);
    glCheckError();
    #endif
    
    #ifndef USE_VAO
    glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        (void*)(3 * sizeof(float))
    );
    glCheckError();
    #endif

    double x = 0.0;
    double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
    double y = 0.0;
    //double y = -(fsScale * metrics.lineHeight + 0);
    if(alignWithTop) y = -(fsScale * metrics.ascenderY);

    const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

    //ACSII
    /*
    for(size_t i = 0; i < text.size(); i++){
        char character = text[i];
        if(character == '\r') continue;

        if(character == '\n'){
            x = 0;
            y -= fsScale * metrics.lineHeight + textParams.lineSpacing;
            continue;
        }

        if(character == ' '){
            float advance = spaceGlyphAdvance;
            if(i < text.size() - 1){
                char nextCharacter = text[i + 1];
                double dAdvance;
                fontGeometry.getAdvance(dAdvance, character, nextCharacter);
                advance = (float)dAdvance;
            }

            x += fsScale * advance + textParams.kerning;
            continue;
        }

        if(character == '\t'){
            // NOTE(Yan): is this right?
            x += 4.0f * (fsScale * spaceGlyphAdvance + textParams.kerning);
            continue;
        }

        auto glyph = fontGeometry.getGlyph(character);
        if(!glyph)
            glyph = fontGeometry.getGlyph('?');
        if(!glyph)
            return;

        double al, ab, ar, at;
        glyph->getQuadAtlasBounds(al, ab, ar, at);
        glm::vec2 texCoordMin((float)al, (float)ab);
        glm::vec2 texCoordMax((float)ar, (float)at);

        double pl, pb, pr, pt;
        glyph->getQuadPlaneBounds(pl, pb, pr, pt);
        glm::vec2 quadMin((float)pl, (float)pb);
        glm::vec2 quadMax((float)pr, (float)pt);

        quadMin *= fsScale, quadMax *= fsScale;
        quadMin += glm::vec2(x, y);
        quadMax += glm::vec2(x, y);

        float texelWidth = 1.0f / fontAtlas->Width();
        float texelHeight = 1.0f / fontAtlas->Height();
        texCoordMin *= glm::vec2(texelWidth, texelHeight);
        texCoordMax *= glm::vec2(texelWidth, texelHeight);

        float vertices[4][5] = {        
            { quadMin.x, quadMin.y, 0, texCoordMin.x, texCoordMin.y },
            { quadMin.x, quadMax.y, 0, texCoordMin.x, texCoordMax.y },
            { quadMax.x, quadMin.y, 0, texCoordMax.x, texCoordMin.y },
            { quadMax.x, quadMax.y, 0, texCoordMax.x, texCoordMax.y }           
        };
        glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glCheckError();

        // render quad
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glCheckError();

        if(i < text.size() - 1){
            double advance = glyph->getAdvance();
            char nextCharacter = text[i + 1];
            fontGeometry.getAdvance(advance, character, nextCharacter);
            x += fsScale * advance + 0; //textParams.Kerning;
        }
    }
    */

    //UTF-8
    for(size_t i = 0; i < text.size(); ){
        int advanceBytes = 0;
        uint32_t codepoint = DecodeUTF8(&text[i], advanceBytes);

        if(codepoint == '\r'){
            i += advanceBytes;
            continue;
        }

        if(codepoint == '\n'){
            x = 0;
            y -= fsScale * metrics.lineHeight + textParams.lineSpacing;
            i += advanceBytes;
            continue;
        }

        if(codepoint == ' '){
            double advance = spaceGlyphAdvance;

            if(i + advanceBytes < text.size()){
                int nextAdvance = 0;
                uint32_t nextCodepoint = DecodeUTF8(&text[i + advanceBytes], nextAdvance);
                fontGeometry.getAdvance(advance, codepoint, nextCodepoint);
            }

            x += fsScale * advance + textParams.kerning;
            i += advanceBytes;
            continue;
        }

        if(codepoint == '\t'){
            x += 4.0f * (fsScale * spaceGlyphAdvance + textParams.kerning);
            i += advanceBytes;
            continue;
        }

        auto glyph = fontGeometry.getGlyph(codepoint);
        if(!glyph) glyph = fontGeometry.getGlyph('?');
        if (!glyph) return;

        // ---- same quad code as before ----
        double al, ab, ar, at;
        glyph->getQuadAtlasBounds(al, ab, ar, at);
        glm::vec2 texCoordMin((float)al, (float)ab);
        glm::vec2 texCoordMax((float)ar, (float)at);

        double pl, pb, pr, pt;
        glyph->getQuadPlaneBounds(pl, pb, pr, pt);
        glm::vec2 quadMin((float)pl, (float)pb);
        glm::vec2 quadMax((float)pr, (float)pt);

        quadMin *= fsScale, quadMax *= fsScale;
        quadMin += glm::vec2(x, y);
        quadMax += glm::vec2(x, y);

        float texelWidth = 1.0f / fontAtlas->Width();
        float texelHeight = 1.0f / fontAtlas->Height();
        texCoordMin *= glm::vec2(texelWidth, texelHeight);
        texCoordMax *= glm::vec2(texelWidth, texelHeight);

        float vertices[4][5] = {        
            { quadMin.x, quadMin.y, 0, texCoordMin.x, texCoordMin.y },
            { quadMin.x, quadMax.y, 0, texCoordMin.x, texCoordMax.y },
            { quadMax.x, quadMin.y, 0, texCoordMax.x, texCoordMin.y },
            { quadMax.x, quadMax.y, 0, texCoordMax.x, texCoordMax.y }           
        };
        glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glCheckError();

        // render quad
        OnDrawAssetsTest();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glCheckError();

        if(i + advanceBytes < text.size()){
            int nextAdvance = 0;
            uint32_t nextCodepoint = DecodeUTF8(&text[i + advanceBytes], nextAdvance);

            double advance = glyph->getAdvance();
            fontGeometry.getAdvance(advance, codepoint, nextCodepoint);
            x += fsScale * advance;
        }

        i += advanceBytes;
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
}

void OpenGLGraphicsDevice::DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& mat, int pass){
    Assert(false && "Deprecated");

    /*Assert(src != nullptr);
    Assert(src->IsValid() == true);

    if(dst == nullptr){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCheckError();
    } else {
        BeginFramebuffer(*dst, 0);
        Graphics::SetViewport(0, 0, dst->Width(), dst->Height());
    }
    
    mat.SetTexture("mainTex", src, pass);
    //BindMaterial(mat);
    Graphics::Clean(0, 0, 0, 1);
    DrawMesh(*fullScreenQuad, mat, Matrix4Identity);*/
}

void OpenGLGraphicsDevice::DrawQuadPostProcessing(Framebuffer* dst, Material& mat, int pass){
    Assert(false && "Deprecated");
    /*if(dst == nullptr){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCheckError();
    } else {
        BeginFramebuffer(*dst, 0);
        Graphics::SetViewport(0, 0, dst->Width(), dst->Height());
    }

    //BindMaterial(mat);
    Graphics::Clean(0, 0, 0, 1);
    DrawMesh(*fullScreenQuad, mat, Matrix4Identity);*/
}

void OpenGLGraphicsDevice::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
    glViewport(x, y, w, h);
    //glScissor(x, y, w, h);
    glCheckError();
}

void OpenGLGraphicsDevice::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){
    GLint value[4];
    glGetIntegerv(GL_VIEWPORT, value);
    *x = value[0]; 
    *y = value[1];
    *w = value[2]; 
    *h = value[3];
    glCheckError();
}

void OpenGLGraphicsDevice::EnableScissor(){
    glEnable(GL_SCISSOR_TEST);
    glCheckError();
}

void OpenGLGraphicsDevice::DisableScissor(){
    glDisable(GL_SCISSOR_TEST);
    glCheckError();
}

void OpenGLGraphicsDevice::Scissor(unsigned int x, unsigned int y, int w, int h){
    glScissor(x, y, w, h);
    glCheckError();
}

void OpenGLGraphicsDevice::MeshBind(Mesh& mesh){
    // --- POSITION ---
    glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.vertexVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);

    // --- UV ---
    if(mesh.glData.uvVbo != 0){
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.uvVbo);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    } else {
        glDisableVertexAttribArray(1);
    }

    // --- NORMAL ---
    if(mesh.glData.normalVbo != 0){
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.normalVbo);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    } else {
        glDisableVertexAttribArray(2);
    }

    // --- COLOR ---
    if(mesh.glData.colorVbo != 0){
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.colorVbo);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
    } else {
        glDisableVertexAttribArray(3);
    }

    // --- TANGENT ---
    if(mesh.glData.tangentVbo != 0){
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.tangentVbo);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    } else {
        glDisableVertexAttribArray(4);
    }

    // --- BONES ---
    if(mesh.glData.jointVbo != 0){
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.jointVbo);
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(IVector4), (void*)0);
    } else {
        glDisableVertexAttribArray(5);
    }

    if(mesh.glData.weightsVbo != 0){
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.weightsVbo);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
    } else {
        glDisableVertexAttribArray(6);
    }

    // --- INDEX BUFFER ---
    if(mesh.glData.ebo != 0){
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.glData.ebo);
    }
}

bool OpenGLGraphicsDevice::MeshCreateOrSubmit(
    Mesh& mesh,
    std::vector<unsigned int>* indices,
    std::vector<Vector3>* vertices,
    std::vector<Vector3>* uv,
    std::vector<Vector3>* normals,
    std::vector<Vector4>* colors,
    std::vector<Vector3>* tangents,
    std::vector<Vector4>* weights,
    std::vector<IVector4>* influences
){
    //Assert(isReadable == true && "Only can Update isReadable Mesh");

    //Destroy();

    MeshDestroy(mesh);

    #ifdef USE_VAO
    if(mesh.glData.vao == 0){
        glGenVertexArrays(1, &mesh.glData.vao);
        glBindVertexArray(mesh.glData.vao);
        glCheckError();
    } else {
        glBindVertexArray(mesh.glData.vao);
    }
    #endif
    
    if(mesh.glData.vertexVbo == 0){
        glGenBuffers(1, &mesh.glData.vertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * vertices->size(), &(*vertices)[0], GL_STATIC_DRAW ); //GL_STATIC_DRAW
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
        glCheckError();
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * vertices->size(), &(*vertices)[0], GL_STATIC_DRAW); //GL_STATIC_DRAW
    }
    mesh.vertexCount = vertices->size();

    if(indices != nullptr){
    if(indices->size() > 0){
        if(mesh.glData.ebo == 0){
            glGenBuffers(1, &mesh.glData.ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.glData.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(unsigned int), &(*indices)[0], GL_STATIC_DRAW );
            glCheckError();
        } else {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.glData.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(unsigned int), &(*indices)[0], GL_STATIC_DRAW);
        }
    }
    mesh.indiceCount = indices->size();
    }

    if(uv != nullptr && uv->size() == vertices->size()){
        if(mesh.glData.uvVbo == 0){
            glGenBuffers(1, &mesh.glData.uvVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.uvVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * uv->size(), &(*uv)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.uvVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * uv->size(), &(*uv)[0], GL_STATIC_DRAW);
            glCheckError();
        }
    }

    if(normals != nullptr && normals->size() == vertices->size()){
        if(mesh.glData.normalVbo == 0){
            glGenBuffers(1, &mesh.glData.normalVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.normalVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * normals->size(), &(*normals)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.normalVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * normals->size(), &(*normals)[0], GL_STATIC_DRAW);
            glCheckError();
        }
    }

    if(colors != nullptr && colors->size() == vertices->size()){
        if(mesh.glData.colorVbo == 0){
            glGenBuffers(1, &mesh.glData.colorVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.colorVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * colors->size(), &(*colors)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.colorVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * colors->size(), &(*colors)[0], GL_STATIC_DRAW);
            glCheckError();
        }
    }

    if(tangents != nullptr && tangents->size() == vertices->size()){
        if(mesh.glData.tangentVbo == 0){
            glGenBuffers(1, &mesh.glData.tangentVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.tangentVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * tangents->size(), &(*tangents)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.tangentVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * tangents->size(), &(*tangents)[0], GL_STATIC_DRAW);
            glCheckError();
        }
    }

    if(influences != nullptr && influences->empty() == false) Assert(influences->size() == vertices->size());
    if(influences != nullptr && influences->size() == vertices->size()){
        if(mesh.glData.jointVbo == 0){
            glGenBuffers(1, &mesh.glData.jointVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.jointVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(IVector4) * influences->size(), &(*influences)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(5);
            glVertexAttribIPointer(5, 4, GL_INT, sizeof(IVector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.jointVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(IVector4) * influences->size(), &(*influences)[0], GL_STATIC_DRAW);
            glCheckError();
        }
    }

    if(weights != nullptr && weights->empty() == false) Assert(weights->size() == vertices->size());
    if(weights != nullptr && weights->size() == vertices->size()){
        if(mesh.glData.weightsVbo == 0){
            glGenBuffers(1, &mesh.glData.weightsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.weightsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * weights->size(), &(*weights)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.weightsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * weights->size(), &(*weights)[0], GL_STATIC_DRAW);
            glCheckError();
        }
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif

    mesh.ramUsage  = mesh.CalculateRamUsage();
    mesh.vramUsage = mesh.CalculateVRamUsage();
    vram.Add(mesh.vramUsage, VRAMTracker::Category::Mesh);
    //LogInfo("Mesh VRam: {}MB/{}MB", mesh.vramUsage / (1024 * 1024), vram.meshBytes / (1024 * 1024));

    return true;
}

void OpenGLGraphicsDevice::MeshSubmitInstancingModelMatrixs(Mesh& mesh){
    #ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #endif

    if(mesh.instancingModelMatrixs.empty() == false){
        if(mesh.glData.instancingModelMatrixsVbo == 0){
            glGenBuffers(1, &mesh.glData.instancingModelMatrixsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo); 
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * mesh.instancingModelMatrixs.size(), &mesh.instancingModelMatrixs[0], GL_STATIC_DRAW); //GL_STREAM_DRAW GL_DYNAMIC_DRAW
            glCheckError();

            std::size_t vec4Size = sizeof(glm::vec4);
            glEnableVertexAttribArray(10); 
            glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
            glEnableVertexAttribArray(11); 
            glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
            glEnableVertexAttribArray(12); 
            glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
            glEnableVertexAttribArray(13); 
            glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
            glCheckError();

            glVertexAttribDivisor(10, 1);
            glVertexAttribDivisor(11, 1);
            glVertexAttribDivisor(12, 1);
            glVertexAttribDivisor(13, 1);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * mesh.instancingModelMatrixs.size(), &mesh.instancingModelMatrixs[0], GL_STATIC_DRAW); //GL_STREAM_DRAW
            glCheckError();
        }
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
}

void OpenGLGraphicsDevice::MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count){
    //OLD
    /*#ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #endif

    if(count > 0){
        if(mesh.glData.instancingModelMatrixsVbo == 0){
            glGenBuffers(1, &mesh.glData.instancingModelMatrixsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * count, modelMatrixs, GL_STATIC_DRAW); //GL_DYNAMIC_DRAW //GL_STREAM_DRAW
            glCheckError();

            std::size_t vec4Size = sizeof(glm::vec4);
            glEnableVertexAttribArray(10); 
            glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
            glEnableVertexAttribArray(11); 
            glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
            glEnableVertexAttribArray(12); 
            glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
            glEnableVertexAttribArray(13); 
            glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
            glCheckError();

            glVertexAttribDivisor(10, 1);
            glVertexAttribDivisor(11, 1);
            glVertexAttribDivisor(12, 1);
            glVertexAttribDivisor(13, 1);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * count, modelMatrixs, GL_STATIC_DRAW); //GL_STREAM_DRAW
            glCheckError();

            #ifndef USE_VAO
            std::size_t vec4Size = sizeof(glm::vec4);
            glEnableVertexAttribArray(10); 
            glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
            glEnableVertexAttribArray(11); 
            glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
            glEnableVertexAttribArray(12); 
            glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
            glEnableVertexAttribArray(13); 
            glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
            glCheckError();

            glVertexAttribDivisor(10, 1);
            glVertexAttribDivisor(11, 1);
            glVertexAttribDivisor(12, 1);
            glVertexAttribDivisor(13, 1);
            glCheckError();
            #endif
        }
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
    return;*/

    /*
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    #ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #endif

    Assert(count > 0);
    if (count <= 0) return;

    const size_t matrixSize = sizeof(Matrix4);
    const size_t frameSize = matrixSize * count;
    const size_t requiredBufferSize = frameSize * MAX_FRAMES_IN_FLIGHT;

    // Allocate or resize buffer if needed
    if (mesh.glData.instancingModelMatrixsVbo == 0 || mesh.glData.instancingBufferCapacity < requiredBufferSize) {
        if (mesh.glData.instancingModelMatrixsVbo != 0)
            glDeleteBuffers(1, &mesh.glData.instancingModelMatrixsVbo);

        glGenBuffers(1, &mesh.glData.instancingModelMatrixsVbo); glCheckError();
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo); glCheckError();
        glBufferStorage(GL_ARRAY_BUFFER, requiredBufferSize, nullptr,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT); glCheckError();
        mesh.glData.instancingBufferCapacity = requiredBufferSize;

        mesh.glData.instancingMappedPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, requiredBufferSize,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT); glCheckError();

        // Setup vertex attributes for mat4 (4 vec4s)
        std::size_t vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(10); 
        glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(0));
        glEnableVertexAttribArray(11); 
        glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(1 * vec4Size));
        glEnableVertexAttribArray(12); 
        glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(2 * vec4Size));
        glEnableVertexAttribArray(13); 
        glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(3 * vec4Size));
        glCheckError();

        glVertexAttribDivisor(10, 1);
        glVertexAttribDivisor(11, 1);
        glVertexAttribDivisor(12, 1);
        glVertexAttribDivisor(13, 1);
        glCheckError();
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo); glCheckError();
    }

    // Use a ring buffer offset
    static uint32_t currentFrame = 0;
    const size_t frameOffset = currentFrame * frameSize;
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    // Copy data to persistent mapped memory
    std::memcpy((char*)mesh.glData.instancingMappedPtr + frameOffset, modelMatrixs, frameSize);

    // Rebind buffer for draw (not needed every time if using VAO, but done here for safety)
    glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo);
    glCheckError();

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
    */  

    /*Assert(count > 0);
    if (count <= 0) return;

    const size_t matrixSize = sizeof(Matrix4);
    const size_t bufferSize = matrixSize * count;

    // Ensure the pool is large enough
    if (curPerInstancingDrawData >= perInstancingDrawData.size()) {
        perInstancingDrawData.resize(curPerInstancingDrawData + 1);
    }

    PerDrawInstanceData& drawData = perInstancingDrawData[curPerInstancingDrawData];
    curPerInstancingDrawData++;

    // Create or resize buffer if needed
    if(drawData.vbo == 0 || drawData.capacity < bufferSize) {
        if(drawData.vbo != 0){
            glDeleteBuffers(1, &drawData.vbo);
            glCheckError();
        }

        glGenBuffers(1, &drawData.vbo); glCheckError();
        glBindBuffer(GL_ARRAY_BUFFER, drawData.vbo); 
        glCheckError();

        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, nullptr,GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT); 
        glCheckError();
        
        drawData.capacity = bufferSize;

        drawData.mappedPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT); 
        glCheckError();
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, drawData.vbo); 
        glCheckError();
    }

    // Copy matrices to mapped buffer
    std::memcpy(drawData.mappedPtr, modelMatrixs, bufferSize);

    // Bind VAO if enabled
    #ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao); glCheckError();
    #endif

    // Set attribute layout for mat4 instance data
    std::size_t vec4Size = sizeof(glm::vec4);
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(0));
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(1 * vec4Size));
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(2 * vec4Size));
    glEnableVertexAttribArray(13);
    glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(3 * vec4Size));
    glCheckError();

    glVertexAttribDivisor(10, 1);
    glVertexAttribDivisor(11, 1);
    glVertexAttribDivisor(12, 1);
    glVertexAttribDivisor(13, 1);
    glCheckError();

    #ifdef USE_VAO
    glBindVertexArray(0); glCheckError();
    #endif*/

    //New
    Assert(sizeof(Matrix4) == sizeof(glm::mat4));
    Assert(sizeof(glm::mat4) == sizeof(float) * 16);

    Assert(count > 0);
    if(count <= 0) return;

    #ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao);
    #endif

    const size_t matrixSize = sizeof(Matrix4);
    const size_t bufferSize = matrixSize * count;

    // Ensure pool
    if(curPerInstancingDrawData >= perInstancingDrawData.size()) {
        perInstancingDrawData.resize(curPerInstancingDrawData + 1);
    }

    PerDrawInstanceData& drawData = perInstancingDrawData[curPerInstancingDrawData];
    curPerInstancingDrawData++;

    if(drawData.vbo == 0){
        glGenBuffers(1, &drawData.vbo);
        glCheckError();
    }

    glBindBuffer(GL_ARRAY_BUFFER, drawData.vbo);
    glCheckError();

    #ifdef OpenGL46 //OPENGL46
    //--------------------------------------------------
    // Modern path (persistent mapping)
    //--------------------------------------------------
    if(drawData.capacity < bufferSize){
        if(drawData.vbo != 0){
            glDeleteBuffers(1, &drawData.vbo);
            glGenBuffers(1, &drawData.vbo);
            glBindBuffer(GL_ARRAY_BUFFER, drawData.vbo);
        }

        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, nullptr,
            GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_COHERENT_BIT);

        drawData.mappedPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize,
            GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_COHERENT_BIT);

        drawData.capacity = bufferSize;
    }

    // just memcpy (no map/unmap per frame)
    std::memcpy(drawData.mappedPtr, modelMatrixs, bufferSize);

    #else
    //--------------------------------------------------
    // OpenGL 3.3 fallback
    //--------------------------------------------------

    // Resize if needed
    if(drawData.capacity < bufferSize){
        glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
        drawData.capacity = bufferSize;
    }

    // Option 1 (recommended): orphan + subdata
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW); // orphan
    glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, modelMatrixs);

    // Option 2 (alternative): map/unmap
    /*
    void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    if(ptr){
        memcpy(ptr, modelMatrixs, bufferSize);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    */

    #endif

    //----------------------------------------
    // Attribute setup (same for both paths)
    //----------------------------------------

    /*#ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao);
    #endif*/

    std::size_t vec4Size = sizeof(glm::vec4);

    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(0));

    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(1 * vec4Size));

    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(2 * vec4Size));

    glEnableVertexAttribArray(13);
    glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4), (void*)(3 * vec4Size));

    glVertexAttribDivisor(10, 1);
    glVertexAttribDivisor(11, 1);
    glVertexAttribDivisor(12, 1);
    glVertexAttribDivisor(13, 1);
    glCheckError();

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
}

void OpenGLGraphicsDevice::MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4x3* modelMatrixs, int count){
    /*Assert(count > 0);
    if(count <= 0) return;

    const size_t matrixSize = sizeof(Matrix4x3);
    const size_t bufferSize = matrixSize * count;

    // Ensure the pool is large enough
    if(curPerInstancingDrawData >= perInstancingDrawData.size()){
        perInstancingDrawData.resize(curPerInstancingDrawData + 1);
    }

    PerDrawInstanceData& drawData = perInstancingDrawData[curPerInstancingDrawData];
    curPerInstancingDrawData++;

    // Create or resize buffer if needed
    if(drawData.vbo == 0 || drawData.capacity < bufferSize){
        if(drawData.vbo != 0){
            glDeleteBuffers(1, &drawData.vbo);
            glCheckError();
        }

        glGenBuffers(1, &drawData.vbo); 
        glCheckError();
        glBindBuffer(GL_ARRAY_BUFFER, drawData.vbo); 
        glCheckError();

        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT); 
        glCheckError();
        
        drawData.capacity = bufferSize;

        drawData.mappedPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT); 
        glCheckError();
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, drawData.vbo); glCheckError();
    }

    // Copy matrices to mapped buffer
    std::memcpy(drawData.mappedPtr, modelMatrixs, bufferSize);

    // Bind VAO if enabled
    #ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao); glCheckError();
    #endif

    // Set attribute layout for mat4 instance data
    std::size_t vec4Size = sizeof(glm::vec4);
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(0));
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(1 * vec4Size));
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(2 * vec4Size));
    glCheckError();

    glVertexAttribDivisor(10, 1);
    glVertexAttribDivisor(11, 1);
    glVertexAttribDivisor(12, 1);
    glCheckError();

    #ifdef USE_VAO
    glBindVertexArray(0); glCheckError();
    #endif*/

    Assert(count > 0);
    if (count <= 0) return;

    const size_t matrixSize = sizeof(Matrix4x3);
    const size_t bufferSize = matrixSize * count;

    // Ensure pool
    if(curPerInstancingDrawData >= perInstancingDrawData.size()){
        perInstancingDrawData.resize(curPerInstancingDrawData + 1);
    }

    PerDrawInstanceData& drawData = perInstancingDrawData[curPerInstancingDrawData];
    curPerInstancingDrawData++;

    if(drawData.vbo == 0){
        glGenBuffers(1, &drawData.vbo);
        glCheckError();
    }

    glBindBuffer(GL_ARRAY_BUFFER, drawData.vbo);
    glCheckError();

#ifdef OPENGL46
    //--------------------------------------------------
    // Persistent mapping path
    //--------------------------------------------------
    if (drawData.capacity < bufferSize){
        if(drawData.vbo != 0) {
            glDeleteBuffers(1, &drawData.vbo);
            glGenBuffers(1, &drawData.vbo);
            glBindBuffer(GL_ARRAY_BUFFER, drawData.vbo);
        }

        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, nullptr,
            GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_COHERENT_BIT);
        glCheckError();

        drawData.mappedPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize,
            GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_COHERENT_BIT);
        glCheckError();

        drawData.capacity = bufferSize;
    }

    // Just memcpy
    std::memcpy(drawData.mappedPtr, modelMatrixs, bufferSize);

#else
    //--------------------------------------------------
    // OpenGL 3.3 fallback
    //--------------------------------------------------

    if(drawData.capacity < bufferSize){
        glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
        drawData.capacity = bufferSize;
    }

    // Orphan + upload (best for avoiding stalls)
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, modelMatrixs);

    // Alternative (if you want mapping instead):
    /*
    void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    if(ptr){
        memcpy(ptr, modelMatrixs, bufferSize);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    */

#endif

    //--------------------------------------------------
    // Attribute setup (3x vec4 = mat4x3)
    //--------------------------------------------------

#ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
#endif

    std::size_t vec4Size = sizeof(glm::vec4);

    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(0));

    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(1 * vec4Size));

    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4x3), (void*)(2 * vec4Size));

    glCheckError();

    glVertexAttribDivisor(10, 1);
    glVertexAttribDivisor(11, 1);
    glVertexAttribDivisor(12, 1);

    glCheckError();

#ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
#endif
}

void OpenGLGraphicsDevice::MeshDestroy(Mesh& mesh){
    if(MeshIsValid(mesh) == false) return;

    if(mesh.glData.vertexVbo != 0) glDeleteBuffers(1, &mesh.glData.vertexVbo);
    if(mesh.glData.normalVbo != 0) glDeleteBuffers(1, &mesh.glData.normalVbo);
    if(mesh.glData.uvVbo != 0) glDeleteBuffers(1, &mesh.glData.uvVbo);
    if(mesh.glData.colorVbo != 0) glDeleteBuffers(1, &mesh.glData.colorVbo);
    if(mesh.glData.tangentVbo != 0) glDeleteBuffers(1, &mesh.glData.tangentVbo);
    if(mesh.glData.instancingModelMatrixsVbo != 0) glDeleteBuffers(1, &mesh.glData.instancingModelMatrixsVbo);
    if(mesh.glData.jointVbo != 0) glDeleteBuffers(1, &mesh.glData.jointVbo);
    if(mesh.glData.weightsVbo != 0) glDeleteBuffers(1, &mesh.glData.weightsVbo);

    if(mesh.glData.ebo != 0) glDeleteBuffers(1, &mesh.glData.ebo);

    glCheckError();

    #ifdef USE_VAO
    if(mesh.glData.vao != 0) glDeleteVertexArrays(1, &mesh.glData.vao);
    #endif

    mesh.vertexCount = 0;
    mesh.indiceCount = 0;
    mesh.glData.vertexVbo = 0;
    mesh.glData.normalVbo = 0;
    mesh.glData.colorVbo = 0;
    mesh.glData.tangentVbo = 0;
    mesh.glData.instancingModelMatrixsVbo = 0;
    mesh.glData.jointVbo = 0;
    mesh.glData.weightsVbo = 0;
    mesh.glData.uvVbo = 0;
    mesh.glData.ebo = 0;

    #ifdef USE_VAO
    mesh.glData.vao = 0;
    #endif

    vram.Free(mesh.vramUsage, VRAMTracker::Category::Mesh);
    mesh.vramUsage = 0;
    mesh.ramUsage  = mesh.CalculateRamUsage();

    glCheckError();
}

bool OpenGLGraphicsDevice::MeshIsValid(Mesh& mesh){
    #ifdef USE_VAO
    return mesh.glData.vao != 0;
    #else
    return mesh.glData.vertexVbo != 0;
    #endif
}

int InternalFormatLookup[] = {
    GL_NONE, GL_RGB, GL_RGBA8, GL_R11F_G11F_B10F, GL_RGB16F, GL_RGBA16F, GL_RGB32F, GL_RGBA32F, GL_R32I, GL_DEPTH24_STENCIL8, GL_DEPTH32F_STENCIL8, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT32F
};

int FormatLookup[] = {
    GL_NONE, GL_RGB, GL_RGBA, GL_RGB, GL_RGB, GL_RGBA, GL_RGB, GL_RGBA, GL_RED_INTEGER, GL_DEPTH_STENCIL, GL_DEPTH_STENCIL, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT
};

int DataTypeLookup[] = {
    GL_NONE,                 // None
    GL_NONE,                 // RGB
    GL_NONE,                 // RGBA8
    GL_NONE,                 // R11F_G11F_B10F
    GL_NONE,                 // RGB16F
    GL_NONE,                 // RGBA16F
    GL_NONE,                 // RGB32F
    GL_NONE,                 // RGBA32F
    GL_NONE,                 // R32I
    GL_UNSIGNED_INT_24_8,    // DEPTH24_STENCIL8
    GL_FLOAT_32_UNSIGNED_INT_24_8_REV, // DEPTH32F_STENCIL8
    GL_UNSIGNED_SHORT,       // DEPTH_COMPONENT16
    GL_UNSIGNED_INT,         // DEPTH_COMPONENT24
    GL_UNSIGNED_INT,         // DEPTH_COMPONENT32
    GL_FLOAT                 // DEPTH_COMPONENT32F
};

int AttachmentLookup[] = {
    GL_NONE,                   // None
    GL_NONE,                   // RGB
    GL_NONE,                   // RGBA8
    GL_NONE,                   // R11F_G11F_B10F
    GL_NONE,                   // RGB16F
    GL_NONE,                   // RGBA16F
    GL_NONE,                   // RGB32F
    GL_NONE,                   // RGBA32F
    GL_NONE,                   // R32I
    GL_DEPTH_STENCIL_ATTACHMENT, // DEPTH24_STENCIL8
    GL_DEPTH_STENCIL_ATTACHMENT, // DEPTH32F_STENCIL8
    GL_DEPTH_ATTACHMENT,      // DEPTH_COMPONENT16
    GL_DEPTH_ATTACHMENT,      // DEPTH_COMPONENT24
    GL_DEPTH_ATTACHMENT,      // DEPTH_COMPONENT32
    GL_DEPTH_ATTACHMENT       // DEPTH_COMPONENT32F
};

bool IsDepthTypeFormat(FramebufferTextureFormat format){
    if((int)format >= 9) return true;
    return false;
}

void OpenGLGraphicsDevice::BeginFramebuffer(Framebuffer& frambuffer, bool clean, Vector4 clearColor, int layer, int mip){
    /*Assert(frambuffer.glData.renderId > 0);
    glBindFramebuffer(GL_FRAMEBUFFER, frambuffer.glData.renderId);
    glCheckError();

    if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frambuffer.glData.depthAttachment, 0, layer);
        glCheckError();
    } else if(frambuffer.specification.type == FramebufferAttachmentType::CUBEMAP){
        Assert(layer >= 0 && layer < 6);
        for(int i = 0; i < frambuffer.glData.colorAttachments.size(); ++i)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, frambuffer.glData.colorAttachments[i], mip);
        if(frambuffer.glData.depthAttachment != 0) 
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, frambuffer.glData.depthAttachment, 0);
        glCheckError();
    }

    // Set Attachment glFramebufferTexture2D
    glCheckError();
    //glViewport(0, 0, frambuffer.specification.width, frambuffer.specification.height);

    if(clean) Clean(clearColor.x, clearColor.y, clearColor.z, clearColor.w);*/

    //-------------New-----------
    Assert(frambuffer.glData.renderId > 0);

    glBindFramebuffer(GL_FRAMEBUFFER, frambuffer.glData.renderId);
    glCheckError();

    if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
        if(frambuffer.glData.depthAttachment){
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frambuffer.glData.depthAttachment, mip, layer);
            glCheckError();
        }

        for(size_t i = 0; i < frambuffer.glData.colorAttachments.size(); i++){
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, frambuffer.glData.colorAttachments[i], mip, layer);
            glCheckError();
        }
    }
    if(frambuffer.specification.type == FramebufferAttachmentType::CUBEMAP){
        Assert(layer >= 0 && layer < 6);

        for(size_t i = 0; i < frambuffer.glData.colorAttachments.size(); i++){
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, frambuffer.glData.colorAttachments[i], mip);
            glCheckError();
        }

        if(frambuffer.glData.depthAttachment){
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, frambuffer.glData.depthAttachment, mip);
            glCheckError();
        }
    }

    if(clean) Clean(clearColor.x, clearColor.y, clearColor.z, clearColor.w);

    debugData.BeginPass(&frambuffer);
}

void OpenGLGraphicsDevice::EndFramebuffer(){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
    debugData.EndPass();
}

void OpenGLGraphicsDevice::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){
    Assert(src != nullptr);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->glData.renderId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst == nullptr ? 0 : dst->glData.renderId);
    glCheckError();

    if(srcPass < 0){
        glBlitFramebuffer(0, 0, src->Width(), src->Height(), 0, 0, src->Width(), src->Height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST); 
        glCheckError();
    } else {
        glReadBuffer(GL_COLOR_ATTACHMENT0 + srcPass); 
        glCheckError();
        glBlitFramebuffer(0, 0, src->Width(), src->Height(), 0, 0, src->Width(), src->Height(), GL_COLOR_BUFFER_BIT, GL_NEAREST); 
        glCheckError();
    }
}

bool OpenGLGraphicsDevice::FramebufferCreate(Framebuffer& fb){
    /*if(frambuffer.type == FramebufferType::Stand){
        frambuffer.specification.colorAttachments = {
            {FramebufferTextureFormat::RGBA8}
        };
        frambuffer.specification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
        frambuffer.specification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
        frambuffer.specification.sample = 1;
    }
    if(frambuffer.type == FramebufferType::Shadowmap){
        frambuffer.specification.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
        frambuffer.specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
    }
    if(frambuffer.type == FramebufferType::Deffered){
        frambuffer.specification.colorAttachments = {
            {FramebufferTextureFormat::RGB32F}, // Pos
            {FramebufferTextureFormat::RGB32F}, // Normal
            {FramebufferTextureFormat::RGBA16F}, // Albedo
            {FramebufferTextureFormat::RGB16F}, // Emission
            {FramebufferTextureFormat::RGB16F}, // Spec, Metalic, AO
            {FramebufferTextureFormat::RED_INTEGER} // Object ID
        };
        frambuffer.specification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
        frambuffer.specification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
        frambuffer.specification.sample = 1;
    }
    //frambuffer.specification = specification;

    auto GenColorAttachment = [&](int index){
        Assert(IsDepthTypeFormat(frambuffer.specification.colorAttachments[index].colorFormat) == false);
    
        GLenum internalFormat = InternalFormatLookup[(int)frambuffer.specification.colorAttachments[index].colorFormat];
        GLenum format = FormatLookup[(int)frambuffer.specification.colorAttachments[index].colorFormat];
        
        bool multisample = frambuffer.specification.sample > 1;
    
        unsigned int colorAttachment;
        glGenTextures(1, &colorAttachment);
        glCheckError();
    
        bool hdr = false;
        if(frambuffer.specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGB16F) hdr = true;
        if(frambuffer.specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGB32F) hdr = true;
        if(frambuffer.specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGBA16F) hdr = true;
        if(frambuffer.specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGBA32F) hdr = true;

        GLenum type = hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
        if(frambuffer.specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGB11B10F) type = GL_UNSIGNED_INT_10F_11F_11F_REV;
  
        if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE){
            #if defined(OpenGLEmscripten)
            Assert(false && "not supported");
            #else
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, frambuffer.specification.sample, internalFormat, frambuffer.specification.width, frambuffer.specification.height, GL_TRUE);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D_MULTISAMPLE, colorAttachment, 0);
            glCheckError();
            #endif
        } else if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D){
            

            glBindTexture(GL_TEXTURE_2D, colorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, frambuffer.specification.width, frambuffer.specification.height, 0, format, type, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, colorAttachment, 0);
            glCheckError();
        } else if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
            //Assert(false);
    
            glBindTexture(GL_TEXTURE_2D_ARRAY, colorAttachment);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, frambuffer.specification.width, frambuffer.specification.height, frambuffer.specification.sample, 0, format, type, NULL);
            glCheckError();
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glCheckError();
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, colorAttachment, 0, 0);

            glCheckError();
        } else if(frambuffer.specification.type == FramebufferAttachmentType::CUBEMAP){
            Assert(frambuffer.specification.width == frambuffer.specification.height);
            glBindTexture(GL_TEXTURE_CUBE_MAP, colorAttachment);
            for(unsigned int i = 0; i < 6; ++i){
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, frambuffer.specification.width, frambuffer.specification.height, 0, format, type, NULL);
                glCheckError();
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, frambuffer.specification.colorAttachments[index].genMip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glCheckError();
            for(unsigned int i = 0; i < 6; ++i){
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, colorAttachment, 0);
                glCheckError();
            }
        }

        if(frambuffer.specification.colorAttachments[index].genMip){
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            glCheckError();
        }
    
        frambuffer.glData.colorAttachments.push_back(colorAttachment);
    };
    
    auto GenDepthAttachment = [&](){
        if(frambuffer.specification.depthAttachment.colorFormat == FramebufferTextureFormat::None) return;
    
        Assert(IsDepthTypeFormat(frambuffer.specification.depthAttachment.colorFormat) == true);
    
        GLenum internalFormat = InternalFormatLookup[(int)frambuffer.specification.depthAttachment.colorFormat]; //GLenum internalFormat = GL_DEPTH24_STENCIL8;
        GLenum format = FormatLookup[(int)frambuffer.specification.depthAttachment.colorFormat]; //GLenum format = GL_DEPTH_STENCIL;
        GLenum type = GL_UNSIGNED_INT_24_8;
        GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    
        if(frambuffer.specification.depthAttachment.colorFormat == FramebufferTextureFormat::DEPTH4STENCIL8){
            internalFormat = GL_DEPTH24_STENCIL8;
            format = GL_DEPTH_STENCIL;
            type = GL_UNSIGNED_INT_24_8;
            attachment = GL_DEPTH_STENCIL_ATTACHMENT;
        }
        
        if(frambuffer.specification.depthAttachment.colorFormat == FramebufferTextureFormat::DEPTH_COMPONENT){
            internalFormat = GL_DEPTH_COMPONENT32F;
            format = GL_DEPTH_COMPONENT;
            type = GL_FLOAT;
            attachment = GL_DEPTH_ATTACHMENT;
        }
    
        bool multisample = frambuffer.specification.sample > 1;
    
        glGenTextures(1, &frambuffer.glData.depthAttachment);
    
        if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE){
            #if defined(OpenGLEmscripten)
            Assert(false && "not supported");
            #else
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, frambuffer.glData.depthAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, frambuffer.specification.sample, internalFormat, frambuffer.specification.width, frambuffer.specification.height, GL_TRUE);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, frambuffer.glData.depthAttachment, 0);
            glCheckError();
            #endif
        } else if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D){
            glBindTexture(GL_TEXTURE_2D, frambuffer.glData.depthAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, frambuffer.specification.width, frambuffer.specification.height, 0, format, type, NULL);
            glCheckError();
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
            glCheckError();
    
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, frambuffer.glData.depthAttachment, 0);
            glCheckError();
        } else if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
            //Assert(false);
            glBindTexture(GL_TEXTURE_2D_ARRAY, frambuffer.glData.depthAttachment);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, frambuffer.specification.width, frambuffer.specification.height, frambuffer.specification.sample, 0, format, type, NULL);
            glCheckError();
    
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
            
            //#if defined(OpenGLEmscripten)
            //Assert(false && "not supported");
            //glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, frambuffer.glData.depthAttachment, 0);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, frambuffer.glData.depthAttachment, 0, 0);
   
            glCheckError();
        } else if(frambuffer.specification.type == FramebufferAttachmentType::CUBEMAP){
            Assert(frambuffer.specification.width == frambuffer.specification.height);
            glBindTexture(GL_TEXTURE_CUBE_MAP, frambuffer.glData.depthAttachment);
            for(unsigned int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, frambuffer.specification.width, frambuffer.specification.height, 0, format, type, NULL);
                glCheckError();
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glCheckError();

            for(unsigned int i = 0; i < 6; ++i) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, frambuffer.glData.depthAttachment, 0);
                glCheckError();
            }
        }
    };

    //Clean Framebuffer
    //if(renderId) 
    FramebufferDestroy(frambuffer);
    
    //Gen Framebuffer
    glGenFramebuffers(1, &frambuffer.glData.renderId);
    glBindFramebuffer(GL_FRAMEBUFFER, frambuffer.glData.renderId);
    glCheckError();

    //Gen Attachments
    for(int i = 0; i < frambuffer.specification.colorAttachments.size(); i++){
        GenColorAttachment(i);
    }
    GenDepthAttachment();

    if(frambuffer.specification.colorAttachments.size() == 0){
        //glDrawBuffer(GL_NONE);
        //glReadBuffer(GL_NONE);
        const GLenum b = GL_NONE;
        glDrawBuffers(1, &b);
        glReadBuffer(GL_NONE);
        glCheckError();
    } else if(frambuffer.specification.colorAttachments.size() > 1){
        //Assert(specification.colorAttachments.size() <= 4);
		
        std::vector<GLenum> buffers;
        for(int i = 0; i < frambuffer.specification.colorAttachments.size(); i++){
            buffers.push_back(GL_COLOR_ATTACHMENT0+i);
        }
		
        glDrawBuffers(frambuffer.specification.colorAttachments.size(), &buffers[0]);
        glCheckError();
    }

    //Check Erros
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        LogError("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
        Assert(false);
    }
    
    //Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
    return true;*/
    
    //-------------New-----------
    FramebufferDestroy(fb);

    if(fb.type == FramebufferType::Stand){
        fb.specification.colorAttachments = {
            {FramebufferTextureFormat::RGBA8}
        };
        fb.specification.depthAttachment = {FramebufferTextureFormat::DEPTH24_STENCIL8};
        fb.specification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
        fb.specification.sample = 1;
    }
    if(fb.type == FramebufferType::Shadowmap){
        fb.specification.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
        fb.specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT16};
    }
    if(fb.type == FramebufferType::Deffered){
        fb.specification.colorAttachments = {
            {FramebufferTextureFormat::RGB32F}, // Pos
            {FramebufferTextureFormat::RGB32F}, // Normal
            {FramebufferTextureFormat::RGBA16F}, // Albedo
            {FramebufferTextureFormat::RGB16F}, // Emission
            {FramebufferTextureFormat::RGB16F}, // Spec, Metalic, AO
            {FramebufferTextureFormat::RED_INTEGER} // Object ID
        };
        fb.specification.depthAttachment = {FramebufferTextureFormat::DEPTH24_STENCIL8};
        fb.specification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
        fb.specification.sample = 1;
    }
    //frambuffer.specification = specification;

    glGenFramebuffers(1, &fb.glData.renderId);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.glData.renderId);
    glCheckError();

    const int width   = fb.specification.width;
    const int height  = fb.specification.height;
    const int samples = fb.specification.sample;

    bool isMSAA  = fb.specification.type == FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
    bool isArray = fb.specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    bool isCube  = fb.specification.type == FramebufferAttachmentType::CUBEMAP;

    // --------------------Calc Vram Usage-------------------------
    auto CalcBytesPerPixel = [&](FramebufferTextureFormat format) -> size_t {
        switch(format){
            case FramebufferTextureFormat::RGBA8:            return 4;
            case FramebufferTextureFormat::RGB:             return 3;
            case FramebufferTextureFormat::RED_INTEGER:      return 4;

            case FramebufferTextureFormat::RGB16F:           return 6;  // 3 * 16bit
            case FramebufferTextureFormat::RGBA16F:          return 8;  // 4 * 16bit

            case FramebufferTextureFormat::RGB32F:           return 12; // 3 * 32bit
            case FramebufferTextureFormat::RGBA32F:          return 16; // 4 * 32bit

            case FramebufferTextureFormat::RGB11B10F:        return 4;

            case FramebufferTextureFormat::DEPTH24_STENCIL8: return 4;
            case FramebufferTextureFormat::DEPTH_COMPONENT24:return 4;
            case FramebufferTextureFormat::DEPTH_COMPONENT32F:return 4;

            default: return 4;
        }
    };

    auto CalcMipSize = [&](int w, int h, int mip) -> size_t {
        return (size_t)std::max(1, w >> mip) * std::max(1, h >> mip);
    };

    auto CalcTextureVRAM = [&](FramebufferTextureFormat format, int mipCount) -> size_t {
        mipCount = std::max(1, std::min(mipCount, 16));

        size_t total = 0;
        size_t bpp = (size_t)CalcBytesPerPixel(format);

        for(int mip = 0; mip < mipCount; mip++){
            size_t w = std::max(1, width  >> mip);
            size_t h = std::max(1, height >> mip);

            total += (w * h * bpp);
        }

        // Apply multipliers carefully
        if(isMSAA){
            total *= (size_t)samples;
        }
        else if(isArray){
            total *= (size_t)samples; // layers
        }
        else if(isCube){
            total *= 6;
        }

        //LogInfo("VRAM Debug: w={} h={} samples={} mip={} result={} bytes", width, height, samples, mipCount, total);

        return total;
    };

    // -------------------------------------------------------
    // Color Attachment Lambda
    // -------------------------------------------------------

    auto GenColorAttachment = [&](int index){
        auto spec = fb.specification.colorAttachments[index];
        auto formatEnum = spec.colorFormat;

        Assert(IsDepthTypeFormat(formatEnum) == false);

        GLenum internalFormat = InternalFormatLookup[(int)formatEnum];
        GLenum format         = FormatLookup[(int)formatEnum];

        GLenum dataType = GL_UNSIGNED_BYTE;

        if(formatEnum == FramebufferTextureFormat::RGB16F  ||
        formatEnum == FramebufferTextureFormat::RGBA16F ||
        formatEnum == FramebufferTextureFormat::RGB32F  ||
        formatEnum == FramebufferTextureFormat::RGBA32F)
            dataType = GL_FLOAT;

        if(formatEnum == FramebufferTextureFormat::RGB11B10F)
            dataType = GL_UNSIGNED_INT_10F_11F_11F_REV;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glCheckError();

        int mipCount = 1;
        if(spec.genMip && !isMSAA) mipCount = spec.mipLevels;// CalculateMipCount(width, height);



        // --------------------
        // MSAA
        // --------------------

        if(isMSAA){
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
            glCheckError();
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D_MULTISAMPLE, tex, 0);
            glCheckError();
        }
        // --------------------
        // 2D
        // --------------------
        else if(!isArray && !isCube){
            glBindTexture(GL_TEXTURE_2D, tex);
            glCheckError();
            
            #ifdef OpenGL46
            glTexStorage2D(GL_TEXTURE_2D, mipCount, internalFormat, width, height);
            #else
            int w = width;
            int h = height;
            for(int mip = 0; mip < mipCount; mip++){
                glTexImage2D(GL_TEXTURE_2D, mip, internalFormat, w, h, 0, format, dataType, nullptr);
                w = std::max(1, w / 2);
                h = std::max(1, h / 2);
            }
            #endif

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, spec.genMip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, tex, 0);
            glCheckError();
        }
        // --------------------
        // Array
        // --------------------
        else if(isArray){
            glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
            glCheckError();
            
            #ifdef OPENGL46
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipCount, internalFormat, width, height, samples);
            #else
            int w = width;
            int h = height;
            for(int mip = 0; mip < mipCount; mip++){
                glTexImage3D(GL_TEXTURE_2D_ARRAY, mip, internalFormat, w, h, samples, 0, format, dataType, nullptr);
                w = std::max(1, w / 2);
                h = std::max(1, h / 2);
            }
            #endif

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, spec.genMip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, tex, 0, 0);
            glCheckError();
        }
        // --------------------
        // Cubemap
        // --------------------
        else if(isCube){
            Assert(width == height);

            glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
            glCheckError();

            #ifdef OPENGL46
            glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipCount, internalFormat, width, height);
            #else
            int w = width;
            int h = height;
            for(int mip = 0; mip < mipCount; mip++){
                for(int face = 0; face < 6; face++){
                    glTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                        mip, internalFormat, w, h, 0,
                        format, dataType, nullptr
                    );
                }
                w = std::max(1, w / 2);
                h = std::max(1, h / 2);
            }
            #endif

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, spec.genMip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_CUBE_MAP_POSITIVE_X, tex, 0);
            glCheckError();
        }

        fb.glData.colorAttachments.push_back(tex);

        size_t bytes = CalcTextureVRAM(formatEnum, mipCount);
        fb.vramUsage += bytes;
    };

    // -------------------------------------------------------
    // Depth Attachment Lambda
    // -------------------------------------------------------
    auto GenDepthAttachment = [&](){
        if(fb.specification.depthAttachment.colorFormat == FramebufferTextureFormat::None) return;

        auto spec = fb.specification.depthAttachment;
        auto formatEnum = spec.colorFormat;

        Assert(IsDepthTypeFormat(formatEnum));

        GLenum internalFormat = InternalFormatLookup[(int)formatEnum];
        GLenum format = FormatLookup[(int)formatEnum];
        GLenum dataType = DataTypeLookup[(int)formatEnum];
        GLenum attachment = AttachmentLookup[(int)formatEnum];

        /*GLenum internalFormat = GL_DEPTH24_STENCIL8;
        GLenum format         = GL_DEPTH_STENCIL;
        GLenum dataType       = GL_UNSIGNED_INT_24_8;
        GLenum attachment     = GL_DEPTH_STENCIL_ATTACHMENT;

        if(formatEnum == FramebufferTextureFormat::DEPTH_COMPONENT24){
            internalFormat = GL_DEPTH_COMPONENT32F;
            format         = GL_DEPTH_COMPONENT;
            dataType       = GL_FLOAT;
            attachment     = GL_DEPTH_ATTACHMENT;
        }*/

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glCheckError();

        const int width  = fb.specification.width;
        const int height = fb.specification.height;
        const int samples = fb.specification.sample;

        bool isMSAA  = fb.specification.type == FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
        bool isArray = fb.specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY;
        bool isCube  = fb.specification.type == FramebufferAttachmentType::CUBEMAP;

        int mipCount = 1;
        if(spec.genMip && !isMSAA) mipCount = spec.mipLevels; //CalculateMipCount(width, height);

        // --------------------
        // MSAA (no mip possible)
        // --------------------
        if(isMSAA){
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_TRUE);
            glCheckError();

            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, tex, 0);
            glCheckError();
        }
        // --------------------
        // 2D
        // --------------------
        else if(!isArray && !isCube){
            glBindTexture(GL_TEXTURE_2D, tex);
            glCheckError();

            if(spec.genMip){
                #ifdef OPENGL46
                glTexStorage2D(GL_TEXTURE_2D, mipCount, internalFormat, width, height);
                #else
                int w = width;
                int h = height;
                for(int mip = 0; mip < mipCount; mip++) {
                    glTexImage2D(GL_TEXTURE_2D, mip, internalFormat, w, h, 0, format, dataType, nullptr);
                    w = std::max(1, w / 2);
                    h = std::max(1, h / 2);
                }
                #endif
            } else {
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, NULL);
            }

            //TODO: Add Filter options
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, spec.genMip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            float borderColor[] = { 1.0f,1.0f,1.0f,1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, tex, 0);
            glCheckError();
        }
        // --------------------
        // Array
        // --------------------
        else if(isArray){
            glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
            glCheckError();

            #ifdef OPENGL46
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipCount, internalFormat, width, height, samples);
            #else
            int w = width;
            int h = height;
            for(int mip = 0; mip < mipCount; mip++){
                glTexImage3D(GL_TEXTURE_2D_ARRAY, mip, internalFormat, w, h, samples, 0, format, dataType, nullptr);
                w = std::max(1, w / 2);
                h = std::max(1, h / 2);
            }
            #endif

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, spec.genMip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            float borderColor[] = { 1.0f,1.0f,1.0f,1.0f };
            glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

            glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, tex, 0, 0);
            glCheckError();
        }
        // --------------------
        // Cubemap
        // --------------------
        else if(isCube){
            Assert(width == height);

            glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
            glCheckError();

            #ifdef OPENGL46
            glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipCount, internalFormat, width, height);
            #else
            int w = width;
            int h = height;
            for(int mip = 0; mip < mipCount; mip++){
                for(int face = 0; face < 6; face++){
                    glTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                        mip, internalFormat, w, h, 0,
                        format, dataType, nullptr
                    );
                }
                w = std::max(1, w / 2);
                h = std::max(1, h / 2);
            }
            #endif

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, spec.genMip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X, tex, 0);
            glCheckError();
        }

        fb.glData.depthAttachment = tex;

        size_t bytes = CalcTextureVRAM(formatEnum, mipCount);
        fb.vramUsage += bytes;
    };

    // -------------------------------------------------------
    // Generate Attachments
    // -------------------------------------------------------

    for(int i = 0; i < fb.specification.colorAttachments.size(); i++){
        GenColorAttachment(i);
    }
    if(fb.specification.createDepth) GenDepthAttachment();

    // -------------------------------------------------------
    // Draw Buffers
    // -------------------------------------------------------
    if(fb.specification.colorAttachments.size() == 0){
        const GLenum b = GL_NONE;
        //OnDrawAssetsTest();
        glDrawBuffers(1, &b);
        glReadBuffer(GL_NONE);
        glCheckError();
    } else if(fb.specification.colorAttachments.size() > 1){
        std::vector<GLenum> buffers;

        for(int i = 0; i < fb.specification.colorAttachments.size(); i++){
            buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        }

        //OnDrawAssetsTest();
        glDrawBuffers((GLsizei)buffers.size(), buffers.data());
        glCheckError();
    }

    // -------------------------------------------------------
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        LogError("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
        Assert(false);
    }

    
    vram.Add(fb.vramUsage, VRAMTracker::Category::Framebuffer);
    //LogInfo("Framebuffer VRam: {}MB/{}MB", fb.vramUsage / (1024 * 1024), vram.framebuffersBytes / (1024 * 1024));
    //LogInfo("Framebuffer VRam: {:.2f}MB / {:.2f}MB", (double)fb.vramUsage / (1024.0 * 1024.0), (double)vram.framebuffersBytes / (1024.0 * 1024.0));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
    return true;
}

void OpenGLGraphicsDevice::FramebufferDestroy(Framebuffer& frambuffer){
    if(frambuffer.glData.renderId == 0) return;

    //Assert(renderId > 0);
    //LogInfo("Framebuffer::Destroy %d", _framebuffer);
    
    glDeleteFramebuffers(1, &frambuffer.glData.renderId);
    glCheckError();

    for(int i = 0; i < frambuffer.specification.colorAttachments.size(); i++){
        //if(_specification.colorAttachments[i].writeOnly){
        //    glDeleteRenderbuffers(1, &_colorAttachments[i]);
        //} else {
            glDeleteTextures(1, &frambuffer.glData.colorAttachments[i]);
        //}
    }
    glCheckError();

    
    glDeleteTextures(1, &frambuffer.glData.depthAttachment);
    
    glCheckError();

    frambuffer.glData.renderId = 0;
    frambuffer.glData.colorAttachments.clear();
    frambuffer.glData.depthAttachment = 0;

    vram.Free(frambuffer.vramUsage, VRAMTracker::Category::Framebuffer);
    frambuffer.vramUsage = 0;
}

bool OpenGLGraphicsDevice::FramebufferIsValid(Framebuffer& frambuffer){
    return frambuffer.glData.renderId > 0;
}

void OpenGLGraphicsDevice::FramebufferGenMipmap(Framebuffer& fb){
    bool isMSAA = fb.specification.type == FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
    if(isMSAA) return;

    bool isArray = fb.specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    bool isCube  = fb.specification.type == FramebufferAttachmentType::CUBEMAP;

    GLenum target = GL_TEXTURE_2D;
    if(isArray) target = GL_TEXTURE_2D_ARRAY;
    if(isCube)  target = GL_TEXTURE_CUBE_MAP;

    // Color
    for(size_t i = 0; i < fb.glData.colorAttachments.size(); i++){
        if(!fb.specification.colorAttachments[i].genMip) continue;

        glBindTexture(target, fb.glData.colorAttachments[i]);
        glGenerateMipmap(target);
        glCheckError();
    }

    // Depth INFO: this is not right If Hi-Z need to make manualy:
    if(fb.specification.createDepth && fb.specification.depthAttachment.genMip && fb.glData.depthAttachment){
        glBindTexture(target, fb.glData.depthAttachment);
        glGenerateMipmap(target);
        glCheckError();
    }

    glCheckError();
}

void* OpenGLGraphicsDevice::FramebufferColorAttachmentId(Framebuffer& framebuffer, int index){
    Assert(index < framebuffer.glData.colorAttachments.size());
    return (void*)(uint64_t)framebuffer.glData.colorAttachments[index]; 
}

void* OpenGLGraphicsDevice::FramebufferDepthAttachmentId(Framebuffer& framebuffer){
    return (void*)(uint64_t)framebuffer.glData.depthAttachment;
}

int OpenGLGraphicsDevice::FramebufferReadPixel(Framebuffer& frambuffer, int attachmentIndex, int x, int y){
    if(frambuffer.IsValid() == false) return 0;

    glBindFramebuffer(GL_FRAMEBUFFER, frambuffer.glData.renderId);

    Assert(attachmentIndex < frambuffer.glData.colorAttachments.size());

    glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
    glCheckError();

    int pixelData;
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
    
    return pixelData;
}

const unsigned int TextureFilterLookup[] = {
    GL_NEAREST,
    GL_LINEAR
};

const unsigned int TextureFilterLookupMipmap[] = {
    GL_NEAREST_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_LINEAR
};

const unsigned int TextureWrappingLookupMipmap[] = {
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER
};

const unsigned int TextureFormatLookupMipmap[] = {
    GL_NONE,
    GL_RGB,
    GL_RGBA,
    //GL_RGB,
    //GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,
};

const unsigned int TextureInternalFormatLookupMipmap[] = {
    GL_NONE,
    GL_RGB,
    GL_RGBA,
    //GL_SRGB,
    //GL_SRGB_ALPHA,

    GL_R8,
    GL_RGB8,
    GL_RGBA8,

    GL_R16,
    GL_RGB16,
    GL_RGBA16,

    GL_R16F,
    GL_RGB16F,
    GL_RGBA16F,

    GL_R32F,
    GL_RGB32F,
    GL_RGBA32F,
};

const unsigned int TextureDataTypeFormatLookupMipmap[] = {
    GL_UNSIGNED_BYTE,
    GL_UNSIGNED_INT,
    GL_INT,
    GL_FLOAT
};

void LoadSettings(const char* filePath, Texture2DSetting& settings){
    std::ifstream stream(std::string(filePath) + ".meta");
    if(stream.fail()) return;

    cereal::JSONInputArchive archive{stream};
    archive(CEREAL_NVP(settings));
}

void OpenGLGraphicsDevice::Texture2DGenerate(Texture2D& tex, unsigned int inWidth, unsigned int inHeight, TextureDataType dataType, void* data){
    tex.width = inWidth;
    tex.height = inHeight;
    
    glGenTextures(1, &tex.glData.id);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, tex.glData.id);
    glTexImage2D(GL_TEXTURE_2D, 0, tex.glData.internalFormat, tex.width, tex.height, 0, tex.glData.imageFormat, TextureDataTypeFormatLookupMipmap[(int)dataType], data);
    glCheckError();
    
    if(tex.mipmap == true) glGenerateMipmap(GL_TEXTURE_2D);
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex.glData.wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex.glData.wrapT);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex.glData.filterMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex.glData.filterMax);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
}

/*
bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, const std::string path){
    Texture2DDestroy(tex);

    tex.path = path;// std::string(path);
    //tex.settings = settings;
    LoadSettings(tex.path.c_str(), tex.settings);
    tex.glData.wrapS = TextureWrappingLookupMipmap[(int)tex.settings.wrap]; //GL_REPEAT;
    tex.glData.wrapT = TextureWrappingLookupMipmap[(int)tex.settings.wrap]; //GL_REPEAT;
    tex.glData.filterMin = TextureFilterLookup[(int)tex.settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(tex.settings.mipmap){
        tex.glData.filterMin = TextureFilterLookupMipmap[(int)tex.settings.filter];
    }
    tex.glData.filterMax = TextureFilterLookup[(int)tex.settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    tex.mipmap = tex.settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load(tex.path.c_str(), &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", tex.path.c_str(), stbi_failure_reason());
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(tex.settings.textureFormat == TextureFormat::Auto){
        if(alpha){
            tex.glData.internalFormat = GL_RGBA; //GL_SRGB_ALPHA; //GL_RGBA;
            tex.glData.imageFormat = GL_RGBA;
        } else {
            tex.glData.internalFormat = GL_RGB; //GL_SRGB_ALPHA; //GL_RGBA;
            tex.glData.imageFormat = GL_RGB;
        }
    } else {
        tex.glData.internalFormat = TextureInternalFormatLookupMipmap[(int)tex.settings.textureFormat];
        tex.glData.imageFormat = TextureFormatLookupMipmap[(int)tex.settings.textureFormat];
    }
    
    Texture2DGenerate(tex, width, height, TextureDataType::UnsignedByte, data);
    stbi_image_free(data);
    tex.isComplete = true;
    return true;
}

bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size){
    Texture2DDestroy(tex);

    //tex.path = "Memory";
    //tex.settings = settings;
    tex.glData.wrapS = TextureWrappingLookupMipmap[(int)tex.settings.wrap]; //GL_REPEAT;
    tex.glData.wrapT = TextureWrappingLookupMipmap[(int)tex.settings.wrap]; //GL_REPEAT;
    tex.glData.filterMin = TextureFilterLookup[(int)tex.settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(tex.settings.mipmap){
        tex.glData.filterMin = TextureFilterLookupMipmap[(int)tex.settings.filter];
    }
    tex.glData.filterMax = TextureFilterLookup[(int)tex.settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    tex.mipmap = tex.settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* _data = stbi_load_from_memory((const stbi_uc*)data, size, &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!_data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", tex.path.c_str(), stbi_failure_reason());
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(tex.settings.textureFormat == TextureFormat::Auto){
        if(alpha){
            tex.glData.internalFormat = GL_RGBA; //GL_SRGB_ALPHA; //GL_RGBA;
            tex.glData.imageFormat = GL_RGBA;
        } else {
            tex.glData.internalFormat = GL_RGB; //GL_SRGB_ALPHA; //GL_RGBA;
            tex.glData.imageFormat = GL_RGB;
        }
    } else {
        tex.glData.internalFormat = TextureInternalFormatLookupMipmap[(int)tex.settings.textureFormat];
        tex.glData.imageFormat = TextureFormatLookupMipmap[(int)tex.settings.textureFormat];
    }

    Texture2DGenerate(tex, width, height, TextureDataType::UnsignedByte, _data);
    stbi_image_free(_data);
    tex.isComplete = true;
    return true;
}*/

bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, int width, int height, TextureDataType dataType){
    Texture2DDestroy(tex);
    glCheckError();

    if(tex.settings.textureFormat == TextureFormat::None){
        LogError("Texture2DCreate: textureFormat invalid");
        return false;
    }

    //tex.path = "Memory";
    tex.settings = tex.settings;
    tex.glData.wrapS = TextureWrappingLookupMipmap[(int)tex.settings.wrap]; //GL_REPEAT;
    tex.glData.wrapT = TextureWrappingLookupMipmap[(int)tex.settings.wrap]; //GL_REPEAT;
    tex.glData.filterMin = TextureFilterLookup[(int)tex.settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(tex.settings.mipmap){
        tex.glData.filterMin = TextureFilterLookupMipmap[(int)tex.settings.filter];
    }
    tex.glData.filterMax = TextureFilterLookup[(int)tex.settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    tex.mipmap = tex.settings.mipmap;

    tex.glData.internalFormat = TextureInternalFormatLookupMipmap[(int)tex.settings.textureFormat];
    tex.glData.imageFormat = TextureFormatLookupMipmap[(int)tex.settings.textureFormat];
    Texture2DGenerate(tex, width, height, dataType, data);
    tex.isComplete = true;

    //--------Compute VRam Usage----------
    size_t bytesPerPixel = 0;
    switch(tex.glData.internalFormat){
        case GL_RGBA: case GL_RGBA8: case GL_SRGB8_ALPHA8:   bytesPerPixel = 4; break;
        case GL_RGB: case GL_RGB8:  case GL_SRGB8:          bytesPerPixel = 3; break;
        case GL_RGB16F:                             bytesPerPixel = 6; break;
        case GL_RGBA16F:                            bytesPerPixel = 8; break;
        case GL_RGB32F:                             bytesPerPixel = 12; break;
        case GL_RGBA32F:                            bytesPerPixel = 16; break;
        case GL_R8:                                 bytesPerPixel = 1; break;
        case GL_RG16F:                              bytesPerPixel = 4; break;
        case GL_R11F_G11F_B10F:                     bytesPerPixel = 4; break;
        default:
            bytesPerPixel = 4; // conservative fallback
            LogWarning("Unknown internal format for VRAM estimation: {}", tex.glData.internalFormat);
    }
    size_t baseBytes = (size_t)tex.width * tex.height * bytesPerPixel;
    size_t totalBytes = baseBytes;
    if(tex.mipmap) totalBytes += baseBytes / 3;// mip chain ≈ +33%

    vram.Add(totalBytes, VRAMTracker::Category::Texture);
    tex.vramUsage = totalBytes;
    //LogInfo("Textures VRam: {}MB/{}MB", tex.vramUsage / (1024 * 1024) ,vram.texturesBytes / (1024 * 1024));

    return true;
}

void OpenGLGraphicsDevice::Texture2DDestroy(Texture2D& tex){
    if(tex.IsValid() == false) return;

    glDeleteTextures(1, &tex.glData.id);
    tex.glData.id = 0;
    glCheckError();

    tex.isComplete = false;

    vram.Free(tex.vramUsage, VRAMTracker::Category::Texture);
    tex.vramUsage = 0;
}

bool OpenGLGraphicsDevice::Texture2DIsValid(Texture2D& tex){
    return tex.glData.id != 0;
}

void* OpenGLGraphicsDevice::Texture2DRenderId(Texture2D& tex){
    //return (void*)(uint64_t)tex.glData.id;
    return (void*)(intptr_t)tex.glData.id;
}

bool OpenGLGraphicsDevice::Texture2DGetPixelData(Texture2D& tex, std::vector<uint8_t>& outData){ 
    if(Texture2DIsValid(tex) == false) return false;

    if(!(tex.settings.textureFormat == TextureFormat::RGBA || tex.settings.textureFormat == TextureFormat::RGB)){
        LogError("Texture format dont Supported");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, tex.glData.id);
    glCheckError();

    if(tex.settings.textureFormat == TextureFormat::RGBA){
        outData.resize(tex.width*tex.height*4);// std::vector<unsigned char> pixels(tex.width*tex.height*4);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, outData.data());
        glCheckError();
    } else {
        outData.resize(tex.width*tex.height*3);// std::vector<unsigned char> pixels(tex.width*tex.height*4);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, outData.data());
        glCheckError();
    }

    return true;
}

bool OpenGLGraphicsDevice::Texture2DArrayCreate(Texture2DArray& tex, const std::vector<std::string>& filePaths){
    stbi_set_flip_vertically_on_load(1);

    glGenTextures(1, &tex.glData.id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.glData.id);
    glCheckError();

    std::vector<unsigned char*> datas;
    
    int width, height, nrComponents;
    unsigned int internalFormat = GL_RGB8;
    unsigned int imageFormat = GL_RGB;
    unsigned int mipLevelCount = 8;
    for(auto& i: filePaths){
        unsigned char* data = stbi_load(i.c_str(), &width, &height, &nrComponents, 0);   // Load the first Image of size 512   X   512  (RGB))

        if(!data){
            LogError("Cannot load file image {}\nSTB Reason: {}\n", tex.path, stbi_failure_reason());
        }

        if(nrComponents > 3){
            internalFormat = GL_RGBA8; //GL_SRGB_ALPHA; //GL_RGBA;
            imageFormat = GL_RGBA;
            //mipLevelCount = 1;
        }

        datas.push_back(data);
    }

    #if OpenGLVersion == 4
    Assert(false);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, internalFormat, width, height, filePaths.size());
    glCheckError();
    #endif

    #if OpenGLVersion == 3
    //Fixme: make this complatible with opengl 3.3
    Assert(false);
    #endif
    
    int _i = 0;
    for(auto i: datas){
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, _i, width, height, 1, imageFormat, GL_UNSIGNED_BYTE, i);
        glCheckError();
        _i += 1;
    }

    if(mipLevelCount > 1) glGenerateMipmap(GL_TEXTURE_2D_ARRAY); 
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glCheckError();

    for(auto i: datas){
        stbi_image_free(i);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glCheckError();
    return true;
}

void OpenGLGraphicsDevice::Texture2DArrayDestroy(Texture2DArray& tex){
    if(Texture2DArrayIsValid(tex) == false) return;

    glDeleteTextures(1, &tex.glData.id);
    tex.glData.id = 0;
    glCheckError();
}

bool OpenGLGraphicsDevice::Texture2DArrayIsValid(Texture2DArray& tex){
    return tex.glData.id != 0;
}

Ref<Texture2D> OpenGLGraphicsDevice::Texture2DCreateBrdfLUTTexture2D(){
    Assert(Graphics::HasBegin() == false);

    // pbr: setup framebuffer
    // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glCheckError();
    
    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);
    glCheckError();

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);
    glCheckError();

    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCheckError();

    DrawMesh(*fullScreenQuad, *brdfMat, Matrix4Identity);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);

    Ref<Texture2D> out = CreateRef<Texture2D>();
    out->glData.id = brdfLUTTexture;
    out->width = 512;
    out->height = 512;
    return out;
}

bool OpenGLGraphicsDevice::CubemapCreateFromFile(
    Cubemap& cubemap,
    const char* right, const char* left, const char* top,
    const char* bottom, const char* front, const char* back
){
    CubemapDestroy(cubemap);

    cubemap.mipmap = true;
    std::vector<const char*> faces;
    faces.push_back(right);
    faces.push_back(left);
    faces.push_back(top);
    faces.push_back(bottom);
    faces.push_back(front);
    faces.push_back(back);

    glGenTextures(1, &cubemap.glData.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.glData.id);
    glCheckError();

    stbi_set_flip_vertically_on_load(0);

    int width, height, nrChannels;
    for(unsigned int i = 0; i < faces.size(); i++){
        unsigned char* data = stbi_load(faces[i], &width, &height, &nrChannels, 0);
        if(data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            glCheckError();
            stbi_image_free(data);
        } else {
            LogError("Cubemap tex failed to load at path: {}", faces[i]);
            //std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }

        LogInfo("Loading Cubemap: {}", faces[i]);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, cubemap.mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if(cubemap.mipmap) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glCheckError();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); 
    glCheckError();
    return true;
}

void OpenGLGraphicsDevice::CubemapDestroy(Cubemap& cubemap){
    if(cubemap.glData.id != 0) glDeleteTextures(1, &cubemap.glData.id);
    cubemap.glData.id = 0;
    glCheckError();
}

bool OpenGLGraphicsDevice::CubemapIsValid(Cubemap& tex){
    return tex.glData.id != 0;
}

glm::mat4 captureProjection2 = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews2[] = {
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

void renderCube(unsigned int& cubeVAO, unsigned int& cubeVBO ){
    float vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        // front face
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        // right face
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
        // bottom face
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        // top face
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
    };

    // initialize (if necessary)
    /*if(cubeVAO == 0){
        
        glGenVertexArrays(1, &cubeVAO);
        glBindVertexArray(cubeVAO);
        //glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    OnDrawAssetsTest();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);*/

    // initialize (if necessary)
    if (cubeVAO == 0){
        glGenBuffers(1, &cubeVBO);

    #ifdef USE_VAO
        glGenVertexArrays(1, &cubeVAO);
        glBindVertexArray(cubeVAO);
    #endif

        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    #ifdef USE_VAO
        // VAO stores attribute state
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    #else
        // no VAO: just unbind buffer after upload (safe here)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    #endif
    }

    // =========================
    // RENDER
    // =========================

#ifdef USE_VAO
    glBindVertexArray(cubeVAO);
#else
    // Rebind everything manually
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

#endif
    OnDrawAssetsTest();
    glDrawArrays(GL_TRIANGLES, 0, 36);

#ifdef USE_VAO
    glBindVertexArray(0);
#else
    // optional cleanup (not strictly required)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

bool OpenGLGraphicsDevice::CubemapCreateFromFileHDR(Cubemap& cubemap, const char* hdri){
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glCheckError();

    // pbr: load the HDR environment map
    // ---------------------------------
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(hdri, &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if(data){
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float
        glCheckError();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glCheckError();

        stbi_image_free(data);
    } else {
        //std::cout << "Failed to load HDR image." << std::endl;
        LogError("Failed to load HDR image.");
        cubemap.glData.id = 0;
        return false;
    }

    // pbr: setup cubemap to render to and attach to framebuffer
    // ---------------------------------------------------------
    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glCheckError();
    for(unsigned int i = 0; i < 6; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        glCheckError();
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    equirectangularToCubemapMat->SetInt("equirectangularMap", 0);
    equirectangularToCubemapMat->SetMatrix4("projection2", captureProjection2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glCheckError();

    unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;

    glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glCheckError();
    for(unsigned int i = 0; i < 6; ++i){
        equirectangularToCubemapMat->SetMatrix4("view2", captureViews2[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCheckError();

        DrawMesh(*_cubeMesh, *equirectangularToCubemapMat, Matrix4Identity);
        //renderCube(cubeVAO, cubeVBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
    // --------------------------------------------------------------------------------
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    glCheckError();
    for (unsigned int i = 0; i < 6; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
        glCheckError();
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
    glCheckError();

    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    irradianceMat->SetInt("environmentMap", 0);
    irradianceMat->SetMatrix4("projection2", captureProjection2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glCheckError();

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glCheckError();

    for(unsigned int i = 0; i < 6; ++i){
        irradianceMat->SetMatrix4("view2", captureViews2[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCheckError();

        DrawMesh(*_cubeMesh, *irradianceMat, Matrix4Identity);
        //renderCube(cubeVAO, cubeVBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    glDeleteTextures(1, &irradianceMap);
    glDeleteTextures(1, &hdrTexture);

    cubemap.glData.id = envCubemap;
    //out->renderId = irradianceMap;
    return true;
}

Ref<Cubemap> OpenGLGraphicsDevice::CubemapCreateFromFileHDR(const char* hdri){
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glCheckError();

    // pbr: load the HDR environment map
    // ---------------------------------
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(hdri, &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if(data){
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float
        glCheckError();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glCheckError();

        stbi_image_free(data);
    } else {
        std::cout << "Failed to load HDR image." << std::endl;
        LogError("Failed to load HDR image.");
        return nullptr;
    }

    // pbr: setup cubemap to render to and attach to framebuffer
    // ---------------------------------------------------------
    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glCheckError();
    for(unsigned int i = 0; i < 6; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        glCheckError();
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    equirectangularToCubemapMat->SetInt("equirectangularMap", 0);
    equirectangularToCubemapMat->SetMatrix4("projection2", captureProjection2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glCheckError();

    unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;

    glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glCheckError();
    for(unsigned int i = 0; i < 6; ++i){
        equirectangularToCubemapMat->SetMatrix4("view2", captureViews2[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCheckError();

        DrawMesh(*_cubeMesh, *equirectangularToCubemapMat, Matrix4Identity);
        //renderCube(cubeVAO, cubeVBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
    // --------------------------------------------------------------------------------
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    glCheckError();
    for (unsigned int i = 0; i < 6; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
        glCheckError();
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
    glCheckError();

    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    irradianceMat->SetInt("environmentMap", 0);
    irradianceMat->SetMatrix4("projection2", captureProjection2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glCheckError();

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glCheckError();

    for(unsigned int i = 0; i < 6; ++i){
        irradianceMat->SetMatrix4("view2", captureViews2[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCheckError();

        DrawMesh(*_cubeMesh, *irradianceMat, Matrix4Identity);
        //renderCube(cubeVAO, cubeVBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    glDeleteTextures(1, &irradianceMap);
    glDeleteTextures(1, &hdrTexture);

    Ref<Cubemap> out = CreateRef<Cubemap>();
    out->glData.id = envCubemap;
    //out->renderId = irradianceMap;
    return out;
}

Ref<Cubemap> OpenGLGraphicsDevice::CubemapCreateIrradianceMapFromCubeMap(const Ref<Cubemap>& cubemap){
    //Assert(Graphics::HasBegin() == false);

    int size = 32*4;

    // pbr: setup framebuffer
    // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glCheckError();

    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
    // --------------------------------------------------------------------------------
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glCheckError();

    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    irradianceMat->SetCubemap("environmentMap", cubemap);
    irradianceMat->SetMatrix4("projection2", captureProjection2);
    
    glViewport(0, 0, size, size); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glCheckError();

    /*unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;*/


    for(unsigned int i = 0; i < 6; ++i){
        irradianceMat->SetMatrix4("view2", captureViews2[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCheckError();

        //renderCube(cubeVAO, cubeVBO);
        DrawMesh(*_cubeMesh, *irradianceMat, Matrix4Identity);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);

    Ref<Cubemap> out = CreateRef<Cubemap>();
    out->glData.id = irradianceMap;
    return out;
}

Ref<Cubemap> OpenGLGraphicsDevice::CubemapCreatePrefilterMapFromCubeMap(const Ref<Cubemap>& cubemap){
    //Assert(Graphics::HasBegin() == false);

     // pbr: setup framebuffer
    // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glCheckError();

    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for(unsigned int i = 0; i < 6; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glCheckError();

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    prefilterMat->SetCubemap("environmentMap", cubemap);
    prefilterMat->SetMatrix4("projection2", captureProjection2);
    /*glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->glData.id);
    glCheckError();*/

    /*unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;*/

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for(unsigned int mip = 0; mip < maxMipLevels; ++mip){
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);
        glCheckError();

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterMat->SetFloat("roughness", roughness);
        for(unsigned int i = 0; i < 6; ++i){
            prefilterMat->SetMatrix4("view2", captureViews2[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            DrawMesh(*_cubeMesh, *prefilterMat, Matrix4Identity);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);

    Ref<Cubemap> out = CreateRef<Cubemap>();
    out->glData.id = prefilterMap;
    return out;
}

#if UseUniformBuffer

// Structure to hold uniform details
/*struct UniformInfo {
    std::string name;
    GLint size;      // Number of elements (for arrays)
    GLint offset;    // Offset in the UBO
    GLenum type;     // Data type (e.g., GL_FLOAT_VEC4)
    GLint arrayStride; // Stride for array elements
};*/

// Function to reflect all uniforms in a block
bool getUniformInfo(GLuint program, const char* blockName, UniformBufferDef& out){
    // Find the uniform block index
    GLuint blockIndex = glGetUniformBlockIndex(program, blockName);
    if(blockIndex == GL_INVALID_INDEX){
        //std::cerr << "Uniform block '" << blockName << "' not found.\n";
        LogWarning("Uniform block {} not found.", blockName);
        return false;
    }

    // Get number of uniforms in the block
    GLint numUniforms;
    glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms);

    if(numUniforms == 0){
        //std::cerr << "Uniform block '" << blockName << "' has no active uniforms.\n";
        LogWarning("Uniform block {} has no active uniforms.", blockName);
        return false;
    }

    GLint totalSize;
    glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &totalSize);
    out.size = totalSize;

    // Get the indices of all uniforms in the block
    std::vector<GLuint> uniformIndices(numUniforms);
    glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, reinterpret_cast<GLint*>(uniformIndices.data()));

    // Get uniform names
    for (int i = 0; i < numUniforms; i++) {
        // Get uniform name length
        GLint nameLength;
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_NAME_LENGTH, &nameLength);

        // Retrieve the name
        std::vector<char> nameBuffer(nameLength);
        GLsizei length;
        glGetActiveUniformName(program, uniformIndices[i], nameLength, &length, nameBuffer.data());

        // Get uniform properties
        //GLint values[4]; // [size, offset, type, arrayStride]
        //GLenum properties[] = {GL_UNIFORM_SIZE, GL_UNIFORM_OFFSET, GL_UNIFORM_TYPE, GL_UNIFORM_ARRAY_STRIDE};
        GLint size, offset, type, arrayStride;
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_SIZE, &size);
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_OFFSET, &offset);
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_TYPE, &type);
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_ARRAY_STRIDE, &arrayStride);

        auto getUniformByteSize = [](GLint size, GLenum type) -> size_t {
            static const std::unordered_map<GLenum, size_t> typeSizeMap = {
                {GL_FLOAT, 4},
                {GL_FLOAT_VEC2, 8},
                {GL_FLOAT_VEC3, 12},
                {GL_FLOAT_VEC4, 16},
                {GL_INT, 4},
                {GL_INT_VEC2, 8},
                {GL_INT_VEC3, 12},
                {GL_INT_VEC4, 16},
                {GL_BOOL, 4},
                {GL_BOOL_VEC2, 8},
                {GL_BOOL_VEC3, 12},
                {GL_BOOL_VEC4, 16},
                {GL_FLOAT_MAT2, 16},
                {GL_FLOAT_MAT3, 36},
                {GL_FLOAT_MAT4, 64},
                // Add other types as needed
            };
        
            auto it = typeSizeMap.find(type);
            if (it != typeSizeMap.end()) {
                return size * it->second;
            } else {
                // Handle unknown type
                Assert(false);
                return 0;
            }
        };

        // Store the uniform info
        /*UniformInfo info;
        info.name = std::string(nameBuffer.data(), length);
        info.size = getUniformByteSize(size, type);// size; //values[0];
        info.offset = offset;// values[1];
        info.type = type;// values[2];
        info.arrayStride = arrayStride;// values[3];
        out.push_back(info);*/

        auto removeZeroSubscript = [](std::string& str) {
            const std::string target = "[0]";
            size_t pos = std::string::npos;
            while ((pos = str.find(target)) != std::string::npos) {
                str.erase(pos, target.length());
            }
        };

        std::string label = std::string(nameBuffer.data(), length);
        removeZeroSubscript(label);

        UniformBufferDef::Member m = {};
        m.pos = offset;
        m.size = getUniformByteSize(size, type);
        m.arrayStride = arrayStride;
        out.members[label] = m;
    }

    return true;
}

#endif

bool OpenGLGraphicsDevice::SubShaderCreateFromBaseSource(
    SubShader& shader,
    std::string& source, std::vector<std::string>& keyworlds,
    ShaderPipeline pipeline, std::vector<std::string>& errors
){
    SubShaderDestroy(shader);
    shader.enabledKeyworlds = keyworlds;
    shader.pipeline = pipeline;

    //std::string vertexToInsert = "#version 330 core\n#define VERTEX\n";
    //std::string fragToInsert = "#version 330 core\n#define FRAGMENT\n";
    
    #if UseUniformBuffer
    std::string vertexToInsert = OpenglHeader "\n#define VERTEX\n#define UseUniformBuffer\n#define OpenGL_API\n";
    std::string fragToInsert = OpenglHeader "\n#define FRAGMENT\n#define UseUniformBuffer\n#define OpenGL_API\n";
    #else
    std::string vertexToInsert = OpenglHeader "\n#define VERTEX\n#define OpenGL_API\n";
    std::string fragToInsert = OpenglHeader "\n#define FRAGMENT\n#define OpenGL_API\n";
    #endif

    auto CompileShader = [&](std::string& baseSource, std::string& toInsert, GLenum type, GLuint program, GLenum& shader) -> bool{
        baseSource.insert(0, toInsert);
        //LogWarning("%s", baseSource.c_str());

        shader = glCreateShader(type);

        const GLchar* sourceCStr = baseSource.c_str();
        glShaderSource(shader, 1, &sourceCStr, 0);
        glCompileShader(shader);
        glCheckError();

        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if(isCompiled == GL_FALSE){
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
            glCheckError();

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
            glCheckError();

            glDeleteShader(shader);
            glCheckError();

            errors.push_back(std::string(&infoLog[0]));

            //printf("%s", infoLog.data());
            //Assert(false && "Shader compilation failure!");
            LogError("Shader compilation failure!");
            LogError("{}", infoLog.data());
            baseSource.erase(0, toInsert.size());
            return false;
        }

        glAttachShader(program, shader);
        glCheckError();
        baseSource.erase(0, toInsert.size());
        return true;
    };

    GLuint program = glCreateProgram();
    glCheckError();
    
    GLenum glShaderIDs[2] = {0, 0};
    //int glShaderIDIndex = 0;

    if(CompileShader(source, vertexToInsert, GL_VERTEX_SHADER, program, glShaderIDs[0]) == false) return false;
    if(CompileShader(source, fragToInsert, GL_FRAGMENT_SHADER, program, glShaderIDs[1]) == false) return false;
    
    shader.glData.id = program;

    // Link our program
    glLinkProgram(program);
    glCheckError();

    // Note the different functions here: glGetProgram* instead of glGetShader*.
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
    glCheckError();

    if (isLinked == GL_FALSE){
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        glCheckError();

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
        glCheckError();

        // We don't need the program anymore.
        glDeleteProgram(program);
        glCheckError();
        
        for(auto id : glShaderIDs){
            glDeleteShader(id);
            glCheckError();
        }

        printf("%s", infoLog.data());
        LogError("Shader link failure!");// LogError("Shader link failure! %s", shader.path.c_str());
        Assert(false && "Shader link failure!");
        return false;
    }

    for(auto id: glShaderIDs){
        if(id == 0) continue;
        glDetachShader(program, id);
        glCheckError();
    }

    glCheckError();

    GLint count;
    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)
    const GLsizei bufSize = 64; // maximum name length
    GLchar name[bufSize]; // variable name in GLSL
    GLsizei length; // name length
    glGetProgramiv(shader.glData.id, GL_ACTIVE_UNIFORMS, &count);
    //printf("Active Uniforms: %d\n", count);
    for(int i = 0; i < count; i++){
        glGetActiveUniform(shader.glData.id, (GLuint)i, bufSize, &length, &size, &type, name);
        
        std::string t = std::string(name);
        std::string s = "[0]";
        std::string::size_type _i = t.find(s);
        if (_i != std::string::npos)
        t.erase(_i, s.length());

        /*GLint location = glGetUniformLocation(shader.glData.id, t.c_str());
        glCheckError();
        shader.glData.uniforms[name] = location;*/

        shader.glData._uniforms.push_back(std::string(t));
        //printf("Uniform #%d Type: %u Name: %s\n", i, type, name);
    }

    // Get uniform buffer block names
    GLint uniformBlockCount = 0;
    glGetProgramiv(shader.glData.id, GL_ACTIVE_UNIFORM_BLOCKS, &uniformBlockCount);

    for (GLuint i = 0; i < static_cast<GLuint>(uniformBlockCount); ++i){
        GLchar blockName[bufSize];
        GLsizei blockNameLength = 0;
        glGetActiveUniformBlockName(shader.glData.id, i, bufSize, &blockNameLength, blockName);
        glCheckError();

        if (blockNameLength > 0){
            shader.glData._uniforms.push_back(std::string(blockName));
        }
    }

    /*UniformBufferDef mainBufferDef;
    if(getUniformInfo(program, "MaterialData", mainBufferDef)){
        LogInfo("--------TotalSize %zd ---------------", mainBufferDef.size);
        for(auto& i: mainBufferDef.members){
           LogInfo("Name: %s", i.first.c_str());
           LogInfo("Pos: %zd", i.second.pos);
           LogInfo("Offset: %zd", i.second.size);
        }
    }*/

    return true;
}

void OpenGLGraphicsDevice::SubShaderDestroy(SubShader& shader){
    //if(shader.IsValid() == false) return;
    //if(shader.glData.id == 0) return;

    glDeleteProgram(shader.glData.id);
    glCheckError();
    shader.glData.id = 0;
}

bool OpenGLGraphicsDevice::SubShaderIsValid(SubShader& shader){
    return shader.glData.id != 0;
}

void OpenGLGraphicsDevice::SubShaderBind(SubShader& shader){
    SetColorMask(shader.pipeline.colorMask);
    SetCullFace(shader.GetCullFace());
    SetDepthTest(shader.GetDepthTest());
    SetDepthMask(shader.IsDepthMask());
    if(shader.IsBlend()){
        SetBlend(true);
        SetBlendFunc(shader.GetSrcBlend(), shader.GetDstBlend());
    } else {
        SetBlend(false);
    }

    glUseProgram(shader.glData.id);
    glCheckError();
    
    stats.shaderBinds += 1;
    debugData.BindShader(&shader);
}

bool OpenGLGraphicsDevice::ShaderCreate(Shader& shader, std::string inPath){
    //LogInfo("Create Shader: %s", inPath.c_str());
    return shader.Create(inPath);
}

void OpenGLGraphicsDevice::ShaderDestroy(Shader& shader){
    shader.Destroy();
}

bool OpenGLGraphicsDevice::MaterialCreate(Material& shader){
    return false;
}

void OpenGLGraphicsDevice::MaterialDestroy(Material& mat){
    if(mat.glData.mainUniformData != nullptr) free(mat.glData.mainUniformData);
    if(mat.glData.mainBuffer != 0) glDeleteBuffers(1, &mat.glData.mainBuffer);
    glCheckError();
}

void OpenGLGraphicsDevice::MaterialOnSetShader(Material& mat){
    #if UseUniformBuffer
    Assert(mat.currentShader.drawTypes[0] != nullptr);

    if(getUniformInfo(mat.currentShader.drawTypes[0]->glData.id, "Main", mat.glData.mainBufferDef)){
        if(mat.glData.mainUniformData != nullptr) free(mat.glData.mainUniformData);
        if(mat.glData.mainBuffer != 0) glDeleteBuffers(1, &mat.glData.mainBuffer);

        mat.glData.mainUniformData = malloc(mat.glData.mainBufferDef.size);
        memset(mat.glData.mainUniformData, 0, mat.glData.mainBufferDef.size);
        glGenBuffers(1, &mat.glData.mainBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, mat.glData.mainBuffer);
        glCheckError();

        /*LogInfo("--------TotalSize %zd ---------------", mat.glData.mainBufferDef.size);
        for(auto& i: mat.glData.mainBufferDef.members){
           LogInfo("Name: %s", i.first.c_str());
           LogInfo("Pos: %zd", i.second.pos);
           LogInfo("Size: %zd", i.second.size);
        }*/
    } else {
        //LogWarning("No Uniform Buffer Main on: {}", mat.shader->Path());
    }
    #endif
}

void OpenGLGraphicsDevice::MaterialOnUnsetShader(Material& shader){}

#pragma region UniformBuffer
bool OpenGLGraphicsDevice::UniformBufferCreate(UniformBuffer& buffer, size_t size){
    glGenBuffers(1, &buffer.glData.id);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer.glData.id);
    glBufferData(
        GL_UNIFORM_BUFFER,
        size,
        nullptr,
        GL_DYNAMIC_DRAW
    );
    glCheckError();
    
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckError();
    return true;
}

void OpenGLGraphicsDevice::UniformBufferDestroy(UniformBuffer& buffer){
    if(buffer.glData.id != 0) glDeleteBuffers(1, &buffer.glData.id);
    buffer.glData.id = 0;
    glCheckError();
}

bool OpenGLGraphicsDevice::UniformBufferIsValid(UniformBuffer& buffer){
    return buffer.glData.id != 0;
}

void OpenGLGraphicsDevice::UniformBufferSetData(UniformBuffer& buffer, const void* data, size_t size, size_t offset){
    Assert(UniformBufferIsValid(buffer) == true);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer.glData.id);
    glCheckError();
    //glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW); //GL_DYNAMIC_DRAW
    glBufferSubData(
        GL_UNIFORM_BUFFER,
        0,
        size,
        data
    );
    glCheckError();
    stats.uniformBufferUpdates += 1;
}
#pragma endregion

#pragma region ComputeBuffer
bool OpenGLGraphicsDevice::ComputeBufferCreate(ComputeBuffer& buffer, size_t size){
    #ifdef OpenGL46 
    glGenBuffers(1, &buffer.glData.id);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.glData.id);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        size,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    buffer.glData.size = size;
    buffer.vramUsage = size;

    return true;
    #else
    return false;
    #endif
}

void OpenGLGraphicsDevice::ComputeBufferDestroy(ComputeBuffer& buffer){
    #ifdef OpenGL46
    if(buffer.glData.id != 0) glDeleteBuffers(1, &buffer.glData.id);
    buffer.glData.id = 0;
    #endif
}

bool OpenGLGraphicsDevice::ComputeBufferIsValid(ComputeBuffer& buffer){ 
    return buffer.glData.id != 0;
}

void OpenGLGraphicsDevice::ComputeBufferSetData(ComputeBuffer& buffer, const void* data, unsigned int size, unsigned int offset){
    #ifdef OpenGL46
    Assert(buffer.glData.id != 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.glData.id);

    glBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        offset,
        size,
        data
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    #endif
}

void OpenGLGraphicsDevice::ComputeBufferGetData(ComputeBuffer& buffer, void* data, unsigned int size, unsigned int offset){
    #ifdef OpenGL46
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.glData.id);

    glGetBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        offset,
        size,
        data
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    #endif
}
#pragma endregion

#pragma region ComputeShader
bool OpenGLGraphicsDevice::ComputeShaderCreate(ComputeShader& shader, const std::string& source){
    #ifdef OpenGL46
    GLuint _shader = glCreateShader(GL_COMPUTE_SHADER);

    const char* src = source.c_str();
    glShaderSource(_shader, 1, &src, nullptr);
    glCompileShader(_shader);

    GLint success;
    glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);

    if (!success){
        char infoLog[1024];
        glGetShaderInfoLog(_shader, 1024, nullptr, infoLog);

        LogError("Compute shader compile error:\n{}", infoLog);
        glDeleteShader(_shader);
        return false;
    }

    shader.glData.id = glCreateProgram();
    glAttachShader(shader.glData.id, _shader);
    glLinkProgram(shader.glData.id);

    glGetProgramiv(shader.glData.id, GL_LINK_STATUS, &success);

    if(!success){
        char infoLog[1024];
        glGetProgramInfoLog(shader.glData.id, 1024, nullptr, infoLog);

        LogError("Compute shader link error:\n{}", infoLog);
        glDeleteShader(_shader);
        return false;
    }

    glDeleteShader(_shader);

    return true;
    #else
    return false;
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderDestroy(ComputeShader& shader){
    #ifdef OpenGL46
    if(shader.glData.id) glDeleteProgram(shader.glData.id);
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderDispatch(ComputeShader& shader, uint32_t x, uint32_t y, uint32_t z){
    #ifdef OpenGL46
    glUseProgram(shader.glData.id);
    glDispatchCompute(x, y, z);
    glMemoryBarrier(
        GL_ALL_BARRIER_BITS
        //GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT
    );

    shader.glData.textureSlot = 0;
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderSetTexture(ComputeShader& shader, const char* name, Ref<Texture2D> tex){
    #ifdef OpenGL46
    glUseProgram(shader.glData.id);

    GLint loc = GetUniformLocation(shader, name);

    glActiveTexture(GL_TEXTURE0 + shader.glData.textureSlot);
    glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)tex->RenderId());

    glUniform1i(loc, shader.glData.textureSlot);

    shader.glData.textureSlot++;
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderSetTexture(ComputeShader& shader, const char* name, Framebuffer* fb, int attachment){
    #ifdef OpenGL46
    glUseProgram(shader.glData.id);

    if(attachment < 0){//TODO: This is just temp, create later SetTexture, and SetImage for write
        GLint loc = GetUniformLocation(shader, name);
        GLuint tex = fb->glData.depthAttachment;

        glActiveTexture(GL_TEXTURE0 + shader.glData.textureSlot);
        glBindTexture(GL_TEXTURE_2D, tex);

        glUniform1i(loc, shader.glData.textureSlot);
    } else {
        GLuint tex = fb->glData.colorAttachments[attachment];// (GLuint)(uintptr_t)fb->ColorAttachmentId(attachment);
        GLenum format = InternalFormatLookup[(int)fb->specification.colorAttachments[attachment].colorFormat]; //FramebufferFormatToGL(spec.format);

        //Assert(format == GL_RGBA16F);

        glBindImageTexture(
            shader.glData.textureSlot,
            tex,
            0,
            GL_FALSE,
            0,
            GL_READ_WRITE,
            format //GL_RGBA16F
        );
    }

    shader.glData.textureSlot++;
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderSetUniformBuffer(ComputeShader& shader, const char* name, Ref<UniformBuffer> buffer, int bind){
    #ifdef OpenGL46
    glBindBufferBase(GL_UNIFORM_BUFFER, bind, buffer->glData.id);
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderSetComputeBuffer(ComputeShader& shader, const char* name, Ref<ComputeBuffer> buffer, int bind){
    #ifdef OpenGL46
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind, buffer->glData.id);
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderSetInt(ComputeShader& shader, const char* name, int v){
    #ifdef OpenGL46
    glUseProgram(shader.glData.id);
    glUniform1i(GetUniformLocation(shader, name), v);
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderSetFloat(ComputeShader& shader, const char* name, float v){
    #ifdef OpenGL46
    glUseProgram(shader.glData.id);
    glUniform1f(GetUniformLocation(shader, name), v);
    #endif
}

void OpenGLGraphicsDevice::ComputeShaderSetVector4(ComputeShader& shader, const char* name, Vector4 v){
    #ifdef OpenGL46
    glUseProgram(shader.glData.id);
    glUniform4f(
        GetUniformLocation(shader, name),
        v.x, v.y, v.z, v.w
    );
    #endif
}

bool OpenGLGraphicsDevice::ComputeShaderIsValid(ComputeShader& shader){
    return shader.glData.id != 0;
}   

GLint OpenGLGraphicsDevice::GetUniformLocation(ComputeShader& shader, const char* name){
    #ifdef OpenGL46
    auto it = shader.glData.uniformCache.find(name);
    if(it != shader.glData.uniformCache.end()) return it->second;

    GLint location = glGetUniformLocation(shader.glData.id, name);
    shader.glData.uniformCache[name] = location;

    return location;
    #else
    return 0;
    #endif
}

bool OpenGLGraphicsDevice::SupportCompute(){
    #ifdef OpenGL46
    return true;
    #endif 
    return false; 
}

#pragma endregion

#pragma region Messure
/*static GLuint g_gpuQuery = 0;
void OpenGLGraphicsDevice::BeginGPUTime(){
    if(g_gpuQuery == 0) glGenQueries(1, &g_gpuQuery);
    glBeginQuery(GL_TIME_ELAPSED, g_gpuQuery);
}

double OpenGLGraphicsDevice::EndGPUTime(){
    glEndQuery(GL_TIME_ELAPSED);

    GLuint64 time = 0;
    glGetQueryObjectui64v(g_gpuQuery, GL_QUERY_RESULT, &time);

    return time / 1000000.0; // milliseconds
}*/

//More Fast
static const int QUERY_COUNT = 4;
static GLuint g_gpuQueries[QUERY_COUNT] = {};
static bool g_queryUsed[QUERY_COUNT] = {};
static int g_gpuQueryIndex = 0;
static double g_lastTimeMs = 0.0;
static bool g_initialized = false;

void OpenGLGraphicsDevice::BeginGPUTime(){
    if(!g_initialized){
        glGenQueries(QUERY_COUNT, g_gpuQueries);
        g_initialized = true;
    }

    glBeginQuery(GL_TIME_ELAPSED, g_gpuQueries[g_gpuQueryIndex]);
}

double OpenGLGraphicsDevice::EndGPUTime(){
    glEndQuery(GL_TIME_ELAPSED);

    // mark current query as valid
    g_queryUsed[g_gpuQueryIndex] = true;

    int readIndex = (g_gpuQueryIndex + 1) % QUERY_COUNT;

    // Only read if this query was actually used before
    if(g_queryUsed[readIndex]){
        GLuint available = 0;
        glGetQueryObjectuiv(g_gpuQueries[readIndex], GL_QUERY_RESULT_AVAILABLE, &available);

        if(available){
            GLuint64 time = 0;
            glGetQueryObjectui64v(g_gpuQueries[readIndex], GL_QUERY_RESULT, &time);
            g_lastTimeMs = time / 1000000.0;
        }
    }

    g_gpuQueryIndex = (g_gpuQueryIndex + 1) % QUERY_COUNT;
    return g_lastTimeMs;
}
#pragma endregion

#pragma region ImGui
bool OpenGLGraphicsDevice::ImGuiSupport(){
    return true;
}

void OpenGLGraphicsDevice::ImGuiNewFrame(){
    ImGui_ImplOpenGL3_NewFrame();
}

void OpenGLGraphicsDevice::ImGuiRenderDrawData(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
    ImVec4 _clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    glViewport(0, 0, w, h);
    glCheckError();
    if(ImGuiLayer::GetCleanAll() == true){
        glClearColor(
            _clear_color.x,// * _clear_color.w, 
            _clear_color.y,// * _clear_color.w, 
            _clear_color.z,// * _clear_color.w, 
            _clear_color.w
        );
        glCheckError();
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckError();
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
#pragma endregion

}
#endif