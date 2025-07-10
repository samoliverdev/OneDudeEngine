#pragma once
#include <OD/Core/Module.h>
#include <OD/Scene/Scene.h>
using namespace OD;

struct SynthCitySample: Module{
    Entity camera;
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};