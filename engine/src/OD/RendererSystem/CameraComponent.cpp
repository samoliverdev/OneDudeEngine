#include "CameraComponent.h"

namespace OD{

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