#pragma once

#include "OD/Core/Math.h"

namespace OD {

struct Camera {
    Matrix4 view = Matrix4::identity;
    Matrix4 projection = Matrix4::identity;

    void LookAt(Vector3 eye, Vector3 center,Vector3 up);
    void SetOrtho(float scale, float near, float far, int width, int height);
    void SetPerspective(float fov, float near, float far, int width, int height);
};

}