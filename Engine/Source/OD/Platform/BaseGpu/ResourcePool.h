#pragma once
#include <vector>

namespace OD{

template <typename T>
struct ResourcePool{
    std::vector<T> data;
    std::vector<uint32_t> freeIds;
    std::vector<uint32_t> idsDestred;
    uint32_t curId = 0;

    uint32_t AllocId(){
        if(freeIds.empty() == false){
            MeshId id = freeIds.back();
            freeIds.pop_back();
            return id;
        }

        MeshId id = curId;
        curId += 1;
        return id;
    }

    void SyncSingleThreadData(){
        data.resize(curId);
        for(auto i: idsDestred){
            freeIds.push_back(i);
        }
        idsDestred.clear();
    }
};

}