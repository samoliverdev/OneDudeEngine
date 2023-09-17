#include "Camera.h"
#include "OD/Core/Application.h"

namespace OD{

void Camera::LookAt(Vector3 eye, Vector3 center, Vector3 up){
    //view = Matrix4LookAt(eye, center, up);
    view = Matrix4::LookAt(eye, center, up);
}

void Camera::SetOrtho(float scale, float near, float far, int width, int height){
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    //projection = Matrix4Ortho(-aspect * scale, aspect * scale, -scale, scale, near, far);
    projection = Matrix4::Ortho(-aspect * scale, aspect * scale, -scale, scale, near, far);
}

void Camera::SetPerspective(float fov, float near, float far, int width, int height){
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    //projection = Matrix4Perspective(Deg2Rad(fov), aspect, near, far);
    projection = Matrix4::Perspective(Mathf::Deg2Rad(fov), aspect, near, far);
}

}