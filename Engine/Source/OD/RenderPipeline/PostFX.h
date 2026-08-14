#pragma once
#include "OD/Defines.h"

namespace OD{

class Framebuffer;
class RenderContext;

class OD_API PostFX{
public:
    bool enable = true;

    virtual ~PostFX(){}
    virtual void OnSetup(){}
    virtual void OnRenderImage(Ref<Framebuffer>& src, Ref<Framebuffer>& dst, RenderContext& context){}
    virtual void OnGui(){}
};

}
