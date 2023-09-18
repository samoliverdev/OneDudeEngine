#include "Scripts.h"
#include <functional>
#include <string>

namespace OD{

void ApplySerializer(Archive& s, std::string name, YAML::Emitter& out){
    LogInfo("Script Name: %s", name.c_str());

    out << YAML::Key << name;
    out << YAML::BeginMap;
    for(auto i: s.values()){
        if(i.type == ArchiveValue::Type::Float) out << YAML::Key << i.name << YAML::Value << (*i.floatValue);
        if(i.type == ArchiveValue::Type::Int) out << YAML::Key << i.name << YAML::Value << (*i.intValue);
        if(i.type == ArchiveValue::Type::String) out << YAML::Key << i.name << YAML::Value << (*i.stringValue);
        if(i.type == ArchiveValue::Type::Vector3) out << YAML::Key << i.name << YAML::Value << (*i.vector3Value);
        if(i.type == ArchiveValue::Type::Vector4) out << YAML::Key << i.name << YAML::Value << (*i.vector4Value);
        if(i.type == ArchiveValue::Type::Quaternion) out << YAML::Key << i.name << YAML::Value << (*i.quaternionValue);
        if(i.type == ArchiveValue::Type::T) ApplySerializer(i.children[0], i.name, out);
        
        /*if(i.type == ArchiveValue::Type::TList){
            out << YAML::BeginSeq;
            for(auto j: i.children){
                ApplySerializer(j, j.name(), out);
            }
            out << YAML::EndSeq;
        }*/
    }
    out << YAML::EndMap;
}

void LoadSerializer(Archive& s, YAML::Node& node){
    for(auto i: s.values()){
        if(i.type == ArchiveValue::Type::Float){
            *i.floatValue = node[i.name].as<float>();
        }
        if(i.type == ArchiveValue::Type::Int){
            *i.intValue = node[i.name].as<int>();
        }
        if(i.type == ArchiveValue::Type::Vector3){
            *i.vector3Value = node[i.name].as<Vector3>();
        }
        if(i.type == ArchiveValue::Type::Vector4){
            *i.vector4Value = node[i.name].as<Vector4>();
        }
        if(i.type == ArchiveValue::Type::Quaternion){
            *i.quaternionValue = node[i.name].as<Quaternion>();
        }
        if(i.type == ArchiveValue::Type::String){
            *i.stringValue = node[i.name].as<std::string>();
        }
        if(i.type == ArchiveValue::Type::T){
            LoadSerializer(i.children[0], node[i.name]);
        }
    }
}

void ScriptComponent::Serialize(YAML::Emitter& out, Entity& e){
    auto& component = e.GetComponent<ScriptComponent>();

    out << YAML::Key << "ScriptComponent";
    out << YAML::BeginMap;

    for(auto i: component._instances){
        Archive s;
        i.second->Serialize(s);
        if(s.values().empty()) continue;

        ApplySerializer(s, s.name(), out);
    }
    
    out << YAML::EndMap;
}

void ScriptComponent::Deserialize(YAML::Node& in, Entity& e){
    for(auto func: SceneManager::Get()._serializeScriptFuncs){
        auto component = in[func.first];
        if(component){
            LogInfo("%s", func.first.c_str());

            Archive s;
            func.second.serialize(e, s);
            LoadSerializer(s, component);
        }
    }
}

void ScriptComponent::OnGui(Entity& e){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    
    std::hash<std::string> hasher;
    ScriptComponent& script = e.GetComponent<ScriptComponent>();

    for(auto i: SceneManager::Get()._serializeScriptFuncs){
        if(i.second.hasComponent(e) == false) continue;

        bool open = ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first.c_str());
        if(open){
            Archive ar;
            i.second.serialize(e, ar);
            SceneManager::DrawArchive(ar);
            
            ImGui::TreePop();
        }
    }
}
    
}