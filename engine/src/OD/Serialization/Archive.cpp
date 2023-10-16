#include "Archive.h"
#include "OD/Core/ImGui.h"

namespace OD{

void ArchiveNode::SaveSerializer(ArchiveNode& s, std::string name, YAML::Emitter& out){
    out << YAML::Key << name;
    out << YAML::BeginMap;
    for(auto i: s.values){
        if(i.second.type == ArchiveNode::Type::Float) out << YAML::Key << i.second.name << YAML::Value << (i.second.floatValue);
        if(i.second.type == ArchiveNode::Type::Int) out << YAML::Key << i.second.name << YAML::Value << (i.second.intValue);
        if(i.second.type == ArchiveNode::Type::String) out << YAML::Key << i.second.name << YAML::Value << (i.second.stringValue);
        if(i.second.type == ArchiveNode::Type::Vector3) out << YAML::Key << i.second.name << YAML::Value << (i.second.AsVector3());
        if(i.second.type == ArchiveNode::Type::Vector4) out << YAML::Key << i.second.name << YAML::Value << (i.second.AsVector4());
        if(i.second.type == ArchiveNode::Type::Quaternion) out << YAML::Key << i.second.name << YAML::Value << (i.second.AsQuaternion());
        if(i.second.type == ArchiveNode::Type::Object) SaveSerializer(i.second, i.second.name, out);

        /*if(i.second.type == ArchiveNode::Type::List){
            out << YAML::Key << i.second.name << YAML::BeginSeq;
            for(auto j: i.second.values){
                out << YAML::BeginMap;
                SaveSerializer(j.second, j.second.name, out);
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }*/
    }
    out << YAML::EndMap;
}

void ArchiveNode::LoadSerializer(ArchiveNode& s, YAML::Node& node){
    for(auto i: s.values){
        if(i.second.type == ArchiveNode::Type::Float){
            *static_cast<float*>(i.second.value) = node[i.second.name].as<float>();
            //*i.floatValue = node[i.name].as<float>();
        }
        if(i.second.type == ArchiveNode::Type::Int){
            *static_cast<int*>(i.second.value) = node[i.second.name].as<int>();
        }
        if(i.second.type == ArchiveNode::Type::Vector3){
            *static_cast<Vector3*>(i.second.value) = node[i.second.name].as<Vector3>();
        }
        if(i.second.type == ArchiveNode::Type::Vector4){
            *static_cast<Vector4*>(i.second.value) = node[i.second.name].as<Vector4>();
        }
        if(i.second.type == ArchiveNode::Type::Quaternion){
            *static_cast<Quaternion*>(i.second.value) = node[i.second.name].as<Quaternion>();
        }
        if(i.second.type == ArchiveNode::Type::String){
            *static_cast<std::string*>(i.second.value) = node[i.second.name].as<std::string>();
        }
        if(i.second.type == ArchiveNode::Type::Object){
            YAML::Node n = node[i.second.name];
            LoadSerializer(i.second, n);
        }
        
        /*if(i.second.type == ArchiveNode::Type::List){
            i.second.listFunctions.clean();
            int index = 0;
            for(auto j: node[i.second.name]){
                i.second.listFunctions.push();
                LoadSerializer(i.second.values[std::to_string(index)], j);
                index += 1;
            }
        }*/
    }
}

void ArchiveNode::DrawArchive(ArchiveNode& ar){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        //ImGuiTreeNodeFlags_DefaultOpen 
        //| ImGuiTreeNodeFlags_Framed 
            ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    
    std::hash<std::string> hasher;

    for(auto i: ar.values){
        if(i.second.type == ArchiveNode::Type::Float){
            ImGui::DragFloat(i.first.c_str(), static_cast<float*>(i.second.value));
        }

        if(i.second.type == ArchiveNode::Type::Int){
            ImGui::DragInt(i.first.c_str(), static_cast<int*>(i.second.value));
        }

        if(i.second.type == ArchiveNode::Type::String){
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), i.second.stringValue.c_str());
            if(ImGui::InputText(i.first.c_str(), buffer, sizeof(buffer))){
                //i.second.stringValue = std::string(buffer);
                *static_cast<std::string*>(i.second.value) = std::string(buffer);
            }
        }

        if(i.second.type == ArchiveNode::Type::Object){
            if(ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first.c_str())){
                DrawArchive(i.second);
                ImGui::TreePop();
            }
        }

        if(i.second.type == ArchiveNode::Type::List){
            if(ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first.c_str())){
                int index = 0;
                for(auto j: i.second.values){
                    if(ImGui::TreeNodeEx((void*)(hasher(i.first)+index), treeNodeFlags, std::to_string(index).c_str())){
                        DrawArchive(j.second);
                        ImGui::TreePop();
                    }
                    index += 1;
                }
                ImGui::TreePop();
            }
        }
    }
}

}