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
    void BuildData(IVector3 coor, ChunkDataHolder& data);

    void BuildMeshOpaque(ChunkDataHolder& data);
    void BuildMeshWater(ChunkDataHolder& data);
    
    void BuildMesh(ChunkDataHolder& data);
    void BuildMesh2(ChunkDataHolder& data);
    
    Ref<Material> materialOpaque;
    Ref<Material> materialWater;
    Ref<Texture2DArray> texture;

private:
    fnl_state noise;
    WorldManagerSystem* world;
};