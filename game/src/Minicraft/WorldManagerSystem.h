#pragma once
#include <OD/OD.h>
#include <chrono>
#include <thread>
#include <future>
#include "OD/Core/ThreadPool.h"
#include "ChunkData.h"
#include "ChunkComponent.h"

using namespace OD;

/*struct _IVector3{
    int x;
    int y; 
    int z;

    _IVector3(int _x, int _y, int _z):x(_x),y(_y),z(_z){}
    _IVector3(IVector3 v):x(v.x),y(v.y),z(v.z){}
};

bool operator<(const _IVector3& a, const _IVector3& b);

inline IVector3 ToIVector3(_IVector3 v){ return IVector3(v.x, v.y, v.z); }*/

Ref<Mesh> _LoadChunk(IVector3 coord);

struct ChunkDataHolder{
    Ref<Mesh> mesh;
    Ref<ChunkData> chunkData;
    Ref<std::atomic<bool>> done;
};

class ChunkBuilderLayer;

class WorldManagerSystem: public System{
public:
    int chunkLoadDistance = 32;
    int chunkSize = 16;
    int chunkHeigtCount = 16;
    int floorY = 192;

    WorldManagerSystem(Scene* scene);
    System* Clone(Scene* inScene) const override { return new WorldManagerSystem(inScene); }
    virtual void Update() override;
    virtual void OnRender() override;
    virtual void OnDrawGizmos() override;

    bool CheckForVoxelNotEmpty(Vector3 worldPos, bool invertZ = true);
    bool CheckForVoxelIsEmpty(Vector3 worldPos, bool invertZ = true);
    ChunkComponent* GetChunkFromWorldPos(Vector3 worldPos);

private:
    Ref<ChunkBuilderLayer> chunkBuilderLayer;
    
    std::unordered_map<IVector3, Entity> loadedChunks;
    std::unordered_map<IVector3, Ref<ChunkData>> loadedChunksB;

    IVector3 lastCoord;
    IVector3 currentCoord;
    Vector3 camPos;
    //std::vector<IVector3> toLoadCoords;
    std::unordered_map<IVector3, bool> toLoadCoords;

    //std::unordered_map<IVector3, ChunkDataHolder> toLoadCoords2;
    std::vector<IVector3> toLoadCoordsA;
    std::vector<ChunkDataHolder> toLoadCoordsB;
    IVector2 loadedIndexRange;
    const int loadedRangeStep = 33;

    std::vector<std::thread> loadingThreadsA;
    int _loadingThreadsA = 0;
    std::vector<std::thread> loadingThreadsB;
    int _loadingThreadsB = 0;

    //Ref<ThreadPool> threadPool = CreateRef<ThreadPool>(4);
    Ref<ThreadPool2> threadPool = CreateRef<ThreadPool2>();

    bool hasPlaceAndHighlightPos;
    Vector3 placeBlockPos;
    Vector3 highlightBlockPos;
    
    void HandleChunkEdit();
    void HandleInput();
    void HandleChunkDirt();

    void LoadChunk(IVector3 coord);
    void UnLoadChunk(IVector3 coord);
    void HandleLoadUnload();
    void HandleLoadUnload2();
};

