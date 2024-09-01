#include "CameraComponent.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Math.h"

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

void CameraComponent::OnGui(Entity& e){
    CameraComponent& cam = e.GetComponent<CameraComponent>();

    ImGui::DrawEnumCombo<CameraComponent::Type>("projection", &cam.type);

    if(cam.type == CameraComponent::Type::Orthographic){
        ImGui::DragFloat("size", &cam.orthographicSize);
    }

    if(cam.type == CameraComponent::Type::Perspective){
        ImGui::DragFloat("fieldOfView", &cam.fieldOfView);
    }

    ImGui::DragFloat("nearClipPlane", &cam.nearClipPlane);
    ImGui::DragFloat("farClipPlane", &cam.farClipPlane);
}

}