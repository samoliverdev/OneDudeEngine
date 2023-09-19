#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Renderer/Mesh.h"
#include "StandRendererSystem.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

namespace OD{

struct CameraComponent{
    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);

    enum class Type{
        Perspective, Orthographic 
    };

    Type type = Type::Perspective;

    bool isMain = true;

    float orthographicSize = 5;
    float fieldOfView = 45;
    float nearClipPlane = 0.1f;
    float farClipPlane = 100;

    inline Camera camera(){ return _camera; }
    
    inline void UpdateCameraData(TransformComponent& transform, int width, int height){
        if(type == Type::Perspective)
            _camera.SetPerspective(fieldOfView, nearClipPlane, farClipPlane, width, height);
        else    
            _camera.SetOrtho(orthographicSize, nearClipPlane, farClipPlane, width, height);

        _camera.view = transform.globalModelMatrix().inverse();
    }

private:
    Camera _camera;
};

}