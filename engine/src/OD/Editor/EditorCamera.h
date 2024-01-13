#pragma once

#include "OD/Core/Transform.h"
#include "OD/Graphics/Camera.h"
#include "OD/RenderPipeline/StandRenderPipeline.h"

namespace OD{

class EditorCamera{
public:
    float moveSpeed = 10;
    float rotSpeed = 50;

    float lastX = 400, lastY = 300;

    float yaw;
    float pitch;

    Transform transform;
    Camera cam;

    void OnStart();
    void OnUpdate();
};

}