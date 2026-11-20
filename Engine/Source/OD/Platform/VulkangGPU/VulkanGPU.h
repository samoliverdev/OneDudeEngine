#pragma once
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"
#include "OD/Platform/BaseGpu/MultithreadRendererContext.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <deque>

namespace OD{
namespace Gfx{   

enum class VulkanPresentMode {
    VSync,
    Immediate
};

class VulkanGPUDevice: public GraphicsDevice, public Device{
public:
    explicit VulkanGPUDevice(VulkanPresentMode presentMode = VulkanPresentMode::VSync);

    virtual GraphicsStats& GetStats() override;
    virtual GPUMemoryStats& GetMemoryStats() override;
    virtual GraphicsDebug& GetGraphicsDebug() override;

    virtual GraphicsDeviceInfo GetInfo() override;

    virtual bool SupportMultithread() override { return true; }
    
    virtual void Shut() override;
    virtual void StartRender() override;
    virtual void UpdateRender() override;

    virtual bool ImGuiSupported() const override { return true; }
    virtual void ImGuiInitialize() override;
    virtual void ImGuiNewFrame() override;
    virtual void SubmitImGuiDrawData(void* data, ImGuiDrawDataDestroyFunction destroy) override;
    virtual void ImGuiShutdown() override;
    void* ImGuiTexture2D(Texture2D texture) override;
    virtual void* ImGuiFramebufferColorAttachment(Framebuffer framebuffer, uint32_t attachment) override;
    virtual bool ImGuiFramebufferNeedsVerticalFlip() const override;

    virtual CommandBuffer* GetCommandBuffer() override;
    virtual FrameBufferLayout GetWindowFrameBufferLayout() override;

    virtual Pipeline CreatePipeline(const char* source, PipelineInfo info) override;
    virtual void DestroyPipeline(Pipeline id) override;

    virtual Buffer CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory) override;
    virtual void UpdatedBuffer(Buffer buffer, const void* data, size_t size) override;
    virtual void DestroyBuffer(Buffer id) override;

    virtual Texture2D CreateTexture2D(Texture2DInfo& info) override;
    virtual void UploadTexture2D(Texture2D texture, const void* data, size_t size, int mipLevel = 0) override;
    virtual void DestroyTexture2D(Texture2D tex) override;
    virtual Texture2DArray CreateTexture2DArray(Texture2DArrayInfo& info) override;
    virtual void UploadTexture2DArray(Texture2DArray texture, const void* data, size_t size, int arrayElement, int mipLevel) override;
    virtual void DestroyTexture2DArray(Texture2DArray tex) override;
    virtual Cubemap CreateCubemap(CubemapInfo& info) override;
    virtual void UploadCubemap(Cubemap cubemap, const void* data, size_t size, int mipLevel = 0) override;
    virtual void UploadCubemap(Cubemap cubemap, const void* data, size_t size, CubemapFace face, int mipLevel = 0) override;
    virtual void DestroyCubemap(Cubemap cubemap) override;

    virtual BindGroupLayout CreateBindGroupLayout(BindGroupLayoutInfo& info) override;
    virtual void DestroyBindGroupLayout(BindGroupLayout layout) override;
    
    virtual BindGroup CreateBindGroup(BindGroupInfo& info) override;
    virtual BindGroup CreateFrameBindGroup(BindGroupInfo& info) override;

    virtual Framebuffer CreateFramebuffer(FrameBufferCreateInfo& info) override;
    bool ResizeFramebuffer(Framebuffer framebuffer, const FrameBufferCreateInfo& info) override;
    virtual void DestroyFramebuffer(Framebuffer destroy) override;

    virtual std::vector<Profile> GetProfiles() override;

private:
    std::vector<Profile> profilesCpu;
    std::vector<Profile> profilesGpu;

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
        VkImageView imguiView = VK_NULL_HANDLE;
        VkDescriptorSet imguiDescriptor = VK_NULL_HANDLE;
        bool baseMipInitialized = false;
    };
    ResourcePool<Texture2DData> texture2DPool;

    struct Texture2DArrayData{
        VkImage image = VK_NULL_HANDLE; VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE; VkSampler sampler = VK_NULL_HANDLE;
        Texture2DArrayInfo info;
    };
    ResourcePool<Texture2DArrayData> texture2DArrayPool;

    struct CubemapData{
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        CubemapInfo info;
    };
    ResourcePool<CubemapData> cubemapPool;

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
        
        //BindGroupInfo info;
        
        BindGroupLayout layout = Gfx::InvalidID;
        std::vector<BindingEntry> entries;
    };
    ResourcePool<BindGroupData> bindGroupPool;

    struct VulkanFramebufferAttachment{
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        bool initialized = false;
        VkDescriptorSet imguiDescriptor = VK_NULL_HANDLE;
    };

    struct FramebufferData{
        uint64_t hash;
        uint32_t id;
        
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> subresourceFramebuffers;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        std::vector<VulkanFramebufferAttachment> colorAttachments;
        VulkanFramebufferAttachment depthAttachment;
        std::vector<VkImageView> subresourceViews;

        uint32_t width = 0;
        uint32_t height = 0;
        // Exact dimensions used to build subresourceFramebuffers. Keep these
        // independent from the layout so command execution cannot calculate a
        // different vector index after creation.
        uint32_t subresourceMipCount = 0;
        uint32_t subresourceLayerCount = 0;

        FrameBufferLayout layout;
        uint32_t activeLayer = 0;
        uint32_t activeMip = 0;
        bool contentsInitialized = false;
    };
    ResourcePool<FramebufferData> framebufferPool;

    struct RenderPasses{
        uint64_t hash;
        uint32_t id;
        bool clean = true;
        VkRenderPass renderPass = VK_NULL_HANDLE;
    };
    std::vector<RenderPasses> renderPasses;

    MultithreadRendererContext multithreadRendererContext;

    struct StagingSlice{
        VkBuffer buffer;
        VkDeviceSize offset;
        const void* data;
    };

    struct StagingPool{
        struct Block{
            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            std::byte* mapped = nullptr;
            VkDeviceSize capacity = 0;
            VkDeviceSize used = 0;
        };

        StagingSlice Copy(const void* data, size_t size);
        const StagingSlice* Stage(const void* data, size_t size);
        void Reset();
        void Clear();

        VkDeviceSize alignment = 48;
        std::vector<Block> blocks;
        std::deque<StagingSlice> queuedSlices;
    };

    // One pool for each CPU frame slot, with the same producer/consumer ownership.
    StagingPool stagingPools[2];
    StagingPool& GetStagingPool(const RenderFrame& frame);
    std::unordered_set<Buffer> gpuOnlyUploadBuffers;
    std::unordered_set<Texture2D> rgbTextures;
    std::unordered_set<Texture2DArray> rgbTextureArrays;
    std::unordered_set<Cubemap> rgbCubemaps;

    VulkanPresentMode presentMode;

    //VkDescriptorSetLayout emptyLayout;

    std::vector<BindGroup> frameBindGroups;
    ResourceCommands submittedDestroyCommands;
    bool frameInFlight = false;
    bool recreateAfterSubmission = false;

    VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
    bool imguiInitialized = false;

    void _Init();
    void _Shut();

    void InitDefaultRenderpass();
    void InitFramebuffers();
    void InitDescriptors();
    bool RecreateSwapchain(bool force = false);

    void Cleanup();


    struct RenderState{
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        uint32_t swapchainImageIndex = 0;
        Pipeline currentPipeline = InvalidID;
        FramebufferData* currentFramebuffer = nullptr;
        bool skipNextEndFramebuffer = false;
        bool hasWindowRenderPass = false;
        bool attemptedAcquire = false;
        bool recreateAfterPresent = false;
    } renderState;
    VkRenderPass windowOverlayRenderPass = VK_NULL_HANDLE;
    void BeginRenderCommands();
    void SubmitRenderCommands();
    void WaitForSubmittedFrame();
    void SyncGpu();
    void RetireFrameResources(const ResourceCommands& destroyCommands);

    void InitializeRendering() override;
#ifdef TEST_NEW_CMD
    void ExecuteResourceCommand(RenderFrame& frame, ResourceCommands::Type type, const void* payload);
#else
    void ExecuteResourceCommand(RenderFrame& frame, const ResourceCommands::Command& cmd);
#endif
#ifdef TEST_NEW_CMD
    void ExecuteDestroyCommand(ResourceCommands::Type type, const void* payload);
#else
    void ExecuteDestroyCommand(const ResourceCommands::Command& cmd);
#endif
#ifdef TEST_NEW_CMD
    void ExecuteRenderCommand(RenderFrame& frame, CommandBuffer::Type type, const void* payload);
#else
    void ExecuteRenderCommand(RenderFrame& frame, const CommandBuffer::Command& cmd);
#endif
    void FinishRenderFrame(RenderFrame& frame);
    void ReplayRenderCommands(RenderFrame& frame, const CommandBuffer& commands);
    void ApplyFramebufferResizes(RenderFrame& frame);
    std::unordered_set<Framebuffer> persistentFramebufferBindings;
    void InitializeResizedFramebuffers(RenderFrame& frame);
    std::unordered_map<Framebuffer, FrameBufferCreateInfo> framebuffersToInitialize;
    void RunRender(RenderFrame& frame);
    void SyncSingleThreadData();

    VkRenderPass GetOrCreate(const FrameBufferLayout& layout, bool clean = true);

    bool _CreatePipeline(PipelineData& data, const char* source, const PipelineInfo& info);
    void _DestroyPipeline(PipelineData& data);

    bool _CreateBuffer(BufferData& data, size_t size, BufferUsage usage, BufferMemory memory);
    void _UpdatedBuffer(BufferData& data, const void* bytes, size_t size, StagingPool& pool, const StagingSlice* staged);
    void _DestroyBuffer(BufferData& data);

    bool _CreateFramebuffer(FramebufferData& data, const FrameBufferCreateInfo& createInfo);
    void _DestroyFramebuffer(FramebufferData& data);

    bool _CreateTexture2D(Texture2DData& data, const Texture2DInfo& info); 
    void _UploadTexture2D(Texture2DData& data, const void* bytes, size_t size, int mipLevel, StagingPool& pool, const StagingSlice* staged = nullptr);
    void _DestroyTexture2D(Texture2DData& data); 
    bool _CreateTexture2DArray(Texture2DArrayData& data, const Texture2DArrayInfo& info);
    void _UploadTexture2DArray(Texture2DArrayData& data, const void* bytes, size_t size, int arrayElement, int mipLevel, StagingPool& pool, const StagingSlice* staged = nullptr);
    void _DestroyTexture2DArray(Texture2DArrayData& data);
    bool _CreateCubemap(CubemapData& data, const CubemapInfo& info);
    void _UploadCubemap(CubemapData& data, const void* rawData, size_t size, int mipLevel, StagingPool& pool, const StagingSlice* staged = nullptr);
    void _UploadCubemapFace(CubemapData& data, const void* rawData, size_t size, CubemapFace face, int mipLevel, StagingPool& pool, const StagingSlice* staged = nullptr);
    void _DestroyCubemap(CubemapData& data);

    bool _CreateBindGroupLayout(BindGroupLayoutData& data, const BindGroupLayoutInfo& info);
    void _DestroyBindGroupLayout(BindGroupLayoutData& data);

    bool _CreateBindGroup(BindGroupData& data, const BindGroupInfo& info, VkDescriptorPool pool);
};

}
}
