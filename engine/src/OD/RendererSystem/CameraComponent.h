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

    float orthographicSize = 5;
    float fieldOfView = 45;
    float nearClipPlane = 0.1f;
    float farClipPlane = 100;

    inline Camera camera(){ return _camera; }
    
    inline void UpdateCameraData(TransformComponent& transform){
        if(type == Type::Perspective)
            _camera.SetPerspective(fieldOfView, nearClipPlane, farClipPlane);
        else    
            _camera.SetOrtho(orthographicSize, nearClipPlane, farClipPlane);

        _camera.view = transform.globalModelMatrix().inverse();
    }

private:
    Camera _camera;
};

}