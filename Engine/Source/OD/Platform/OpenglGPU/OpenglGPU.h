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

    virtual MeshId AllocMeshId() override;
    virtual PipelineId AllocPipelineId() override;
};

}