#pragma once
#include <OD/OD.h>

using namespace OD;

struct CharacterControllerSample: OD::Module {
    Entity camera;
    Navmesh navmesh;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};