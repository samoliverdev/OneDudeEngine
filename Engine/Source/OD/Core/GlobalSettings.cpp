#include "OD/pch.h"
#include "GlobalSettings.h"
#include "ImGui.h"

namespace OD {

void GlobalSettings::Save(const std::string& path){
    std::ofstream os(path);
    cereal::JSONOutputArchive ar(os);

    for(auto& [name, section] : sections){
        if(section.saveFunc) section.saveFunc(ar);
    }
}

void GlobalSettings::Load(const std::string& path){
    /*std::ifstream stream(path);
    cereal::JSONInputArchive ar(stream);

    for(auto& [name, section] : sections){
        if(section.loadFunc) section.loadFunc(ar);
    }*/

    bool success = false;

    try{
        std::ifstream stream(path);
        if(stream.is_open()){
            cereal::JSONInputArchive ar(stream);
            /*if(name.empty()){
                ArchiveDumpNVP(ar, data);
            } else {
                ArchiveDumpNamed(ar, name, data);
            }*/

            for(auto& [name, section] : sections){
                if(section.loadFunc) section.loadFunc(ar);
            }
            success = true;
        }
    } catch(const std::exception& e){
        LogError("Failed to load archive: {}", e.what());
    }

    if(!success){
        std::ofstream os(path);
        if(!os.is_open()){
            LogError("Failed to open file for writing: {}", path.c_str());
            return;
        }

        cereal::JSONOutputArchive ar(os);
        /*if(name.empty()){
            ArchiveDumpNVP(ar, data);
        } else {
            ArchiveDumpNamed(ar, name, data);
        }*/
        for(auto& [name, section] : sections){
            if(section.loadFunc) section.saveFunc(ar);
        }
    }
}

void GlobalSettings::OnImGuiRender(){
    /*if(ImGui::Begin("Global Settings")){
        for(auto& [name, section] : sections){
            if(ImGui::CollapsingHeader(name.c_str())){
                if(section.drawer)
                    section.drawer();
            }
        }
    }
    ImGui::End();*/

    static std::string selected = "";

    if(ImGui::Begin("Global Settings")){
        if(ImGui::BeginTable("SettingsTable", 2, ImGuiTableFlags_Resizable)){
            ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // LEFT
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("##left_panel", ImVec2(0,0), true);
            for(auto& [name, section] : sections){
                if(ImGui::Selectable(name.c_str(), selected == name)){
                    selected = name;
                }
            }
            ImGui::EndChild();

            // RIGHT
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("##right_panel", ImVec2(0,0), true);

            if(!selected.empty()){
                auto it = sections.find(selected);
                if(it != sections.end() && it->second.drawer){
                    it->second.drawer();
                }
            }

            ImGui::EndChild();

            ImGui::EndTable();
        }
    }
    ImGui::End();   
}

} // namespace OD
