#pragma once
#if defined(WEBGPU_SUPPORT)

#include <webgpu/webgpu.h>
#include "OD/Graphics/RendererTypes.h"

namespace OD{

struct WGMeshData{
    WGPUBuffer vertexBuffer = nullptr;
    WGPUBuffer uvBuffer = nullptr;
    WGPUBuffer normalBuffer = nullptr;
    WGPUBuffer colorBuffer = nullptr;
    WGPUBuffer tangentBuffer = nullptr;

    WGPUBuffer jointBuffer = nullptr; 
    WGPUBuffer weightsBuffer = nullptr;  

    /*
    WGPUBuffer instancingModelMatrixsVbo;
    */

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
    WGPUPipelineLayout layout = nullptr;
    WGPUBindGroupLayout bindGroupLayout = nullptr;
    UniformBufferDef mainUnformDef;
};

struct WGShaderData{
    
};

struct WGMaterialData{
    WGPUBuffer mainUniformBuffer;
    UniformBufferDef mainUnformDef;
    void* mainUniformData = nullptr;
    WGPUBindGroup mainBindGroup;
};

}

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