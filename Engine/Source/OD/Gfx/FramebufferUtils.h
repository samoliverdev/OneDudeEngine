#pragma once
#include "Gfx.h"

namespace OD { namespace Gfx {

inline bool IsValidFramebufferCreateInfo(const FrameBufferCreateInfo& info){
    if(info.width == 0 || info.height == 0 || info.mipLevels == 0) return false;
    if(info.layout.samples > 1 && info.mipLevels != 1) return false;
    if(info.layout.type == FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE && info.mipLevels != 1) return false;
    uint32_t extent = info.width > info.height ? info.width : info.height;
    uint32_t maxMipLevels = 1;
    while(extent > 1){
        extent >>= 1;
        ++maxMipLevels;
    }
    return info.mipLevels <= maxMipLevels;
}

inline void RecordFramebufferInitialization(CommandBuffer& commands, Framebuffer framebuffer,
                                            const FrameBufferLayout& layout, uint32_t mipCount){
    const uint32_t layerCount = layout.type == FramebufferAttachmentType::CUBEMAP ? 6u :
        layout.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY
            ? (layout.layers > 0 ? layout.layers : 1u) : 1u;

    for(uint32_t mip = 0; mip < mipCount; ++mip){
        for(uint32_t layer = 0; layer < layerCount; ++layer){
            commands.BeginFramebuffer(framebuffer, layer, mip, true,
                ClearFlags::Color | ClearFlags::Depth, {{0.0f, 0.0f, 0.0f, 1.0f}});
            commands.EndFramebuffer();
        }
    }
}

} }
