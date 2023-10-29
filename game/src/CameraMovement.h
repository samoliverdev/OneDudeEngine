#pragma once

#include <OD/OD.h>
using namespace OD;

class CameraMovement{
public:
    float moveSpeed = 10;
    float rotSpeed = 50;

    float lastX = 400, lastY = 300;

    float yaw;
    float pitch;

    void OnStart();
    void OnUpdate();

    Transform* transform;
};

class CameraMovementScript: public Script{
public:
    float moveSpeed = 10;
    float rotSpeed = 50;

    float lastX = 400, lastY = 300;

    float yaw;
    float pitch;

    void OnStart() override;
    void OnUpdate() override;

    void Serialize(ArchiveNode& s) override {
        s.typeName("CameraMovementScript");
        s.Add(&moveSpeed, "moveSpeed");
        s.Add(&rotSpeed, "rotSpeed");
    }
};
