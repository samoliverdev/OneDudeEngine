#include "MeshRendererComponent.h"
#include "StandRenderPipeline.h"
#include "OD/Utils/ImGuiCustomDraw.h"
#include <filesystem>

namespace OD{

void MeshRendererComponent::OnGui(Entity& e){
    MeshRendererComponent& mesh = e.GetComponent<MeshRendererComponent>();

    if(mesh.model == nullptr){
        ImGui::Text("Path: None");
    } else {
        ImGui::Text("Path: %s", mesh.model->Path().c_str());
    }

    int subMeshIndex = mesh.subMeshIndex;
    if(ImGui::DragInt("subMeshIndex", &subMeshIndex)){
        mesh.SetSubMeshIndex(subMeshIndex);
    }

    if(ImGui::TreeNode("materialsOverride")){
        int index = 0;
        for(auto i: mesh.materialsOverride){
            if(ImGui::TreeNode(std::to_string(index).c_str())){
                /*ImGui::BeginGroup();
                if(i == nullptr)
                    ImGui::Text("None");
                else 
                    ImGui::Text("Path: %s", i->path().c_str());

                ImGui::SameLine();
                if(ImGui::SmallButton("X")){
                    mesh._materialsOverride[index] = nullptr;
                }
                ImGui::EndGroup();*/

                ImGui::DrawMaterialAsset("override", mesh.materialsOverride[index]);
    
                ImGui::TreePop();
            }

            /*ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
                if(path->string().empty() == false && path->extension() == ".material"){
                    mesh._materialsOverride[index] = AssetManager::Get().LoadMaterial(path->string());
                }
            });*/

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

void MeshRendererComponent::SetModel(Ref<Model> m){
    model = m;
    materialsOverride.resize(model->materials.size());

    boundingVolume = Model::GenerateAABB(*model);
    boundingVolumeSphere = Model::GenerateSphereBV(*model);
}

AABB MeshRendererComponent::GetAABB(){
    return boundingVolume;
}

AABB MeshRendererComponent::GetGlobalAABB(TransformComponent& transform){
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
    result.Expand(transform.LocalScale() * 2.0f);
    return result;
}

void SkinnedMeshRendererComponent::OnGui(Entity& e){
    SkinnedMeshRendererComponent& mesh = e.GetComponent<SkinnedMeshRendererComponent>();

    if(mesh.model == nullptr){
        ImGui::Text("Path: None");
    } else {
        ImGui::Text("Path: %s", mesh.model->Path().c_str());
    }

    int subMeshIndex = mesh.subMeshIndex;
    if(ImGui::DragInt("subMeshIndex", &subMeshIndex)){
        mesh.SetSubMeshIndex(subMeshIndex);
    }

    if(ImGui::TreeNode("materialsOverride")){
        int index = 0;
        for(auto i: mesh.materialsOverride){
            if(ImGui::TreeNode(std::to_string(index).c_str())){
                /*ImGui::BeginGroup();
                if(i == nullptr)
                    ImGui::Text("None");
                else 
                    ImGui::Text("Path: %s", i->path().c_str());

                ImGui::SameLine();
                if(ImGui::SmallButton("X")){
                    mesh._materialsOverride[index] = nullptr;
                }
                ImGui::EndGroup();*/

                ImGui::DrawMaterialAsset("override", mesh.materialsOverride[index]);
    
                ImGui::TreePop();
            }

            /*ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
                if(path->string().empty() == false && path->extension() == ".material"){
                    mesh._materialsOverride[index] = AssetManager::Get().LoadMaterial(path->string());
                }
            });*/

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

}