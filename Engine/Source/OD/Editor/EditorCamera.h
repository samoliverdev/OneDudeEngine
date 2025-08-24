#pragma once
#include "OD/Core/Transform.h"
#include "OD/Graphics/Camera.h"

namespace OD{

class OD_API EditorCamera{
public:
    float moveSpeed = 10;
    float fastMoveSpeed = 50;
    float rotSpeed = 50;

    float lastX = 400, lastY = 300;

    float yaw;
    float pitch;

    Transform transform;
    Camera cam;

    void OnStart();
    void OnUpdate();
};

class OD_API AssetPreviewCamera {
public:
    float orbitSpeed = 0.4f;
    float zoomSpeed = 2.0f;

    float yaw = 0.0f;
    float pitch = 0.0f;
    float distance = 5.0f;

    float lastX = 400, lastY = 300;

    Vector3 target = Vector3(0, 0, 0); // Ponto de foco
    Transform transform;
    Camera cam;

    void OnStart();

    void OnUpdate();
};

}