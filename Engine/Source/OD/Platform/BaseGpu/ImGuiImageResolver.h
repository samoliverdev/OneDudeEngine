#pragma once
#include "OD/Gfx/ImGuiImages.h"
#include <imgui/imgui.h>

namespace OD::Gfx{

// Only modify the frame-owned snapshot, never ImGui's live draw lists.
template<typename Resolve>
void ResolveImGuiImages(ImDrawData& data, const ImGuiImages& images, Resolve&& resolve){
    for(int list = 0; list < data.CmdListsCount; ++list){
        for(auto& command : data.CmdLists[list]->CmdBuffer){
            if(command.UserCallback != nullptr || command.ElemCount == 0) continue;
            uint32_t framebuffer, attachment;
            if(!images.Find(command.GetTexID(), framebuffer, attachment)) continue;
            void* native = resolve(framebuffer, attachment);
            command.TextureId = native;
            if(native == nullptr){
                command.ElemCount = 0;
                // Stock backends test clipping before binding textures, but
                // do not necessarily skip a zero element count on its own.
                command.ClipRect = ImVec4(0, 0, 0, 0);
            }
        }
    }
}

}
