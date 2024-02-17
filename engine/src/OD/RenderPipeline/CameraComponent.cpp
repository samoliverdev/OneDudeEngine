#include "CameraComponent.h"

namespace OD{

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

    const char* projectionTypeString[] = {"Perspective", "Orthographic"};
    const char* curProjectionTypeString = projectionTypeString[(int)cam.type];
    if(ImGui::BeginCombo("Projection", curProjectionTypeString)){
        for(int i = 0; i < 2; i++){
            bool isSelected = curProjectionTypeString == projectionTypeString[i];
            if(ImGui::Selectable(projectionTypeString[i], isSelected)){
                curProjectionTypeString = projectionTypeString[i];
                cam.type = (CameraComponent::Type)i;
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
            
        }

        ImGui::EndCombo();
    }

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