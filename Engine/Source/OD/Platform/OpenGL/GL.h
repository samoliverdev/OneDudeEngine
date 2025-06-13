#pragma once
#if defined(OPENGL_SUPPORT)

#ifdef __EMSCRIPTEN__
#define OpenGLEmscripten
#else
#define OpenGL46
//#define OpenGL33
#endif

#if defined(OpenGL46)
    //#include <glad46core/glad.h>
    #include <glad.h>
    #define OpenGLVersion 4
    #define OPENGL_CHECK_ERRORS 1
    #define OpenglHeader "#version 330 core"
#endif
#if defined(OpenGL33)
    #include <glad33Core/glad.h>
    #define OpenGLVersion 3
    #define OPENGL_CHECK_ERRORS 1
    #define OpenglHeader "#version 330 core"
 #endif
#if defined(OpenGLEmscripten)
    #include <emscripten.h>
    #include <GL/gl.h>
    #include <GLES3/gl3.h>
    #define OpenglHeader "#version 300 es"
    //#define GL_GLEXT_PROTOTYPES
    //#define EGL_EGLEXT_PROTOTYPES
    #define OpenGLVersion 3
    #define OPENGL_CHECK_ERRORS 1
#endif

#define USE_VAO 1

#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

struct GLMeshData{
    //#ifdef USE_VAO
    unsigned int vao = 0;
    //#endif

    unsigned int vertexVbo = 0;
    unsigned int uvVbo = 0;
    unsigned int normalVbo = 0;
    unsigned int colorVbo = 0;
    unsigned int tangentVbo = 0;

    unsigned int instancingModelMatrixsVbo = 0;
    
    unsigned int jointVbo = 0; 
    unsigned int weightsVbo = 0;  
    
    unsigned int ebo = 0;

    void* instancingMappedPtr = nullptr;
    size_t instancingBufferCapacity = 0;
};

struct GLFramebufferData{
    unsigned int renderId = 0;
    unsigned int depthAttachment = 0;
    std::vector<unsigned int> colorAttachments;
};

struct GLTexture2DData{
    unsigned int id = 0;
    unsigned int internalFormat = 0;
    unsigned int imageFormat = 0;
    unsigned int wrapS = 0;
    unsigned int wrapT = 0;
    unsigned int filterMin = 0;
    unsigned int filterMax = 0;
};

struct GLTexture2DArrayData{
    unsigned int id = 0;
};

struct GLCubemapData{
    unsigned int id = 0;
};

struct GLSubShaderData{
    unsigned int id = 0;
    std::unordered_map<std::string, int> uniforms;
    std::vector<std::string> _uniforms;

    std::unordered_map<std::string, int> uniformsBuffers;
};

struct GLShaderData{

};

struct GLUniformBufferData{
    unsigned int id = 0;
};

struct UniformBufferDef{
    struct Member{
        size_t pos = 0;
        size_t size = 0;
        size_t arrayStride = 0; // <- novo campo
    };
    std::string name;
    size_t size = 0;
    std::unordered_map<std::string, Member> members{};
};

struct GLMaterialData{
    UniformBufferDef mainBufferDef{};
    void* mainUniformData = nullptr;
    unsigned int mainBuffer = 0;
};

#define MeshDataGL GLMeshData glData;
#define FramebufferDataGL GLFramebufferData glData;
#define Texture2DDataGL GLTexture2DData glData;
#define Texture2DArrayDataGL GLTexture2DArrayData glData;
#define CubemapDataGL GLCubemapData glData;
#define SubShaderDataGL GLSubShaderData glData;
#define ShaderDataGL GLShaderData glData;
#define MaterialDataGL GLMaterialData glData{};
#define UniformBufferDataGL GLUniformBufferData glData{};

int glCheckError_(const char *file, int line, std::function<void()> callback = nullptr);

#if OPENGL_CHECK_ERRORS
    #define glCheckError() glCheckError_(__FILE__, __LINE__)
    #define glCheckError2(...) glCheckError_(__FILE__, __LINE__, __VA_ARGS__)
#else
    #define glCheckError()
    #define glCheckError2(...)
#endif

#else

#define MeshDataGL
#define FramebufferDataGL
#define Texture2DDataGL
#define Texture2DArrayDataGL
#define CubemapDataGL
#define SubShaderDataGL
#define ShaderDataGL 
#define MaterialDataGL
#define UniformBufferDataGL

#endif