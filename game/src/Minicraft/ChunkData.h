#pragma once

#include <vector>

struct Voxel{
    char id = 0;
};

struct ChunkData{
    ChunkData(int inSize = 32){
        if(inSize <= 0) inSize = 1;
        SetSize(inSize);
    }

    inline int GetSize(){
        return size;
    }

    inline void SetSize(int inSize){
        size = inSize;
        voxels.resize(size * size * size);
    }

    inline Voxel GetVoxel(int x, int y, int z){
        return voxels[to1D(x, y, z)];
    }

    inline void SetVoxel(int x, int y, int z, Voxel v){
        voxels[to1D(x, y, z)] = v;
    }

    inline bool IsOffChunk(int x, int y, int z){
        if(x < 0 || x >= size) return true;
        if(y < 0 || y >= size) return true;
        if(z < 0 || z >= size) return true;
        return false;
    }

private:
    int size = 32;
    std::vector<Voxel> voxels;

    inline void CheckResize(){
        if(voxels.size() < (size * size * size)){
            voxels.resize(size * size * size);
        }
    }

    inline int to1D(int x, int y, int z){
        return x + size * (y + size * z);
    }   
};