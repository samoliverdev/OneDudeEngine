#pragma once

#include "OD/Core/Math.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/SubShader.h"
#include <stb/stb_image.h>
#include <algorithm>
#include <array>
#include <string>

namespace GPUSampleIBL {

struct CaptureUniform{
    OD::Matrix4 projection;
    OD::Matrix4 view;
    float roughness = 0.0f;
    float padding[3] = {};
};

struct BrdfUniform{
    float noBug = 0.0f;
    float padding[3] = {};
};

inline std::string LoadShader(const char* path){
    OD::ShaderSourceData source;
    if(!OD::ShaderLoadFile(path, source)) return {};
    // ShaderLoadFile converts the first BeginPass/EndPass block to Pass_0.
    // Gfx compiles the same source twice with VERTEX and FRAGMENT, so select
    // that pass explicitly as well.
    return "#define Pass_0\n" + source.baseSource;
}

inline std::array<OD::Matrix4, 6> CaptureViews(){
    using namespace OD;
    const Vector3 origin(0.0f);
    return {
        static_cast<Matrix4>(glm::lookAt(origin, Vector3( 1,  0,  0), Vector3(0, -1,  0))),
        static_cast<Matrix4>(glm::lookAt(origin, Vector3(-1,  0,  0), Vector3(0, -1,  0))),
        static_cast<Matrix4>(glm::lookAt(origin, Vector3( 0,  1,  0), Vector3(0,  0,  1))),
        static_cast<Matrix4>(glm::lookAt(origin, Vector3( 0, -1,  0), Vector3(0,  0, -1))),
        static_cast<Matrix4>(glm::lookAt(origin, Vector3( 0,  0,  1), Vector3(0, -1,  0))),
        static_cast<Matrix4>(glm::lookAt(origin, Vector3( 0,  0, -1), Vector3(0, -1,  0)))
    };
}

inline OD::Matrix4 CaptureProjection(){
    return static_cast<OD::Matrix4>(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
}

inline OD::Gfx::Buffer CreateCube(OD::Gfx::Device* device){
    static const float vertices[] = {
        -1,-1,-1, 1,-1,-1, 1,1,-1, 1,1,-1, -1,1,-1, -1,-1,-1,
        -1,-1, 1, -1,1,1, 1,1,1, 1,1,1, 1,-1,1, -1,-1,1,
        -1,1,1, -1,1,-1, 1,1,-1, 1,1,-1, 1,1,1, -1,1,1,
        -1,-1,-1, 1,-1,-1, 1,-1,1, 1,-1,1, -1,-1,1, -1,-1,-1,
        1,-1,-1, 1,1,-1, 1,1,1, 1,1,1, 1,-1,1, 1,-1,-1,
        -1,-1,-1, -1,-1,1, -1,1,1, -1,1,1, -1,1,-1, -1,-1,-1
    };
    auto buffer = device->CreateBuffer(sizeof(vertices), OD::Gfx::BufferUsage::Vertex, OD::Gfx::BufferMemory::GPUOnly);
    device->UpdatedBuffer(buffer, vertices, sizeof(vertices));
    return buffer;
}

inline OD::Gfx::Pipeline CreateCapturePipeline(OD::Gfx::Device* device, const std::string& source,
                                                OD::Gfx::BindGroupLayout bindGroupLayout,
                                                const OD::Gfx::FrameBufferLayout& framebufferLayout){
    OD::Gfx::PipelineInfo info{};
    info.vertexLayout.attributes[0] = {OD::Gfx::VertexSemantic::Position, OD::Gfx::VertexFormat::Float3, 0, 0};
    info.vertexLayout.attributeCount = 1;
    info.vertexLayout.buffers[0] = {sizeof(float) * 3, OD::Gfx::VertexInputRate::Vertex};
    info.vertexLayout.bufferCount = 1;
    info.bindGroupLayouts[0] = bindGroupLayout;
    info.bindGroupLayoutCount = 1;
    info.framebufferLayout = framebufferLayout;
    // Capture renders the cube from inside it. Back-face culling would discard
    // the faces that contain the environment projection.
    info.cullFace = OD::Gfx::CullFace::NONE;
    return device->CreatePipeline(source.c_str(), info);
}

inline OD::Gfx::BindGroupLayout CreateCaptureLayout(OD::Gfx::Device* device, OD::Gfx::BindingType textureType, uint32_t uniformSize = sizeof(CaptureUniform)){
    OD::Gfx::BindGroupLayoutInfo info{};
    info.entries[0] = {0, OD::Gfx::BindingType::UniformBuffer, uniformSize, false};
    info.entries[1] = {1, textureType, 0, false};
    info.entriesCount = 2;
    return device->CreateBindGroupLayout(info);
}

inline OD::Gfx::BindGroup CreateCaptureGroup(OD::Gfx::Device* device, OD::Gfx::BindGroupLayout layout, OD::Gfx::Buffer uniforms, OD::Gfx::Texture2D texture, uint32_t uniformSize = sizeof(CaptureUniform)){
    OD::Gfx::BindGroupInfo info{};
    info.layout = layout;
    OD::Gfx::BindingEntry bindGroupEntries[2];
    //info.entries[0] = {0, uniforms, 0, uniformSize, false};
    bindGroupEntries[0].binding = 0;
    bindGroupEntries[0].buffer = uniforms;
    bindGroupEntries[0].size = uniformSize;
    bindGroupEntries[0].dynamicOffset = false;
    bindGroupEntries[1].binding = 1;
    bindGroupEntries[1].texture = texture;
    info.entriesCount = 2;
    info.entries = bindGroupEntries;
    return device->CreateBindGroup(info);
}

inline OD::Gfx::BindGroup CreateCaptureCubeGroup(OD::Gfx::Device* device, OD::Gfx::BindGroupLayout layout, OD::Gfx::Buffer uniforms, OD::Gfx::Cubemap cubemap){
    OD::Gfx::BindGroupInfo info{};
    info.layout = layout;
    OD::Gfx::BindingEntry bindGroupEntries[2];
    //info.entries[0] = {0, uniforms, 0, sizeof(CaptureUniform), false};
    bindGroupEntries[0].binding = 0;
    bindGroupEntries[0].buffer = uniforms;
    bindGroupEntries[0].size = sizeof(CaptureUniform);
    bindGroupEntries[0].dynamicOffset = false;
    bindGroupEntries[1].binding = 1;
    bindGroupEntries[1].cubemap = cubemap;
    info.entriesCount = 2;
    info.entries = bindGroupEntries;
    return device->CreateBindGroup(info);
}

inline OD::Gfx::Framebuffer CreateCubemapFramebuffer(OD::Gfx::Device* device, uint32_t size, uint32_t mipLevels){
    OD::Gfx::FrameBufferCreateInfo info{};
    info.width = size;
    info.height = size;
    info.layout.type = OD::Gfx::FramebufferAttachmentType::CUBEMAP;
    info.layout.colorAttachments[0].format = OD::Gfx::FramebufferTextureFormat::RGBA16F;
    info.layout.colorAttachments[0].mipLevels = static_cast<uint8_t>(mipLevels);
    info.layout.colorAttachmentsCount = 1;
    info.layout.depthAttachment.format = OD::Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT16;
    info.layout.depthAttachment.mipLevels = static_cast<uint8_t>(mipLevels);
    return device->CreateFramebuffer(info);
}

inline OD::Gfx::Framebuffer CreateTextureFramebuffer(OD::Gfx::Device* device, uint32_t width, uint32_t height){
    OD::Gfx::FrameBufferCreateInfo info{};
    info.width = width;
    info.height = height;
    info.layout.colorAttachments[0].format = OD::Gfx::FramebufferTextureFormat::RGBA16F;
    info.layout.colorAttachmentsCount = 1;
    info.layout.depthAttachment.format = OD::Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT16;
    return device->CreateFramebuffer(info);
}

inline OD::Gfx::Texture2D LoadHDR(OD::Gfx::Device* device, const char* path){
    int width = 0, height = 0, channels = 0;
    // Keep the source orientation consistent with the equirectangular
    // conversion shader and the LearnOpenGL IBL workflow.
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(path, &width, &height, &channels, 4);
    if(data == nullptr) return OD::Gfx::InvalidID;
    OD::Gfx::Texture2DInfo info{};
    info.width = static_cast<uint32_t>(width);
    info.height = static_cast<uint32_t>(height);
    // stbi_loadf returns 32-bit floats. Keep the source texture RGBA32F so
    // Vulkan's raw buffer-to-image upload does not reinterpret float32 bytes
    // as half-float channels. The capture framebuffers remain RGBA16F.
    info.format = OD::Gfx::ImageFormat::RGBA32F;
    info.filter = OD::Gfx::TextureFilter::Linear;
    info.wrapping = OD::Gfx::TextureWrapping::ClampToEdge;
    info.mipmap = false;
    auto texture = device->CreateTexture2D(info);
    device->UploadTexture2D(texture, data, static_cast<size_t>(width) * height * 4 * sizeof(float));
    stbi_image_free(data);
    return texture;
}

}
