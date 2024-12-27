#include "CameraComponent.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Math.h"
#include "OD/Core/Lua.h"

namespace OD{

Camera CameraComponent::GetCamera(){ 
    return camera; 
}

void CameraComponent::UpdateCameraData(TransformComponent& transform, int width, int height){
    if(type == Type::Perspective)
        camera.SetPerspective(fieldOfView, nearClipPlane, farClipPlane, width, height);
    else    
        camera.SetOrtho(orthographicSize, nearClipPlane, farClipPlane, width, height);

    camera.width = width;
    camera.height = height;
    camera.viewportRect = viewportRect;
    camera.viewPos = transform.Position();
    camera.view = math::inverse(transform.GlobalModelMatrix());
    Transform _trans = Transform(transform.Position(), transform.Rotation(), transform.LocalScale());
    //Transform _trans = Transform(transform.GlobalModelMatrix());

    /*if(type == Type::Perspective){
        camera.frustum = CreateFrustumFromCamera(
            _trans, 
            static_cast<float>(width) / static_cast<float>(height), 
            Mathf::Deg2Rad(fieldOfView), 
            nearClipPlane, 
            farClipPlane
        );
    } else {
        camera.frustum = CreateFrustumFromOthor(
            _trans, 
            orthographicSize, 
            static_cast<float>(width) / static_cast<float>(height), 
            nearClipPlane, 
            farClipPlane
        );
    }*/  

    camera.frustum = CreateFrustumFromMatrix2(math::transpose( camera.projection * camera.view ));
    //camera.frustum = CreateFrustumFromMatrix2(camera.view * camera.projection);
}

void CameraComponent::OnGui(Entity& e, Scene& scene){
    CameraComponent& cam = scene.GetComponent<CameraComponent>(e);

    ImGui::DrawEnumCombo<CameraComponent::Type>("projection", &cam.type);
    ImGui::DrawEnumCombo<CameraComponent::RenderingPath>("renderingPath", &cam.renderingPath);

    if(cam.type == CameraComponent::Type::Orthographic){
        ImGui::DragFloat("size", &cam.orthographicSize);
    }

    if(cam.type == CameraComponent::Type::Perspective){
        ImGui::DragFloat("fieldOfView", &cam.fieldOfView);
    }

    ImGui::DragFloat("nearClipPlane", &cam.nearClipPlane);
    ImGui::DragFloat("farClipPlane", &cam.farClipPlane);
}

void CameraComponent::CreateLuaBind(sol::state& lua){
        lua.new_enum(
        "CameraComponentType",
            "Orthographic", CameraComponent::Type::Orthographic,
            "Perspective", CameraComponent::Type::Perspective
        );
        Scene::RegisterMetaComponent<CameraComponent>();
        lua.new_usertype<CameraComponent>(
            "CameraComponent",
            sol::call_constructor,
            sol::factories([](){ return CameraComponent(); }),
            "type", &CameraComponent::type,
            "isMain", &CameraComponent::isMain,
            "orthographicSize", &CameraComponent::orthographicSize,
            "fieldOfView", &CameraComponent::fieldOfView,
            "nearClipPlane", &CameraComponent::nearClipPlane,
            "farClipPlane", &CameraComponent::farClipPlane
        );
    }

}