#pragma once
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"
#include "OD/Platform/BaseGpu/MultithreadRendererContext.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace OD{
namespace Gfx{   

class VulkanGPUDevice: public GraphicsDevice, public Device{
public:
    VulkanGPUDevice();

    virtual GraphicsStats& GetStats() override;
    virtual GPUMemoryStats& GetMemoryStats() override;
    virtual GraphicsDebug& GetGraphicsDebug() override;

    virtual GraphicsDeviceInfo GetInfo() override;

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

private:
    struct BufferData{
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        BufferUsage usage;
        BufferMemory memory;
        VkDeviceSize size = 0;
    };
    ResourcePool<BufferData> bufferPool;

    struct Texture2DData{
        VkImage image = VK_NULL_HANDLE;
	    VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
    };
    ResourcePool<Texture2DData> texture2DPool;

    struct PipelineData{
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        PipelineInfo info;
    };
    ResourcePool<PipelineData> pipelinePool;

    struct BindGroupLayoutData{
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        BindGroupLayoutInfo info;
    };
    ResourcePool<BindGroupLayoutData> bindGroupLayoutPool;

    struct BindGroupData{
        VkDescriptorSet descriptorSet  = VK_NULL_HANDLE;
        BindGroupInfo info;
    };
    ResourcePool<BindGroupData> bindGroupPool;

    MultithreadRendererContext multithreadRendererContext;

    bool multithread;

    void _Init();
    void _Shut();

    void CreateVulkanPipeline(Pipeline id, const char* source, const PipelineInfo& info);
    void Cleanup();

    void RunRender(RenderFrame& frame);
    void SyncSingleThreadData();
};

}
}