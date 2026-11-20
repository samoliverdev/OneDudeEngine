#pragma once
#include <cstdint>
#include <deque>

namespace OD::Gfx{

// Image IDs are owned by one RenderFrame and must not be cached across frames.
class ImGuiImages{
public:
    void* ColorAttachment(uint32_t framebuffer, uint32_t attachment){
        images.push_back({framebuffer, attachment});
        return &images.back();
    }

    bool Find(void* id, uint32_t& framebuffer, uint32_t& attachment) const {
        for(const auto& image : images){
            if(id != &image) continue;
            framebuffer = image.framebuffer;
            attachment = image.attachment;
            return true;
        }
        return false;
    }

    void* Texture(uint32_t texture){
        textures.push_back(texture);
        return &textures.back();
    }

    bool FindTexture(void* id, uint32_t& texture) const{
        for(const auto& entry : textures){
            if(id != &entry) continue;
            texture = entry;
            return true;
        }
        return false;
    }

    void Clear(){ images.clear(); textures.clear(); }

private:
    struct Image{ uint32_t framebuffer, attachment; };
    std::deque<Image> images; // Appending must not invalidate previously returned IDs.
    std::deque<uint32_t> textures;
};

}
