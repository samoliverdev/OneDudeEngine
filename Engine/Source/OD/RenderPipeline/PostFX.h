#pragma once
#include "OD/Defines.h"

namespace OD{

class OD_API PostFX{
public:
    bool enable = true;

    virtual ~PostFX(){}
    virtual void OnSetup(){}
    virtual void OnRenderImage(class Framebuffer* src, class Framebuffer* dst, class RenderContext* context){}
    virtual void OnGui(){}
};

}
