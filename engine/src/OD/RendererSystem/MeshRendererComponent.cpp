#include "MeshRendererComponent.h"
#include "StandRendererSystem.h"
#include "OD/Utils/ImGuiCustomDraw.h"
#include <filesystem>

namespace OD{

void MeshRendererComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "MeshRendererComponent";
    out << YAML::BeginMap;

    auto& mesh = e.GetComponent<MeshRendererComponent>();
    out << YAML::Key << "_model" << YAML::Value << (mesh.model() == nullptr ? std::string("") : mesh.model()->path());
    out << YAML::Key << "_subMeshIndex" << YAML::Value << mesh._subMeshIndex;

    out << YAML::Key << "_materialsOverride" << YAML::Value << YAML::BeginSeq;
    for(auto i: mesh._materialsOverride){
        out << YAML::BeginMap;
        out << YAML::Key << "path" << YAML::Value << (i == nullptr ? std::string("") :  i->path());
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;


    out << YAML::EndMap;
}

void MeshRendererComponent::Deserialize(YAML::Node& in, Entity& e){
    auto& mc = e.AddOrGetComponent<MeshRendererComponent>();

    std::string modelPath = in["_model"].as<std::string>();
    //LogInfo("Model %s", modelPath.c_str());

    if(modelPath.empty() == false){
        Ref<Model> model = AssetManager::Get().LoadModel(modelPath);
        mc.model(model);
    }

    mc._subMeshIndex = in["_subMeshIndex"].as<int>();

    int index = 0;
    for(auto i: in["_materialsOverride"]){
        std::string materialPath = i["path"].as<std::string>();
        if(materialPath.empty() == false){
            mc._materialsOverride[index] = AssetManager::Get().LoadMaterial(materialPath);
        } 
        index += 1;
    }

    LogInfo("Model %s", mc.model()->path().c_str());
    LogInfo("Shader %s", mc.model()->materials[0]->shader()->path().c_str());
}

void MeshRendererComponent::OnGui(Entity& e){
    MeshRendererComponent& mesh = e.GetComponent<MeshRendererComponent>();

    if(mesh.model() == nullptr){
        ImGui::Text("Path: None");
    } else {
        ImGui::Text("Path: %s", mesh.model()->path().c_str());
    }

    int subMeshIndex = mesh.subMeshIndex();
    if(ImGui::DragInt("subMeshIndex", &subMeshIndex)){
        mesh.subMeshIndex(subMeshIndex);
    }

    if(ImGui::TreeNode("materialsOverride")){
        int index = 0;
        for(auto i: mesh._materialsOverride){
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

                ImGui::DrawMaterialAsset("override", mesh._materialsOverride[index]);
    
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

    if(mesh.model() != nullptr && ImGui::TreeNode("Info")){
        ImGui::Text("Mesh: %zd", mesh.model()->meshs.size());
        ImGui::Text("Materials: %zd", mesh.model()->materials.size());
        //ImGui::Text("Animations: %zd", mesh.model()->animations.size());
        ImGui::TreePop();
    }
}

AABB MeshRendererComponent::getGlobalAABB(TransformComponent& transform){
    //Get global scale thanks to our transform
    const Vector3 globalCenter{ transform.globalModelMatrix() * Vector4(_boundingVolume.center, 1) };

    // Scaled orientation
    const Vector3 right = transform.right() * _boundingVolume.extents.x;
    const Vector3 up = transform.up() * _boundingVolume.extents.y;
    const Vector3 forward = transform.forward() * _boundingVolume.extents.z;

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
    //result.Expand(transform.localScale());
    return result;
}

}