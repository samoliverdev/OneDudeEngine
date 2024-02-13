#pragma once
#include <OD/OD.h>
#include "ChunkData.h"

struct ChunkComponent{
    ChunkData data;
    bool isDirt = true;

    template<class Archive>
    void serialize(Archive& a){}

    inline static void OnGui(Entity& e){}
};