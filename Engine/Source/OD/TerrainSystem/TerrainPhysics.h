#pragma once
#include "OD/Scene/Scene.h"
#include <memory>

namespace OD{
class OD_API TerrainCollisionSystem final : public System{
public:
    TerrainCollisionSystem();
    ~TerrainCollisionSystem() override;

    int Type() override{
        return SystemType::PrePhysics;
    }

    int ExecutionSortPriority(SystemType) override{
        return -10;
    }

    void OnStart(Scene& scene) override;
    void PrePhysicsUpdate(Scene& scene) override;
    void OnStop(Scene& scene) override;
    void OnEnd(Scene& scene) override;

private:
    struct State;
    std::unique_ptr<State> state;
};
}
