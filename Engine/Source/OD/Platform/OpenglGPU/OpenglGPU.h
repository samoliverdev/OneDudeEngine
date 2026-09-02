#pragma once
#include "OD/GPU/GPU.h"
#include "OD/Graphics/GraphicsDevice.h"

namespace OD{

class OpenglGPUDevice: public GraphicsDevice, public GPUDevice{
public:
    OpenglGPUDevice();

    virtual GraphicsStats& GetStats() override;
    virtual GPUMemoryStats& GetMemoryStats() override;
    virtual GraphicsDebug& GetGraphicsDebug() override;

    virtual GraphicsDeviceInfo GetInfo() override;

    virtual void LoadContext(void* data) override;

    virtual bool SupportMultithread() override { return true; }
    
    virtual void Init() override;
    virtual void Shut() override;
    virtual void RunRender(GPURenderFrame& frame) override;

    virtual void SyncSingleThreadData() override;

    virtual MeshId AllocBufferId() override;
    virtual PipelineId AllocPipelineId() override;
    virtual BindGroupLayoutId AllocCreateBindGroupLayoutId() override;
    virtual BindGroupId AllocCreateBindGroupId() override;
    virtual Texture2DId AllocTexture2DId() override;

    virtual BufferId CreateBuffer(const void* data, size_t size, GPUBufferUsage usage, GPUBufferMemory memory = GPUBufferMemory::GPUOnly) override;
    virtual BindGroupLayoutId CreateBindGroupLayout(GPUBindGroupLayoutInfo& info) override;
    virtual BindGroupId CreateBindGroup(GPUBindGroupInfo& info) override;

    GPUResourceStats GetBufferStats(BufferId id) override; 
};

}