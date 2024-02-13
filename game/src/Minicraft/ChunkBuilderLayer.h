#pragma once
#include <OD/OD.h>
#include "VextexData.h"
#include "ChunkData.h"

using namespace OD;

class ChunkBuilderLayer{
public:
    ChunkBuilderLayer();
    void BuildMesh(ChunkData& chunkData, MeshRendererComponent& mesh);
    
private:
    Ref<Material> material;
};