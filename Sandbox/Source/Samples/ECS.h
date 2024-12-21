#pragma once
#include <OD/OD.h>

using namespace OD;

struct ECSSample: public OD::Module {
    //CameraMovement camMove;

    Entity mainEntity;
    Entity camera;
    Entity light;
    Entity otherEntity;

    Transform gismoTransform;

    std::future<void> playMusic;

    /*SoLoud::Soloud soloud;
    SoLoud::Wav sample;*/

    void AddTransparent(Vector3 pos);

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override ;

};