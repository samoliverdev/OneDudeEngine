#pragma once
#include "OD/Graphics/ColorReadback.h"
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Core/Math.h"
#include "OD/Gfx/ImGuiImages.h"
#include "OD/Gfx/RenderingMode.h"
#include <unordered_map>
#include <unordered_set>
#ifdef TEST_NEW_CMD
#include "OD/Gfx/FastCommandBuffer.h"
#endif
#include <thread>
#include <mutex>

namespace OD{
namespace Gfx{   

using ImGuiDrawDataDestroyFunction = void(*)(void*);

static constexpr uint32_t InvalidID = (std::numeric_limits<uint32_t>::max)();
using Texture = uint32_t;
using Pipeline = uint32_t;
using Framebuffer = uint32_t;
using Buffer = uint32_t;
using BindGroupLayout = uint32_t;
using BindGroup = uint32_t;
using Texture2D = uint32_t;
using Texture2DArray = uint32_t;
using Cubemap = uint32_t;

enum class CubemapFace : uint8_t{
    // Values follow cubemap image layer order: +X, -X, +Y, -Y, +Z, -Z.
    PositiveX,
    NegativeX,
    PositiveY,
    NegativeY,
    PositiveZ,
    NegativeZ
};

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
};

struct OD_API FramebufferDepthAttachment{
    FramebufferDepthTextureFormat format;
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
    uint32_t mipLevels = 1; // Allocated count for all attachments, including level 0.
    TextureFilter filter = TextureFilter::Linear;
    TextureWrapping wrapping = TextureWrapping::Repeat;
};

//////////////////////////////////////

enum class ImageFormat{
    R8_UNORM,

    R8G8B8_UNORM,
    R8G8B8_SRGB,

    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    RGB11B10F, RGB16F, RGBA16F, RGB32F, RGBA32F, RED_INTEGER,
    DEPTH24_STENCIL8, DEPTH32F_STENCIL8, DEPTH_COMPONENT16,
    DEPTH_COMPONENT24, DEPTH_COMPONENT32, DEPTH_COMPONENT32F,
    R32F
};

struct OD_API Texture2DInfo{
    uint32_t width = 1;
    uint32_t height = 1;
    ImageFormat format = ImageFormat::R8G8B8A8_UNORM;
    TextureFilter filter = TextureFilter::Linear;
    TextureWrapping wrapping = TextureWrapping::Repeat;
    bool mipmap = true;
    // Zero selects the full chain when mipmap is enabled. A nonzero value
    // allocates that many levels and disables automatic mip generation.
    uint32_t mipLevels = 0;
};

struct OD_API Texture2DArrayInfo{
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t arrayElements = 1;
    ImageFormat format = ImageFormat::R8G8B8A8_UNORM;
    TextureFilter filter = TextureFilter::Linear;
    TextureWrapping wrapping = TextureWrapping::Repeat;
    bool mipmap = true;
    // Zero selects the full chain when mipmap is enabled. A nonzero value
    // allocates that many levels and disables automatic mip generation.
    uint32_t mipLevels = 0;
};

struct OD_API CubemapInfo{
    uint32_t width = 1;
    uint32_t height = 1;
    ImageFormat format = ImageFormat::R8G8B8A8_UNORM;
    TextureFilter filter = TextureFilter::Linear;
    TextureWrapping wrapping = TextureWrapping::Repeat;
    bool mipmap = true;
    // Zero selects the full chain when mipmap is enabled. A nonzero value
    // allocates that many levels and disables automatic mip generation.
    uint32_t mipLevels = 0;
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

enum class BindingType: uint8_t{
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
    BindLayoutEntry entries[64];
    uint32_t entriesCount = 0;
};

//////////////////////////////////////

struct BindingEntry{
    uint32_t binding = 0;

    /*union{
        struct{
            Buffer buffer;
            size_t offset;
            size_t size;
        };

        Texture2D texture;
        Cubemap cubemap;

        struct {
            Framebuffer framebuffer;
            uint16_t framebufferAttacement;
            uint16_t framebufferLayer;
        };
    };
    bool dynamicOffset = false;
    */

    Buffer buffer = InvalidID;
    size_t offset = 0;
    size_t size = 0;
    bool dynamicOffset = false;

    Texture2D texture = InvalidID;
    Texture2DArray textureArray = InvalidID;
    Cubemap cubemap = InvalidID;

    Framebuffer framebuffer = InvalidID;
    int framebufferAttacement = 0;
    uint32_t framebufferLayer = 0;
};

struct BindGroupInfo{
    BindGroupLayout layout = Gfx::InvalidID;
    BindingEntry* entries = nullptr;
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
    static constexpr size_t DefaultBlockSize = 64 * 1024;

    UploadBuffer(size_t blockSize = DefaultBlockSize): blockSize(std::max<size_t>(blockSize, 1)){}
    UploadBuffer(const UploadBuffer&) = delete;
    UploadBuffer& operator=(const UploadBuffer&) = delete;
    UploadBuffer(UploadBuffer&&) noexcept = default;
    UploadBuffer& operator=(UploadBuffer&&) noexcept = default;

    template<typename T>
    T* Allocate(size_t count = 1){
        return static_cast<T*>(AllocateData(sizeof(T) * count, alignof(T)));
    }

    void* AllocateData(size_t size, size_t alignment = alignof(std::max_align_t)){
        /*if(size == 0) return nullptr;

        if(blocks.empty()){
            blocks.push_back(std::make_unique<MemoryBlock>((std::max)(DefaultBlockSize, size)));
        }

        MemoryBlock* block = blocks[currentBlock].get();

        if(void* ptr = block->Allocate(size, alignment)) return ptr;

        // Current block doesn't have enough space.
        currentBlock++;

        if(currentBlock >= blocks.size()){
            blocks.push_back(std::make_unique<MemoryBlock>((std::max)(DefaultBlockSize, size)));
        }

        return blocks[currentBlock]->Allocate(size, alignment);*/

        if(size == 0) return nullptr;

        while(currentBlock < blocks.size()){
            if(void* ptr = blocks[currentBlock]->Allocate(size, alignment)) return ptr;
            ++currentBlock;
        }

        blocks.push_back(std::make_unique<MemoryBlock>((std::max)(DefaultBlockSize, size)));
        currentBlock = blocks.size() - 1;

        return blocks[currentBlock]->Allocate(size, alignment);
    }

    void Clear(){
        // Don't free the blocks.
        // Just reset them for reuse.
        for(auto& block : blocks)
            block->used = 0;

        currentBlock = 0;
    }

    // Free unused blocks from the end.
    //
    // Safe to call during use because blocks containing allocations
    // are not removed.
    //
    // Example:
    //      uploadBuffer.FreeUnusedBlocks(2);
    //
    // Keeps at least 2 blocks allocated.
    void FreeUnusedBlocks(size_t keepBlockCount = 1){
        while(
            blocks.size() > keepBlockCount &&
            !blocks.empty() &&
            blocks.back()->used == 0
        ){
            blocks.pop_back();
        }

        if(blocks.empty()){
            currentBlock = 0;
        }
        else if(currentBlock >= blocks.size()){
            currentBlock = blocks.size() - 1;
        }
    }

    // Completely release all memory.
    //
    // IMPORTANT: pointers previously returned by Allocate()
    // become invalid.
    void FreeAll(){
        blocks.clear();
        currentBlock = 0;
    }

    size_t GetBlockSize() const{
        return blockSize;
    }

    size_t GetBlockCount() const{
        return blocks.size();
    }

    size_t GetAllocatedCapacity() const{
        size_t result = 0;

        for(const auto& block : blocks)
            result += block->capacity;

        return result;
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
    size_t blockSize = DefaultBlockSize;
};

struct OD_API ResourceCommands{
#ifdef TEST_NEW_CMD
    enum class Type : uint16_t{
#else
    enum class Type{
#endif
        CreatePipeline,
        DestroyPipeline,

        CreateBuffer,
        UpdateBuffer,
        DestroyBuffer,

        CreateTexture2D,
        UploadTexture2D,
        DestroyTexture2D,
        CreateTexture2DArray,
        UploadTexture2DArray,
        DestroyTexture2DArray,
        CreateCubemap,
        UploadCubemap,
        UploadCubemapFace,
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
                const char* source;
                PipelineInfo info;
            } createPipeline;

            struct{
                Pipeline id;
            } destroyPipeline;

            struct {
                BindGroupLayout id; const BindGroupLayoutInfo* info;
            } createBindGroupLayout;

            struct {
                BindGroup id; BindGroupInfo info;
            } createBindGroup;

            struct {
                BindGroup id; BindGroupInfo info;
            } createFrameBindGroup;

            struct {
                Texture2D id;
                Texture2DInfo info;
            } createTexture2D;

            struct {
                Texture2D id;
                const void* data;
                size_t size;
                int mipLevel;
                const void* staging;
            } uploadTexture2D;

            struct { Texture2DArray id; Texture2DArrayInfo info; } createTexture2DArray;
            struct { Texture2DArray id; const void* data; size_t size; int arrayElement; int mipLevel; const void* staging; } uploadTexture2DArray;
            struct { Texture2DArray id; } destroyTexture2DArray;
            
            struct {
                Framebuffer framebuffer;
                FrameBufferCreateInfo info;
            } createFramebuffer;
            
            struct{
                Buffer id;
                const void* data;
                size_t size;
                const void* staging;
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
                int mipLevel;
                const void* staging;
            } uploadCubemap;

            struct {
                Cubemap id;
                const void* data;
                size_t size;
                CubemapFace face;
                int mipLevel;
                const void* staging;
            } uploadCubemapFace;

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


#ifdef TEST_NEW_CMD
    using FastCommands = FCB::FastCommandBuffer<Type, 8>;
    using ExecuteCommand = void (*)(void*, Type, const void*);

    void SetImmediateExecutor(void* context, ExecuteCommand executor){
        Assert(executeImmediate == nullptr && executor != nullptr && commands.GetUsedBytes() == 0);
        if(executeImmediate == nullptr && commands.GetUsedBytes() == 0){
            executorContext = context;
            executeImmediate = executor;
        }
    }
#else
    using ExecuteCommand = void (*)(void*, const Command&);

    void SetImmediateExecutor(void* context, ExecuteCommand executor){
        Assert(executeImmediate == nullptr && executor != nullptr && commands.empty());
        if(executeImmediate == nullptr && commands.empty()){
            executorContext = context;
            executeImmediate = executor;
        }
    }

    bool ExecutesImmediately() const { return executeImmediate != nullptr; }

    void Dispatch(const Command& cmd){
        if(executeImmediate != nullptr){
            executeImmediate(executorContext, cmd);
        } else {
            commands.push_back(cmd);
        }
    }
#endif

#ifdef TEST_NEW_CMD
    bool ExecutesImmediately() const { return executeImmediate != nullptr; }
#endif

    inline void Clear(){
#ifdef TEST_NEW_CMD
        commands.Reset();
#else
        commands.clear();
#endif
        smallUploads.Clear();
        renderUploads.Clear();
        resourceUploads.Clear();
    }

    inline void CreatePipeline(Pipeline id, const char* source, PipelineInfo info){
        size_t size = std::strlen(source) + 1;
        const auto* copyData = ExecutesImmediately() ? source : CopyUpload(resourceUploads, source, size);

#ifdef TEST_NEW_CMD
        DispatchFast<Type::CreatePipeline, &Command::createPipeline>(id, copyData, info);
#else
        Command cmd{};
        cmd.type = Type::CreatePipeline;
        cmd.createPipeline.id = id;
        cmd.createPipeline.source = copyData;
        cmd.createPipeline.info = info;
        Dispatch(cmd);
#endif
    }

    inline void DestroyPipeline(Pipeline id){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DestroyPipeline, &Command::destroyPipeline>(id);
#else
        Command cmd{};
        cmd.type = Type::DestroyPipeline;
        cmd.destroyPipeline.id = id;
        Dispatch(cmd);
#endif
    }

    inline void CreateBuffer(Buffer id, size_t size, BufferUsage usage, BufferMemory memory = BufferMemory::GPUOnly){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::CreateBuffer, &Command::createBuffer>(id, usage, memory, size);
#else
        Command cmd{};
        cmd.type = Type::CreateBuffer;
        cmd.createBuffer.usage = usage;
        cmd.createBuffer.id = id;
        cmd.createBuffer.size = size;
        cmd.createBuffer.memory = memory;
        Dispatch(cmd);
#endif
    }

    inline void UpdatedBuffer(Buffer id, const void* data, size_t size, const void* staging = nullptr){
        const auto* copyData = (ExecutesImmediately() || staging != nullptr) ? data : CopyUpload(renderUploads, data, size);

#ifdef TEST_NEW_CMD
        DispatchFast<Type::UpdateBuffer, &Command::updateBuffer>(id, copyData, size, staging);
#else
        Command cmd{};
        cmd.type = Type::UpdateBuffer;
        cmd.updateBuffer.id = id;
        cmd.updateBuffer.data = copyData;
        cmd.updateBuffer.size = size;
        cmd.updateBuffer.staging = staging;
        Dispatch(cmd);
#endif
    }

    inline void DestroyBuffer(Buffer id){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DestroyBuffer, &Command::destroyBuffer>(id);
#else
        Command cmd{};
        cmd.type = Type::DestroyBuffer;
        cmd.destroyBuffer.id = id;
        Dispatch(cmd);
#endif
    }

    inline void CreateTexture2D(Texture2D id, Texture2DInfo& info){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::CreateTexture2D, &Command::createTexture2D>(id, info);
#else
        Command cmd{};
        cmd.type = Type::CreateTexture2D;
        cmd.createTexture2D.id = id;
        cmd.createTexture2D.info = info;
        Dispatch(cmd);
#endif
    }

    inline void UploadTexture2D(Texture2D id, const void* data, size_t size, int mipLevel = 0, const void* staging = nullptr){
        const auto* copyData = (ExecutesImmediately() || staging != nullptr) ? data : CopyUpload(resourceUploads, data, size);

#ifdef TEST_NEW_CMD
        DispatchFast<Type::UploadTexture2D, &Command::uploadTexture2D>(id, copyData, size, mipLevel, staging);
#else
        Command cmd{};
        cmd.type = Type::UploadTexture2D;
        cmd.uploadTexture2D.id = id;
        cmd.uploadTexture2D.data = copyData;
        cmd.uploadTexture2D.size = size;
        cmd.uploadTexture2D.mipLevel = mipLevel;
        cmd.uploadTexture2D.staging = staging;
        Dispatch(cmd);
#endif
    }

    inline void DestroyTexture2D(Texture2D id){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DestroyTexture2D, &Command::destroyTexture2D>(id);
#else
        Command cmd{};
        cmd.type = Type::DestroyTexture2D;
        cmd.destroyTexture2D.id = id;
        Dispatch(cmd);
#endif
    }

    inline void CreateTexture2DArray(Texture2DArray id, Texture2DArrayInfo& info){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::CreateTexture2DArray, &Command::createTexture2DArray>(id, info);
#else
        Command cmd{}; cmd.type = Type::CreateTexture2DArray;
        cmd.createTexture2DArray.id = id; cmd.createTexture2DArray.info = info;
        Dispatch(cmd);
#endif
    }

    inline void UploadTexture2DArray(Texture2DArray id, const void* data, size_t size, int arrayElement, int mipLevel, const void* staging = nullptr){
        const auto* copyData = (ExecutesImmediately() || staging != nullptr) ? data : CopyUpload(resourceUploads, data, size);
#ifdef TEST_NEW_CMD
        DispatchFast<Type::UploadTexture2DArray, &Command::uploadTexture2DArray>(id, copyData, size, arrayElement, mipLevel, staging);
#else
        Command cmd{}; cmd.type = Type::UploadTexture2DArray;
        cmd.uploadTexture2DArray.id = id; cmd.uploadTexture2DArray.data = copyData;
        cmd.uploadTexture2DArray.size = size; cmd.uploadTexture2DArray.arrayElement = arrayElement;
        cmd.uploadTexture2DArray.mipLevel = mipLevel;
        cmd.uploadTexture2DArray.staging = staging;
        Dispatch(cmd);
#endif
    }

    inline void DestroyTexture2DArray(Texture2DArray id){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DestroyTexture2DArray, &Command::destroyTexture2DArray>(id);
#else
        Command cmd{}; cmd.type = Type::DestroyTexture2DArray; cmd.destroyTexture2DArray.id = id;
        Dispatch(cmd);
#endif
    }

    inline void CreateCubemap(Cubemap id, CubemapInfo& info){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::CreateCubemap, &Command::createCubemap>(id, info);
#else
        Command cmd{};
        cmd.type = Type::CreateCubemap;
        cmd.createCubemap.id = id;
        cmd.createCubemap.info = info;
        Dispatch(cmd);
#endif
    }

    inline void UploadCubemap(Cubemap id, const void* data, size_t size, int mipLevel = 0, const void* staging = nullptr){
        const auto* copyData = (ExecutesImmediately() || staging != nullptr) ? data : CopyUpload(resourceUploads, data, size);
#ifdef TEST_NEW_CMD
        DispatchFast<Type::UploadCubemap, &Command::uploadCubemap>(id, copyData, size, mipLevel, staging);
#else
        Command cmd{};
        cmd.type = Type::UploadCubemap;
        cmd.uploadCubemap.id = id;
        cmd.uploadCubemap.data = copyData;
        cmd.uploadCubemap.size = size;
        cmd.uploadCubemap.mipLevel = mipLevel;
        cmd.uploadCubemap.staging = staging;
        Dispatch(cmd);
#endif
    }

    inline void UploadCubemap(Cubemap id, const void* data, size_t size, CubemapFace face, int mipLevel = 0, const void* staging = nullptr){
        const auto* copyData = (ExecutesImmediately() || staging != nullptr) ? data : CopyUpload(resourceUploads, data, size);
#ifdef TEST_NEW_CMD
        DispatchFast<Type::UploadCubemapFace, &Command::uploadCubemapFace>(id, copyData, size, face, mipLevel, staging);
#else
        Command cmd{};
        cmd.type = Type::UploadCubemapFace;
        cmd.uploadCubemapFace.id = id;
        cmd.uploadCubemapFace.data = copyData;
        cmd.uploadCubemapFace.size = size;
        cmd.uploadCubemapFace.face = face;
        cmd.uploadCubemapFace.mipLevel = mipLevel;
        cmd.uploadCubemapFace.staging = staging;
        Dispatch(cmd);
#endif
    }

    inline void DestroyCubemap(Cubemap id){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DestroyCubemap, &Command::destroyCubemap>(id);
#else
        Command cmd{};
        cmd.type = Type::DestroyCubemap;
        cmd.destroyCubemap.id = id;
        Dispatch(cmd);
#endif
    }

    inline void CreateBindGroupLayout(BindGroupLayout id, BindGroupLayoutInfo& info){
        const BindGroupLayoutInfo* copyData = &info;
        if(!ExecutesImmediately()){
            auto* owned = smallUploads.Allocate<BindGroupLayoutInfo>();
            ::new(static_cast<void*>(owned)) BindGroupLayoutInfo(info);
            copyData = owned;
        }
        
#ifdef TEST_NEW_CMD
        DispatchFast<Type::CreateBindGroupLayout, &Command::createBindGroupLayout>(id, copyData);
#else
        Command cmd{};
        cmd.type = Type::CreateBindGroupLayout;
        cmd.createBindGroupLayout.id = id;
        cmd.createBindGroupLayout.info = copyData;
        Dispatch(cmd);
#endif
    }

    inline void DestroyBindGroupLayout(BindGroupLayout id){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DestroyBindGroupLayout, &Command::destroyBindGroupLayout>(id);
#else
        Command cmd{};
        cmd.type = Type::DestroyBindGroupLayout;
        cmd.destroyBindGroupLayout.id = id;
        Dispatch(cmd);
#endif
    }

    inline void CreateBindGroup(BindGroup id, BindGroupInfo& info){
        auto* entries = info.entries;
        if(!ExecutesImmediately() && info.entriesCount != 0){
            auto* owned = smallUploads.Allocate<BindingEntry>(info.entriesCount);
            std::memcpy(owned, info.entries, sizeof(BindingEntry) * info.entriesCount);
            entries = owned;
        }
        
#ifdef TEST_NEW_CMD
        auto recordedInfo = info;
        recordedInfo.entries = entries;
        DispatchFast<Type::CreateBindGroup, &Command::createBindGroup>(id, recordedInfo);
#else
        Command cmd{};
        cmd.type = Type::CreateBindGroup;
        cmd.createBindGroup.id = id;
        cmd.createBindGroup.info = info;
        cmd.createBindGroup.info.entries = entries;
        Dispatch(cmd);
#endif
    }

    inline void CreateFrameBindGroup(BindGroup id, BindGroupInfo& info){
        auto* entries = info.entries;
        if(!ExecutesImmediately() && info.entriesCount != 0){
            auto* owned = smallUploads.Allocate<BindingEntry>(info.entriesCount);
            std::memcpy(owned, info.entries, sizeof(BindingEntry) * info.entriesCount);
            entries = owned;
        }
        
#ifdef TEST_NEW_CMD
        auto recordedInfo = info;
        recordedInfo.entries = entries;
        DispatchFast<Type::CreateFrameBindGroup, &Command::createFrameBindGroup>(id, recordedInfo);
#else
        Command cmd{};
        cmd.type = Type::CreateFrameBindGroup;
        cmd.createFrameBindGroup.id = id;
        cmd.createFrameBindGroup.info = info;
        cmd.createFrameBindGroup.info.entries = entries;
        Dispatch(cmd);
#endif
    }

    inline void CreateFramebuffer(Framebuffer framebuffer, FrameBufferCreateInfo& info){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::CreateFramebuffer, &Command::createFramebuffer>(framebuffer, info);
#else
        Command cmd{};
        cmd.type = Type::CreateFramebuffer;
        cmd.createFramebuffer.framebuffer = framebuffer;
        cmd.createFramebuffer.info = info;
        Dispatch(cmd);
#endif
    }

    inline void DestroyFramebuffer(Texture2D id){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DestroyFramebuffer, &Command::destroyFramebuffer>(id);
#else
        Command cmd{};
        cmd.type = Type::DestroyFramebuffer;
        cmd.destroyFramebuffer.id = id;
        Dispatch(cmd);
#endif
    }

#ifdef TEST_NEW_CMD
    FastCommands commands;
#else
    std::vector<Command> commands;
#endif

    UploadBuffer smallUploads{64 * 1024}; // 64 KB
    UploadBuffer renderUploads{256 * 1024};  // 256 KB
    UploadBuffer resourceUploads{1024 * 1024}; // 1 MB
private:
    template<typename Arena, typename T>
    static const T* CopyUpload(Arena& arena, const T* data, size_t size){
        if(size == 0) return nullptr;
        auto* copy = static_cast<T*>(arena.AllocateData(size));
        std::memcpy(copy, data, size);
        return copy;
    }

#ifdef TEST_NEW_CMD
    template<Type Tag, auto Member, typename... Args>
    void DispatchFast(Args&&... args){
        using Payload = std::remove_reference_t<decltype(std::declval<Command&>().*Member)>;
        if(executeImmediate != nullptr){
            const Payload payload{std::forward<Args>(args)...};
            executeImmediate(executorContext, Tag, &payload);
        } else {
            commands.Emplace<Tag, Payload>(std::forward<Args>(args)...);
        }
    }
#endif
    void* executorContext = nullptr;
    ExecuteCommand executeImmediate = nullptr;
};

struct OD_API CommandBuffer{
#ifdef TEST_NEW_CMD
    CommandBuffer() = default;
    CommandBuffer(const CommandBuffer&) = default;
    CommandBuffer& operator=(const CommandBuffer&) = default;

    CommandBuffer(CommandBuffer&& other) noexcept
        : commands(std::move(other.commands)),
          windowEnd(std::exchange(other.windowEnd, NoWindowEnd)),
          waitingForWindowEnd(std::exchange(other.waitingForWindowEnd, false)),
          framebufferUses(std::move(other.framebufferUses)),
          framebufferPassOpen(std::exchange(other.framebufferPassOpen, false)){}

    CommandBuffer& operator=(CommandBuffer&& other) noexcept{
        if(this != &other){
            commands = std::move(other.commands);
            windowEnd = std::exchange(other.windowEnd, NoWindowEnd);
            waitingForWindowEnd = std::exchange(other.waitingForWindowEnd, false);
            framebufferUses = std::move(other.framebufferUses);
            framebufferPassOpen = std::exchange(other.framebufferPassOpen, false);
        }
        return *this;
    }
#endif

#ifdef TEST_NEW_CMD
    enum class Type : uint16_t{
#else
    enum class Type{
#endif
        Clear,
        Viewport,
        Scissor,
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
        CopyTexture2D,
        CopyCubemap,

        CreateBindGroup,
    };

    struct Command{
        Type type;

        union{
#ifdef TEST_NEW_CMD
            struct {} endFramebuffer;
#endif

            struct {
                bool preserveColor;
            } beginWindowFramebuffer;

            struct {
                uint32_t x, y, w, h;
            } viewport;

            struct {
                uint32_t x, y;
                int32_t w, h;
            } scissor;

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

            struct { Framebuffer src; int attachment; Texture2D dst; } copyTexture2D;
            struct { Framebuffer src; int attachment; Cubemap dst; } copyCubemap;
        };
    };


#ifdef TEST_NEW_CMD
    using FastCommands = FCB::FastCommandBuffer<Type, 8>;
    using ExecuteCommand = void (*)(void*, Type, const void*);

    void SetImmediateExecutor(void* context, ExecuteCommand executor){
        Assert(executeImmediate == nullptr && executor != nullptr && commands.GetUsedBytes() == 0);
        if(executeImmediate == nullptr && commands.GetUsedBytes() == 0){
            executorContext = context;
            executeImmediate = executor;
        }
    }
#else
    using ExecuteCommand = void (*)(void*, const Command&);

    void SetImmediateExecutor(void* context, ExecuteCommand executor){
        Assert(executeImmediate == nullptr && executor != nullptr && commands.empty());
        if(executeImmediate == nullptr && commands.empty()){
            executorContext = context;
            executeImmediate = executor;
        }
    }

    bool ExecutesImmediately() const { return executeImmediate != nullptr; }

    void Dispatch(const Command& cmd){
        if(executeImmediate != nullptr){
            executeImmediate(executorContext, cmd);
        } else {
            commands.push_back(cmd);
        }
    }
#endif

#ifdef TEST_NEW_CMD
    bool ExecutesImmediately() const { return executeImmediate != nullptr; }
#endif

    inline void ClearCmds(){
        framebufferUses.clear();
        framebufferPassOpen = false;
#ifdef TEST_NEW_CMD
        commands.Reset();
        windowEnd = NoWindowEnd;
        waitingForWindowEnd = false;
#else
        commands.clear();
#endif
    }

    inline void Clean(ClearFlags flags, const ClearValue& clearValue){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::Clear, &Command::clear>(flags, clearValue);
#else
        Command cmd{};
        cmd.type = Type::Clear;
        cmd.clear.flags = flags;
        cmd.clear.clearValue = clearValue;
        Dispatch(cmd);
#endif
    }

    inline void Viewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::Viewport, &Command::viewport>(x, y, w, h);
#else
        Command cmd{};
        cmd.type = Type::Viewport;
        cmd.viewport.x = x;
        cmd.viewport.y = y;
        cmd.viewport.w = w;
        cmd.viewport.h = h;
        Dispatch(cmd);
#endif
    }

    inline void Scissor(uint32_t x, uint32_t y, int32_t w, int32_t h){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::Scissor, &Command::scissor>(x, y, w, h);
#else
        Command cmd{};
        cmd.type = Type::Scissor;
        cmd.scissor.x = x;
        cmd.scissor.y = y;
        cmd.scissor.w = w;
        cmd.scissor.h = h;
        Dispatch(cmd);
#endif
    }

    inline void SetPipeline(Pipeline pipeline){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::SetPipeline, &Command::setPipeline>(pipeline);
#else
        Command cmd{};
        cmd.type = Type::SetPipeline;
        cmd.setPipeline.id = pipeline;
        Dispatch(cmd);
#endif
    }

    inline void SetVertexBuffer(uint32_t slot, Buffer buffer){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::SetVertexBuffer, &Command::setVertexBuffer>(slot, buffer);
#else
        Command cmd{};
        cmd.type = Type::SetVertexBuffer;
        cmd.setVertexBuffer.slot = slot;
        cmd.setVertexBuffer.buffer = buffer;
        Dispatch(cmd);
#endif
    }

    inline void SetIndexBuffer(Buffer buffer){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::SetIndexBuffer, &Command::setIndexBuffer>(buffer);
#else
        Command cmd{};
        cmd.type = Type::SetIndexBuffer;
        cmd.setIndexBuffer.buffer = buffer;
        Dispatch(cmd);
#endif
    }
    
    inline void SetBindGroup(uint8_t slot, BindGroup group){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::SetBindGroup, &Command::setBindGroup>(slot, group);
#else
        Command cmd{};
        cmd.type = Type::SetBindGroup;
        cmd.setBindGroup.slot = slot;
        cmd.setBindGroup.group = group;
        Dispatch(cmd);
#endif
    }

    inline void Draw(uint32_t vertexCount){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::Draw, &Command::draw>(vertexCount);
#else
        Command cmd{};
        cmd.type = Type::Draw;
        cmd.draw.vertexCount = vertexCount;
        Dispatch(cmd);
#endif
    }

    inline void DrawIndexed(uint32_t indexCount){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DrawIndexed, &Command::drawIndexed>(indexCount);
#else
        Command cmd{};
        cmd.type = Type::DrawIndexed;
        cmd.drawIndexed.indexCount = indexCount;
        Dispatch(cmd);
#endif
    }

    inline void DrawInstanced(uint32_t vertexCount, uint32_t count){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DrawInstanced, &Command::drawInstanced>(vertexCount, count);
#else
        Command cmd{};
        cmd.type = Type::DrawInstanced;
        cmd.drawInstanced.vertexCount = vertexCount;
        cmd.drawInstanced.count = count;
        Dispatch(cmd);
#endif
    }

    inline void DrawIndexedInstanced(uint32_t indexCount, uint32_t count){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::DrawIndexedInstanced, &Command::drawIndexedInstanced>(indexCount, count);
#else
        Command cmd{};
        cmd.type = Type::DrawIndexedInstanced;
        cmd.drawIndexedInstanced.indexCount = indexCount;
        cmd.drawIndexedInstanced.count = count;
        Dispatch(cmd);
#endif
    }

    inline void RenderImGui(uint32_t snapshotIndex){
#ifdef TEST_NEW_CMD
        DispatchFast<Type::RenderImGui, &Command::renderImGui>(snapshotIndex);
#else
        Command cmd{};
        cmd.type = Type::RenderImGui;
        cmd.renderImGui.snapshotIndex = snapshotIndex;
        Dispatch(cmd);
#endif
    }

    inline void BeginWindowFramebuffer(bool preserveColor = false){
        framebufferPassOpen = true;
#ifdef TEST_NEW_CMD
        waitingForWindowEnd = true;
        windowEnd = NoWindowEnd;
        DispatchFast<Type::BeginWindowFramebuffer, &Command::beginWindowFramebuffer>(preserveColor);
#else
        Command cmd{};
        cmd.type = Type::BeginWindowFramebuffer;
        cmd.beginWindowFramebuffer.preserveColor = preserveColor;
        Dispatch(cmd);
#endif
    }

    inline void BeginFramebuffer(
        Framebuffer framebuffer, uint32_t layer = 0, uint32_t mip = 0,
        bool clean = true, ClearFlags clearFlags = ClearFlags::Color | ClearFlags::Depth,ClearValue clearValue = {}
    ){
        TrackFramebufferUse(framebuffer);
        framebufferPassOpen = true;
#ifdef TEST_NEW_CMD
        DispatchFast<Type::BeginFramebuffer, &Command::beginFramebuffer>(framebuffer, layer, mip, clean, clearFlags, clearValue);
#else
        Command cmd{};
        cmd.type = Type::BeginFramebuffer;
        cmd.beginFramebuffer.framebuffer = framebuffer;
        cmd.beginFramebuffer.layer = layer;
        cmd.beginFramebuffer.mip = mip;
        cmd.beginFramebuffer.clean = clean;
        cmd.beginFramebuffer.clearFlags = clearFlags;
        cmd.beginFramebuffer.clearValue = clearValue;
        Dispatch(cmd);
#endif
    }

    inline void EndFramebuffer(){
#ifdef TEST_NEW_CMD
        if(waitingForWindowEnd){
            windowEnd = commands.GetUsedBytes();
            waitingForWindowEnd = false;
        }
        DispatchFast<Type::EndFramebuffer, &Command::endFramebuffer>();
#else
        Command cmd{};
        cmd.type = Type::EndFramebuffer;
        Dispatch(cmd);
#endif
        framebufferPassOpen = false;
    }

    inline void BlitFramebuffer(Framebuffer src, Framebuffer dst = InvalidID, int srcPass = 0){
        TrackFramebufferUse(src);
        TrackFramebufferUse(dst);
#ifdef TEST_NEW_CMD
        DispatchFast<Type::BlitFramebuffer, &Command::blitFramebuffer>(src, dst, srcPass);
#else
        Command cmd{};
        cmd.type = Type::BlitFramebuffer;
        cmd.blitFramebuffer.src = src;
        cmd.blitFramebuffer.dst = dst;
        cmd.blitFramebuffer.srcPass = srcPass;
        Dispatch(cmd);
#endif
    }

    inline void CopyTexture(Framebuffer src, int attachment, Texture2D dst){
        TrackFramebufferUse(src);
#ifdef TEST_NEW_CMD
        DispatchFast<Type::CopyTexture2D, &Command::copyTexture2D>(src, attachment, dst);
#else
        Command cmd{};
        cmd.type = Type::CopyTexture2D;
        cmd.copyTexture2D.src = src;
        cmd.copyTexture2D.attachment = attachment;
        cmd.copyTexture2D.dst = dst;
        Dispatch(cmd);
#endif
    }

    inline void CopyTextureCubemap(Framebuffer src, int attachment, Cubemap dst){
        TrackFramebufferUse(src);
#ifdef TEST_NEW_CMD
        DispatchFast<Type::CopyCubemap, &Command::copyCubemap>(src, attachment, dst);
#else
        Command cmd{};
        cmd.type = Type::CopyCubemap;
        cmd.copyCubemap.src = src;
        cmd.copyCubemap.attachment = attachment;
        cmd.copyCubemap.dst = dst;
        Dispatch(cmd);
#endif
    }

#ifdef TEST_NEW_CMD
    bool InsertImGuiBeforeWindowEnd(uint32_t snapshotIndex){
        if(ExecutesImmediately() || windowEnd == NoWindowEnd) return false;

        const size_t tailBytes = commands.GetUsedBytes() - windowEnd;
        commands.Insert<Type::RenderImGui, decltype(Command::renderImGui)>(windowEnd, snapshotIndex);
        windowEnd = commands.GetUsedBytes() - tailBytes;
        return true;
    }

#endif

    void TrackFramebufferUse(Framebuffer framebuffer){
        if(framebuffer != InvalidID) framebufferUses.insert(framebuffer);
    }

    bool HasFramebufferUse(Framebuffer framebuffer) const{
        return framebufferUses.find(framebuffer) != framebufferUses.end();
    }

    bool HasOpenFramebufferPass() const { return framebufferPassOpen; }

#ifdef TEST_NEW_CMD
    FastCommands commands;
#else
    std::vector<Command> commands;
#endif

private:
#ifdef TEST_NEW_CMD
    template<Type Tag, auto Member, typename... Args>
    void DispatchFast(Args&&... args){
        using Payload = std::remove_reference_t<decltype(std::declval<Command&>().*Member)>;
        if(executeImmediate != nullptr){
            const Payload payload{std::forward<Args>(args)...};
            executeImmediate(executorContext, Tag, &payload);
        } else {
            commands.Emplace<Tag, Payload>(std::forward<Args>(args)...);
        }
    }
    static constexpr size_t NoWindowEnd = (std::numeric_limits<size_t>::max)();
    size_t windowEnd = NoWindowEnd;
    bool waitingForWindowEnd = false;
#endif
    std::unordered_set<Framebuffer> framebufferUses;
    bool framebufferPassOpen = false;
    void* executorContext = nullptr;
    ExecuteCommand executeImmediate = nullptr;
};

struct OD_API RenderFrame{
    struct ImGuiSnapshot{
        void* data = nullptr;
        ImGuiDrawDataDestroyFunction destroy = nullptr;
    };

    ResourceCommands resourceCommands = {};
    ResourceCommands resourcePostCommands = {};
    CommandBuffer renderCommands = {};
    std::unordered_map<Framebuffer, FrameBufferCreateInfo> framebufferResizes;
    struct ColorReadbackRequest{
        Framebuffer framebuffer;
        uint32_t attachment;
        ColorReadbackCallback complete;
    };
    std::vector<ColorReadbackRequest> colorReadbacks;
    std::vector<ImGuiSnapshot> imguiSnapshots;
    ImGuiImages imguiImages;

    bool TrackFramebufferBindings(const BindGroupInfo& info){
        bool usesFramebuffer = false;
        for(uint32_t i = 0; i < info.entriesCount; ++i){
            const auto framebuffer = info.entries[i].framebuffer;
            if(framebuffer != InvalidID){
                renderCommands.TrackFramebufferUse(framebuffer);
                usesFramebuffer = true;
            }
        }
        return usesFramebuffer;
    }

    inline void RecordImGuiDrawData(void* data, ImGuiDrawDataDestroyFunction destroy){
        const uint32_t snapshotIndex = static_cast<uint32_t>(imguiSnapshots.size());
        imguiSnapshots.push_back({data, destroy});

        if(renderCommands.ExecutesImmediately()) return;

#ifdef TEST_NEW_CMD
        if(!renderCommands.InsertImGuiBeforeWindowEnd(snapshotIndex)){
            // An editor may only render to offscreen targets.
            renderCommands.BeginWindowFramebuffer();
            renderCommands.RenderImGui(snapshotIndex);
            renderCommands.EndFramebuffer();
        }
#else
        auto insertAt = renderCommands.commands.end();
        for(auto it = renderCommands.commands.end(); it != renderCommands.commands.begin();){
            --it;
            if(it->type == CommandBuffer::Type::BeginWindowFramebuffer){
                auto end = it + 1;
                while(end != renderCommands.commands.end() && end->type != CommandBuffer::Type::EndFramebuffer) ++end;
                insertAt = end;
                break;
            }
        }

        if(insertAt == renderCommands.commands.end()){
            // An editor may render only to offscreen targets. ImGui needs a
            // window pass, after those targets have become shader-readable.
            renderCommands.BeginWindowFramebuffer();
            renderCommands.RenderImGui(snapshotIndex);
            renderCommands.EndFramebuffer();
            return;
        }

        CommandBuffer::Command command{};
        command.type = CommandBuffer::Type::RenderImGui;
        command.renderImGui.snapshotIndex = snapshotIndex;
        renderCommands.commands.insert(insertAt, command);
#endif
    }

    void DrawImGuiOverlay(){
        if(imguiSnapshots.empty()) return;

        renderCommands.BeginWindowFramebuffer(true);
        for(uint32_t index = 0; index < imguiSnapshots.size(); ++index){
            renderCommands.RenderImGui(index);
        }
        renderCommands.EndFramebuffer();
    }

    inline void Clear(){
        for(auto& snapshot : imguiSnapshots){
            if(snapshot.destroy != nullptr && snapshot.data != nullptr)
                snapshot.destroy(snapshot.data);
        }
        colorReadbacks.clear();
        imguiSnapshots.clear();
        imguiImages.Clear();
        resourceCommands.Clear();
        resourcePostCommands.Clear();
        renderCommands.ClearCmds();
        framebufferResizes.clear();
    }
};

class OD_API Device{
public:
    struct Profile{
        std::string name;
        float time;
    };

    virtual ~Device(){}

    virtual bool SupportMultithread(){ return false; }

    void Init(RenderingMode mode){
        Assert(!initialized);
        if(initialized) return;
        renderingMode = mode;
        initialized = true;
        InitializeRendering();
    }

    RenderingMode GetRenderingMode() const { return renderingMode; }
    virtual void Shut(){}
    virtual void StartRender(){}
    virtual void UpdateRender(){}

    // These methods are called by the platform layer and renderer thread.
    virtual bool ImGuiSupported() const { return false; }
    virtual void ImGuiInitialize(){}
    virtual void ImGuiNewFrame(){}
    virtual void SubmitImGuiDrawData(void* data, ImGuiDrawDataDestroyFunction destroy){}
    virtual void ImGuiShutdown(){}
    // Frame-local ImTextureID. Obtain again each UI frame; never dereference it.
    virtual void ReadFramebufferColor(Framebuffer, uint32_t, ColorReadbackCallback complete){
        complete({ColorReadbackResult::Unsupported});
    }
    virtual void* ImGuiFramebufferColorAttachment(Framebuffer framebuffer, uint32_t attachment){ return nullptr; }
    virtual void* ImGuiTexture2D(Texture2D texture){ return nullptr; }
    virtual bool ImGuiFramebufferNeedsVerticalFlip() const { return true; }

    // The caller owns and resets this counter on the recording thread.
    void SetUploadCounter(std::uint64_t* counter){ uploadCounter = counter; }

    virtual ResourceStats GetBufferStats(Buffer id){ return {}; }

    virtual CommandBuffer* GetCommandBuffer(){ return nullptr; }
    virtual FrameBufferLayout GetWindowFrameBufferLayout(){ return {}; }

    virtual Pipeline CreatePipeline(const char* source, PipelineInfo info){ return InvalidID; }
    virtual void DestroyPipeline(Pipeline id){}

    virtual Buffer CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory){ return InvalidID; }
    void UpdatedBuffer(Buffer buffer, const void* data, size_t size){
        UpdateBufferImpl(buffer, data, size);
        if(uploadCounter != nullptr && data != nullptr) *uploadCounter += size;
    }
    virtual void DestroyBuffer(Buffer id){}

    virtual Texture2D CreateTexture2D(Texture2DInfo& info){ return InvalidID; }
    void UploadTexture2D(Texture2D texture, const void* data, size_t size, int mipLevel = 0){
        UploadTexture2DImpl(texture, data, size, mipLevel);
        if(uploadCounter != nullptr && data != nullptr) *uploadCounter += size;
    }
    virtual void DestroyTexture2D(Texture2D tex){}
    virtual Texture2DArray CreateTexture2DArray(Texture2DArrayInfo& info){ return InvalidID; }
    void UploadTexture2DArray(Texture2DArray texture, const void* data, size_t size, int arrayElement, int mipLevel = 0){
        UploadTexture2DArrayImpl(texture, data, size, arrayElement, mipLevel);
        if(uploadCounter != nullptr && data != nullptr) *uploadCounter += size;
    }
    virtual void DestroyTexture2DArray(Texture2DArray tex){}
    virtual Cubemap CreateCubemap(CubemapInfo& info){ return InvalidID; }
    void UploadCubemap(Cubemap cubemap, const void* data, size_t size, int mipLevel = 0){
        UploadCubemapImpl(cubemap, data, size, mipLevel);
        if(uploadCounter != nullptr && data != nullptr) *uploadCounter += size;
    }
    void UploadCubemap(Cubemap cubemap, const void* data, size_t size, CubemapFace face, int mipLevel = 0){
        UploadCubemapImpl(cubemap, data, size, face, mipLevel);
        if(uploadCounter != nullptr && data != nullptr) *uploadCounter += size;
    }
    virtual void DestroyCubemap(Cubemap cubemap){}

    virtual BindGroupLayout CreateBindGroupLayout(BindGroupLayoutInfo& info){ return InvalidID; }
    virtual void DestroyBindGroupLayout(BindGroupLayout layout){}

    virtual BindGroup CreateBindGroup(BindGroupInfo& info){ return InvalidID; }
    virtual BindGroup CreateFrameBindGroup(BindGroupInfo& info){ return InvalidID; }

    virtual Framebuffer CreateFramebuffer(FrameBufferCreateInfo& info){ return InvalidID; }
    // Returns false when the caller needs a separate target for the new size.
    virtual bool ResizeFramebuffer(Framebuffer framebuffer, const FrameBufferCreateInfo& info){ return false; }
    virtual void DestroyFramebuffer(Framebuffer destroy){}

    virtual std::vector<Profile> GetProfiles(){ return {}; }

protected:
    virtual void UpdateBufferImpl(Buffer buffer, const void* data, size_t size){}
    virtual void UploadTexture2DImpl(Texture2D texture, const void* data, size_t size, int mipLevel){}
    virtual void UploadTexture2DArrayImpl(Texture2DArray texture, const void* data, size_t size, int arrayElement, int mipLevel){}
    virtual void UploadCubemapImpl(Cubemap cubemap, const void* data, size_t size, int mipLevel){}
    virtual void UploadCubemapImpl(Cubemap cubemap, const void* data, size_t size, CubemapFace face, int mipLevel){}
    virtual void InitializeRendering(){}
    bool IsThreaded() const { return renderingMode == RenderingMode::Threaded; }

private:
    std::uint64_t* uploadCounter = nullptr;
    RenderingMode renderingMode = RenderingMode::Threaded;
    bool initialized = false;
};

}
}
