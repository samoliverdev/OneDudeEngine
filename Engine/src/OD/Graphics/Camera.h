#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "Culling.h"

namespace OD {

struct OD_API Camera {
    Matrix4 view = Matrix4Identity;
    Matrix4 projection = Matrix4Identity;
    float nearClip;
    float farClip;
    float fov;
    int width;
    int height;
    Vector3 cleanColor;
    Vector3 viewPos;
    Frustum frustum;
    Vector4 viewportRect = Vector4(0, 0, 1, 1);
    bool isDebug = false;

    void LookAt(Vector3 eye, Vector3 center,Vector3 up);
    void SetOrtho(float scale, float near, float far, int width, int height);
    void SetPerspective(float fov, float near, float far, int width, int height);
};

}