#pragma once

namespace OD{

template<typename T>
class ChunkedVector {
public:
    using Chunk = std::vector<T>;

    ChunkedVector():m_chunks(1){}

    ChunkedVector(size_t chunkCount)
        : m_chunks(chunkCount)
    {}
    
    // Access chunk by index
    Chunk& operator[](size_t chunkIndex) {
        assert(chunkIndex < m_chunks.size());
        return m_chunks[chunkIndex];
    }

    const Chunk& operator[](size_t chunkIndex) const {
        assert(chunkIndex < m_chunks.size());
        return m_chunks[chunkIndex];
    }

    T& GetNew(int chunkIndex){
        m_chunks[chunkIndex].emplace_back();
        return m_chunks[chunkIndex][m_chunks[chunkIndex].size()-1];
    }

    // Number of chunks
    size_t chunk_count() const {
        return m_chunks.size();
    }

private:
    std::vector<Chunk> m_chunks;
};


}