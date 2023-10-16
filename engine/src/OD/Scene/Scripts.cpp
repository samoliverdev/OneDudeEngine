#include "Scripts.h"
#include <functional>
#include <string>

namespace OD{

void ScriptComponent::Serialize(YAML::Emitter& out, Entity& e){
    auto& component = e.GetComponent<ScriptComponent>();

    out << YAML::Key << "ScriptComponent";
    out << YAML::BeginMap;

    for(auto i: component._instances){
        ArchiveNode s(ArchiveNode::Type::Object, "", nullptr, false);
        i.second->Serialize(s);
        if(s.values.empty()) continue;

        //ApplySerializer(s, s.name(), out);
        ArchiveNode::SaveSerializer(s, s.name, out);
    }
    
    out << YAML::EndMap;
}

void ScriptComponent::Deserialize(YAML::Node& in, Entity& e){
    for(auto func: SceneManager::Get()._serializeScriptFuncs){
        auto component = in[func.first];
        if(component){
            LogInfo("%s", func.first.c_str());

            ArchiveNode s(ArchiveNode::Type::Object, "", nullptr, true);
            func.second.serialize(e, s);
            //LoadSerializer(s, component);
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

    for(auto i: SceneManager::Get()._serializeScriptFuncs){
        if(i.second.hasComponent(e) == false) continue;

        bool open = ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first.c_str());
        if(open){
            ArchiveNode ar(ArchiveNode::Type::Object, i.first, nullptr, false);
            i.second.serialize(e, ar);
            //SceneManager::DrawArchive(ar);
            ArchiveNode::DrawArchive(ar);
            
            ImGui::TreePop();
        }
    }
}
    
}