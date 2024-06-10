#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

struct OD_API CameraComponent{
    enum class Type{
        Perspective, Orthographic 
    };

    Type type = Type::Perspective;

    bool isMain = true;

    float orthographicSize = 5;
    float fieldOfView = 45;
    float nearClipPlane = 0.1f;
    float farClipPlane = 100;
    Vector4 viewportRect = Vector4(0, 0, 1, 1);

    Camera GetCamera();
    
    void UpdateCameraData(TransformComponent& transform, int width, int height);

    static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(isMain));
        ArchiveDump(ar, CEREAL_NVP(orthographicSize));
        ArchiveDump(ar, CEREAL_NVP(fieldOfView));
        ArchiveDump(ar, CEREAL_NVP(nearClipPlane));
        ArchiveDump(ar, CEREAL_NVP(farClipPlane));
    }

private:
    Camera camera;
};

}