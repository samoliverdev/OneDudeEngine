#pragma once
#include "OD/Renderer/Framebuffer.h"
#include "OD/Renderer/Camera.h"
#include "OD/Core/Transform.h"

namespace OD{

class BaseRenderPipeline: public System{
public:
    BaseRenderPipeline(Scene* inScene):System(inScene){}

    virtual void SetOverrideFrameBuffer(Framebuffer* out) = 0;
    virtual void SetOverrideCamera(Camera* cam, Transform trans) = 0;
    virtual Framebuffer* FinalColor() = 0;
};

}