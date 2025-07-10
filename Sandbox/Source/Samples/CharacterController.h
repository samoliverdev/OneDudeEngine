#pragma once
#include <OD/Core/Module.h>
#include <OD/Scene/Scene.h>
#include <OD/Navmesh/Navmesh.h>

using namespace OD;

struct CharacterControllerSample: Module {
    Entity camera;
    Navmesh navmesh;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};