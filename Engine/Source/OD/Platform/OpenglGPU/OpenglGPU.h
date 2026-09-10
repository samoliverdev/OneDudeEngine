#pragma once
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"
#include "OD/Platform/BaseGpu/MultithreadRendererContext.h"
#include <glad.h>

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

    virtual Buffer CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory) override;
    virtual void UpdatedBuffer(Buffer buffer, const void* data, size_t size) override;
    virtual void DestroyBuffer(Buffer id) override;

    virtual Texture2D CreateTexture2D(Texture2DInfo& info) override;
    virtual void UploadTexture2D(Texture2D texture, const void* data, size_t size) override;
    virtual void DestroyTexture2D(Texture2D tex) override;

    virtual BindGroupLayout CreateBindGroupLayout(BindGroupLayoutInfo& info) override;
    virtual void DestroyBindGroupLayout(BindGroupLayout layout) override;
    
    virtual BindGroup CreateBindGroup(BindGroupInfo& info) override;

    virtual Framebuffer CreateFramebuffer(FrameBufferCreateInfo& info) override;
    virtual void DestroyFramebuffer(Framebuffer destroy) override;

    ResourceStats GetBufferStats(Buffer id) override; 

private:
    bool multithread;

    struct BufferData{
        uint32_t buffer = 0;
        BufferUsage usage;
        BufferMemory memory;
        GLenum type;
    };
    ResourcePool<BufferData> bufferPool;

    struct Texture2DData{
        uint32_t tex = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };
    ResourcePool<Texture2DData> texture2DPool;

    struct BindGroupLookUp{
        GLuint bindingsLookUp[20];
    };

    struct PipelineData{
        uint32_t program = 0;
        PipelineInfo info;
        BindGroupLookUp groupsLookUp[4];
    };
    ResourcePool<PipelineData> pipelinePool;

    struct BindGroupLayoutData{
        BindGroupLayoutInfo info;
    };
    ResourcePool<BindGroupLayoutData> bindGroupLayoutPool;

    struct BindGroupData{
        BindGroupInfo info;
    };
    ResourcePool<BindGroupData> bindGroupPool;

    struct FramebufferData{
        unsigned int framebuffer = 0;
        unsigned int depthAttachment = 0;
        std::vector<unsigned int> colorAttachments;
        uint32_t width;
        uint32_t height;
        FrameBufferLayout layout;
        FrameBufferCreateInfo info;
    };
    ResourcePool<FramebufferData> framebufferPool;

    GraphicsDeviceInfo info;
    GraphicsStats _GraphicsStats;
    GPUMemoryStats _GPUMemoryStats;
    GraphicsDebug _GraphicsDebug;

    MultithreadRendererContext multithreadRendererContext;

    unsigned int globalVAO = 0;

    void RunRender(RenderFrame& frame);
    void SyncSingleThreadData();

    void _Init();
    void _Shut();

    bool _CreatePipeline(PipelineData& data, const char* source, const PipelineInfo& info);
    void _DestroyPipeline(PipelineData& data);

    bool _CreateBuffer(BufferData& data, size_t size, BufferUsage usage, BufferMemory memory);
    void _UpdatedBuffer(BufferData& data, const void* _data, size_t size);
    void _DestroyBuffer(BufferData& data);

    bool _CreateFramebuffer(FramebufferData& data, const FrameBufferCreateInfo& createInfo);
    void _DestroyFramebuffer(FramebufferData& data);

    bool _CreateTexture2D(Texture2DData& data, const Texture2DInfo& info); 
    void _UploadTexture2D(Texture2DData& data, const void* _data, size_t size);
    void _DestroyTexture2D(Texture2DData& data); 

    bool _CreateBindGroupLayout(BindGroupLayoutData& data, BindGroupLayoutInfo& info);
    void _DestroyBindGroupLayout(BindGroupLayoutData& data);

    bool _CreateBindGroup(BindGroupData& data, BindGroupInfo& info);
};

}
}
