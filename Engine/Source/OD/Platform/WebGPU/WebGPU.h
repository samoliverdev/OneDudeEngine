#pragma once
#if defined(WEBGPU_SUPPORT)

#include <webgpu/webgpu.h>

struct WGMeshData{
    WGPUBuffer vertexBuffer = nullptr;
    WGPUBuffer uvBuffer = nullptr;

    /*WGPUBuffer uvBuffer;
    WGPUBuffer normalVbo;
    WGPUBuffer colorVbo;
    WGPUBuffer tangentVbo;
    WGPUBuffer instancingModelMatrixsVbo;
    WGPUBuffer jointVbo = 0; 
    WGPUBuffer weightsVbo = 0;  
    unsigned int ebo = 0;*/

    WGPUBuffer indexBuffer = nullptr;
};

struct WGFramebufferData{
    
};

struct WGTexture2DData{
    
};

struct WGTexture2DArrayData{
    
};

struct WGCubemapData{
    
};

struct WGSubShaderData{
    WGPUShaderModule shaderModuleVertex = nullptr;
    WGPUShaderModule shaderModuleFrag = nullptr;
    WGPURenderPipeline pipeline = nullptr;
};

struct WGShaderData{
    
};

struct WGMaterialData{

};

#define MeshDataWG WGMeshData wgData;
#define FramebufferDataWG WGFramebufferData wgData;
#define Texture2DDataWG WGTexture2DData wgData;
#define Texture2DArrayDataWG WGTexture2DArrayData wgData;
#define CubemapDataWG WGCubemapData wgData;
#define SubShaderDataWG WGSubShaderData wgData;
#define ShaderDataWG WGShaderData wgData;
#define MaterialDataWG WGMaterialData wgData;


#else

#define MeshDataWG
#define FramebufferDataWG
#define Texture2DDataWG
#define Texture2DArrayDataWG
#define CubemapDataWG
#define SubShaderDataWG
#define ShaderDataWG
#define MaterialDataWG

#endif