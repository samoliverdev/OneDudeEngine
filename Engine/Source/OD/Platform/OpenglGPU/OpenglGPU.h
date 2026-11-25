#pragma once
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"
#include "OD/Platform/BaseGpu/MultithreadRendererContext.h"
#include "OD/Core/Instrumentor.h"
#include <glad.h>

namespace OD{
namespace Gfx{   

class OpenglGPUDevice: public GraphicsDevice, public Device{
public:
    static constexpr const char* Name = "OpenGL";
    const char* GetName() const override { return Name; }

    OpenglGPUDevice();

    virtual GraphicsStats& GetStats() override;
    virtual GPUMemoryStats& GetMemoryStats() override;
    virtual GraphicsDebug& GetGraphicsDebug() override;

    virtual GraphicsDeviceInfo GetInfo() override;

    virtual void LoadContext(void* data) override;
    virtual FrameBufferLayout GetWindowFrameBufferLayout() override;

    virtual bool SupportMultithread() override { return true; }
    
    virtual void Shut() override;
    virtual void StartRender() override;
    virtual void UpdateRender() override;

    virtual bool ImGuiSupported() const override;
    virtual void ImGuiInitialize() override;
    virtual void ImGuiNewFrame() override;
    virtual void SubmitImGuiDrawData(void* data, ImGuiDrawDataDestroyFunction destroy) override;
    virtual void ImGuiShutdown() override;
    void* ImGuiTexture2D(Texture2D texture) override;
    void ReadFramebufferColor(Framebuffer framebuffer, uint32_t attachment, ColorReadbackCallback complete) override;
    virtual void* ImGuiFramebufferColorAttachment(Framebuffer framebuffer, uint32_t attachment) override;
    
    virtual CommandBuffer* GetCommandBuffer() override;

    virtual Pipeline CreatePipeline(const char* source, PipelineInfo info) override;
    virtual void DestroyPipeline(Pipeline id) override;

    virtual Buffer CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory) override;
    virtual void DestroyBuffer(Buffer id) override;

    virtual Texture2D CreateTexture2D(Texture2DInfo& info) override;
    virtual void DestroyTexture2D(Texture2D tex) override;
    virtual Texture2DArray CreateTexture2DArray(Texture2DArrayInfo& info) override;
    virtual void DestroyTexture2DArray(Texture2DArray tex) override;
    virtual Cubemap CreateCubemap(CubemapInfo& info) override;
    virtual void DestroyCubemap(Cubemap cubemap) override;

    virtual BindGroupLayout CreateBindGroupLayout(BindGroupLayoutInfo& info) override;
    virtual void DestroyBindGroupLayout(BindGroupLayout layout) override;
    
    virtual BindGroup CreateBindGroup(BindGroupInfo& info) override;
    virtual BindGroup CreateFrameBindGroup(BindGroupInfo& info) override;

    virtual Framebuffer CreateFramebuffer(FrameBufferCreateInfo& info) override;
    bool ResizeFramebuffer(Framebuffer framebuffer, const FrameBufferCreateInfo& info) override;
    virtual void DestroyFramebuffer(Framebuffer destroy) override;

    ResourceStats GetBufferStats(Buffer id) override; 

    virtual std::vector<Profile> GetProfiles() override;

private:
    void UpdateBufferImpl(Buffer buffer, const void* data, size_t size) override;
    void UploadTexture2DImpl(Texture2D texture, const void* data, size_t size, int mipLevel) override;
    void UploadTexture2DArrayImpl(Texture2DArray texture, const void* data, size_t size, int arrayElement, int mipLevel) override;
    void UploadCubemapImpl(Cubemap cubemap, const void* data, size_t size, int mipLevel) override;
    void UploadCubemapImpl(Cubemap cubemap, const void* data, size_t size, CubemapFace face, int mipLevel) override;

    std::vector<Profile> profilesCpu;
    std::vector<Profile> profilesGpu;


    struct BufferData{
        uint32_t buffer = 0;
        BufferUsage usage;
        BufferMemory memory;
        GLenum type;
        size_t size;
    };
    ResourcePool<BufferData> bufferPool;

    struct Texture2DData{
        uint32_t tex = 0;
        Texture2DInfo info;
    };
    ResourcePool<Texture2DData> texture2DPool;

    struct Texture2DArrayData{ uint32_t tex = 0; Texture2DArrayInfo info; };
    ResourcePool<Texture2DArrayData> texture2DArrayPool;

    struct CubemapData{
        uint32_t tex = 0;
        CubemapInfo info;
    };
    ResourcePool<CubemapData> cubemapPool;

    struct BindGroupLookUpData{
        size_t bufferSize = 0;
        GLuint blockIndex = GL_INVALID_INDEX;

        GLint uniformLoc = -1;
    };

    #define MaxBindingsLookUp 20
    struct BindGroupLookUp{
        BindGroupLookUpData bindingsLookUp[MaxBindingsLookUp];
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
        //BindGroupInfo info;

        BindGroupLayout layout = Gfx::InvalidID;
        std::vector<BindingEntry> entries;
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
    FrameBufferLayout windowFrameBufferLayout;
    GraphicsStats _GraphicsStats;
    GPUMemoryStats _GPUMemoryStats;
    GraphicsDebug _GraphicsDebug;

    MultithreadRendererContext multithreadRendererContext;

    unsigned int globalVAO = 0;

    std::vector<BindGroup> frameBindGroups;


    struct RenderState{
        Pipeline currentPipeline = InvalidID;
        PipelineInfo currentPipelineInfo{};
        GLint textureUnits[4][MaxBindingsLookUp]{};
        GLuint uniformBindingPoints[4][MaxBindingsLookUp]{};
        bool skipFramebufferPass = false;
    } renderState;
    void BeginRenderFrame();

    void InitializeRendering() override;
#ifdef TEST_NEW_CMD
    void ExecuteResourceCommand(ResourceCommands::Type type, const void* payload);
#else
    void ExecuteResourceCommand(const ResourceCommands::Command& cmd);
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
    void _UploadTexture2D(Texture2DData& data, const void* _data, size_t size, int mipLevel);
    void _DestroyTexture2D(Texture2DData& data); 
    bool _CreateTexture2DArray(Texture2DArrayData& data, const Texture2DArrayInfo& info);
    void _UploadTexture2DArray(Texture2DArrayData& data, const void* bytes, size_t size, int arrayElement, int mipLevel);
    void _DestroyTexture2DArray(Texture2DArrayData& data);
    bool _CreateCubemap(CubemapData& data, const CubemapInfo& info);
    void _UploadCubemap(CubemapData& data, const void* rawData, size_t size, int mipLevel);
    void _UploadCubemapFace(CubemapData& data, const void* rawData, size_t size, CubemapFace face, int mipLevel);
    void _DestroyCubemap(CubemapData& data);

    bool _CreateBindGroupLayout(BindGroupLayoutData& data, const BindGroupLayoutInfo& info);
    void _DestroyBindGroupLayout(BindGroupLayoutData& data);

    bool _CreateBindGroup(BindGroupData& data, const BindGroupInfo& info);
    
};

}
}
