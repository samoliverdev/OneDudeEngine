#pragma once
#include <OD/Core/Module.h>
#include <OD/Scene/Scene.h>
#include "Ultis/CameraMovement.h"
#include <future>
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_speech.h>
#include <soloud_thread.h>

using namespace OD;

struct RenderPipelineSample: public OD::Module {
    //CameraMovement camMove;

    Entity mainEntity;
    Entity camera;
    Entity light;
    Entity otherEntity;

    Transform gismoTransform;

    std::future<void> playMusic;

    SoLoud::Soloud soloud;
    SoLoud::Wav sample;

    void AddTransparent(Vector3 pos);

    RenderPipelineSample(){ name = "RenderPipelineSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};