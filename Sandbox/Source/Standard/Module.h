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
    StandardAssetSystem();
    ~StandardAssetSystem() override;
    virtual int Type() override { return SystemType::Stand | SystemType::FixedPhysics | SystemType::Late; }
    virtual bool ExecuteAlways() override { return true; }
    virtual void Update(Scene& scene) override;
    virtual void FixedPhysicsUpdate(Scene& scene) override;
    virtual void LateUpdate(Scene& scene) override;
private:
    Ref<Material> defaultMaterial = nullptr;
};

};