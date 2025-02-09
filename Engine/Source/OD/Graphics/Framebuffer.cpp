#include "Framebuffer.h"
#include "Graphics.h"
#include "OD/Core/Lua.h"
#include "OD/Defines.h"
#include "SubShader.h"

namespace OD{


Framebuffer::Framebuffer(FrameBufferSpecification _specification){
    specification = _specification;
    Invalidate();
}

Framebuffer::~Framebuffer(){
    Graphics::FramebufferDestroy(*this);
}

void Framebuffer::Reload(FrameBufferSpecification _specification){
    specification = _specification;
    Invalidate();
}

bool Framebuffer::IsValid(){
    return isComplete;
}

void Framebuffer::Resize(int width, int height){
    if(specification.width == width && specification.height == height) return;

    specification.width = width;
    specification.height = height;
    
    Invalidate();
}

void Framebuffer::Invalidate(){
    Graphics::FramebufferCreate(*this, specification);
}

/*
void Framebuffer::BindColorAttachmentTexture(Shader& shader, int index){
    Assert(colorAttachments.size() != 0);
    shader.Bind();
    glBindTexture(GL_TEXTURE_2D, colorAttachments[0]);
}
*/

unsigned int Framebuffer::ColorAttachmentId(int index){ 
    Assert(false && "Outdata");
    return 0;
    //Assert(index < colorAttachments.size());
    //return colorAttachments[index]; 
}

unsigned int Framebuffer::DepthAttachmentId(){
    Assert(false && "Outdata");
    return 0;
    //return depthAttachment;
}

int Framebuffer::ReadPixel(int attachmentIndex, int x, int y){
    return Graphics::FramebufferReadPixel(*this, attachmentIndex, x, y);
}

void Framebuffer::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Framebuffer>(
        "Framebuffer",
        "Resize", &Framebuffer::Resize,
        "IsValid", &Framebuffer::IsValid,
        //"Destroy", &Framebuffer::Destroy,
        "Invalidate", &Framebuffer::Invalidate,
        "ColorAttachmentId", &Framebuffer::ColorAttachmentId,
        "DepthAttachmentId", &Framebuffer::DepthAttachmentId
    );
}

}