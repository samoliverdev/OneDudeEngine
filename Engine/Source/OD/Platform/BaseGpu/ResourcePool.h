#pragma once
#include "OD/Core/Log.h"
#include "OD/Gfx/Gfx.h"
#include <vector>

namespace OD{

template <typename T, uint32_t ChunkSize = 1024>
struct ResourcePool{
    struct Chunk{
        std::array<T, ChunkSize> data;
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

    inline uint32_t AllocId(){
        if(!freeIds.empty()){
            uint32_t id = freeIds.back();
            freeIds.pop_back();
            return id;
        }

        uint32_t id = curId;
        curId++;

        EnsureChunk(id);
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
        Assert(false);
        /*singleThreadIds.push_back(id);
        singleThreadDatas.push_back(resource);

        resourceStatus.resize(curId);
        resourceStatus[id].type = GPUResourceStatsType::Created;
        resourceStatus[id].erroMessage = "";*/
    }

    inline void GpuPushResourceStatus(uint32_t id, Gfx::ResourceStats status){
        gpuToCpuResourceStatesIds.push_back(id);
        gpuToCpuResourceStatesData.push_back(status);
    };

    inline void SyncSingleThreadData(){
        for(auto id : idsDestred){
            freeIds.push_back(id);
        }
        idsDestred.clear();

        /*Assert(singleThreadIds.size() == singleThreadDatas.size());

        for(size_t i = 0; i < singleThreadIds.size(); i++){
            Get(singleThreadIds[i]) = singleThreadDatas[i];
        }
        singleThreadIds.clear();
        singleThreadDatas.clear();*/

        Assert(gpuToCpuResourceStatesIds.size() == gpuToCpuResourceStatesData.size());
        for(size_t i = 0; i < gpuToCpuResourceStatesIds.size(); i++){
            GetStatus(gpuToCpuResourceStatesIds[i]) = gpuToCpuResourceStatesData[i];
        }
        gpuToCpuResourceStatesIds.clear();
        gpuToCpuResourceStatesData.clear();
    }

    template <typename Func>
    inline void ForEach(Func&& func){
        for(uint32_t id = 0; id < curId; ++id){
            func(id, Get(id));
        }
    }
};

}