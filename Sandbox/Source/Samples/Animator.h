#pragma once
#include <OD/Core/Module.h>
#include <OD/Scene/Scene.h>

using namespace OD;

struct AnimatorSample: public OD::Module {
    Entity camera;
    //std::vector<FastClip> clips;

    AnimatorSample(){ name = "AnimatorSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};