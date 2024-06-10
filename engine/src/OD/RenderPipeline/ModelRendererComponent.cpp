#include "ModelRendererComponent.h"
#include "StandRenderPipeline.h"
#include "OD/Utils/ImGuiCustomDraw.h"
#include "OD/Serialization/CerealImGui.h"
#include <filesystem>

namespace OD{

void ModelRendererComponent::OnGui(Entity& e){
    ModelRendererComponent& mesh = e.GetComponent<ModelRendererComponent>();

    /*cereal::ImGuiArchive uiArchive;
    uiArchive(mesh);
    return;*/

    if(ImGui::TreeNode("localTransform")){
        Transform::OnGui(mesh.localTransform);
        ImGui::TreePop();
    }

    ImGui::DrawAsset<Model>(std::string("model"), mesh.model);

    /*if(mesh.model == nullptr){
        ImGui::Text("Path: None");
    } else {
        ImGui::Text("Path: %s", mesh.model->Path().c_str());
    }*/

    int subMeshIndex = mesh.subMeshIndex;
    if(ImGui::DragInt("subMeshIndex", &subMeshIndex)){
        mesh.SetSubMeshIndex(subMeshIndex);
    }

    if(ImGui::TreeNode("materialSlots")){
        int index = 0;
        for(auto i: mesh.materialsOverride){
            std::string name = "["+std::to_string(index)+"]";
            ImGui::DrawMaterialAsset(name, mesh.materialsOverride[index], mesh.model->materials[index]);
            index += 1;
        }

        ImGui::TreePop();
    }

    ImGui::Spacing(); ImGui::Spacing(); 

    if(mesh.model != nullptr && ImGui::TreeNode("Info")){
        ImGui::Text("Mesh: %zd", mesh.model->meshs.size());
        ImGui::Text("Materials: %zd", mesh.model->materials.size());
        //ImGui::Text("Animations: %zd", mesh.model()->animations.size());
        ImGui::TreePop();
    }
}

void ModelRendererComponent::SetModel(Ref<Model> m){
    Assert(m != nullptr);
    //Assert(m->materials.size() > 0);

    model = m;
    materialsOverride.resize(model->materials.size());

    boundingVolume = Model::GenerateAABB(*model);
    boundingVolumeSphere = Model::GenerateSphereBV(*model);
}

AABB ModelRendererComponent::GetAABB(){
    return boundingVolume;
}

AABB ModelRendererComponent::GetGlobalAABB(TransformComponent& transform){
    //Get global scale thanks to our transform
    const Vector3 globalCenter{ transform.GlobalModelMatrix() * Vector4(boundingVolume.center, 1) };

    // Scaled orientation
    const Vector3 right = transform.Right() * boundingVolume.extents.x;
    const Vector3 up = transform.Up() * boundingVolume.extents.y;
    const Vector3 forward = transform.Forward() * boundingVolume.extents.z;

    const float newIi = math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, forward));

    const float newIj = math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, forward));

    const float newIk = math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, forward));

    AABB result = AABB(globalCenter, newIi, newIj, newIk);
    result.Expand(transform.Scale());
    return result;
}

AABB ModelRendererComponent::GetGlobalAABB(Transform& transform){
    //Get global scale thanks to our transform
    const Vector3 globalCenter{ transform.GetLocalModelMatrix() * Vector4(boundingVolume.center, 1) };

    // Scaled orientation
    const Vector3 right = transform.Right() * boundingVolume.extents.x;
    const Vector3 up = transform.Up() * boundingVolume.extents.y;
    const Vector3 forward = transform.Forward() * boundingVolume.extents.z;

    const float newIi = math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, forward));

    const float newIj = math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, forward));

    const float newIk = math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, forward));

    AABB result = AABB(globalCenter, newIi, newIj, newIk);
    result.Expand(transform.LocalScale());
    return result;
}

void DrawPoseNode(Skeleton& skeleton, Pose& pose, int index){
    std::string name = skeleton.GetJointName(index) + "_" + std::to_string(index);
    std::vector<int> ch = pose.GetChildrens(index);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if(ch.size() == 0) flags |= ImGuiTreeNodeFlags_Leaf;

    if(ImGui::TreeNodeEx(name.c_str(), flags)){
        for(auto i: ch){
            DrawPoseNode(skeleton, pose, i);
        }
        ImGui::TreePop();
    }
}

void SkinnedModelRendererComponent::OnGui(Entity& e){
    SkinnedModelRendererComponent& mesh = e.GetComponent<SkinnedModelRendererComponent>();

    if(ImGui::TreeNode("localTransform")){
        Transform::OnGui(mesh.localTransform);
    }

    if(mesh.model == nullptr){
        ImGui::Text("Path: None");
    } else {
        ImGui::Text("Path: %s", mesh.model->Path().c_str());
    }

    int subMeshIndex = mesh.subMeshIndex;
    if(ImGui::DragInt("subMeshIndex", &subMeshIndex)){
        mesh.SetSubMeshIndex(subMeshIndex);
    }

    if(ImGui::TreeNode("materialSlots")){
        int index = 0;
        for(auto i: mesh.materialsOverride){
            std::string name = "["+std::to_string(index)+"]";
            ImGui::DrawMaterialAsset(name, mesh.materialsOverride[index], mesh.model->materials[index]);
            index += 1;
        }

        ImGui::TreePop();
    }

    ImGui::Spacing(); ImGui::Spacing(); 

    if(mesh.model != nullptr && ImGui::TreeNode("Info")){
        ImGui::Text("Mesh: %zd", mesh.model->meshs.size());
        ImGui::Text("Materials: %zd", mesh.model->materials.size());
        //ImGui::Text("Animations: %zd", mesh.model()->animations.size());
        ImGui::TreePop();
    }

    if(mesh.model == nullptr) return;

    if(ImGui::TreeNode("Debug")){
        Pose& pose = mesh.model->skeleton.GetBindPose();
        for(int i = 0; i < pose.Size(); i++){
            if(pose.GetParent(i) >= 0) continue;

            DrawPoseNode(mesh.model->skeleton, pose, i);
        }

        ImGui::TreePop();
    }
}

}