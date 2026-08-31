#pragma once
#include "OD/GPU/GPU.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"
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
    
    virtual void Init() override;
    virtual void Shut() override;
    virtual void RunRender(GPURenderFrame& frame) override;

    virtual void SyncSingleThreadData() override;

    virtual MeshId AllocBufferId() override;
    virtual PipelineId AllocPipelineId() override;

    virtual BindGroupLayoutId AllocCreateBindGroupLayoutId() override;
    virtual BindGroupId AllocCreateBindGroupId() override;

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

    void CreateVulkanPipeline(PipelineId id, const char* source, const GPUPipelineInfo& info);
    void Cleanup();
};

}