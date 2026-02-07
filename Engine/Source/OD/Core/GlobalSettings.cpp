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
    if(ImGui::Begin("Global Settings")){
        for(auto& [name, section] : sections){
            if(ImGui::CollapsingHeader(name.c_str()/*, ImGuiTreeNodeFlags_DefaultOpen*/)){
                if(section.drawer)
                    section.drawer();
            }
        }
    }
    ImGui::End();
}

} // namespace OD
