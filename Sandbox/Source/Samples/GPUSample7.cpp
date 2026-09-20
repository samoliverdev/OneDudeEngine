#include "OD/pch.h"
#include "GPUSample7.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Core/Application.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"

using namespace OD;

namespace {
Gfx::Framebuffer source2D = Gfx::InvalidID;
Gfx::Framebuffer sourceCube = Gfx::InvalidID;
Gfx::Texture2D colorCopy = Gfx::InvalidID;
Gfx::Texture2D depthCopy = Gfx::InvalidID;
Gfx::Cubemap cubeCopy = Gfx::InvalidID;
}

void GPUSample7::OnInit(){
    auto* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    Assert(device);

    Gfx::FrameBufferCreateInfo framebuffer = {};
    framebuffer.width = 512;
    framebuffer.height = 512;
    framebuffer.layout.colorAttachmentsCount = 1;
    framebuffer.layout.colorAttachments[0] = {Gfx::FramebufferTextureFormat::RGBA8, 1};
    framebuffer.layout.depthAttachment = {Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT16, 1};
    framebuffer.layout.type = Gfx::FramebufferAttachmentType::TEXTURE_2D;
    source2D = device->CreateFramebuffer(framebuffer);
    framebuffer.layout.type = Gfx::FramebufferAttachmentType::CUBEMAP;
    sourceCube = device->CreateFramebuffer(framebuffer);

    Gfx::Texture2DInfo texture = {};
    texture.width = texture.height = 512;
    texture.mipmap = false;
    texture.mipLevels = 1;
    texture.format = Gfx::ImageFormat::R8G8B8A8_UNORM;
    colorCopy = device->CreateTexture2D(texture);
    texture.format = Gfx::ImageFormat::DEPTH_COMPONENT16;
    depthCopy = device->CreateTexture2D(texture);

    Gfx::CubemapInfo cube = {};
    cube.width = cube.height = 512;
    cube.mipmap = false;
    cube.mipLevels = 1;
    cube.format = Gfx::ImageFormat::R8G8B8A8_UNORM;
    cubeCopy = device->CreateCubemap(cube);
}

void GPUSample7::OnRender(float){
    auto* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    auto* commands = device->GetCommandBuffer();

    commands->BeginFramebuffer(source2D, 0, 0, true);
    commands->Viewport(0, 0, 512, 512);
    commands->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{1.0f, 0.0f, 0.0f, 1.0f}});
    commands->EndFramebuffer();

    for(uint32_t face = 0; face < 6; ++face){
        commands->BeginFramebuffer(sourceCube, face, 0, true);
        commands->Viewport(0, 0, 512, 512);
        commands->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{1.0f, 0.0f, 0.0f, 1.0f}});
        commands->EndFramebuffer();
    }

    commands->CopyTexture(source2D, 0, colorCopy);
    commands->CopyTexture(source2D, -1, depthCopy);
    commands->CopyTextureCubemap(sourceCube, 0, cubeCopy);

    commands->BeginWindowFramebuffer();
    commands->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    commands->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{1, 0.0f, 0.3f, 1.0f}});
    commands->EndFramebuffer();
}
