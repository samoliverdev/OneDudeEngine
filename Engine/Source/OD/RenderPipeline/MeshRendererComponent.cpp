#include "MeshRendererComponent.h"
#include "OD/Core/ImGui.h"

namespace OD{

void MeshRendererComponent::OnGui(Entity& e, Scene& scene){
    MeshRendererComponent& mesh = scene.GetComponent<MeshRendererComponent>(e);

    std::string s1("mesh");
    std::string s2("material");
    ImGui::DrawAsset<Mesh>(s1, mesh.mesh);
    ImGui::DrawAsset<Material>(s2, mesh.material);
}

}