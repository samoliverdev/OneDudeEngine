#pragma once
#include "OD/Scene/Scene.h"

namespace OD{
    class Material;
}

using namespace OD;

namespace Standard{

void ModuleInit(); 

class StandardAssetSystem: public System{
public:
    StandardAssetSystem(Scene* scene);
    ~StandardAssetSystem() override;
    virtual int Type() override { return SystemType::Stand | SystemType::Late; }
    virtual bool ExecuteAlways() override { return true; }
    virtual void Update() override;
    virtual void LateUpdate() override;
private:
    Ref<Material> defaultMaterial = nullptr;
};

};