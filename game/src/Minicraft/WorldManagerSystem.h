#pragma once
#include <OD/OD.h>
#include <unordered_map>
#include <map>
#include "ChunkBuilderLayer.h"

using namespace OD;

struct _IVector3{
    int x;
    int y; 
    int z;

    _IVector3(int _x, int _y, int _z):x(_x),y(_y),z(_z){}
    _IVector3(IVector3 v):x(v.x),y(v.y),z(v.z){}
};

bool operator<(const _IVector3& a, const _IVector3& b);

inline IVector3 ToIVector3(_IVector3 v){ return IVector3(v.x, v.y, v.z); }

class WorldManagerSystem: public System{
public:
    int chunkLoadDistance = 16;
    int chunkSize = 16;
    int chunkHeigtCount = 24;
    int floorY = 80;

    WorldManagerSystem(Scene* scene);
    System* Clone(Scene* inScene) const override { return new WorldManagerSystem(inScene); }
    virtual void Update() override;
    virtual void OnDrawGizmos() override;

private:
    ChunkBuilderLayer chunkBuilderLayer;
    std::map<_IVector3, Entity> loadedChunks;

    Vector3 camPos;
    std::vector<IVector3> toLoadCoords;

    void LoadChunk(IVector3 coord);
    void UnLoadChunk(IVector3 coord);
    void HandleLoadUnload();
};

