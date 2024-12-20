#include "MeshRendererComponent.h"
#include "OD/Core/ImGui.h"

namespace OD{

void MeshRendererComponent::OnGui(Entity& e){
    MeshRendererComponent& mesh = e.GetComponent<MeshRendererComponent>();

    std::string s1("mesh");
    std::string s2("material");
    ImGui::DrawAsset<Mesh>(s1, mesh.mesh);
    ImGui::DrawAsset<Material>(s2, mesh.material);
}

}