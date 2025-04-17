#pragma once

namespace OD{

//INFO: Experimental Allocator
template<typename T>
class ArenaLinearAllocator {
public:
    // Static factory method to create and initialize an allocator
    static std::shared_ptr<ArenaLinearAllocator<T>> Create(int _chunkCapacity) {
        auto allocator = std::make_shared<ArenaLinearAllocator<T>>();
        allocator->Init(_chunkCapacity);
        return allocator;
    }

    // Initialize the allocator (for manual creation)
    void Init(int _chunkCapacity) {
        if (_chunkCapacity <= 0) {
            throw std::invalid_argument("Chunk capacity must be positive");
        }
        chunkCapacity = _chunkCapacity;
        chunks.emplace_back();
        Chunk& chunk = chunks.back();
        chunk.data = malloc(sizeof(T) * chunkCapacity); // Allocate slots * sizeof(T)
        if (!chunk.data) {
            throw std::bad_alloc();
        }
        chunk.curIndex = 0;
        chunk.usedIndexs.resize(chunkCapacity, false);
        curChunk = 0;
    }

    // Original Alloc: Returns raw T*
    template<typename... Args>
    T* Alloc(Args&&... args) {
        // Check for reusable free indices
        if (!freeIndexs.empty()) {
            FreeIndex free = freeIndexs.back();
            freeIndexs.pop_back();
            Chunk& chunk = chunks[free.chunkIndex];
            chunk.usedIndexs[free.index] = true;
            T* ptr = reinterpret_cast<T*>(chunk.data) + free.index; // Fastest cast
            new(ptr) T(std::forward<Args>(args)...); // Construct T
            return ptr;
        }

        // Use the current chunk
        Chunk& currentChunk = chunks[curChunk];
        if (currentChunk.curIndex + 1 > static_cast<int>(chunkCapacity)) {
            AddNewChunk();
            curChunk = chunks.size() - 1;
            currentChunk = chunks.back();
        }

        // Allocate from the current chunk
        currentChunk.usedIndexs[currentChunk.curIndex] = true;
        T* ptr = reinterpret_cast<T*>(currentChunk.data) + currentChunk.curIndex;
        new(ptr) T(std::forward<Args>(args)...); // Construct T
        ++currentChunk.curIndex;
        return ptr;
    }

    // New Alloc: Returns shared_ptr<T>
    template<typename... Args>
    std::shared_ptr<T> AllocShared(Args&&... args) {
        T* ptr = Alloc(std::forward<Args>(args)...); // Reuse Alloc logic
        return std::shared_ptr<T>(ptr, [this](T* p) { this->FreeRaw(p); });
    }

    // Original Free: Takes raw T*
    void Free(T* p) {
        FreeRaw(p); // Delegate to FreeRaw
    }

    // New Free: Takes shared_ptr<T>
    void Free(std::shared_ptr<T>& sp) {
        if (!sp) return;
        FreeRaw(sp.get()); // Free the raw pointer
        sp.reset(); // Release ownership
    }

    // ForEach with raw T* (can change to shared_ptr if needed)
    void ForEach(std::function<void(T*)> func) {
        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            for (size_t j = 0; j < chunk.usedIndexs.size(); ++j) {
                if (chunk.usedIndexs[j]) {
                    T* ptr = reinterpret_cast<T*>(chunk.data) + j;
                    func(ptr);
                }
            }
        }
    }

    void Reset() {
        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            for (size_t j = 0; j < chunk.usedIndexs.size(); ++j) {
                if (chunk.usedIndexs[j]) {
                    T* ptr = reinterpret_cast<T*>(chunk.data) + j;
                    ptr->~T();
                }
            }
        }
        freeIndexs.clear();
        for (Chunk& chunk : chunks) {
            chunk.curIndex = 0;
            std::fill(chunk.usedIndexs.begin(), chunk.usedIndexs.end(), false);
        }
        curChunk = 0;
    }

    ~ArenaLinearAllocator() {
        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            for (size_t j = 0; j < chunk.usedIndexs.size(); ++j) {
                if (chunk.usedIndexs[j]) {
                    T* ptr = reinterpret_cast<T*>(chunk.data) + j;
                    ptr->~T();
                }
            }
            free(chunk.data);
        }
    }

public: // private:
    struct Chunk {
        void* data;
        int curIndex = 0; // Tracks slot index
        std::vector<bool> usedIndexs; // Tracks used slots (true = used, false = free)
    };

    struct FreeIndex {
        int chunkIndex;
        int index; // Slot index
    };

    // Internal free function for raw pointers (used by Free, AllocShared deleter)
    void FreeRaw(T* p) {
        if (!p) return;

        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            T* chunkStart = reinterpret_cast<T*>(chunk.data);
            T* chunkEnd = chunkStart + chunkCapacity;
            if (p >= chunkStart && p < chunkEnd) {
                int index = static_cast<int>(p - chunkStart);
                if (index >= 0 && static_cast<size_t>(index) < chunk.usedIndexs.size() && chunk.usedIndexs[index]) {
                    p->~T(); // Call destructor
                    chunk.usedIndexs[index] = false;
                    freeIndexs.push_back({static_cast<int>(i), index});
                    if (i < curChunk) {
                        curChunk = i; // Move curChunk back
                    }
                } else {
                    throw std::runtime_error("Attempt to free unallocated or invalid pointer");
                }
                return;
            }
        }
        throw std::runtime_error("Pointer does not belong to any chunk");
    }

    void AddNewChunk() {
        chunks.emplace_back();
        Chunk& newChunk = chunks.back();
        newChunk.data = malloc(sizeof(T) * chunkCapacity); // Allocate slots * sizeof(T)
        if (!newChunk.data) {
            throw std::bad_alloc();
        }
        newChunk.curIndex = 0;
        newChunk.usedIndexs.resize(chunkCapacity, false);
    }

    std::vector<Chunk> chunks;
    std::vector<FreeIndex> freeIndexs;
    size_t chunkCapacity = 0; // Number of slots
    size_t curChunk = 0; // Current chunk index
};   

}