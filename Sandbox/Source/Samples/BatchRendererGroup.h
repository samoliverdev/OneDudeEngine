#pragma once
#include <OD/Core/Module.h>
#include "Others/BatchRendererGroup.h"

struct BatchRendererGroupSample : public OD::Module {
    BatchRendererGroupSample() : batch() { name = "BatchRendererGroupSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;

private:
    OD::BatchRendererGroup batch;
};
