#pragma once
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/GraphicsDevice.h"

namespace OD{
namespace Gfx{   

class OpenglGPUDevice: public GraphicsDevice, public Device{
public:
    OpenglGPUDevice();

    virtual GraphicsStats& GetStats() override;
    virtual GPUMemoryStats& GetMemoryStats() override;
    virtual GraphicsDebug& GetGraphicsDebug() override;

    virtual GraphicsDeviceInfo GetInfo() override;

    virtual void LoadContext(void* data) override;

    virtual bool SupportMultithread() override { return true; }
    
    virtual void Init(bool multithread) override;
    virtual void Shut() override;
    virtual void StartRender() override;
    virtual void UpdateRender() override;
    
    virtual CommandBuffer* GetCommandBuffer() override;

    virtual Pipeline CreatePipeline(const char* source, PipelineInfo info) override;
    virtual void DestroyPipeline(Pipeline id) override;
    virtual Buffer CreateBuffer(const void* data, size_t size, BufferUsage usage, BufferMemory memory) override;
    virtual void DestroyBuffer(Buffer id) override;
    virtual BindGroupLayout CreateBindGroupLayout(BindGroupLayoutInfo& info) override;
    virtual BindGroup CreateBindGroup(BindGroupInfo& info) override;
    virtual Texture2D CreateTexture2D(Texture2DInfo& info, void* data, size_t size) override;
    virtual Framebuffer CreateFramebuffer(FrameBufferCreateInfo& info) override;

    ResourceStats GetBufferStats(Buffer id) override; 

private:
    bool multithread;

    void RunRender(RenderFrame& frame);
    void SyncSingleThreadData();
};

}
}