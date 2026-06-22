#include "OD/pch.h"
#include "CameraComponent.h"
#include "OD/Scene/SceneMeta.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Math.h"
#include "OD/Core/Lua.h"
#include <imgui/imgui_internal.h>

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
    //camera.view = math::inverse(transform.GlobalModelMatrix());
    camera.cleanColor = cleanColor;
    Transform _trans = Transform(transform.Position(), transform.Rotation(), transform.LocalScale());
    //Transform _trans = Transform(transform.GlobalModelMatrix());

    Vector3 pos = transform.Position();
    Quaternion rot = glm::normalize(transform.Rotation());
    Matrix4 R = mat4_cast(rot);
    glm::mat4 view = glm::transpose(R) * glm::translate(glm::mat4(1.0f), -pos);
    camera.view = view;

    /*glm::vec3 position = transform.Position();
    glm::quat orientation = glm::normalize(transform.Rotation());
    // Camera view matrix = inverse of camera transform
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), -position);
    glm::mat4 rotate   = glm::mat4_cast(orientation);
    glm::mat4 view     = translate * glm::transpose(rotate);   // or rotate^{-1} * translate
    camera.view = view;*/

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

    //camera.frustum = CreateFrustumFromMatrix2(math::transpose( camera.projection * camera.view ));
    camera.frustum = CreateFrustumFromMatrix(camera.projection * camera.view);
}

void CameraComponent::OnGui(Entity& e, Scene& scene){
    CameraComponent& cam = scene.GetComponent<CameraComponent>(e);

    /*ImGui::DrawEnumCombo<CameraComponent::Type>("projection", &cam.type);
    ImGui::DrawEnumCombo<CameraComponent::RenderingPath>("renderingPath", &cam.renderingPath);

    if(cam.type == CameraComponent::Type::Orthographic){
        ImGui::DragFloat("size", &cam.orthographicSize);
    }

    if(cam.type == CameraComponent::Type::Perspective){
        ImGui::DragFloat("fieldOfView", &cam.fieldOfView);
    }

    ImGui::DragFloat("nearClipPlane", &cam.nearClipPlane);
    ImGui::DragFloat("farClipPlane", &cam.farClipPlane);*/

    /*ImGui::BeginTableEx(
        "CameraProperties", ImGui::GlobalTableID, 2,
        ImGuiTableFlags_NoSavedSettings |
        ImGuiTableFlags_Resizable |                    // Permite redimensionar colunas manualmente
        //ImGuiTableFlags_SizingStretchSame |            // Faz com que as colunas preencham o espaço igualmente
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_NoPadOuterX | 
        ImGuiTableFlags_BordersInnerV
    );

    // Configura as colunas com comportamento Unreal-like
    //ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_None, 90.0f);  // Não usar WidthFixed!
    //ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);    // Stretch permite expandir o restante
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);

    // Função auxiliar para adicionar linha
    auto AddTableRow = [&](const char* label, const char* id, auto&& renderControl) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-1);
        renderControl(id);
        ImGui::PopItemWidth();
    };

    // Exemplo de uso
    AddTableRow("projection", "##projection", [&](const char* id) {
        ImGui::DrawEnumCombo<CameraComponent::Type>(id, &cam.type);
    });
    AddTableRow("renderingPath", "##renderingPath", [&](const char* id) {
        ImGui::DrawEnumCombo<CameraComponent::RenderingPath>(id, &cam.renderingPath);
    });
    if(cam.type == CameraComponent::Type::Orthographic){
        AddTableRow("size", "##size", [&](const char* id) {
            ImGui::DragFloat(id, &cam.orthographicSize);
        });
    }
    if(cam.type == CameraComponent::Type::Perspective){
        AddTableRow("fieldOfView", "##fieldOfView", [&](const char* id) {
            ImGui::DragFloat(id, &cam.fieldOfView);
        });
    }
    AddTableRow("nearClipPlane", "##nearClipPlane", [&](const char* id) {
        ImGui::DragFloat(id, &cam.nearClipPlane);
    });
    AddTableRow("farClipPlane", "##farClipPlane", [&](const char* id) {
        ImGui::DragFloat(id, &cam.farClipPlane);
    });

    ImGui::EndTable();*/

    IMGUI_BeginGlobalTable("CameraProperties");

    IMGUI_GlobalTableRow("projection", ImGui::DrawEnumCombo<CameraComponent::Type>("##projection", &cam.type));
    IMGUI_GlobalTableRow("IsMain", ImGui::Checkbox("#IsMain", &cam.isMain));
    IMGUI_GlobalTableRow("renderingPath", ImGui::DrawEnumCombo<CameraComponent::RenderingPath>("##renderingPath", &cam.renderingPath));
    if(cam.type == CameraComponent::Type::Orthographic){
        IMGUI_GlobalTableRow("size", ImGui::DragFloat("##size", &cam.orthographicSize));
    }
    if(cam.type == CameraComponent::Type::Perspective){
        IMGUI_GlobalTableRow("fieldOfView", ImGui::DragFloat("##fieldOfView", &cam.fieldOfView));
    }
    IMGUI_GlobalTableRow("nearClipPlane", ImGui::DragFloat("##nearClipPlane", &cam.nearClipPlane));
    IMGUI_GlobalTableRow("farClipPlane", 
        {
            ImGui::DragFloat("##farClipPlane", &cam.farClipPlane);
        }
    );

    IMGUI_GlobalTableRow("cleanColor", {
        ImGui::ColorEdit4("##cleanColor", &cam.cleanColor.x);
    });

    IMGUI_EndGlobalTable();
}

void CameraComponent::CreateLuaBind(sol::state& lua){
        lua.new_enum(
        "CameraComponentType",
            "Orthographic", CameraComponent::Type::Orthographic,
            "Perspective", CameraComponent::Type::Perspective
        );
        SceneMeta::RegisterMetaComponent<CameraComponent>();
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