#include "MeshRendererComponent.h"
#include "StandRendererSystem.h"

namespace OD{

void MeshRendererComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "MeshRendererComponent";
    out << YAML::BeginMap;

    auto& mesh = e.GetComponent<MeshRendererComponent>();
    out << YAML::Key << "modelPath" << YAML::Value << (mesh.model() == nullptr ? std::string("") : mesh.model()->path());

    out << YAML::Key << "materials" << YAML::BeginSeq;
    for(auto i: mesh.model()->materials){
        out << YAML::BeginMap;
        out << YAML::Key << "materialPath" << YAML::Value << i->path();
        out << YAML::Key << "materialShader" << YAML::Value << (i->shader == nullptr ? "" : i->shader->path());
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;


    out << YAML::EndMap;
}

void MeshRendererComponent::Deserialize(YAML::Node& in, Entity& e){
    auto& mc = e.AddOrGetComponent<MeshRendererComponent>();

    std::string modelPath = in["modelPath"].as<std::string>();
    //LogInfo("Model %s", modelPath.c_str());

    if(modelPath.empty() == false){
        Ref<Model> model = AssetManager::Get().LoadModel(modelPath);
        mc.model(model);

        int index = 0;
        for(auto i: in["materials"]){
            std::string materialPath = i["materialPath"].as<std::string>();
            std::string shaderPath = i["materialShader"].as<std::string>();

            if(materialPath.empty() == false){
                model->materials[index] = AssetManager::Get().LoadMaterial(materialPath);
            } else {
                model->materials[index]->shader = AssetManager::Get().LoadShaderFromFile(shaderPath);
            }
            index += 1;
        }
    }

    LogInfo("Model %s", mc.model()->path().c_str());
    LogInfo("Shader %s", mc.model()->materials[0]->shader->path().c_str());
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
                if(i == nullptr)
                    ImGui::Text("None");
                else 
                    ImGui::Text("Path: %s", i->path().c_str());
    
                ImGui::TreePop();
            }
            index += 1;
        }

        ImGui::TreePop();
    }

    ImGui::Spacing(); ImGui::Spacing(); 

    if(mesh.model() != nullptr && ImGui::TreeNode("Info")){
        ImGui::Text("Mesh: %zd", mesh.model()->meshs.size());
        ImGui::Text("Materials: %zd", mesh.model()->materials.size());
        ImGui::Text("Animations: %zd", mesh.model()->animations.size());
        ImGui::TreePop();
    }
}

}