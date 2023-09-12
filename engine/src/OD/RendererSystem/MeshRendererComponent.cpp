#include "MeshRendererComponent.h"

namespace OD{

void MeshRendererComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "MeshRendererComponent";
    out << YAML::BeginMap;

    auto& mesh = e.GetComponent<MeshRendererComponent>();
    out << YAML::Key << "modelPath" << YAML::Value << (mesh.mesh == nullptr ? std::string("") : mesh.mesh->path());

    out << YAML::Key << "materials" << YAML::BeginSeq;
    for(auto i: mesh.mesh->materials){
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
        mc.mesh = model;

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

    LogInfo("Model %s", mc.mesh->path().c_str());
    LogInfo("Shader %s", mc.mesh->materials[0]->shader->path().c_str());
}

void MeshRendererComponent::OnGui(Entity& e){
    
}

}