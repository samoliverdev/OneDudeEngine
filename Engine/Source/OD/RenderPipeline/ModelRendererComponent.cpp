#include "ModelRendererComponent.h"
#include "StandRenderPipeline.h"
#include "OD/Graphics/Model.h"
#include "OD/Core/ImGui.h"
#include "OD/Serialization/CerealImGui.h"
#include <filesystem>

namespace OD{

void ModelRendererComponent::OnGui(Entity& e, Scene& scene){
    ModelRendererComponent& mesh = scene.GetComponent<ModelRendererComponent>(e);

    if(ImGui::TreeNode("localTransform")){
        Transform::OnGui(mesh.localTransform);
        ImGui::TreePop();
    }

    std::string ss("model");
    if(ImGui::DrawAsset<Model>(ss, mesh.model)){
        if(mesh.model != nullptr) mesh.boundingVolume = Model::GenerateAABB(*mesh.model);
        mesh.SetModel(mesh.model);
    }

    int subMeshIndex = mesh.subMeshIndex;
    if(ImGui::DragInt("subMeshIndex", &subMeshIndex)){
        mesh.SetSubMeshIndex(subMeshIndex);
    }

    if(ImGui::TreeNode("materialSlots")){
        int index = 0;
        for(auto i: mesh.materialsOverride){
            std::string name = "["+std::to_string(index)+"]";
            ImGui::DrawAsset<Material>(name, mesh.materialsOverride[index], mesh.model->materials[index]);
            index += 1;
        }

        ImGui::TreePop();
    }

    if(mesh.model != nullptr && mesh.model->renderTargets.size() != mesh.renderTargetVisibility.size()){
        mesh.renderTargetVisibility.resize(mesh.model->renderTargets.size());
        for(auto& i: mesh.renderTargetVisibility) i = true;
    }
    if(ImGui::TreeNode("renderTargetVisibility")){
        for(int i = 0; i < mesh.renderTargetVisibility.size(); ++i){
            bool temp = mesh.renderTargetVisibility[i];
            if(ImGui::Checkbox(
                mesh.model->skeleton.GetJointName(mesh.model->renderTargets[i].bindPoseIndex).c_str(), 
                &temp
            )){
                mesh.renderTargetVisibility[i] = temp;
            }
        }
        ImGui::TreePop();
    }

    /*ImGui::Spacing(); ImGui::Spacing(); 

    if(mesh.model != nullptr && ImGui::TreeNode("Info")){
        ImGui::Text("Mesh: %zd", mesh.model->meshs.size());
        ImGui::Text("Materials: %zd", mesh.model->materials.size());
        //ImGui::Text("Animations: %zd", mesh.model()->animations.size());
        ImGui::TreePop();
    }*/
}

void ModelRendererComponent::SetModel(Ref<Model> m){
    Assert(m != nullptr);
    Assert(this != nullptr);
    //Assert(m->materials.size() > 0);

    model = m;
    materialsOverride.resize(model->materials.size());
    renderTargetVisibility.resize(model->renderTargets.size());
    for(auto& i: renderTargetVisibility) i = true;

    boundingVolume = Model::GenerateAABB(*model);
    //boundingVolumeSphere = Model::GenerateSphereBV(*model);
}

AABB ModelRendererComponent::GetAABB() const {
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
    const Vector3 globalCenter{ transform.GetModelMatrix() * Vector4(boundingVolume.center, 1) };

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

void SkinnedModelRendererComponent::CreateSkeletonEntites(Entity& selfEntity, Scene& scene){
    skeletonEntities.clear();
    UpdatePosePalette();
    Pose& pose = model->skeleton.GetBindPose();
    TransformComponent& selfTransform = scene.GetComponent<TransformComponent>(selfEntity);

    for(int i = 0; i < pose.Size(); i++){
        Entity e = scene.AddEntity(model->skeleton.GetJointName(i));
        scene.AddComponent<GizmosDrawComponent>(e);
        skeletonEntities.push_back(e);
    }

    for(int i = 0; i < pose.Size(); i++){
        TransformComponent& trans = scene.GetComponent<TransformComponent>(skeletonEntities[i]);
        int parent = pose.GetParent(i);

        if(parent < 0){
            scene.SetParent(selfEntity, skeletonEntities[i]);
            trans.SetLocalModelMatrix(localTransform.GetModelMatrix() * skeletonTransform.GetModelMatrix() * pose.GetLocalMatrix(i));
        } else {
            scene.SetParent(skeletonEntities[parent], skeletonEntities[i]);
            trans.SetLocalModelMatrix(pose.GetLocalMatrix(i));
        }  
    }
}

void SkinnedModelRendererComponent::UpdateSkeletonEntites(Pose& pose, Scene& scene){
    for(int i = 0; i < skeletonEntities.size(); i++){
        TransformComponent& trans = scene.GetComponent<TransformComponent>(skeletonEntities[i]);
        InfoComponent& info = scene.GetComponent<InfoComponent>(skeletonEntities[i]);
        int parent = pose.GetParent(i);
        
        if(parent < 0){
            trans.SetLocalModelMatrix(localTransform.GetModelMatrix() * skeletonTransform.GetModelMatrix() * pose.GetLocalMatrix(i));
        } else {
            trans.SetLocalModelMatrix(pose.GetLocalMatrix(i));
        }  
        
        /*Transform t = Transform(animatedPose.GetLocalMatrix(i));
        trans.LocalPosition(t.LocalPosition());
        trans.LocalRotation(t.LocalRotation());*/
        
        //LogWarning("Update Bone: %s", info.name.c_str());
    }
}

void SkinnedModelRendererComponent::UpdateSkeletonEntitesIn(Pose& pose, Scene& scene){
    for(int i = 0; i < skeletonEntities.size(); i++){
        TransformComponent& trans = scene.GetComponent<TransformComponent>(skeletonEntities[i]);
        pose.SetLocalTransform(i, Transform(trans.LocalPosition(), trans.LocalRotation(), trans.LocalScale()));
    }
}

void SkinnedModelRendererComponent::OnGui(Entity& e, Scene& scene){
    SkinnedModelRendererComponent& mesh = scene.GetComponent<SkinnedModelRendererComponent>(e);

    ImGui::Checkbox("Draw", &mesh.draw);

    if(ImGui::TreeNode("localTransform")){
        Transform::OnGui(mesh.localTransform);
    }
    if(ImGui::TreeNode("skeletonTransform")){
        Transform::OnGui(mesh.skeletonTransform);
    }

    std::string ss("model");
    if(ImGui::DrawAsset<Model>(ss, mesh.model)){
        mesh.SetModel(mesh.model);
    }

    int subMeshIndex = mesh.subMeshIndex;
    if(ImGui::DragInt("subMeshIndex", &subMeshIndex)){
        mesh.SetSubMeshIndex(subMeshIndex);
    }

    if(ImGui::TreeNode("materialSlots")){
        int index = 0;
        for(auto i: mesh.materialsOverride){
            std::string name = "["+std::to_string(index)+"]";
            ImGui::DrawAsset<Material>(name, mesh.materialsOverride[index], mesh.model->materials[index]);
            index += 1;
        }

        ImGui::TreePop();
    }   

    if(mesh.model != nullptr && mesh.model->renderTargets.size() != mesh.renderTargetVisibility.size()){
        mesh.renderTargetVisibility.resize(mesh.model->renderTargets.size());
        for(auto& i: mesh.renderTargetVisibility) i = true;
    }
    if(ImGui::TreeNode("renderTargetVisibility")){
        for(int i = 0; i < mesh.renderTargetVisibility.size(); ++i){
            bool temp = mesh.renderTargetVisibility[i];
            if(ImGui::Checkbox(
                mesh.model->skeleton.GetJointName(mesh.model->renderTargets[i].bindPoseIndex).c_str(), 
                &temp
            )){
                mesh.renderTargetVisibility[i] = temp;
            }
        }
        ImGui::TreePop();
    }

    ImGui::Checkbox("updateWhenOffscreen", &mesh.updateWhenOffscreen);

    /*if(mesh.model == nullptr){
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
            //ImGui::DrawMaterialAsset(name, mesh.materialsOverride[index], mesh.model->materials[index]);
            ImGui::DrawAsset<Material>(name, mesh.materialsOverride[index], mesh.model->materials[index]);
            index += 1;
        }

        ImGui::TreePop();
    }

    ImGui::Spacing(); ImGui::Spacing();*/ 

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