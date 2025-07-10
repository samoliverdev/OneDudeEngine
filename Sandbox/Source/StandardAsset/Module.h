#pragma once
#include "OD/Scene/Scene.h"
using namespace OD;

namespace Standard{

void ModuleInit(); 

class StandardAssetSystem: public System{
public:
    StandardAssetSystem(Scene* scene);
    ~StandardAssetSystem() override;
    virtual int Type() override { return SystemType::Stand; }
    virtual bool ExecuteAlways() override { return true; }
    virtual void Update() override;
};

};