#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include <thread>
#include <mutex>

namespace OD{

static constexpr uint32_t InvalidID = std::numeric_limits<uint32_t>::max();
using TextureId = uint32_t;
using MeshId = uint32_t;
using PipelineId = uint32_t;
using FramebufferId = uint32_t;
using BufferId = uint32_t;

/////////////////////////////////////

constexpr uint32_t MAX_COLOR_ATTACHMENTS = 8;

enum class OD_API_IMPORT GPUFramebufferTextureFormat: uint8_t{
    None, RGB, RGBA8, RGB11B10F, RGB16F, RGBA16F, RGB32F, RGBA32F, RED_INTEGER
};

enum class OD_API_IMPORT GPUFramebufferDepthTextureFormat: uint8_t{
    None, DEPTH24_STENCIL8, DEPTH32F_STENCIL8, DEPTH_COMPONENT16, DEPTH_COMPONENT24, DEPTH_COMPONENT32, DEPTH_COMPONENT32F
};

enum class OD_API_IMPORT GPUFramebufferAttachmentType: uint8_t{
    TEXTURE_2D,
    TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_ARRAY,
    CUBEMAP
};

struct OD_API GPUFramebufferAttachment{
    GPUFramebufferTextureFormat format;
    uint8_t mipLevels = 1;
};

struct OD_API GPUFramebufferDepthAttachment{
    GPUFramebufferDepthTextureFormat format;
    uint8_t mipLevels = 1;
};

struct OD_API GPUFrameBufferLayout{
    GPUFramebufferAttachmentType type = GPUFramebufferAttachmentType::TEXTURE_2D;
    GPUFramebufferAttachment colorAttachments[MAX_COLOR_ATTACHMENTS];
    uint8_t colorAttachmentsCount = 0;
    GPUFramebufferDepthAttachment depthAttachment;
    uint8_t samples = 1;
    bool swapChainTarget = false;
};

//////////////////////////////////////

constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 16;
constexpr uint32_t MAX_VERTEX_BUFFERS = 16;

enum class GPUVertexSemantic: uint8_t{
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
    Custom3
};

enum class GPUVertexFormat: uint8_t{
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

enum class GPUVertexInputRate: uint8_t{
    Vertex,
    Instance
};

struct OD_API GPUVertexAttribute{
    GPUVertexSemantic semantic;
    GPUVertexFormat format;

    // Vertex buffer binding this attribute comes from.
    uint8_t bufferSlot = 0;

    // Byte offset inside the vertex.
    size_t offset = 0;
};

struct OD_API GPUVertexBufferLayout{
    // Distance in bytes between two consecutive
    // vertices/instances in this buffer.
    size_t stride = 0;

    GPUVertexInputRate inputRate = GPUVertexInputRate::Vertex;
};

struct OD_API GPUMeshLayout{
    GPUVertexAttribute attributes[MAX_VERTEX_ATTRIBUTES];
    uint32_t attributeCount = 0;

    GPUVertexBufferLayout buffers[MAX_VERTEX_BUFFERS];
    uint32_t bufferCount = 0;
};

//////////////////////////////////////

enum class OD_API_IMPORT GPUDepthTest: uint8_t{
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

enum class OD_API_IMPORT GPUCullFace: uint8_t{
    NONE            = 0,
    BACK            = 1,
    FRONT           = 2,
    FRONT_AND_BACK  = 3
};

enum class OD_API_IMPORT GPUBlendMode: uint8_t{
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

enum class OD_API_IMPORT GPUBlendOp: uint8_t{
    FUNC_ADD,
    FUNC_SUBTRACT,
    FUNC_REVERSE_SUBTRACT,
    MIN,
    MAX
};  

struct OD_API GPUPipelineInfo{
    GPUMeshLayout vertexLayout;
    GPUFrameBufferLayout framebufferLayout;
    GPUCullFace cullFace = GPUCullFace::BACK;
    GPUDepthTest depthTest = GPUDepthTest::LESS;
    bool depthMask = true;
    Vector4 colorMask = {1, 1, 1, 1};
    bool blend = false;
    GPUBlendMode srcBlend;
    GPUBlendMode dstBlend;
    GPUBlendMode srcAlphaBlend;
    GPUBlendMode dstAlphaBlend;
    GPUBlendOp opBlend = GPUBlendOp::FUNC_ADD;
};

//////////////////////////////////////

enum class GPUBufferUsage: uint8_t{
    Vertex,
    Index,
    Uniform,
    Storage,
    /*Indirect,
    CopySource,
    CopyDestination*/
};

enum class GPUBufferMemory : uint8_t{
    GPUOnly,
    CPUToGPU,
    GPUToCPU,
    CPUOnly
};

/////////////////////////////////////

enum class GPUClearFlags : uint8_t{
    None    = 0,
    Color   = 1 << 0,
    Depth   = 1 << 1,
    Stencil = 1 << 2
};

struct GPUClearValue{
    Vector4 color = {0, 0, 0, 0};
    float depth = 1.0f;
    uint32_t stencil = 0;
};

constexpr GPUClearFlags operator|(GPUClearFlags a, GPUClearFlags b){
    return static_cast<GPUClearFlags>(
        static_cast<uint8_t>(a) |
        static_cast<uint8_t>(b)
    );
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

struct OD_API GPUResourceCommands{
    enum class Type{
        CreatePipeline,
        DestroyPipeline,
        CreateBuffer,
        DestroyBuffer,
    };

    struct Command{
        Type type;

        union{
            struct{
                BufferId id;
                GPUBufferUsage usage;
                GPUBufferMemory memory;
                const void* data;
                size_t size;
            } createBuffer;

            struct{
                BufferId id;
            } destroyBuffer;

            struct{
                PipelineId id;
                const char* source;
                GPUPipelineInfo info;
            } createPipeline;

            struct{
                PipelineId id;
            } destroyPipeline;

            struct{
                const void* data;
                size_t size;
            } uploadBuffer;
        };
    };

    void Clear(){
        commands.clear();
        uploadBuffer.Clear();
    }

    void CreatePipeline(PipelineId id, const char* source, GPUPipelineInfo info){
        Command cmd{};
        cmd.type = Type::CreatePipeline;
        cmd.createPipeline.id = id;
        cmd.createPipeline.source = source;
        cmd.createPipeline.info = info;
        commands.push_back(cmd);
    }

    void DestroyPipeline(PipelineId id){
        Command cmd{};
        cmd.type = Type::DestroyPipeline;
        cmd.destroyPipeline.id = id;
        commands.push_back(cmd);
    }

    void CreateBuffer(BufferId id, const void* data, size_t size, GPUBufferUsage usage, GPUBufferMemory memory = GPUBufferMemory::GPUOnly){
        void* copyData = uploadBuffer.AllocateData(size);
        std::memcpy(copyData, data, size);

        Command cmd{};
        cmd.type = Type::CreateBuffer;
        cmd.createBuffer.usage = usage;
        cmd.createBuffer.id = id;
        cmd.createBuffer.data = copyData;
        cmd.createBuffer.size = size;
        cmd.createBuffer.memory = memory;
        commands.push_back(cmd);
    }

    void DestroyBuffer(BufferId id){
        Command cmd{};
        cmd.type = Type::DestroyBuffer;
        cmd.destroyBuffer.id = id;
        commands.push_back(cmd);
    }

    std::vector<Command> commands;
    UploadBuffer uploadBuffer = {};
};

struct OD_API GPUCommandBuffer{
    enum class Type{
        Clear,
        Viewport,
        SetPipeline,
        SetVertexBuffer,
        SetIndexBuffer,
        Draw,
        DrawIndexed,
    };

    struct Command{
        Type type;

        union{
            struct {
                uint32_t x, y, w, h;
            } viewport;

            struct{
                GPUClearFlags flags;
                GPUClearValue clearValue;
            } clear;

            struct{
                PipelineId id;
            } setPipeline;

            struct{
                uint32_t slot;
                BufferId buffer;
            } setVertexBuffer;

            struct {
                BufferId buffer;
            } setIndexBuffer;

            struct{
                uint32_t vertexCount;
            } draw;

            struct{
                uint32_t indexCount;
            } drawIndexed;
        };
    };

    void ClearCmds(){
        commands.clear();
    }

    void Clean(GPUClearFlags flags, const GPUClearValue& clearValue){
        Command cmd{};
        cmd.type = Type::Clear;
        cmd.clear.flags = flags;
        cmd.clear.clearValue = clearValue;
        commands.push_back(cmd);
    }

    void Viewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h){
        Command cmd{};
        cmd.type = Type::Viewport;
        cmd.viewport.x = x;
        cmd.viewport.y = y;
        cmd.viewport.w = w;
        cmd.viewport.h = h;
        commands.push_back(cmd);
    }

    void SetPipeline(PipelineId pipeline){
        Command cmd{};
        cmd.type = Type::SetPipeline;
        cmd.setPipeline.id = pipeline;
        commands.push_back(cmd);
    }

    void SetVertexBuffer(uint32_t slot, BufferId buffer){
        Command cmd{};
        cmd.type = Type::SetVertexBuffer;
        cmd.setVertexBuffer.slot = slot;
        cmd.setVertexBuffer.buffer = buffer;
        commands.push_back(cmd);
    }

    void SetIndexBuffer(BufferId buffer){
        Command cmd{};
        cmd.type = Type::SetIndexBuffer;
        cmd.setIndexBuffer.buffer = buffer;
        commands.push_back(cmd);
    }
    
    void Draw(uint32_t vertexCount){
        Command cmd{};
        cmd.type = Type::Draw;
        cmd.draw.vertexCount = vertexCount;
        commands.push_back(cmd);
    }

    void DrawIndexed(uint32_t indexCount){
        Command cmd{};
        cmd.type = Type::DrawIndexed;
        cmd.drawIndexed.indexCount = indexCount;
        commands.push_back(cmd);
    }

    std::vector<Command> commands;
};

struct OD_API GPURenderFrame{
    GPUResourceCommands resourceCommands = {};
    GPUCommandBuffer renderCommands = {};

    inline void Clear(){
        resourceCommands.Clear();
        renderCommands.ClearCmds();
    }
};

class OD_API GPUDevice{
public:
    virtual ~GPUDevice(){}

    virtual bool SupportMultithread(){ return false; }

    virtual void Init(){}
    virtual void Shut(){}
    virtual void RunRender(GPURenderFrame& frame){}

    virtual void SyncSingleThreadData(){}

    /*inline MeshId CreateMesh(GPURenderFrame& frame, const void* data, size_t size){
        MeshId id = AllocMeshId();
        frame.resourceCommands.CreateMesh(id, data, size);
        return id;
    }

    void DestroyMesh(GPURenderFrame& frame, MeshId& id){
        if(id == InvalidID) return;
        frame.resourceCommands.DestroyMesh(id);
        id = InvalidID;
    }*/

    virtual BufferId AllocBufferId(){ return InvalidID; }
    virtual PipelineId AllocPipelineId(){ return InvalidID; }
};

}