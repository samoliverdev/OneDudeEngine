#pragma once
#include <OD/Core/Module.h>
#include <OD/Scene/Scripts.h>

using namespace OD;

struct PhysicsCubeS: public Script{
    float t;
    float timeToDestroy = 5;

    void OnStart() override;
    void OnUpdate() override;
    void OnDestroy() override;

    template <class Archive>
    void serialize(Archive & ar){}
};

struct PhysicsSample: OD::Module {
    Entity camera;

    PhysicsSample(){ name = "PhysicsSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override; 
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};