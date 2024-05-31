#pragma once
#include <OD/OD.h>
#include "VextexData.h"
#include "ChunkData.h"
#include "Ultis/FastNoiseLite.h"

using namespace OD;

class WorldManagerSystem;

class ChunkBuilderLayer{
public:
    ChunkBuilderLayer(WorldManagerSystem* inWorld);
    void BuildData(IVector3 coor, Ref<ChunkData> chunkData);
    
    void BuildMesh(ChunkData& chunkData, Ref<Mesh>& mesh);
    void BuildMesh2(/*IVector3 coord,*/ ChunkData& chunkData, Ref<Mesh>& mesh/*, std::unordered_map<IVector3, Ref<ChunkData>>& loadedChunkDatas*/);
    
    Ref<Material> material;
    Ref<Texture2DArray> texture;

private:
    fnl_state noise;
    WorldManagerSystem* world;
};