#include "OD/pch.h"
#include "Framebuffer.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Core/Lua.h"
#include "OD/Defines.h"
#include "SubShader.h"
#include "OD/Gfx/Gfx.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;
extern Gfx::Device* gfxDevice;

struct FramebufferRenderPassData{
    std::string name;
    RenderPassInfo info;
    Gfx::FrameBufferLayout layout;
};
FramebufferRenderPassData renderPassDatas[MAX_FRAMEBUFFER_RENDER_PASSES];
int curRenderPass = 0;

Gfx::FramebufferAttachmentType GetType(OD::FramebufferAttachmentType type){
    switch(type){
        case OD::FramebufferAttachmentType::TEXTURE_2D: return Gfx::FramebufferAttachmentType::TEXTURE_2D;
        case OD::FramebufferAttachmentType::TEXTURE_2D_ARRAY: return Gfx::FramebufferAttachmentType::TEXTURE_2D_ARRAY;
        case OD::FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE: return Gfx::FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
        case OD::FramebufferAttachmentType::CUBEMAP: return Gfx::FramebufferAttachmentType::CUBEMAP;
    }

    Assert(false);
    return Gfx::FramebufferAttachmentType::TEXTURE_2D;
}

Gfx::FramebufferTextureFormat GetColorFormat(OD::FramebufferTextureFormat format){
    switch(format){
        case OD::FramebufferTextureFormat::RGB: return Gfx::FramebufferTextureFormat::RGB;
        case OD::FramebufferTextureFormat::RGBA8: return Gfx::FramebufferTextureFormat::RGBA8;
        case OD::FramebufferTextureFormat::RGBA16F: return Gfx::FramebufferTextureFormat::RGBA16F;
    }

    Assert(false);
    return Gfx::FramebufferTextureFormat::None;
}

Gfx::FramebufferDepthTextureFormat GetDepthFormat(OD::FramebufferTextureFormat format){
    switch(format){
        case OD::FramebufferTextureFormat::DEPTH24_STENCIL8: return Gfx::FramebufferDepthTextureFormat::DEPTH24_STENCIL8;
        case OD::FramebufferTextureFormat::DEPTH32F_STENCIL8: return Gfx::FramebufferDepthTextureFormat::DEPTH32F_STENCIL8;
        case OD::FramebufferTextureFormat::DEPTH_COMPONENT16: return Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT16;
        case OD::FramebufferTextureFormat::DEPTH_COMPONENT24: return Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT24;
        case OD::FramebufferTextureFormat::DEPTH_COMPONENT32: return Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT32;
        case OD::FramebufferTextureFormat::DEPTH_COMPONENT32F: return Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT32F;
    }

    Assert(false);
    return Gfx::FramebufferDepthTextureFormat::None;
}

bool FramebufferRenderPass::RegisterRenderPass(const std::string& name, RenderPassInfo& info){
    #ifdef TestNewGPU_API
    
    if(name == "DefaultWindows"){
        renderPassDatas[curRenderPass].layout = gfxDevice->GetWindowFrameBufferLayout();
    } else {
        Gfx::FrameBufferLayout layout = {};
        layout.type = GetType(info.type);

        layout.colorAttachmentsCount = info.colorAttachments.size();
        for(int i = 0; i  < info.colorAttachments.size(); i++){
            layout.colorAttachments[i].format = GetColorFormat(info.colorAttachments[i].colorFormat);
            layout.colorAttachments[i].mipLevels = info.colorAttachments[i].mipLevels;
        }
        layout.depthAttachment.format = info.createDepth == false ? Gfx::FramebufferDepthTextureFormat::None : GetDepthFormat(info.depthAttachment.colorFormat);
        layout.depthAttachment.mipLevels = info.depthAttachment.mipLevels;

        renderPassDatas[curRenderPass].layout = layout;
    }

    #endif

    renderPassDatas[curRenderPass].name = name;
    renderPassDatas[curRenderPass].info = info;
    curRenderPass += 1;

    return true;
}

int FramebufferRenderPass::GetRenderPassIndex(const std::string& name){
    for(int i = 0; i < (curRenderPass + 1); i++){
        if(renderPassDatas[i].name == name) return i;
    }
    return -1;
}

Gfx::FrameBufferLayout FramebufferRenderPass::GetRenderPassLayout(const int index){
    return renderPassDatas[index].layout;
}

Framebuffer::Framebuffer(FramebufferType inType, int width, int height, int layers){
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    type = inType;
    specification.width = width;
    specification.height = height;
    if(type == FramebufferType::Shadowmap) specification.sample = layers;
    graphicsDevice->FramebufferCreate(*this);
    #endif
}

Framebuffer::Framebuffer(FrameBufferSpecification inSpecification){
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    //Assert(false && "Not work for now!!!");
    type = FramebufferType::Dynamic;
    specification = inSpecification;
    Invalidate();
    #endif
}

Framebuffer::Framebuffer(const std::string& name, int width, int height, int layers){
    FramebufferRenderPassData* data = nullptr;

    for(int i = 0; i < (curRenderPass + 1); i++){
        if(renderPassDatas[i].name == name){
            data = &renderPassDatas[i];
            passIndex = i;
            break;
        }
    }

    Assert(data != nullptr);
    type = FramebufferType::Dynamic;
    specification.width = width;
    specification.height = height;
    specification.sample = layers;
    specification.type = data->info.type;
    specification.colorAttachments = data->info.colorAttachments;
    specification.depthAttachment = data->info.depthAttachment;
    specification.createDepth = data->info.createDepth;
    specification.swapChainTarget = data->info.swapChainTarget;

    passName = name;

    Gfx::FrameBufferCreateInfo info = {};
    info.layout = data->layout;
    info.width = width;
    info.height = height;
    framebuffer = gfxDevice->CreateFramebuffer(info);
    Assert(framebuffer != Gfx::InvalidID);
}

Framebuffer::~Framebuffer(){
    #ifdef TestNewGPU_API
    if(framebuffer != Gfx::InvalidID) gfxDevice->DestroyFramebuffer(framebuffer);
    #else
    graphicsDevice->FramebufferDestroy(*this);
    #endif
}

void Framebuffer::Reload(FrameBufferSpecification inSpecification){
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    specification = inSpecification;
    Invalidate();
    #endif
}

void Framebuffer::Resize(int width, int height){
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    if(width == 0 || height == 0) return; // avoid crash

    Assert(width != 0);
    Assert(height != 0);
    if(specification.width == width && specification.height == height) return;
    specification.width = width;
    specification.height = height;
    Invalidate();
    #endif
}

bool Framebuffer::IsValid(){
    #ifdef TestNewGPU_API
    Assert(false);
    return false;
    #else
    return graphicsDevice->FramebufferIsValid(*this);
    #endif
}

void Framebuffer::GenMipmap(){
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    graphicsDevice->FramebufferGenMipmap(*this);
    #endif
}

void Framebuffer::Invalidate(){
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    graphicsDevice->FramebufferCreate(*this);
    #endif
}

int Framebuffer::ReadPixel(int attachmentIndex, int x, int y){
    #ifdef TestNewGPU_API
    Assert(false);
    return 0;
    #else
    return graphicsDevice->FramebufferReadPixel(*this, attachmentIndex, x, y);
    #endif
}

void* Framebuffer::ColorAttachmentId(int index){
    #ifdef TestNewGPU_API
    Assert(false);
    return nullptr;
    #else
    return graphicsDevice->FramebufferColorAttachmentId(*this, index);
    #endif
}

void* Framebuffer::DepthAttachmentId(){
    #ifdef TestNewGPU_API
    Assert(false);
    return nullptr;
    #else
    return graphicsDevice->FramebufferDepthAttachmentId(*this);
    #endif
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