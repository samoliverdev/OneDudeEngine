#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

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

    inline Camera GetCamera(){ return camera; }
    
    void UpdateCameraData(TransformComponent& transform, int width, int height);

    /*inline void UpdateCameraDataLocal(TransformComponent& transform, int width, int height){
        if(type == Type::Perspective)
            camera.SetPerspective(fieldOfView, nearClipPlane, farClipPlane, width, height);
        else    
            camera.SetOrtho(orthographicSize, nearClipPlane, farClipPlane, width, height);

        camera.width = width;
        camera.height = height;
        camera.viewportRect = viewportRect;
        camera.viewPos = transform.Position();
        camera.view = math::inverse(transform.GetLocalModelMatrix());
        //Transform _trans = Transform(transform.Position(), transform.Rotation(), transform.LocalScale());
        Transform _trans = Transform(transform.GetLocalModelMatrix());
        camera.frustum = CreateFrustumFromCamera(
            _trans, 
            static_cast<float>(width) / static_cast<float>(height), 
            Mathf::Deg2Rad(fieldOfView), 
            nearClipPlane, 
            farClipPlane
        );
    }*/

    static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive & ar){
        /*ar(
            CEREAL_NVP(type),
            CEREAL_NVP(isMain),
            CEREAL_NVP(orthographicSize),
            CEREAL_NVP(fieldOfView),
            CEREAL_NVP(nearClipPlane),
            CEREAL_NVP(farClipPlane)
        );*/
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