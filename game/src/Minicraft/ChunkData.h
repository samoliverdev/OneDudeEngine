#pragma once

#include <OD/OD.h>
#include <vector>

using namespace OD;

struct Voxel{
    //unsigned char id = 0;
    unsigned short id = 0;
    unsigned char light = 0;
};

struct ChunkData{
    inline ChunkData(IVector3 inCoord, Vector3 inCoordPos, int inSize = 32, int inHeight = 32){
        if(inSize <= 0) inSize = 1;
        if(inHeight <= 0) inHeight = 1;
        SetSize(inSize, inHeight);
        coord = inCoord;
        coordPos = inCoordPos;
    }

    inline ~ChunkData(){
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

    inline bool IsOffChunkX(int x){
        if(x < 0 || x >= size) return true;
        return false;
    }
    inline bool IsOffChunkY(int y){
        if(y < 0 || y >= height) return true;
        return false;
    }
    inline bool IsOffChunkZ(int z){
        if(z < 0 || z >= size) return true;
        return false;
    }

    inline bool SetVoxelFromGlobalVector3(Vector3 pos, Voxel voxel){
        int xCheck = math::floor(pos.x);
        int yCheck = math::floor(pos.y);
        int zCheck = math::floor(pos.z);

        xCheck -= math::floor(coordPos.x);
        zCheck -= math::floor(coordPos.z);

        LogInfo("GetVoxelFromGlobalVector3 x: %d y: %d z: %d", xCheck, yCheck, zCheck);
        if(IsOffChunk(xCheck, yCheck, zCheck)) return false;
        SetVoxel(xCheck, yCheck, zCheck, voxel);
        return true;
    }

    inline Voxel GetVoxelFromGlobalVector3(Vector3 pos){
        int xCheck = math::floor(pos.x);
        int yCheck = math::floor(pos.y);
        int zCheck = math::floor(pos.z);

        xCheck -= math::floor(coordPos.x);
        zCheck -= math::floor(coordPos.z);

        //LogInfo("GetVoxelFromGlobalVector3 x: %d y: %d z: %d", xCheck, yCheck, zCheck);
        if(IsOffChunk(xCheck, yCheck, zCheck)) return Voxel{0};
        return GetVoxel(xCheck, yCheck, zCheck);
    }

    inline bool VoxelFromGlobalVector3IsOffChunk(Vector3 pos){
        int xCheck = math::floor(pos.x);
        int yCheck = math::floor(pos.y);
        int zCheck = math::floor(pos.z);

        xCheck -= math::floor(coordPos.x);
        zCheck -= math::floor(coordPos.z);

        //LogInfo("GetVoxelFromGlobalVector3 x: %d y: %d z: %d", xCheck, yCheck, zCheck);
        if(IsOffChunk(xCheck, yCheck, zCheck)) return true;
        return false;
    }

    /*inline Voxel GetVoxelFromGlobalVector3(Vector3 pos, Vector3 chunkPos){
        int xCheck = math::floor(pos.x);
        int yCheck = math::floor(pos.y);
        int zCheck = math::floor(pos.z);

        xCheck -= math::floor(chunkPos.x);
        zCheck -= math::floor(chunkPos.z);

        LogInfo("GetVoxelFromGlobalVector3 x: %d y: %d z: %d", xCheck, yCheck, zCheck);
        if(IsOffChunk(xCheck, yCheck, zCheck)) return Voxel{0};
        return GetVoxel(xCheck, yCheck, zCheck);
    }*/

    inline Vector3 CoordPos(){ return coordPos; }
    inline Vector3 Coord(){ return coord; }
    
private:
    int size = 32;
    int height = 32;
    std::vector<Voxel> voxels;
    Vector3 coordPos;
    IVector3 coord;

    inline int to1D(int x, int y, int z){
        // x* width + y +z*( width * height)
        //return x * size + y + z * (size * height);
        return (z * size * height) + (y * size) + x;
        //return x + height * (y + size * z);
        //return x + size * (y + size * z);
    }   
};

struct ChunkDataHolder{
    Ref<ChunkData> chunkData = nullptr;
    Ref<Mesh> opaqueMesh = nullptr;
    Ref<Mesh> transparentMesh = nullptr;
    Ref<Mesh> grassMesh = nullptr;
    Ref<Mesh> waterMesh = nullptr;
};