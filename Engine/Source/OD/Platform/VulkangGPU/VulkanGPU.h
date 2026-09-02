#pragma once
#include "OD/GPU/GPU.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"
#include "OD/Platform/BaseGpu/MultithreadRendererContext.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace OD{

class VulkanGPUDevice: public GraphicsDevice, public GPUDevice{
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
    virtual GPURenderFrame* GetRenderFrame() override;

    virtual MeshId AllocBufferId() override;
    virtual PipelineId AllocPipelineId() override;

    virtual BindGroupLayoutId AllocCreateBindGroupLayoutId() override;
    virtual BindGroupId AllocCreateBindGroupId() override;

    virtual Texture2DId AllocTexture2DId() override;

    virtual BindGroupLayoutId CreateBindGroupLayout(GPUBindGroupLayoutInfo& info) override;
    virtual BindGroupId CreateBindGroup(GPUBindGroupInfo& info) override;

private:
    struct BufferData{
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        GPUBufferUsage usage;
        GPUBufferMemory memory;
        VkDeviceSize size = 0;
    };
    ResourcePool<BufferData> bufferPool;

    struct Texture2DData{
        VkImage image = VK_NULL_HANDLE;
	    VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
    };
    ResourcePool<Texture2DData> texture2DDataPool;

    struct PipelineData{
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        GPUPipelineInfo info;
    };
    ResourcePool<PipelineData> pipelinePool;

    struct BindGroupLayoutData{
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        GPUBindGroupLayoutInfo info;
    };
    ResourcePool<BindGroupLayoutData> bindGroupLayoutPool;

    struct BindGroupData{
        VkDescriptorSet descriptorSet  = VK_NULL_HANDLE;
        GPUBindGroupInfo info;
    };
    ResourcePool<BindGroupData> bindGroupPool;

    MultithreadRendererContext multithreadRendererContext;

    bool multithread;

    void _Init();
    void _Shut();

    void CreateVulkanPipeline(PipelineId id, const char* source, const GPUPipelineInfo& info);
    void Cleanup();

    void RunRender(GPURenderFrame& frame);
    void SyncSingleThreadData();
};

}