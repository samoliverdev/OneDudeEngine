#include "MeshRendererComponent.h"
#include "OD/Core/ImGui.h"

namespace OD{

void MeshRendererComponent::OnGui(Entity& e){
    MeshRendererComponent& mesh = e.GetComponent<MeshRendererComponent>();

    ImGui::DrawAsset<Mesh>(std::string("mesh"), mesh.mesh);
    ImGui::DrawAsset<Material>(std::string("material"), mesh.material);
}

}