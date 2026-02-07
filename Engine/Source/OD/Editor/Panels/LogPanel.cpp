#include "OD/pch.h"
#include "LogPanel.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Log.h"

namespace OD{

LogPanel::LogPanel(){
    name = "LogPanel";
    show = true;
}

void LogPanel::OnGui(){
    /*if(ImGui::Begin("LogPanel", &show)){

        OD::Log::DrainQueue();
        for(const auto& e : OD::Log::GetEntries()){
            ImVec4 color;
            switch(e.level){
                case Log::Level::Warning: color = {1,1,0,1}; break;
                case Log::Level::Error:   color = {1,0.4f,0.4f,1}; break;
                case Log::Level::Fatal:   color = {1,0,0,1}; break;
                default:                color = {1,1,1,1}; break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(e.message.c_str());
            ImGui::PopStyleColor();
        }

    }
    ImGui::End();*/


    if(!ImGui::Begin("LogPanel", &show)){
        ImGui::End();
        return;
    }

    // 🔄 Drain async logs first
    OD::Log::DrainQueue();

    // =========================
    // Header (toolbar)
    // =========================
    if(ImGui::Button("Clear")) OD::Log::EntriesClear();

    ImGui::SameLine();

    ImGui::Checkbox("Info",    &showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warning", &showWarning);
    ImGui::SameLine();
    ImGui::Checkbox("Error",   &showError);
    ImGui::SameLine();
    ImGui::Checkbox("Fatal",   &showFatal);

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();

    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for(const auto& e : OD::Log::GetEntries()){
        bool visible = false;

        switch(e.level){
            case Log::Level::Info:    visible = showInfo;    break;
            case Log::Level::Warning: visible = showWarning; break;
            case Log::Level::Error:   visible = showError;   break;
            case Log::Level::Fatal:   visible = showFatal;   break;
        }

        if(!visible) continue;

        ImVec4 color;
        switch(e.level){
            case Log::Level::Warning: color = {1,1,0,1}; break;
            case Log::Level::Error:   color = {1,0.4f,0.4f,1}; break;
            case Log::Level::Fatal:   color = {1,0,0,1}; break;
            default:                  color = {1,1,1,1}; break;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(e.message.c_str());
        ImGui::PopStyleColor();
    }

    if(autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();


}

}