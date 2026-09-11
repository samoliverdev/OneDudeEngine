#pragma once
#include "OD/Core/Log.h"
#include "OD/Gfx/Gfx.h"
#include <vector>

namespace OD{

//#define ENABLE_THREAD_DEBUG

struct DebugRWGuard {
#ifdef ENABLE_THREAD_DEBUG
    std::atomic<int> readers{0};
    std::atomic<bool> writer{false};

    void BeginRead() {
        Assert(!writer.load(std::memory_order_acquire) && "Reading while another thread is writing");

        readers.fetch_add(1, std::memory_order_acquire);

        // Writer could theoretically start between the
        // previous check and increment, so check again.
        Assert(!writer.load(std::memory_order_acquire) && "Writer started while reading");
    }

    void EndRead() {
        readers.fetch_sub(1, std::memory_order_release);
    }

    void BeginWrite() {
        bool expected = false;

        bool success = writer.compare_exchange_strong(expected, true, std::memory_order_acq_rel);

        Assert(success && "Multiple writers detected");

        Assert(readers.load(std::memory_order_acquire) == 0 && "Writing while another thread is reading");
    }

    void EndWrite() {
        writer.store(false, std::memory_order_release);
    }
#else
    void BeginRead() {}
    void EndRead() {}
    void BeginWrite() {}
    void EndWrite() {}
#endif
};


template <typename T, uint32_t ChunkSize = 1024>
struct ResourcePool{
    uint32_t InvalidID = std::numeric_limits<uint32_t>::max();

    DebugRWGuard poolGuard;

    inline bool IsValid(uint32_t id){
        if(id == InvalidID) return false;

        poolGuard.BeginRead();

        uint32_t chunkIndex = id / ChunkSize;
        uint32_t index      = id % ChunkSize;
        if(chunks[chunkIndex]->isValid[index] == false){
            poolGuard.EndRead();
            return false;
        }

        poolGuard.EndRead();
        return true;
    }

    inline uint32_t AllocId(){
        poolGuard.BeginWrite();

        if(!freeIds.empty()){
            uint32_t id = freeIds.back();
            freeIds.pop_back();
            SetIsValid(id, true);
            poolGuard.EndWrite();
            return id;
        }

        uint32_t id = curId;
        curId++;

        EnsureChunk(id);

        SetIsValid(id, true);
        poolGuard.EndWrite();
        return id;
    }

    inline void EnsureChunk(uint32_t id){
        uint32_t chunkIndex = id / ChunkSize;

        if(chunkIndex >= chunks.size()){
            chunks.resize(chunkIndex + 1);

            if(!chunks[chunkIndex]) chunks[chunkIndex] = std::make_unique<Chunk>();
        }
    }

    inline Gfx::ResourceStats& GetStatus(uint32_t id){
        uint32_t chunkIndex = id / ChunkSize;
        uint32_t index      = id % ChunkSize;
        return chunks[chunkIndex]->status[index];
    }

    inline T& Get(uint32_t id){
        uint32_t chunkIndex = id / ChunkSize;
        uint32_t index      = id % ChunkSize;
        return chunks[chunkIndex]->data[index];
    }

    inline const T& Get(uint32_t id) const {
        uint32_t chunkIndex = id / ChunkSize;
        uint32_t index      = id % ChunkSize;
        return chunks[chunkIndex]->data[index];
    }

    inline void CpuPushResource(uint32_t id, T& resource){
        Get(id) = resource;

        //Assert(false);
        //singleThreadIds.push_back(id);
        //singleThreadDatas.push_back(resource);

        /*resourceStatus.resize(curId);
        resourceStatus[id].type = GPUResourceStatsType::Created;
        resourceStatus[id].erroMessage = "";*/
    }

    inline void GpuPushResourceStatus(uint32_t id, Gfx::ResourceStats status){
        gpuToCpuResourceStatesIds.push_back(id);
        gpuToCpuResourceStatesData.push_back(status);
    };

    inline void SyncSingleThreadData(){
        poolGuard.BeginWrite();

        for(auto id : idsDestred){
            freeIds.push_back(id);
        }
        idsDestred.clear();

        Assert(singleThreadIds.size() == singleThreadDatas.size());
        for(size_t i = 0; i < singleThreadIds.size(); i++){
            Get(singleThreadIds[i]) = singleThreadDatas[i];
        }
        singleThreadIds.clear();
        singleThreadDatas.clear();

        Assert(gpuToCpuResourceStatesIds.size() == gpuToCpuResourceStatesData.size());
        for(size_t i = 0; i < gpuToCpuResourceStatesIds.size(); i++){
            GetStatus(gpuToCpuResourceStatesIds[i]) = gpuToCpuResourceStatesData[i];
        }
        gpuToCpuResourceStatesIds.clear();
        gpuToCpuResourceStatesData.clear();

        poolGuard.EndWrite();
    }

    inline void AddDestroyedId(uint32_t id){
        poolGuard.BeginWrite();
        idsDestred.push_back(id);
        uint32_t chunkIndex = id / ChunkSize;
        uint32_t index      = id % ChunkSize;
        chunks[chunkIndex]->isValid[index] = false;
        poolGuard.EndWrite();
    }

    template <typename Func>
    inline void ForEach(Func&& func){
        for(uint32_t id = 0; id < curId; ++id){
            uint32_t chunkIndex = id / ChunkSize;
            uint32_t index      = id % ChunkSize;

            if(chunks[chunkIndex]->isValid[index] == false) continue;
            func(id, chunks[chunkIndex]->data[index]);
        }
    }

private:
    struct Chunk{
        std::array<T, ChunkSize> data;
        std::array<bool, ChunkSize> isValid;
        std::array<Gfx::ResourceStats, ChunkSize> status;
    };

    std::vector<std::unique_ptr<Chunk>> chunks;

    std::vector<uint32_t> freeIds;
    std::vector<uint32_t> idsDestred;
    uint32_t curId = 0;

    std::vector<uint32_t> singleThreadIds;
    std::vector<T> singleThreadDatas;

    //std::vector<GPUResourceStats> resourceStatus;
    std::vector<uint32_t> gpuToCpuResourceStatesIds;
    std::vector<Gfx::ResourceStats> gpuToCpuResourceStatesData;

    inline void SetIsValid(uint32_t id, bool value){
        uint32_t chunkIndex = id / ChunkSize;
        uint32_t index      = id % ChunkSize;
        chunks[chunkIndex]->isValid[index] = value;
    }
};

}