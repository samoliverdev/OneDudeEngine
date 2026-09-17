#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Core/Math.h"
#include <thread>
#include <mutex>

namespace OD{
namespace Gfx{   

using ImGuiDrawDataDestroyFunction = void(*)(void*);

static constexpr uint32_t InvalidID = std::numeric_limits<uint32_t>::max();
using Texture = uint32_t;
using Pipeline = uint32_t;
using Framebuffer = uint32_t;
using Buffer = uint32_t;
using BindGroupLayout = uint32_t;
using BindGroup = uint32_t;
using Texture2D = uint32_t;
using Cubemap = uint32_t;

enum class OD_API_IMPORT TextureFilter : uint8_t{
    Nearest,
    Linear
};

enum class OD_API_IMPORT TextureWrapping : uint8_t{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

/////////////////////////////////////

enum class ResourceStatsType{
    None, Created, Failed
};

struct ResourceStats{
    ResourceStatsType type = ResourceStatsType::None;
    std::string erroMessage;
};

/////////////////////////////////////

constexpr uint32_t MAX_COLOR_ATTACHMENTS = 7;

enum class OD_API_IMPORT FramebufferTextureFormat: uint8_t{
    None, RGB, RGBA8, RGB11B10F, RGB16F, RGBA16F, RGB32F, RGBA32F, RED_INTEGER
};

enum class OD_API_IMPORT FramebufferDepthTextureFormat: uint8_t{
    None, DEPTH24_STENCIL8, DEPTH32F_STENCIL8, DEPTH_COMPONENT16, DEPTH_COMPONENT24, DEPTH_COMPONENT32, DEPTH_COMPONENT32F
};

enum class OD_API_IMPORT FramebufferAttachmentType: uint8_t{
    TEXTURE_2D,
    TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_ARRAY,
    CUBEMAP
};

struct OD_API FramebufferAttachment{
    FramebufferTextureFormat format;
    uint8_t mipLevels = 1;
};

struct OD_API FramebufferDepthAttachment{
    FramebufferDepthTextureFormat format;
    uint8_t mipLevels = 1;
};

struct OD_API FrameBufferLayout{
    FramebufferAttachmentType type = FramebufferAttachmentType::TEXTURE_2D;
    FramebufferAttachment colorAttachments[MAX_COLOR_ATTACHMENTS];
    uint8_t colorAttachmentsCount = 0;
    FramebufferDepthAttachment depthAttachment;
    uint8_t samples = 1;
    uint8_t layers = 1;
    bool swapChainTarget = false;
};

struct OD_API FrameBufferCreateInfo{
    FrameBufferLayout layout;
    uint32_t width = 1;
    uint32_t height = 1;
    TextureFilter filter = TextureFilter::Linear;
    TextureWrapping wrapping = TextureWrapping::Repeat;
    //bool mipmap = true;
};

//////////////////////////////////////

enum class ImageFormat{
    R8_UNORM,

    R8G8B8_UNORM,
    R8G8B8_SRGB,

    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB
};

struct OD_API Texture2DInfo{
    uint32_t width = 1;
    uint32_t height = 1;
    ImageFormat format = ImageFormat::R8G8B8A8_UNORM;
    TextureFilter filter = TextureFilter::Linear;
    TextureWrapping wrapping = TextureWrapping::Repeat;
    bool mipmap = true;
};

struct OD_API CubemapInfo{
    uint32_t width = 1;
    uint32_t height = 1;
    ImageFormat format = ImageFormat::R8G8B8A8_UNORM;
    TextureFilter filter = TextureFilter::Linear;
    TextureWrapping wrapping = TextureWrapping::Repeat;
    bool mipmap = true;
};

//////////////////////////////////////

constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 16;
constexpr uint32_t MAX_VERTEX_BUFFERS = 16;

enum class VertexSemantic: uint8_t{
    Position,
    Normal,
    Tangent,

    UV0,
    UV1,
    UV2,
    UV3,

    Color0,
    Color1,

    Weights,
    Influences,

    Custom0,
    Custom1,
    Custom2,
    Custom3,

    Intancing0,
    Intancing1,
    Intancing2,
    Intancing3,
    
    Invalid,
};

enum class VertexFormat: uint8_t{
    Float,
    Float2,
    Float3,
    Float4,

    Int,
    Int2,
    Int3,
    Int4,

    UInt,
    UInt2,
    UInt3,
    UInt4,

    UByte4,
    UByte4Normalized,

    Byte4,
    Byte4Normalized,

    UShort2,
    UShort4,
    UShort2Normalized,
    UShort4Normalized,

    Short2,
    Short4,
    Short2Normalized,
    Short4Normalized,

    Half2,
    Half4
};

enum class VertexInputRate: uint8_t{
    Vertex,
    Instance
};

struct OD_API VertexAttribute{
    VertexSemantic semantic; //INFO: hlsl i think this is the binding Index
    //uint8_t customBindIndex = UINT8_MAX; //For now add this to override the semantic bind index, so when implement direct x, handle the this design later
    VertexFormat format;

    // Vertex buffer binding this attribute comes from.
    uint8_t bufferSlot = 0;

    // Byte offset inside the vertex.
    size_t offset = 0;
};

struct OD_API VertexBufferLayout{
    // Distance in bytes between two consecutive
    // vertices/instances in this buffer.
    size_t stride = 0;

    VertexInputRate inputRate = VertexInputRate::Vertex;
};

struct OD_API MeshLayout{
    VertexAttribute attributes[MAX_VERTEX_ATTRIBUTES];
    uint32_t attributeCount = 0;

    VertexBufferLayout buffers[MAX_VERTEX_BUFFERS];
    uint32_t bufferCount = 0;
};

inline uint32_t VertexSemanticToSlot(VertexSemantic semantic){
    switch(semantic){
        case VertexSemantic::Position:      return 0;
        case VertexSemantic::UV0:           return 1;
        case VertexSemantic::Normal:        return 2;
        case VertexSemantic::Color0:        return 3;
        case VertexSemantic::Tangent:       return 4;

        case VertexSemantic::Influences:    return 5;
        case VertexSemantic::Weights:       return 6;
        
        case VertexSemantic::Intancing0:    return 10;
        case VertexSemantic::Intancing1:    return 11;
        case VertexSemantic::Intancing2:    return 12;
        case VertexSemantic::Intancing3:    return 13;
    }
    Assert(false);
    return 0;
}

inline VertexSemantic SlotToVertexSemanticTo(uint32_t slot){
    switch(slot){
        case 0:         return VertexSemantic::Position;
        case 1:         return VertexSemantic::UV0;
        case 2:         return VertexSemantic::Normal;
        case 3:         return VertexSemantic::Color0;
        case 4:         return VertexSemantic::Tangent;
        
        case 5:         return VertexSemantic::Influences;
        case 6:         return VertexSemantic::Weights;

        case 10:        return VertexSemantic::Intancing0;
        case 11:        return VertexSemantic::Intancing1;
        case 12:        return VertexSemantic::Intancing2;
        case 13:        return VertexSemantic::Intancing3;
    }
    Assert(false);
    return VertexSemantic::Invalid;
}

/////////////////////////////////////

constexpr uint32_t MAX_BINDGROUP_COUT = 4;

enum class BindingType{
    UniformBuffer,
    StorageBuffer,
    Texture2D, //Texture + sampler
    Texture2DArray, 
    TextureCube,
    TextureCubeArray,

    // Texture3D,
    // StorageTexture,

    //Texture, // Maybe will dont have
    //Sampler, // Maybe will dont have
    //StorageTexture, //Will be Add later
};

struct BindLayoutEntry{
    uint32_t binding;
    BindingType type;

    //GPUShaderStage visibility; //Will be default for all shader stages

    size_t minUniformBufferSize;
    bool dynamicOffset = false;

    // Only meaningful for StorageTexture 
    //GPUTextureFormat textureFormat; //Will be Add later
    //GPUTextureViewDimension viewDimension; //Will be Add later
};

struct BindGroupLayoutInfo{
    BindLayoutEntry entries[4];
    uint32_t entriesCount = 0;
};

//////////////////////////////////////

struct BindingEntry{
    uint32_t binding;

    Buffer buffer;
    size_t offset;
    size_t size;
    bool dynamicOffset;

    Texture2D texture = InvalidID;
    Cubemap cubemap = InvalidID;

    Framebuffer framebuffer = InvalidID;
    int framebufferAttacement = 0;
    uint32_t framebufferLayer = 0;
};

struct BindGroupInfo{
    BindGroupLayout layout;
    BindingEntry entries[64];
    uint32_t entriesCount = 0;
};

//////////////////////////////////////

enum class OD_API_IMPORT DepthTest: uint8_t{
    DISABLE         = 0,
    LESS            = 1,
    LESS_EQUAL      = 2,
    EQUAL           = 3,
    GREATER         = 4,
    GREATER_EQUAL   = 5,
    DIFFERENT       = 6,
    NEVER           = 7,
    ALWAYS          = 8
};

enum class OD_API_IMPORT CullFace: uint8_t{
    NONE            = 0,
    BACK            = 1,
    FRONT           = 2,
    FRONT_AND_BACK  = 3
};

enum class OD_API_IMPORT BlendMode: uint8_t{
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA	
};

enum class OD_API_IMPORT BlendOp: uint8_t{
    FUNC_ADD,
    FUNC_SUBTRACT,
    FUNC_REVERSE_SUBTRACT,
    MIN,
    MAX
};  

struct OD_API PipelineInfo{
    MeshLayout vertexLayout;
    CullFace cullFace = CullFace::BACK;
    DepthTest depthTest = DepthTest::LESS;
    bool depthMask = true;
    Vector4 colorMask = {1, 1, 1, 1};
    bool blend = false;
    BlendMode srcBlend;
    BlendMode dstBlend;
    BlendMode srcAlphaBlend;
    BlendMode dstAlphaBlend;
    BlendOp opBlend = BlendOp::FUNC_ADD;

    uint32_t bindGroupLayoutCount = 0;
    BindGroupLayout bindGroupLayouts[MAX_BINDGROUP_COUT];

    FrameBufferLayout framebufferLayout = {};
};

///////////////////////////////////////

enum class BufferUsage: uint8_t{
    Vertex,
    Index,
    Uniform,
    Storage,
    /*Indirect,
    CopySource,
    CopyDestination*/
};

enum class BufferMemory : uint8_t{
    GPUOnly,
    CPUToGPU,
    GPUToCPU,
    CPUOnly
};

/////////////////////////////////////

enum class ClearFlags : uint8_t{
    None    = 0,
    Color   = 1 << 0,
    Depth   = 1 << 1,
    Stencil = 1 << 2
};

struct ClearValue{
    Vector4 color = {0, 0, 0, 0};
    float depth = 1.0f;
    uint32_t stencil = 0;
};

constexpr ClearFlags operator|(ClearFlags a, ClearFlags b){
    return static_cast<ClearFlags>(
        static_cast<uint8_t>(a) |
        static_cast<uint8_t>(b)
    );
}

constexpr bool HasFlag(ClearFlags value, ClearFlags flag){
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

/////////////////////////////////////

class OD_API UploadBuffer{
public:
    UploadBuffer() = default;
    UploadBuffer(const UploadBuffer&) = delete;
    UploadBuffer& operator=(const UploadBuffer&) = delete;
    UploadBuffer(UploadBuffer&&) noexcept = default;
    UploadBuffer& operator=(UploadBuffer&&) noexcept = default;

    template<typename T>
    T* Allocate(size_t count = 1){
        return static_cast<T*>(AllocateData(sizeof(T) * count, alignof(T)));
    }

    void* AllocateData(size_t size, size_t alignment = alignof(std::max_align_t)){
        if(size == 0) return nullptr;

        if(blocks.empty()){
            blocks.push_back(
                std::make_unique<MemoryBlock>(
                    std::max(DefaultBlockSize, size)
                )
            );
        }

        MemoryBlock* block = blocks[currentBlock].get();

        if(void* ptr = block->Allocate(size, alignment))
            return ptr;

        // Current block doesn't have enough space.
        currentBlock++;

        if(currentBlock >= blocks.size()){
            blocks.push_back(
                std::make_unique<MemoryBlock>(
                    std::max(DefaultBlockSize, size)
                )
            );
        }

        return blocks[currentBlock]->Allocate(size, alignment);
    }

    void Clear(){
        // Don't free the blocks.
        // Just reset them for reuse.
        for(auto& block : blocks)
            block->used = 0;

        currentBlock = 0;
    }
private:
    struct MemoryBlock{
        std::unique_ptr<uint8_t[]> data;
        size_t capacity = 0;
        size_t used = 0;

        MemoryBlock(size_t size): data(std::make_unique<uint8_t[]>(size)), capacity(size){}

        void* Allocate(size_t size, size_t alignment){
            size_t current = reinterpret_cast<size_t>(data.get()) + used;

            size_t aligned =
                (current + alignment - 1) &
                ~(alignment - 1);

            size_t offset = aligned - reinterpret_cast<size_t>(data.get());

            if(offset + size > capacity)
                return nullptr;

            used = offset + size;

            return data.get() + offset;
        }
    };

    std::vector<std::unique_ptr<MemoryBlock>> blocks;
    size_t currentBlock = 0;
    static constexpr size_t DefaultBlockSize = 64 * 1024;
};

struct OD_API ResourceCommands{
    enum class Type{
        CreatePipeline,
        DestroyPipeline,

        CreateBuffer,
        UpdateBuffer,
        DestroyBuffer,

        CreateTexture2D,
        UploadTexture2D,
        DestroyTexture2D,
        CreateCubemap,
        UploadCubemap,
        DestroyCubemap,

        CreateBindGroupLayout,
        DestroyBindGroupLayout,

        CreateBindGroup,
        CreateFrameBindGroup,

        CreateFramebuffer,
        DestroyFramebuffer,
    };

    struct Command{
        Type type;

        union{
            struct{
                Buffer id;
                BufferUsage usage;
                BufferMemory memory;
                size_t size;
            } createBuffer;

            struct{
                Buffer id;
            } destroyBuffer;

            struct{
                Pipeline id;
                char* source;
                PipelineInfo info;
            } createPipeline;

            struct{
                Pipeline id;
            } destroyPipeline;

            struct {
                BindGroupLayout id; BindGroupLayoutInfo* info;
            } createBindGroupLayout;

            struct {
                BindGroup id; BindGroupInfo* info;
            } createBindGroup;

            struct {
                BindGroup id; BindGroupInfo* info;
            } createFrameBindGroup;

            struct {
                Texture2D id;
                Texture2DInfo info;
            } createTexture2D;

            struct {
                Texture2D id;
                const void* data;
                size_t size;
            } uploadTexture2D;
            
            struct {
                Framebuffer framebuffer;
                FrameBufferCreateInfo info;
            } createFramebuffer;
            
            struct{
                Buffer id;
                const void* data;
                size_t size;
            } updateBuffer;

            struct {
                Texture2D id; 
            } destroyTexture2D;

            struct {
                Cubemap id;
                CubemapInfo info;
            } createCubemap;

            struct {
                Cubemap id;
                const void* data;
                size_t size;
            } uploadCubemap;

            struct {
                Cubemap id;
            } destroyCubemap;

            struct {
                BindGroupLayout id; 
            } destroyBindGroupLayout;

            struct {
                BindGroup id; 
            } destroyBindGroup;

            struct {
                Framebuffer id; 
            } destroyFramebuffer;
        };
    };

    inline void Clear(){
        commands.clear();
        uploadBuffer.Clear();
    }

    inline void CreatePipeline(Pipeline id, const char* source, PipelineInfo info){
        size_t size = std::strlen(source) + 1;
        char* copyData = static_cast<char*>(uploadBuffer.AllocateData(size));
        std::memcpy(copyData, source, size);

        Command cmd{};
        cmd.type = Type::CreatePipeline;
        cmd.createPipeline.id = id;
        cmd.createPipeline.source = copyData; //source;
        cmd.createPipeline.info = info;
        commands.push_back(cmd);
    }

    inline void DestroyPipeline(Pipeline id){
        Command cmd{};
        cmd.type = Type::DestroyPipeline;
        cmd.destroyPipeline.id = id;
        commands.push_back(cmd);
    }

    inline void CreateBuffer(Buffer id, size_t size, BufferUsage usage, BufferMemory memory = BufferMemory::GPUOnly){
        Command cmd{};
        cmd.type = Type::CreateBuffer;
        cmd.createBuffer.usage = usage;
        cmd.createBuffer.id = id;
        cmd.createBuffer.size = size;
        cmd.createBuffer.memory = memory;
        commands.push_back(cmd);
    }

    inline void UpdatedBuffer(Buffer id, const void* data, size_t size){
        void* copyData = uploadBuffer.AllocateData(size);
        std::memcpy(copyData, data, size);

        Command cmd{};
        cmd.type = Type::UpdateBuffer;
        cmd.updateBuffer.id = id;
        cmd.updateBuffer.data = copyData;
        cmd.updateBuffer.size = size;
        commands.push_back(cmd);
    }

    inline void DestroyBuffer(Buffer id){
        Command cmd{};
        cmd.type = Type::DestroyBuffer;
        cmd.destroyBuffer.id = id;
        commands.push_back(cmd);
    }

    inline void CreateTexture2D(Texture2D id, Texture2DInfo& info){
        Command cmd{};
        cmd.type = Type::CreateTexture2D;
        cmd.createTexture2D.id = id;
        cmd.createTexture2D.info = info;
        commands.push_back(cmd);
    }

    inline void UploadTexture2D(Texture2D id, const void* data, size_t size){
        void *copyData = uploadBuffer.AllocateData(size);
        std::memcpy(copyData, data, size);

        Command cmd{};
        cmd.type = Type::UploadTexture2D;
        cmd.uploadTexture2D.id = id;
        cmd.uploadTexture2D.data = copyData;
        cmd.uploadTexture2D.size = size;
        commands.push_back(cmd);
    }

    inline void DestroyTexture2D(Texture2D id){
        Command cmd{};
        cmd.type = Type::DestroyTexture2D;
        cmd.destroyTexture2D.id = id;
        commands.push_back(cmd);
    }

    inline void CreateCubemap(Cubemap id, CubemapInfo& info){
        Command cmd{};
        cmd.type = Type::CreateCubemap;
        cmd.createCubemap.id = id;
        cmd.createCubemap.info = info;
        commands.push_back(cmd);
    }

    inline void UploadCubemap(Cubemap id, const void* data, size_t size){
        void* copyData = uploadBuffer.AllocateData(size);
        std::memcpy(copyData, data, size);
        Command cmd{};
        cmd.type = Type::UploadCubemap;
        cmd.uploadCubemap.id = id;
        cmd.uploadCubemap.data = copyData;
        cmd.uploadCubemap.size = size;
        commands.push_back(cmd);
    }

    inline void DestroyCubemap(Cubemap id){
        Command cmd{};
        cmd.type = Type::DestroyCubemap;
        cmd.destroyCubemap.id = id;
        commands.push_back(cmd);
    }

    inline void CreateBindGroupLayout(BindGroupLayout id, BindGroupLayoutInfo& info){
        BindGroupLayoutInfo* copyData = uploadBuffer.Allocate<BindGroupLayoutInfo>();
        std::memcpy(copyData, &info, sizeof(BindGroupLayoutInfo));
        
        Command cmd{};
        cmd.type = Type::CreateBindGroupLayout;
        cmd.createBindGroupLayout.id = id;
        cmd.createBindGroupLayout.info = copyData;
        commands.push_back(cmd);
    }

    inline void DestroyBindGroupLayout(BindGroupLayout id){
        Command cmd{};
        cmd.type = Type::DestroyBindGroupLayout;
        cmd.destroyBindGroupLayout.id = id;
        commands.push_back(cmd);
    }

    inline void CreateBindGroup(BindGroup id, BindGroupInfo& info){
        BindGroupInfo* copyData = uploadBuffer.Allocate<BindGroupInfo>();
        std::memcpy(copyData, &info, sizeof(BindGroupInfo));
        
        Command cmd{};
        cmd.type = Type::CreateBindGroup;
        cmd.createBindGroup.id = id;
        cmd.createBindGroup.info = copyData;
        commands.push_back(cmd);
    }

    inline void CreateFrameBindGroup(BindGroup id, BindGroupInfo& info){
        BindGroupInfo* copyData = uploadBuffer.Allocate<BindGroupInfo>();
        std::memcpy(copyData, &info, sizeof(BindGroupInfo));
        
        Command cmd{};
        cmd.type = Type::CreateFrameBindGroup;
        cmd.createFrameBindGroup.id = id;
        cmd.createFrameBindGroup.info = copyData;
        commands.push_back(cmd);
    }

    inline void CreateFramebuffer(Framebuffer framebuffer, FrameBufferCreateInfo& info){
        Command cmd{};
        cmd.type = Type::CreateFramebuffer;
        cmd.createFramebuffer.framebuffer = framebuffer;
        cmd.createFramebuffer.info = info;
        commands.push_back(cmd);
    }

    inline void DestroyFramebuffer(Texture2D id){
        Command cmd{};
        cmd.type = Type::DestroyFramebuffer;
        cmd.destroyFramebuffer.id = id;
        commands.push_back(cmd);
    }

    std::vector<Command> commands;
    UploadBuffer uploadBuffer = {};
};

struct OD_API CommandBuffer{
    enum class Type{
        Clear,
        Viewport,
        SetPipeline,
        SetVertexBuffer,
        SetIndexBuffer,
        SetBindGroup,
        Draw,
        DrawIndexed,
        DrawInstanced,
        DrawIndexedInstanced,

        RenderImGui,

        BeginWindowFramebuffer,
        BeginFramebuffer,
        EndFramebuffer,
        BlitFramebuffer,

        CreateBindGroup,
    };

    struct Command{
        Type type;

        union{
            struct {
                uint32_t x, y, w, h;
            } viewport;

            struct{
                ClearFlags flags;
                ClearValue clearValue;
            } clear;

            struct{
                Pipeline id;
            } setPipeline;

            struct{
                uint32_t slot;
                Buffer buffer;
            } setVertexBuffer;

            struct {
                Buffer buffer;
            } setIndexBuffer;

            struct {
                uint8_t slot;
                BindGroup group;
            } setBindGroup;

            struct{
                uint32_t vertexCount;
            } draw;

            struct{
                uint32_t indexCount;
            } drawIndexed;

            struct{
                uint32_t vertexCount;
                uint32_t count;
            } drawInstanced;

            struct{
                uint32_t indexCount;
                uint32_t count;
            } drawIndexedInstanced;

            struct {
                uint32_t snapshotIndex;
            } renderImGui;

            struct {
                Framebuffer framebuffer;
                uint32_t layer;
                uint32_t mip;
                bool clean;
                ClearFlags clearFlags;
                ClearValue clearValue;
            } beginFramebuffer;

            struct {
                Framebuffer src;
                Framebuffer dst;
                int srcPass;
            } blitFramebuffer;
        };
    };

    inline void ClearCmds(){
        commands.clear();
    }

    inline void Clean(ClearFlags flags, const ClearValue& clearValue){
        Command cmd{};
        cmd.type = Type::Clear;
        cmd.clear.flags = flags;
        cmd.clear.clearValue = clearValue;
        commands.push_back(cmd);
    }

    inline void Viewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h){
        Command cmd{};
        cmd.type = Type::Viewport;
        cmd.viewport.x = x;
        cmd.viewport.y = y;
        cmd.viewport.w = w;
        cmd.viewport.h = h;
        commands.push_back(cmd);
    }

    inline void SetPipeline(Pipeline pipeline){
        Command cmd{};
        cmd.type = Type::SetPipeline;
        cmd.setPipeline.id = pipeline;
        commands.push_back(cmd);
    }

    inline void SetVertexBuffer(uint32_t slot, Buffer buffer){
        Command cmd{};
        cmd.type = Type::SetVertexBuffer;
        cmd.setVertexBuffer.slot = slot;
        cmd.setVertexBuffer.buffer = buffer;
        commands.push_back(cmd);
    }

    inline void SetIndexBuffer(Buffer buffer){
        Command cmd{};
        cmd.type = Type::SetIndexBuffer;
        cmd.setIndexBuffer.buffer = buffer;
        commands.push_back(cmd);
    }
    
    inline void SetBindGroup(uint8_t slot, BindGroup group){
        Command cmd{};
        cmd.type = Type::SetBindGroup;
        cmd.setBindGroup.slot = slot;
        cmd.setBindGroup.group = group;
        commands.push_back(cmd);
    }

    inline void Draw(uint32_t vertexCount){
        Command cmd{};
        cmd.type = Type::Draw;
        cmd.draw.vertexCount = vertexCount;
        commands.push_back(cmd);
    }

    inline void DrawIndexed(uint32_t indexCount){
        Command cmd{};
        cmd.type = Type::DrawIndexed;
        cmd.drawIndexed.indexCount = indexCount;
        commands.push_back(cmd);
    }

    inline void DrawInstanced(uint32_t vertexCount, uint32_t count){
        Command cmd{};
        cmd.type = Type::DrawInstanced;
        cmd.drawInstanced.vertexCount = vertexCount;
        cmd.drawInstanced.count = count;
        commands.push_back(cmd);
    }

    inline void DrawIndexedInstanced(uint32_t indexCount, uint32_t count){
        Command cmd{};
        cmd.type = Type::DrawIndexedInstanced;
        cmd.drawIndexedInstanced.indexCount = indexCount;
        cmd.drawIndexedInstanced.count = count;
        commands.push_back(cmd);
    }

    inline void RenderImGui(uint32_t snapshotIndex){
        Command cmd{};
        cmd.type = Type::RenderImGui;
        cmd.renderImGui.snapshotIndex = snapshotIndex;
        commands.push_back(cmd);
    }

    inline void BeginWindowFramebuffer(){
        Command cmd{};
        cmd.type = Type::BeginWindowFramebuffer;
        commands.push_back(cmd);
    }

    inline void BeginFramebuffer(
        Framebuffer framebuffer, uint32_t layer = 0, uint32_t mip = 0,
        bool clean = true, ClearFlags clearFlags = ClearFlags::Color | ClearFlags::Depth,ClearValue clearValue = {}
    ){
        Command cmd{};
        cmd.type = Type::BeginFramebuffer;
        cmd.beginFramebuffer.framebuffer = framebuffer;
        cmd.beginFramebuffer.layer = layer;
        cmd.beginFramebuffer.mip = mip;
        cmd.beginFramebuffer.clean = clean;
        cmd.beginFramebuffer.clearFlags = clearFlags;
        cmd.beginFramebuffer.clearValue = clearValue;
        commands.push_back(cmd);
    }

    inline void EndFramebuffer(){
        Command cmd{};
        cmd.type = Type::EndFramebuffer;
        commands.push_back(cmd);
    }

    inline void BlitFramebuffer(Framebuffer src, Framebuffer dst = InvalidID, int srcPass = -1){
        Command cmd{};
        cmd.type = Type::BlitFramebuffer;
        cmd.blitFramebuffer.src = src;
        cmd.blitFramebuffer.dst = dst;
        cmd.blitFramebuffer.srcPass = srcPass;
        commands.push_back(cmd);
    }

    std::vector<Command> commands;
};

struct OD_API RenderFrame{
    struct ImGuiSnapshot{
        void* data = nullptr;
        ImGuiDrawDataDestroyFunction destroy = nullptr;
    };

    ResourceCommands resourceCommands = {};
    CommandBuffer renderCommands = {};
    std::vector<ImGuiSnapshot> imguiSnapshots;

    inline void RecordImGuiDrawData(void* data, ImGuiDrawDataDestroyFunction destroy){
        const uint32_t snapshotIndex = static_cast<uint32_t>(imguiSnapshots.size());
        imguiSnapshots.push_back({data, destroy});

        auto insertAt = renderCommands.commands.end();
        for(auto it = renderCommands.commands.end(); it != renderCommands.commands.begin();){
            --it;
            if(it->type == CommandBuffer::Type::EndFramebuffer){
                insertAt = it;
                break;
            }
        }

        if(insertAt == renderCommands.commands.end()){
            renderCommands.RenderImGui(snapshotIndex);
            return;
        }

        CommandBuffer::Command command{};
        command.type = CommandBuffer::Type::RenderImGui;
        command.renderImGui.snapshotIndex = snapshotIndex;
        renderCommands.commands.insert(insertAt, command);
    }

    inline void Clear(){
        for(auto& snapshot : imguiSnapshots){
            if(snapshot.destroy != nullptr && snapshot.data != nullptr)
                snapshot.destroy(snapshot.data);
        }
        imguiSnapshots.clear();
        resourceCommands.Clear();
        renderCommands.ClearCmds();
    }
};

class OD_API Device{
public:
    virtual ~Device(){}

    virtual bool SupportMultithread(){ return false; }

    virtual void Init(bool multithread){}
    virtual void Shut(){}
    virtual void StartRender(){}
    virtual void UpdateRender(){}

    // These methods are called by the platform layer and renderer thread.
    virtual bool ImGuiSupported() const { return false; }
    virtual void ImGuiInitialize(){}
    virtual void ImGuiNewFrame(){}
    virtual void SubmitImGuiDrawData(void* data, ImGuiDrawDataDestroyFunction destroy){}
    virtual void ImGuiShutdown(){}

    virtual ResourceStats GetBufferStats(Buffer id){ return {}; }

    virtual CommandBuffer* GetCommandBuffer(){ return nullptr; }
    virtual FrameBufferLayout GetWindowFrameBufferLayout(){ return {}; }

    virtual Pipeline CreatePipeline(const char* source, PipelineInfo info){ return InvalidID; }
    virtual void DestroyPipeline(Pipeline id){}

    virtual Buffer CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory){ return InvalidID; }
    virtual void UpdatedBuffer(Buffer buffer, const void* data, size_t size){}
    virtual void DestroyBuffer(Buffer id){}

    virtual Texture2D CreateTexture2D(Texture2DInfo& info){ return InvalidID; }
    virtual void UploadTexture2D(Texture2D texture, const void* data, size_t size){}
    virtual void DestroyTexture2D(Texture2D tex){}
    virtual Cubemap CreateCubemap(CubemapInfo& info){ return InvalidID; }
    virtual void UploadCubemap(Cubemap cubemap, const void* data, size_t size){}
    virtual void DestroyCubemap(Cubemap cubemap){}

    virtual BindGroupLayout CreateBindGroupLayout(BindGroupLayoutInfo& info){ return InvalidID; }
    virtual void DestroyBindGroupLayout(BindGroupLayout layout){}

    virtual BindGroup CreateBindGroup(BindGroupInfo& info){ return InvalidID; }
    virtual BindGroup CreateFrameBindGroup(BindGroupInfo& info){ return InvalidID; }

    virtual Framebuffer CreateFramebuffer(FrameBufferCreateInfo& info){ return InvalidID; }
    virtual void DestroyFramebuffer(Framebuffer destroy){}
};

}
}
