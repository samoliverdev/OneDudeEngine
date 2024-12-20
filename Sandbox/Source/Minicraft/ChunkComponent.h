#pragma once
#include <OD/OD.h>
#include "ChunkData.h"

struct ChunkComponent{
    ChunkDataHolder data;
    Entity opaqueMesh;
    Entity transparentMesh;
    Entity grassMesh;
    Entity waterMesh;
    bool isDirt = true;

    template<class Archive>
    void serialize(Archive& a){}

    inline static void OnGui(Entity& e){}
};