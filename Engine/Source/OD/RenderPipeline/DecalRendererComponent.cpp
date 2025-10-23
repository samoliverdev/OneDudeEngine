#include "DecalRendererComponent.h"
#include "OD/Core/ImGui.h"

namespace OD{

void DecalRendererComponent::OnGui(Entity& e, Scene& scene){
    DecalRendererComponent& decal = scene.GetComponent<DecalRendererComponent>(e);

    ImGui::Checkbox("useCustomOffsetAndSize", &decal.useCustomOffsetAndSize);

    if(decal.useCustomOffsetAndSize){
        ImGui::DragFloat3("offset", &decal.offset.x);
        ImGui::DragFloat3("size", &decal.size.x);
    }

    ImGui::DrawAsset<Material>("material", decal.material);
}

}