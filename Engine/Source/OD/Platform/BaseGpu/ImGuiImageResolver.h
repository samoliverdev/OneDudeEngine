#pragma once
#include "OD/Gfx/ImGuiImages.h"
#include <imgui/imgui.h>

namespace OD::Gfx{

// Only modify the frame-owned snapshot, never ImGui's live draw lists.
template<typename Resolve, typename ResolveTexture = void* (*)(uint32_t)>
void ResolveImGuiImages(ImDrawData& data, const ImGuiImages& images, Resolve&& resolve,
                        ResolveTexture resolveTexture = [](uint32_t) -> void* { return nullptr; }){
    for(int list = 0; list < data.CmdListsCount; ++list){
        for(auto& command : data.CmdLists[list]->CmdBuffer){
            if(command.UserCallback != nullptr || command.ElemCount == 0) continue;
            uint32_t framebuffer, attachment, texture;
            void* native;
            if(images.Find(command.GetTexID(), framebuffer, attachment)){
                native = resolve(framebuffer, attachment);
            } else if(images.FindTexture(command.GetTexID(), texture)){
                native = resolveTexture(texture);
            } else {
                continue;
            }
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
