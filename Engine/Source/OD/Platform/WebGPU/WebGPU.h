#pragma once
#if defined(WEBGPU_SUPPORT)

#include <webgpu/webgpu.h>
#include "OD/Platform/Spirv.h"
#include <vector>

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
    WGPUTexture texture = nullptr;
    WGPUTextureView textureView = nullptr;
    WGPUTexture depthTexture = nullptr;
    WGPUTextureView depthTextureView = nullptr;
    WGPUSampler sampler = nullptr;
};

struct WGTexture2DData{
    WGPUTexture texture = nullptr;
    WGPUTextureView textureView = nullptr;
    WGPUSampler sampler = nullptr;
};

struct WGTexture2DArrayData{
    
};

struct WGCubemapData{
    
};

struct WGSubShaderData{
    WGPUShaderModule shaderModuleVertex = nullptr;
    WGPUShaderModule shaderModuleFrag = nullptr;
    WGPUPipelineLayout layout = nullptr;
    //WGPURenderPipeline pipeline = nullptr;
    std::vector<WGPURenderPipeline> pipelines;
    WGPUBindGroupLayout bindGroupLayout = nullptr;
    MaterialMainSetDef materialMainSetDef;
};

struct WGShaderData{
    
};

struct WGMaterialData{
    MaterialMainSetDef materialMainSetDef;

    WGPUBuffer mainUniformBuffer = nullptr;
    void* mainUniformData = nullptr;
    WGPUBindGroup mainBindGroup = nullptr;
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