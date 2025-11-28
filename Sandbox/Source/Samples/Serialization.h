#pragma once
#include <OD/Core/Module.h>

namespace OD{
    class Scene;
}

struct SerializationSample: public OD::Module{
    OD::Scene* scene;

    SerializationSample(){ name = "SerializationSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};