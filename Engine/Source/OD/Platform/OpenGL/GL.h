#pragma once
#if defined(OPENGL_SUPPORT)

#include <glad/glad.h>
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

#define OPENGL_CHECK_ERRORS 1
#define USE_VAO 1

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
};

struct GLFramebufferData{
    unsigned int renderId = 0;
    unsigned int depthAttachment = 0;
    std::vector<unsigned int> colorAttachments;
};

struct GLTexture2DData{
    unsigned int id = 0;
    unsigned int internalFormat;
    unsigned int imageFormat;
    unsigned int wrapS;
    unsigned int wrapT;
    unsigned int filterMin;
    unsigned int filterMax;
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
};

struct GLShaderData{

};

struct GLMaterialData{

};

#define MeshDataGL GLMeshData glData;
#define FramebufferDataGL GLFramebufferData glData;
#define Texture2DDataGL GLTexture2DData glData;
#define Texture2DArrayDataGL GLTexture2DArrayData glData;
#define CubemapDataGL GLCubemapData glData;
#define SubShaderDataGL GLSubShaderData glData;
#define ShaderDataGL GLShaderData glData;
#define MaterialDataGL GLMaterialData glData;

GLenum glCheckError_(const char *file, int line, std::function<void()> callback = nullptr);

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

#endif