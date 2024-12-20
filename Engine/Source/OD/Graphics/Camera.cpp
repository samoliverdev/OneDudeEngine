#include "Camera.h"
#include "OD/Core/Application.h"
#include "OD/Core/Lua.h"

namespace OD{

void Camera::LookAt(Vector3 eye, Vector3 center, Vector3 up){
    //view = Matrix4LookAt(eye, center, up);
    view = math::lookAt(eye, center, up);
}

void Camera::SetOrtho(float scale, float near, float far, int width, int height){
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    projection = math::ortho(-aspect * scale, aspect * scale, -scale, scale, near, far);

    farClip = far;
    nearClip = near;
    this->fov = 0;
    this->width = width;
    this->height = height;
}

void Camera::SetPerspective(float fov, float near, float far, int width, int height){
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    projection = math::perspective(Mathf::Deg2Rad(fov), aspect, near, far);

    farClip = far;
    nearClip = near;
    this->fov = Mathf::Deg2Rad(fov);
    this->width = width;
    this->height = height;
}

void Camera::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Camera>(
        "Camera",
        "nearClip", &Camera::nearClip,
        "farClip", &Camera::farClip,
        "fov", &Camera::fov,
        "width", &Camera::width,
        "height", &Camera::height,
        "cleanColor", &Camera::cleanColor,
        "viewPos", &Camera::viewPos,
        "viewportRect", &Camera::viewportRect,
        "isDebug", &Camera::isDebug
    );
}

}