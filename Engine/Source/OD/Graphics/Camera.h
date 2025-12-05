#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "Culling.h"

namespace sol{ class state; }

namespace OD {

struct OD_API Camera {
    enum class Type: std::int8_t { 
        Stand, SceneView, Preview, Reflection
    };

    Matrix4 view = Matrix4Identity;
    Matrix4 projection = Matrix4Identity;
    float nearClip;
    float farClip;
    float fov;
    int width;
    int height;
    Vector4 cleanColor = {0, 0, 0, 1};
    Vector3 viewPos;
    Frustum frustum;
    Vector4 viewportRect = Vector4(0, 0, 1, 1);
    Type type = Type::Stand;

    void LookAt(Vector3 eye, Vector3 center,Vector3 up);
    void SetOrtho(float scale, float near, float far, int width, int height);
    void SetPerspective(float fov, float near, float far, int width, int height);

    static void CreateLuaBind(sol::state& lua);
};

}