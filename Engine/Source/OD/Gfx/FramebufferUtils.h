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

} }
