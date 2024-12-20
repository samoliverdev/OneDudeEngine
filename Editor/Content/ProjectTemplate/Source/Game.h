#pragma once
//#include <OD/OD.h>
#include <OD/Core/Module.h>

extern "C"{
    EXPORT_FN OD::Module* CreateInstance();

    EXPORT_FN void GameOnInit();
    EXPORT_FN void GameOnExit();
    EXPORT_FN void GameOnUpdate();
}

class Game: public OD::Module{
public:
    void OnInit() override;
    void OnExit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;

    //tf::Taskflow taskflow;
    //tf::Executor executor;
};