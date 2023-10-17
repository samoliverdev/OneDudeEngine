#include "Archive.h"
#include "OD/Core/ImGui.h"

namespace OD{

void ArchiveNode::SaveSerializer(ArchiveNode& s, std::string name, YAML::Emitter& out){
    out << YAML::Key << name;
    out << YAML::BeginMap;
    for(auto i: s.values){
        if(i.second.type() == ArchiveNode::Type::Float) out << YAML::Key << i.second.name() << YAML::Value << *static_cast<float*>(i.second.value);
        if(i.second.type() == ArchiveNode::Type::Int) out << YAML::Key << i.second.name() << YAML::Value << *static_cast<int*>(i.second.value);
        if(i.second.type() == ArchiveNode::Type::String) out << YAML::Key << i.second.name() << YAML::Value << *static_cast<std::string*>(i.second.value);
        if(i.second.type() == ArchiveNode::Type::Vector3) out << YAML::Key << i.second.name() << YAML::Value << *static_cast<Vector3*>(i.second.value);
        if(i.second.type() == ArchiveNode::Type::Vector4) out << YAML::Key << i.second.name() << YAML::Value << *static_cast<Vector4*>(i.second.value);
        if(i.second.type() == ArchiveNode::Type::Quaternion) out << YAML::Key << i.second.name() << YAML::Value << *static_cast<Quaternion*>(i.second.value);
        if(i.second.type() == ArchiveNode::Type::Object) SaveSerializer(i.second, i.second.name(), out);

        if(i.second.type() == ArchiveNode::Type::List){
            out << YAML::Key << i.second.name() << YAML::BeginSeq;
            for(auto j: i.second.values){
                out << YAML::BeginMap;
                SaveSerializer(j.second, j.second.name(), out);
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }
    }
    out << YAML::EndMap;
}

void ArchiveNode::LoadSerializer(ArchiveNode& s, YAML::Node& node){
    LogInfo("Node Name: %s", s.name().c_str());

    for(auto i: s.values){
        if(i.second.type() == ArchiveNode::Type::Float){
            LogInfo("Float Value Name: %s", i.second.name().c_str());
            *static_cast<float*>(i.second.value) = node[i.second.name()].as<float>();
            //*i.floatValue = node[i.name].as<float>();
        }
        if(i.second.type() == ArchiveNode::Type::Int){
            LogInfo("Int Value Name: %s", i.second.name().c_str());
            *static_cast<int*>(i.second.value) = node[i.second.name()].as<int>();
        }
        if(i.second.type() == ArchiveNode::Type::Vector3){
            LogInfo("Vector3 Value Name: %s", i.second.name().c_str());
            *static_cast<Vector3*>(i.second.value) = node[i.second.name()].as<Vector3>();
        }
        if(i.second.type() == ArchiveNode::Type::Vector4){
            LogInfo("Vector4 Value Name: %s", i.second.name().c_str());
            *static_cast<Vector4*>(i.second.value) = node[i.second.name()].as<Vector4>();
        }
        if(i.second.type() == ArchiveNode::Type::Quaternion){
            LogInfo("Quaternion Value Name: %s", i.second.name().c_str());
            *static_cast<Quaternion*>(i.second.value) = node[i.second.name()].as<Quaternion>();
        }
        if(i.second.type() == ArchiveNode::Type::String){
            LogInfo("String Value Name: %s", i.second.name().c_str());
            Assert(node[i.second.name()]);
            *static_cast<std::string*>(i.second.value) = node[i.second.name()].as<std::string>();
        }
        if(i.second.type() == ArchiveNode::Type::Object){
            LogInfo("Object Value Name: %s", i.second.name().c_str());
            YAML::Node n = node[i.first];
            LoadSerializer(i.second, n);
        }
        
        if(i.second.type() == ArchiveNode::Type::List){
            LogInfo("List Value Name: %s", i.second.name().c_str());
            Assert(node[i.second.name()]);

            i.second.listFunctions.clean(i.second);
            int index = 0;
            for(auto j: node[i.second.name()]){
                std::string indexName = std::to_string(index);
                auto _j = j[indexName];

                i.second.listFunctions.push(i.second);
                LoadSerializer(i.second.values[indexName], _j);
                index += 1;
            }
        }
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
        if(i.second.type() == ArchiveNode::Type::Float){
            ImGui::DragFloat(i.first.c_str(), static_cast<float*>(i.second.value));
        }

        if(i.second.type() == ArchiveNode::Type::Int){
            ImGui::DragInt(i.first.c_str(), static_cast<int*>(i.second.value));
        }

        if(i.second.type() == ArchiveNode::Type::String){
            std::string* s = static_cast<std::string*>(i.second.value);

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), s->c_str());
            if(ImGui::InputText(i.first.c_str(), buffer, sizeof(buffer))){
                //i.second.stringValue = std::string(buffer);
                *s = std::string(buffer);
            }
        }

        if(i.second.type() == ArchiveNode::Type::Object){
            if(ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first.c_str())){
                DrawArchive(i.second);
                ImGui::TreePop();
            }
        }

        if(i.second.type() == ArchiveNode::Type::List){
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