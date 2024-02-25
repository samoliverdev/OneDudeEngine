#pragma once
#include <OD/OD.h>

extern "C"{
    EXPORT_FN OD::Module* CreateInstance();
}

class DynamicModule: public OD::Module{
public:
    void OnInit() override;
    void OnExit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
};