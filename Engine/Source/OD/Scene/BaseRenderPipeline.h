#pragma once
#include "Scene.h"
#include "OD/Core/Transform.h"

namespace OD{

struct Camera;
class Framebuffer;

class OD_API BaseRenderPipeline: public System{
public:
    //BaseRenderPipeline(Scene* inScene):System(inScene){}

    virtual void SetOverrideFrameBuffer(Ref<Framebuffer> out) = 0;
    virtual void SetOverrideCamera(Camera* cam, Transform trans) = 0;
    virtual Ref<Framebuffer> FinalColor() = 0;

    virtual int ReadEntityId(int x, int y){ return 0; }
};

}
