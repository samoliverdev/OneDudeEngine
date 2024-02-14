#pragma once

#include <OD/OD.h>
#include <vector>

struct Voxel{
    unsigned char id = 0;
};

struct ChunkData{
    ChunkData(int inSize = 32, int inHeight = 32){
        if(inSize <= 0) inSize = 1;
        if(inHeight <= 0) inHeight = 1;
        SetSize(inSize, inHeight);
    }

    ~ChunkData(){
        voxels.clear();
        //delete voxels;
    }

    inline int GetSize(){ return size; }
    inline int GetHeight(){ return height; }

    inline void SetSize(int inSize, int inHeight){
        size = inSize;
        height = inHeight;

        int _size = size * inHeight * size;
        voxels.resize(_size);
        //LogInfo("Size: %d", _size);
        //voxels = new Voxel[size * inHeight * size];
    }

    inline Voxel GetVoxel(int x, int y, int z){
        //Assert(to1D(x, y, z) < voxels.size());
        return voxels[to1D(x, y, z)];
    }

    inline void SetVoxel(int x, int y, int z, Voxel v){
        int index = to1D(x, y, z);
        //if(index >= voxels.size()){
        //    Assert(false);
        //}
        //Assert(index < voxels.size());
        voxels[to1D(x, y, z)] = v;
    }

    inline bool IsOffChunk(int x, int y, int z){
        if(x < 0 || x >= size) return true;
        if(y < 0 || y >= height) return true;
        if(z < 0 || z >= size) return true;
        return false;
    }

private:
    int size = 32;
    int height = 32;
    std::vector<Voxel> voxels;
    //Voxel* voxels;

    inline int to1D(int x, int y, int z){
        // x* width + y +z*( width * height)
        //return x * size + y + z * (size * height);
        return (z * size * height) + (y * size) + x;
        //return x + height * (y + size * z);
        //return x + size * (y + size * z);
    }   
};