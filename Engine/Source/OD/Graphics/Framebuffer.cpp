#include "OD/pch.h"
#include "Framebuffer.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Core/Lua.h"
#include "OD/Defines.h"
#include "SubShader.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;

Framebuffer::Framebuffer(FramebufferType inType, int width, int height, int layers){
    type = inType;
    specification.width = width;
    specification.height = height;
    if(type == FramebufferType::Shadowmap) specification.sample = layers;
    graphicsDevice->FramebufferCreate(*this);
}

Framebuffer::Framebuffer(FrameBufferSpecification inSpecification){
    //Assert(false && "Not work for now!!!");
    type = FramebufferType::Dynamic;
    specification = inSpecification;
    Invalidate();
}

Framebuffer::~Framebuffer(){
    graphicsDevice->FramebufferDestroy(*this);
}

void Framebuffer::Reload(FrameBufferSpecification inSpecification){
    specification = inSpecification;
    Invalidate();
}

void Framebuffer::Resize(int width, int height){
    if(specification.width == width && specification.height == height) return;
    specification.width = width;
    specification.height = height;
    Invalidate();
}

bool Framebuffer::IsValid(){
    return graphicsDevice->FramebufferIsValid(*this);
}

void Framebuffer::GenMipmap(){
    graphicsDevice->FramebufferGenMipmap(*this);
}

void Framebuffer::Invalidate(){
    graphicsDevice->FramebufferCreate(*this);
}

int Framebuffer::ReadPixel(int attachmentIndex, int x, int y){
    return graphicsDevice->FramebufferReadPixel(*this, attachmentIndex, x, y);
}

void* Framebuffer::ColorAttachmentId(int index){
    return graphicsDevice->FramebufferColorAttachmentId(*this, index);
}

void* Framebuffer::DepthAttachmentId(){
    return graphicsDevice->FramebufferDepthAttachmentId(*this);
}

void Framebuffer::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Framebuffer>(
        "Framebuffer",
        "Resize", &Framebuffer::Resize,
        "IsValid", &Framebuffer::IsValid,
        //"Destroy", &Framebuffer::Destroy,
        "Invalidate", &Framebuffer::Invalidate
        //"ColorAttachmentId", &Framebuffer::ColorAttachmentId,
        //"DepthAttachmentId", &Framebuffer::DepthAttachmentId
    );
}

}