#pragma once
#include <OD/OD.h>
#include "VextexData.h"
#include "ChunkData.h"
#include "Ultis/FastNoiseLite.h"

using namespace OD;

class ChunkBuilderLayer{
public:
    ChunkBuilderLayer();
    void BuildData(IVector3 coor, ChunkData& chunkData);
    void BuildMesh(ChunkData& chunkData, MeshRendererComponent& mesh);
    
private:
    Ref<Material> material;
    fnl_state noise;
};