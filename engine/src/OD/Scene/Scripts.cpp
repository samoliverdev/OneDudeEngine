#include "Scripts.h"
#include <functional>
#include <string>

namespace OD{

void ScriptComponent::Serialize(YAML::Emitter& out, Entity& e){
    auto& component = e.GetComponent<ScriptComponent>();

    out << YAML::Key << "ScriptComponent";
    out << YAML::BeginMap;

    for(auto func: SceneManager::Get()._scriptsSerializer){
        if(func.second.hasComponent(e)){
            ArchiveNode s(ArchiveNode::Type::Object, func.first, nullptr);
            func.second.serialize(e, s); Assert(s.values.empty() == false);
            ArchiveNode::SaveSerializer(s, out);
        }
    }

    out << YAML::EndMap;
}

void ScriptComponent::Deserialize(YAML::Node& in, Entity& e){
    for(auto func: SceneManager::Get()._scriptsSerializer){
        auto component = in[func.first];
        if(component){
            ArchiveNode s(ArchiveNode::Type::Object, func.first, nullptr);
            func.second.serialize(e, s);
            ArchiveNode::LoadSerializer(s, component);
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

    for(auto i: SceneManager::Get()._scriptsSerializer){
        if(i.second.hasComponent(e) == false) continue;

        bool open = ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first);
        if(open){
            ArchiveNode ar(ArchiveNode::Type::Object, i.first, nullptr);
            i.second.serialize(e, ar);
            //SceneManager::DrawArchive(ar);
            ArchiveNode::DrawArchive(ar);
            
            ImGui::TreePop();
        }
    }
}
    
}