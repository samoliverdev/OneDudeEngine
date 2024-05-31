#pragma once
#include <OD/OD.h>
#include "ChunkData.h"

struct ChunkComponent{
    Ref<ChunkData> data = nullptr;
    bool isDirt = true;

    template<class Archive>
    void serialize(Archive& a){}

    inline static void OnGui(Entity& e){}
};