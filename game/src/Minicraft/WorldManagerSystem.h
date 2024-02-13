#pragma once
#include <OD/OD.h>
#include "ChunkBuilderLayer.h"

using namespace OD;

class WorldManagerSystem: public System{
public:
    WorldManagerSystem(Scene* scene);
    System* Clone(Scene* inScene) const override { return new WorldManagerSystem(inScene); }

    virtual void Update() override;

private:
    ChunkBuilderLayer chunkBuilderLayer;
};

