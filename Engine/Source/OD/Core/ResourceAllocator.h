#pragma once
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <functional>
#include "Resource.h"

namespace OD {

template<typename T>
class IResourceView {
public:
    virtual T* Get(uint32_t id) = 0;
    virtual ~IResourceView() = default;
};

template<typename T>
class ResourceAllocator: public IResourceView<T> {
    static_assert(std::is_base_of<Resource, T>::value, "T must derive from Asset");
public:
    static std::shared_ptr<ResourceAllocator<T>> Create(int _chunkCapacity){
        auto allocator = std::make_shared<ResourceAllocator<T>>();
        allocator->Init(_chunkCapacity);
        return allocator;
    }

    void Init(int _chunkCapacity){
        if(_chunkCapacity <= 0)
            throw std::invalid_argument("Chunk capacity must be positive");

        chunkCapacity = _chunkCapacity;
        AddNewChunk();
        curChunk = 0;
    }

    // =========================
    // ALLOC
    // =========================
    template<typename... Args>
    T* Alloc(Args&&... args){
        Chunk* chunk;
        int index;
        uint32_t chunkIndex;

        // reuse free slot
        if(!freeSlots.empty()){
            auto slot = freeSlots.back();
            freeSlots.pop_back();

            chunkIndex = slot.chunkIndex;
            index = slot.index;
            chunk = &chunks[chunkIndex];
        } else {
            chunkIndex = curChunk;
            chunk = &chunks[chunkIndex];

            if(chunk->curIndex >= chunkCapacity){
                AddNewChunk();
                curChunk = chunks.size() - 1;
                chunkIndex = curChunk;
                chunk = &chunks[chunkIndex];
            }

            index = chunk->curIndex++;
        }

        chunk->used[index] = true;

        T* ptr = reinterpret_cast<T*>(chunk->data) + index;
        new(ptr) T(std::forward<Args>(args)...);

        uint32_t id = EncodeId(chunkIndex, index);
        ptr->resourceId = id;

        return ptr;
    }

    template<typename... Args>
    std::shared_ptr<T> AllocShared(Args&&... args){
        T* ptr = Alloc(std::forward<Args>(args)...); // Reuse Alloc logic
        return std::shared_ptr<T>(ptr, [this](T* p){ this->Free(p->GetId()); });
    }

    bool IsValid(uint32_t id) const {
        if(id == INVALID_RESOURCE_ID) return false;

        uint32_t chunkIndex, index;
        DecodeId(id, chunkIndex, index);

        if(chunkIndex >= chunks.size()) return false;

        const Chunk& chunk = chunks[chunkIndex];

        if(index >= chunk.used.size()) return false;

        return chunk.used[index];
    }

    // =========================
    // GET BY ID
    // =========================
    T* Get(uint32_t id) override {
        if(id == INVALID_RESOURCE_ID) return nullptr;

        uint32_t chunkIndex, index;
        DecodeId(id, chunkIndex, index);

        if(chunkIndex >= chunks.size()) return nullptr;

        Chunk& chunk = chunks[chunkIndex];

        if(index >= chunk.used.size()) return nullptr;
        if(!chunk.used[index]) return nullptr;

        return reinterpret_cast<T*>(chunk.data) + index;
    }

    // =========================
    // FREE BY ID
    // =========================
    void Free(uint32_t id){
        uint32_t chunkIndex, index;
        DecodeId(id, chunkIndex, index);

        if(chunkIndex >= chunks.size())
            throw std::runtime_error("Invalid resource ID");

        Chunk& chunk = chunks[chunkIndex];

        if(index >= chunk.used.size() || !chunk.used[index])
            throw std::runtime_error("Double free or invalid ID");

        T* ptr = reinterpret_cast<T*>(chunk.data) + index;

        ptr->~T(); //printf("~T()\n");
        ptr->resourceId = INVALID_RESOURCE_ID;

        chunk.used[index] = false;
        freeSlots.push_back({ chunkIndex, index });

        if((size_t)chunkIndex < curChunk)
            curChunk = chunkIndex;
    }

    // =========================
    // ITERATION
    // =========================
    void ForEach(std::function<void(T*)> func){
        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            for (size_t j = 0; j < chunk.used.size(); ++j) {
                if (chunk.used[j]) {
                    T* ptr = reinterpret_cast<T*>(chunk.data) + j;
                    func(ptr);
                }
            }
        }
    }

    // =========================
    // RESET
    // =========================
    void Reset(){
        for(auto& chunk : chunks){
            for(size_t i = 0; i < chunk.used.size(); ++i){
                if (chunk.used[i]) {
                    T* ptr = reinterpret_cast<T*>(chunk.data) + i;
                    ptr->~T(); //printf("~T()\n");
                }
            }
            chunk.curIndex = 0;
            std::fill(chunk.used.begin(), chunk.used.end(), false);
        }

        freeSlots.clear();
        curChunk = 0;
    }

    ~ResourceAllocator(){
        for(auto& chunk : chunks){
            for(size_t i = 0; i < chunk.used.size(); ++i){
                if(chunk.used[i]){
                    T* ptr = reinterpret_cast<T*>(chunk.data) + i;
                    ptr->~T(); //printf("~T()\n");
                }
            }
            free(chunk.data);
        }
    }

private:
    struct Chunk{
        void* data = nullptr;
        int curIndex = 0;
        std::vector<bool> used;
    };

    struct Slot{
        uint32_t chunkIndex;
        uint32_t index;
    };

    //max chunks = 65535
    //max elements per chunk = 65535
    static constexpr uint32_t INDEX_BITS = 16;
    static constexpr uint32_t INDEX_MASK = (1u << INDEX_BITS) - 1;

    uint32_t EncodeId(uint32_t chunkIndex, uint32_t index) const {
        return (chunkIndex << INDEX_BITS) | index;
    }

    void DecodeId(uint32_t id, uint32_t& chunkIndex, uint32_t& index) const {
        chunkIndex = id >> INDEX_BITS;
        index = id & INDEX_MASK;
    }

    void AddNewChunk(){
        chunks.emplace_back();
        Chunk& chunk = chunks.back();

        chunk.data = malloc(sizeof(T) * chunkCapacity);
        if(!chunk.data)
            throw std::bad_alloc();

        chunk.used.resize(chunkCapacity, false);
        chunk.curIndex = 0;
    }

private:
    std::vector<Chunk> chunks;
    std::vector<Slot> freeSlots;

    size_t chunkCapacity = 0;
    size_t curChunk = 0;
};

}