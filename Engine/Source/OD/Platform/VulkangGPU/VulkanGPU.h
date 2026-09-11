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
    virtual FrameBufferLayout GetWindowFrameBufferLayout() override;

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
        uint32_t width = 0;
        uint32_t height = 0;
	    VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        Texture2DInfo info;
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
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        BindGroupInfo info;
    };
    ResourcePool<BindGroupData> bindGroupPool;

    struct VulkanFramebufferAttachment{
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        bool initialized = false;
    };

    struct FramebufferData{
        uint64_t hash;
        uint32_t id;
        
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        std::vector<VulkanFramebufferAttachment> colorAttachments;
        VulkanFramebufferAttachment depthAttachment;

        uint32_t width = 0;
        uint32_t height = 0;

        FrameBufferLayout layout;
    };
    ResourcePool<FramebufferData> framebufferPool;

    struct RenderPasses{
        uint64_t hash;
        uint32_t id;
        VkRenderPass renderPass;
    };
    std::vector<RenderPasses> renderPasses;

    MultithreadRendererContext multithreadRendererContext;

    bool multithread;

    //VkDescriptorSetLayout emptyLayout;

    void _Init();
    void _Shut();

    void InitDefaultRenderpass();
    void InitFramebuffers();
    void InitDescriptors();

    void Cleanup();

    void RunRender(RenderFrame& frame);
    void SyncSingleThreadData();

    VkRenderPass GetOrCreate(const FrameBufferLayout& layout);

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
