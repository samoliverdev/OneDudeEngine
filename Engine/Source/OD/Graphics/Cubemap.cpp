#include "OD/pch.h"
#include "Cubemap.h"
#include "SubShader.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Core/Lua.h"
#include <stb/stb_image.h>

namespace OD{

extern GraphicsDevice* graphicsDevice;
extern Gfx::Device* gfxDevice;

#ifdef TestNewGPU_API
namespace {

struct IblEnvironmentUniform{
    Matrix4 projection;
    Matrix4 view;
};

struct IblCaptureUniform{
    Matrix4 projection;
    Matrix4 view;
    float roughness = 0.0f;
    float padding[3] = {};
};

std::array<Matrix4, 6> IblCaptureViews(){
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

Matrix4 IblCaptureProjection(){
    return static_cast<Matrix4>(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
}

std::string IblLoadShader(const char* path){
    ShaderSourceData source;
    if(!ShaderLoadFile(path, source)) return {};
    return "#define Pass_0\n" + source.baseSource;
}

Gfx::Buffer IblCreateCube(Gfx::Device* device){
    static const float vertices[] = {
        -1,-1,-1, 1,-1,-1, 1,1,-1, 1,1,-1, -1,1,-1, -1,-1,-1,
        -1,-1, 1, -1,1,1, 1,1,1, 1,1,1, 1,-1,1, -1,-1,1,
        -1,1,1, -1,1,-1, 1,1,-1, 1,1,-1, 1,1,1, -1,1,1,
        -1,-1,-1, 1,-1,-1, 1,-1,1, 1,-1,1, -1,-1,1, -1,-1,-1,
        1,-1,-1, 1,1,-1, 1,1,1, 1,1,1, 1,-1,1, 1,-1,-1,
        -1,-1,-1, -1,-1,1, -1,1,1, -1,1,1, -1,1,-1, -1,-1,-1
    };
    const auto buffer = device->CreateBuffer(sizeof(vertices), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    device->UpdatedBuffer(buffer, vertices, sizeof(vertices));
    return buffer;
}

Gfx::FrameBufferLayout IblFramebufferLayout(uint32_t mipLevels){
    Gfx::FrameBufferLayout layout{};
    layout.type = Gfx::FramebufferAttachmentType::CUBEMAP;
    layout.colorAttachments[0].format = Gfx::FramebufferTextureFormat::RGBA16F;
    layout.colorAttachments[0].mipLevels = static_cast<uint8_t>(mipLevels);
    layout.colorAttachmentsCount = 1;
    layout.depthAttachment.format = Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT16;
    layout.depthAttachment.mipLevels = static_cast<uint8_t>(mipLevels);
    return layout;
}

Gfx::Framebuffer IblCreateFramebuffer(Gfx::Device* device, uint32_t size, uint32_t mipLevels){
    Gfx::FrameBufferCreateInfo info{};
    info.width = size;
    info.height = size;
    info.layout = IblFramebufferLayout(mipLevels);
    return device->CreateFramebuffer(info);
}

Gfx::BindGroupLayout IblCreateLayout(Gfx::Device* device, Gfx::BindingType textureType, uint32_t uniformSize){
    Gfx::BindGroupLayoutInfo info{};
    info.entries[0] = {0, Gfx::BindingType::UniformBuffer, uniformSize, false};
    info.entries[1] = {1, textureType, 0, false};
    info.entriesCount = 2;
    return device->CreateBindGroupLayout(info);
}

Gfx::BindGroup IblCreateTextureGroup(Gfx::Device* device, Gfx::BindGroupLayout layout,
                                     Gfx::Buffer uniforms, Gfx::Texture2D texture, uint32_t uniformSize){
    Gfx::BindGroupInfo info{};
    info.layout = layout;
    info.entries[0] = {0, uniforms, 0, uniformSize, false};
    info.entries[1].binding = 1;
    info.entries[1].texture = texture;
    info.entriesCount = 2;
    return device->CreateBindGroup(info);
}

Gfx::BindGroup IblCreateCubemapGroup(Gfx::Device* device, Gfx::BindGroupLayout layout,
                                     Gfx::Buffer uniforms, Gfx::Cubemap texture){
    Gfx::BindGroupInfo info{};
    info.layout = layout;
    info.entries[0] = {0, uniforms, 0, sizeof(IblCaptureUniform), false};
    info.entries[1].binding = 1;
    info.entries[1].cubemap = texture;
    info.entriesCount = 2;
    return device->CreateBindGroup(info);
}

Gfx::Pipeline IblCreatePipeline(Gfx::Device* device, const std::string& source,
                                Gfx::BindGroupLayout layout, const Gfx::FrameBufferLayout& framebufferLayout){
    Gfx::PipelineInfo info{};
    info.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    info.vertexLayout.attributeCount = 1;
    info.vertexLayout.buffers[0] = {sizeof(float) * 3, Gfx::VertexInputRate::Vertex};
    info.vertexLayout.bufferCount = 1;
    info.bindGroupLayouts[0] = layout;
    info.bindGroupLayoutCount = 1;
    info.framebufferLayout = framebufferLayout;
    info.cullFace = Gfx::CullFace::NONE;
    return device->CreatePipeline(source.c_str(), info);
}

}
#endif

Cubemap::Cubemap(){
    //LogInfo("OnCreation");
}

Cubemap::~Cubemap(){
    //LogInfo("OnDestroy: {}", path);
    #ifdef TestNewGPU_API
    if(tex != Gfx::InvalidID) gfxDevice->DestroyCubemap(tex);
    #endif
}

Ref<Cubemap> Cubemap::CreateFromFile(const char* right, const char* left, const char* top, const char* bottom, const char* front, const char* back){
    #ifdef TestNewGPU_API
    constexpr size_t CubemapFaceCount = 6;
    const char* faces[CubemapFaceCount] = {right, left, top, bottom, front, back};
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels;

    stbi_set_flip_vertically_on_load(0);

    for(size_t face = 0; face < CubemapFaceCount; ++face){
        int faceWidth = 0;
        int faceHeight = 0;
        int channels = 0;
        unsigned char* data = stbi_load(faces[face], &faceWidth, &faceHeight, &channels, 4);
        if(data == nullptr){
            LogError("Cubemap tex failed to load at path: {}", faces[face]);
            return nullptr;
        }

        if(face == 0){
            width = faceWidth;
            height = faceHeight;
            pixels.resize(static_cast<size_t>(width) * height * 4 * CubemapFaceCount);
        } else if(faceWidth != width || faceHeight != height){
            LogError("Cubemap face dimensions do not match at path: {}", faces[face]);
            stbi_image_free(data);
            return nullptr;
        }

        const size_t faceSize = static_cast<size_t>(width) * height * 4;
        std::memcpy(pixels.data() + face * faceSize, data, faceSize);
        stbi_image_free(data);
    }

    Ref<Cubemap> out = CreateRef<Cubemap>();
    Gfx::CubemapInfo info = {};
    info.width = static_cast<uint32_t>(width);
    info.height = static_cast<uint32_t>(height);
    info.format = Gfx::ImageFormat::R8G8B8A8_UNORM;
    info.filter = Gfx::TextureFilter::Linear;
    info.wrapping = Gfx::TextureWrapping::ClampToEdge;
    info.mipmap = true;

    out->tex = gfxDevice->CreateCubemap(info);
    if(out->tex == Gfx::InvalidID) return nullptr;

    gfxDevice->UploadCubemap(out->tex, pixels.data(), pixels.size());
    out->mipmap = info.mipmap;
    out->ramUsage = pixels.size();
    out->vramUsage = pixels.size();
    return out;
    #else
    bool mipmap = true;
    Ref<Cubemap> out = CreateRef<Cubemap>();
    if(graphicsDevice->CubemapCreateFromFile(*out, right, left, top, bottom, front, back) == false){
        return nullptr;
    }
    return out;
    #endif
}

Ref<Cubemap> Cubemap::CreateFromFileHDR(const char* hdri){
#ifdef TestNewGPU_API
    if(gfxDevice == nullptr) return nullptr;

    int width = 0;
    int height = 0;
    int channels = 0;
    // The equirectangular conversion shader follows the LearnOpenGL
    // convention, which expects the HDR image to be loaded bottom-up.
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(hdri, &width, &height, &channels, 4);
    if(data == nullptr){
        LogError("Failed to load HDR image: {}", hdri);
        return nullptr;
    }

    Gfx::Texture2DInfo sourceInfo{};
    sourceInfo.width = static_cast<uint32_t>(width);
    sourceInfo.height = static_cast<uint32_t>(height);
    sourceInfo.format = Gfx::ImageFormat::RGBA32F;
    sourceInfo.filter = Gfx::TextureFilter::Linear;
    sourceInfo.wrapping = Gfx::TextureWrapping::ClampToEdge;
    sourceInfo.mipmap = false;
    sourceInfo.mipLevels = 1;
    const auto sourceTexture = gfxDevice->CreateTexture2D(sourceInfo);
    gfxDevice->UploadTexture2D(sourceTexture, data, static_cast<size_t>(width) * static_cast<size_t>(height) * 4 * sizeof(float));
    stbi_image_free(data);
    if(sourceTexture == Gfx::InvalidID) return nullptr;

    constexpr uint32_t size = 512;
    constexpr uint32_t mipLevels = 10;
    Gfx::CubemapInfo destinationInfo{};
    destinationInfo.width = size;
    destinationInfo.height = size;
    destinationInfo.format = Gfx::ImageFormat::RGBA16F;
    destinationInfo.filter = Gfx::TextureFilter::Linear;
    destinationInfo.wrapping = Gfx::TextureWrapping::ClampToEdge;
    destinationInfo.mipmap = true;
    destinationInfo.mipLevels = mipLevels;

    Ref<Cubemap> out = CreateRef<Cubemap>();
    out->tex = gfxDevice->CreateCubemap(destinationInfo);
    if(out->tex == Gfx::InvalidID) return nullptr;
    out->mipmap = true;

    const auto framebuffer = IblCreateFramebuffer(gfxDevice, size, mipLevels);
    const auto cube = IblCreateCube(gfxDevice);
    const auto layout = IblCreateLayout(gfxDevice, Gfx::BindingType::Texture2D, sizeof(IblEnvironmentUniform));
    const auto source = IblLoadShader("Engine/Shaders/EquirectangularToCubemap.glsl");
    if(source.empty()) return nullptr;
    const auto pipeline = IblCreatePipeline(gfxDevice, source, layout, IblFramebufferLayout(mipLevels));
    const auto projection = IblCaptureProjection();
    const auto views = IblCaptureViews();
    auto* commands = gfxDevice->GetCommandBuffer();

    for(uint32_t mip = 0; mip < mipLevels; ++mip){
        const uint32_t mipSize = std::max(1u, size >> mip);
        for(uint32_t face = 0; face < 6; ++face){
            IblEnvironmentUniform uniformData{};
            uniformData.projection = projection;
            uniformData.view = views[face];
            const auto uniform = gfxDevice->CreateBuffer(sizeof(uniformData), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
            gfxDevice->UpdatedBuffer(uniform, &uniformData, sizeof(uniformData));
            const auto group = IblCreateTextureGroup(gfxDevice, layout, uniform, sourceTexture, sizeof(uniformData));

            commands->BeginFramebuffer(framebuffer, face, mip, true);
            commands->Viewport(0, 0, mipSize, mipSize);
            commands->SetPipeline(pipeline);
            commands->SetBindGroup(0, group);
            commands->SetVertexBuffer(0, cube);
            commands->Draw(36);
            commands->EndFramebuffer();
        }
    }
    commands->CopyTextureCubemap(framebuffer, 0, out->tex);
    return out;
#else
    return graphicsDevice->CubemapCreateFromFileHDR(hdri);
#endif

    Assert(false && "Not Work for now");
    return nullptr;
}

Ref<Cubemap> Cubemap::CreateIrradianceMapFromCubeMap(const Ref<Cubemap>& cubemap){
#ifdef TestNewGPU_API
    if(gfxDevice == nullptr || cubemap == nullptr || cubemap->tex == Gfx::InvalidID) return nullptr;

    constexpr uint32_t size = 128;
    constexpr uint32_t mipLevels = 1;
    Gfx::CubemapInfo destinationInfo{};
    destinationInfo.width = size;
    destinationInfo.height = size;
    destinationInfo.format = Gfx::ImageFormat::RGBA16F;
    destinationInfo.filter = Gfx::TextureFilter::Linear;
    destinationInfo.wrapping = Gfx::TextureWrapping::ClampToEdge;
    destinationInfo.mipmap = false;
    destinationInfo.mipLevels = mipLevels;

    Ref<Cubemap> out = CreateRef<Cubemap>();
    out->tex = gfxDevice->CreateCubemap(destinationInfo);
    if(out->tex == Gfx::InvalidID) return nullptr;
    out->mipmap = false;

    const auto framebuffer = IblCreateFramebuffer(gfxDevice, size, mipLevels);
    const auto cube = IblCreateCube(gfxDevice);
    const auto layout = IblCreateLayout(gfxDevice, Gfx::BindingType::TextureCube, sizeof(IblCaptureUniform));
    const auto source = IblLoadShader("Engine/Shaders/IrradianceConvolution.glsl");
    if(source.empty()) return nullptr;
    const auto pipeline = IblCreatePipeline(gfxDevice, source, layout, IblFramebufferLayout(mipLevels));
    const auto projection = IblCaptureProjection();
    const auto views = IblCaptureViews();
    auto* commands = gfxDevice->GetCommandBuffer();

    for(uint32_t face = 0; face < 6; ++face){
        IblCaptureUniform uniformData{};
        uniformData.projection = projection;
        uniformData.view = views[face];
        const auto uniform = gfxDevice->CreateBuffer(sizeof(uniformData), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
        gfxDevice->UpdatedBuffer(uniform, &uniformData, sizeof(uniformData));
        const auto group = IblCreateCubemapGroup(gfxDevice, layout, uniform, cubemap->tex);

        commands->BeginFramebuffer(framebuffer, face, 0, true);
        commands->Viewport(0, 0, size, size);
        commands->SetPipeline(pipeline);
        commands->SetBindGroup(0, group);
        commands->SetVertexBuffer(0, cube);
        commands->Draw(36);
        commands->EndFramebuffer();
    }
    commands->CopyTextureCubemap(framebuffer, 0, out->tex);
    return out;
#else
    return graphicsDevice->CubemapCreateIrradianceMapFromCubeMap(cubemap);
#endif

    Assert(false && "Not Work for now");
    return nullptr;
}

Ref<Cubemap> Cubemap::CreatePrefilterMapFromCubeMap(const Ref<Cubemap>& cubemap){
#ifdef TestNewGPU_API
    if(gfxDevice == nullptr || cubemap == nullptr || cubemap->tex == Gfx::InvalidID) return nullptr;

    constexpr uint32_t size = 128;
    constexpr uint32_t mipLevels = 5;
    Gfx::CubemapInfo destinationInfo{};
    destinationInfo.width = size;
    destinationInfo.height = size;
    destinationInfo.format = Gfx::ImageFormat::RGBA16F;
    destinationInfo.filter = Gfx::TextureFilter::Linear;
    destinationInfo.wrapping = Gfx::TextureWrapping::ClampToEdge;
    destinationInfo.mipmap = true;
    destinationInfo.mipLevels = mipLevels;

    Ref<Cubemap> out = CreateRef<Cubemap>();
    out->tex = gfxDevice->CreateCubemap(destinationInfo);
    if(out->tex == Gfx::InvalidID) return nullptr;
    out->mipmap = true;

    const auto framebuffer = IblCreateFramebuffer(gfxDevice, size, mipLevels);
    const auto cube = IblCreateCube(gfxDevice);
    const auto layout = IblCreateLayout(gfxDevice, Gfx::BindingType::TextureCube, sizeof(IblCaptureUniform));
    const auto source = IblLoadShader("Engine/Shaders/Prefilter.glsl");
    if(source.empty()) return nullptr;
    const auto pipeline = IblCreatePipeline(gfxDevice, source, layout, IblFramebufferLayout(mipLevels));
    const auto projection = IblCaptureProjection();
    const auto views = IblCaptureViews();
    auto* commands = gfxDevice->GetCommandBuffer();

    for(uint32_t mip = 0; mip < mipLevels; ++mip){
        const uint32_t mipSize = std::max(1u, size >> mip);
        const float roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);
        for(uint32_t face = 0; face < 6; ++face){
            IblCaptureUniform uniformData{};
            uniformData.projection = projection;
            uniformData.view = views[face];
            uniformData.roughness = roughness;
            const auto uniform = gfxDevice->CreateBuffer(sizeof(uniformData), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
            gfxDevice->UpdatedBuffer(uniform, &uniformData, sizeof(uniformData));
            const auto group = IblCreateCubemapGroup(gfxDevice, layout, uniform, cubemap->tex);

            commands->BeginFramebuffer(framebuffer, face, mip, true);
            commands->Viewport(0, 0, mipSize, mipSize);
            commands->SetPipeline(pipeline);
            commands->SetBindGroup(0, group);
            commands->SetVertexBuffer(0, cube);
            commands->Draw(36);
            commands->EndFramebuffer();
        }
    }
    commands->CopyTextureCubemap(framebuffer, 0, out->tex);
    return out;
#else
    return graphicsDevice->CubemapCreatePrefilterMapFromCubeMap(cubemap);
#endif

    Assert(false && "Not Work for now");
    return nullptr;
    /*
    //FIXME: Destrey gl objects
    Assert(Graphics::HasBegin() == false);

    // pbr: setup framebuffer
    // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glCheckError();

    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for(unsigned int i = 0; i < 6; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glCheckError();

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    Ref<SubShader> prefilterShader = SubShader::CreateFromFile("Engine/Shaders/Prefilter.glsl");
    SubShader::Bind(*prefilterShader);
    prefilterShader->SetInt("environmentMap", 0);
    prefilterShader->SetMatrix4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->renderId);
    glCheckError();

    unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for(unsigned int mip = 0; mip < maxMipLevels; ++mip){
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);
        glCheckError();

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader->SetFloat("roughness", roughness);
        for(unsigned int i = 0; i < 6; ++i){
            prefilterShader->SetMatrix4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube(cubeVAO, cubeVBO);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    Ref<Cubemap> out = CreateRef<Cubemap>();
    out->renderId = prefilterMap;
    return out;
    */
}

bool Cubemap::LoadFromFile(const std::string& path){
    this->path = path; 
    return graphicsDevice->CubemapCreateFromFileHDR(*this, path.c_str());
}

std::vector<std::string> Cubemap::GetFileAssociations(){
    return {".hdr"};
}

void Cubemap::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Cubemap>(
        "Cubemap",
        "CreateFromFile", Cubemap::CreateFromFile,
        "CreateFromFileHDR", Cubemap::CreateFromFileHDR,
        "CreateIrradianceMapFromCubeMap", Cubemap::CreateIrradianceMapFromCubeMap,
        "CreatePrefilterMapFromCubeMap", Cubemap::CreatePrefilterMapFromCubeMap
        //"Destroy", &Cubemap::Destroy,
        //"Bind", &Cubemap::Bind,
        //"IsValid", &Cubemap::IsValid
    );
}

}
