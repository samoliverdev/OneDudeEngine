#pragma once
#include <OD/Core/Module.h>
#include <string>

struct LoadSceneSample: public OD::Module {
    LoadSceneSample(){ name = "LoadSceneSample"; }
    std::string scenePath;
    std::string mode;
    std::string capturePath;
    std::string startupError;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};