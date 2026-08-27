#pragma once
#include <thread>
#include <mutex>

namespace OD{

//#define InvalidID UINT32_MAX
static constexpr uint32_t InvalidID = std::numeric_limits<uint32_t>::max();

using TextureId = uint32_t;
using MeshId = uint32_t;
using PipelineId = uint32_t;

class UploadBuffer{
public:
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

struct GPUCommandBuffer{
    enum class Type{
        Clean,
        Viewport,
        CreateMesh,
        DestroyMesh,
        CreatePipeline,
        DestroyPipeline,
        SetRenderTarget,
        SetPipeline,
        WriteBuffer,
        Draw
    };

    struct Command{
        Type type;

        union{
            TextureId target;
            PipelineId pipeline;
            MeshId mesh;

            struct {
                uint32_t x, y, w, h;
            } viewport;

            struct {
                MeshId mesh;
                const void* data;
                size_t size;
            } createMesh;

            struct {
                PipelineId id;
                const char* data;
                size_t size;
            } createPipeline;

            struct{
                uint32_t r, g, b, a;
            } cleanColor;

            struct{
                const void* data;
                uint32_t size;
            } writeBuffer;

            struct{
                MeshId mesh;
                PipelineId pipeline;
                uint32_t vertexCount;
            } draw;
        };
    };

    void ClearCmds(){
        commands.clear();
    }

    void Clean(uint32_t r, uint32_t g, uint32_t b, uint32_t a){
        Command cmd{};
        cmd.type = Type::Clean;
        cmd.cleanColor.r = r;
        cmd.cleanColor.g = g;
        cmd.cleanColor.b = b;
        cmd.cleanColor.a = a;
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

    void CreateMesh(MeshId id, const void* data, size_t size){
        Command cmd{};
        cmd.type = Type::CreateMesh;
        cmd.createMesh.mesh = id;
        cmd.createMesh.data = data;
        cmd.createMesh.size = size;
        commands.push_back(cmd);
    }

    void DestroyMesh(MeshId& id){
        Command cmd{};
        cmd.type = Type::DestroyMesh;
        cmd.mesh = id;
        id = InvalidID;
        commands.push_back(cmd);
    }

    void CreatePipeline(PipelineId id, const char* data, size_t size){
        Command cmd{};
        cmd.type = Type::CreatePipeline;
        cmd.createPipeline.id = id;
        cmd.createPipeline.data = data;
        cmd.createPipeline.size = size;
        commands.push_back(cmd);
    }

    void DestroyPipeline(PipelineId id){
        Command cmd{};
        cmd.type = Type::DestroyPipeline;
        cmd.pipeline = id;
        commands.push_back(cmd);
    }

    void SetRenderTarget(TextureId target){
        Command cmd{};
        cmd.type = Type::SetRenderTarget;
        cmd.target = target;
        commands.push_back(cmd);
    }

    void SetPipeline(PipelineId pipeline){
        Command cmd{};
        cmd.type = Type::SetPipeline;
        cmd.pipeline = pipeline;

        commands.push_back(cmd);
    }

    void WriteBuffer(const void* data, uint32_t size){
        Command cmd{};
        cmd.type = Type::WriteBuffer;

        cmd.writeBuffer.data = data;
        cmd.writeBuffer.size = size;

        commands.push_back(cmd);
    }

    void Draw(MeshId mesh, PipelineId pipeline, uint32_t vertexCount){
        Command cmd{};
        cmd.type = Type::Draw;
        cmd.draw.mesh = mesh;
        cmd.draw.pipeline = pipeline;
        cmd.draw.vertexCount = vertexCount;
        commands.push_back(cmd);
    }

    std::vector<Command> commands;
};

struct GPURenderFrame{
    GPUCommandBuffer commands = {};
    UploadBuffer uploadBuffer = {};

    inline void Clear(){
        commands.ClearCmds();
        uploadBuffer.Clear();
    }
};

class GPUDevice{
public:
    virtual ~GPUDevice(){}

    virtual bool SupportMultithread(){ return false; }

    virtual void Init(){}
    virtual void Shut(){}
    virtual void RunRender(GPURenderFrame& frame){}

    virtual void SyncSingleThreadData(){}

    virtual MeshId AllocMeshId(){ return InvalidID; }
    virtual PipelineId AllocPipelineId(){ return InvalidID; }
};

}