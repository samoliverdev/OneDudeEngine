#include "CameraComponent.h"
#include "OD/Core/ImGui.h"
#include <imgui/imgui_internal.h>
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

    ImGui::BeginTableEx(
        "CameraProperties", (ImGuiID)23443434, 2,
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

    ImGui::EndTable();
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