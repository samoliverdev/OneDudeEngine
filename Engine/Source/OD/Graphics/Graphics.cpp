#include "OD/pch.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "Font.h"
#include "Mesh.h"
#include "Model.h"
#include "SubShader.h"
#include "Cubemap.h"
#include "Texture.h"
#include "InstancingBuffer.h"
#include "UniformBuffer.h"
#include "Common.h"
#include "OD/Defines.h"
#include "OD/Core/Lua.h"
#include "OD/Gfx/Gfx.h"

//#define ENGINE_RESOURCE_PATH "res/Engine/"

#include "OD/Platform/Headless/HeadlessGraphicsDevice.h"

#include "OD/Platform/OpenGL/OpenGLGraphicsDevice.h"
#if defined(WEBGPU_SUPPORT)
#include "OD/Platform/WebGPU/WebGPUGraphicsDevice.h"
#endif

#include "OD/Platform/OpenglGPU/OpenglGPU.h"
#include "OD/Platform/VulkangGPU/VulkanGPU.h"

namespace OD{

void GraphicsModuleInit(){
    //TODO: Make a function what will auto do this by Asset
    ResourceTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Texture2D>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Texture2D>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Texture2D>(".texturebin", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Texture2D>(path); });

    ResourceTypesDB::Get().RegisterAssetType<Cubemap>(".hdr", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Cubemap>(path); });

    ResourceTypesDB::Get().RegisterAssetType<Material>(".material", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Material>(path); });
    
    ResourceTypesDB::Get().RegisterAssetType<Model>(".model", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Model>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Model>(".obj", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Model>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Model>(".glb", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Model>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Model>(".gltf", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Model>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Model>(".fbx", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Model>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Model>(".dae", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Model>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Model>(".modelbin", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Model>(path); });

    ResourceTypesDB::Get().RegisterAssetType<Shader>(".glsl", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Shader>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Shader>(".shader", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Shader>(path); });
    ResourceTypesDB::Get().RegisterAssetType<Shader>(".shaderbin", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Shader>(path); });

    ResourceTypesDB::Get().RegisterAssetType<Mesh>(".bin", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Mesh>(path); });

    ResourceTypesDB::Get().RegisterAssetType<Font>(".ttf", [](const std::string& path){ return ResourceManager::Get().LoadByPath<Font>(path); });

    LuaBindsDB::Get().RegisterLuaBind<Camera>();
    LuaBindsDB::Get().RegisterLuaBind<Cubemap>();
    LuaBindsDB::Get().RegisterLuaBind<Font>();
    LuaBindsDB::Get().RegisterLuaBind<Framebuffer>();
    LuaBindsDB::Get().RegisterLuaBind<Graphics>();
    LuaBindsDB::Get().RegisterLuaBind<Material>();
    LuaBindsDB::Get().RegisterLuaBind<Texture2D>();
}

std::vector<std::function<GraphicsDevice*()>> supportedGraphicsDevices = {
    [](){ return new HeadlessGraphicsDevice(); },

    #if defined(__EMSCRIPTEN__)
        #if defined(OPENGL_SUPPORT) 
        [](){ return new OpenGLGraphicsDevice(); },
        #endif
    #else
        #if defined(OPENGL_SUPPORT) 
        [](){ return new OpenGLGraphicsDevice(); },
        #endif
        #if defined(WEBGPU_SUPPORT)
        [](){ return new WebGPUGraphicsDevice(); },
        #endif
    #endif
};

constexpr int MaxInstancesPerDraw = 1000;
constexpr int MaxBonesPerDraw = 120;

int curGraphicsDevice = 1;
GraphicsDevice* graphicsDevice = nullptr;
Gfx::Device* gfxDevice = nullptr;

Gfx::BindGroupLayout emptyLayout;
Gfx::BindGroupLayout camGroupLayout;
Gfx::BindGroupLayout drawDrawMeshGroupLayout;
Gfx::BindGroupLayout drawMeshSkinnedGroupLayout;

Gfx::BindGroup emptyBindGroup;
Gfx::BindGroup camBindGroup;
Gfx::BindGroup emptyModelBindGroup;

Gfx::Buffer camBuffer;
Gfx::Buffer emptyModelBuffer;
Gfx::Buffer emptyInstacingVbo;

std::string curFramebufferRenderPassName;
int curFramebufferRenderPassIndex = -1;

Gfx::BindGroup curCameraBindGroup;

constexpr uint32_t MaxCachedVertexBufferSlots = 16;
Gfx::Pipeline cachedPipeline = Gfx::InvalidID;
Gfx::Buffer cachedVertexBuffers[MaxCachedVertexBufferSlots];
Gfx::Buffer cachedIndexBuffer = Gfx::InvalidID;
Gfx::BindGroup cachedBindGroups[4];

inline void ResetBindingCache(){
    cachedPipeline = Gfx::InvalidID;
    cachedIndexBuffer = Gfx::InvalidID;

    for(auto& buffer : cachedVertexBuffers) buffer = Gfx::InvalidID;
    for(auto& bindGroup : cachedBindGroups) bindGroup = Gfx::InvalidID;
}

inline void SetPipelineCached(Gfx::CommandBuffer* cmd, Gfx::Pipeline pipeline){
    if(cachedPipeline == pipeline) return;

    cmd->SetPipeline(pipeline);
    cachedPipeline = pipeline;

    // OpenGL configures vertex attributes against the active pipeline, so all
    // vertex bindings must be reapplied after a pipeline change.
    cachedIndexBuffer = Gfx::InvalidID;
    for(auto& buffer : cachedVertexBuffers) buffer = Gfx::InvalidID;
    for(auto& bindGroup : cachedBindGroups) bindGroup = Gfx::InvalidID;
}

inline void SetBindGroupCached(Gfx::CommandBuffer* cmd, uint8_t slot, Gfx::BindGroup bindGroup){
    Assert(slot < 4);
    if(cachedBindGroups[slot] == bindGroup) return;

    cmd->SetBindGroup(slot, bindGroup);
    cachedBindGroups[slot] = bindGroup;
}

inline void SetVertexBufferCached(Gfx::CommandBuffer* cmd, uint32_t slot, Gfx::Buffer buffer){
    Assert(slot < MaxCachedVertexBufferSlots);
    if(cachedVertexBuffers[slot] == buffer) return;

    cmd->SetVertexBuffer(slot, buffer);
    cachedVertexBuffers[slot] = buffer;
}

inline void SetIndexBufferCached(Gfx::CommandBuffer* cmd, Gfx::Buffer buffer){
    if(cachedIndexBuffer == buffer) return;

    cmd->SetIndexBuffer(buffer);
    cachedIndexBuffer = buffer;
}

Gfx::Texture2D defaultTex;
Gfx::Cubemap defaultCubemap;
Gfx::Buffer defaultBuffer;
#ifdef TestNewGPU_API
Ref<Mesh> fullScreenQuad;
#endif

struct UniformBufferPool{
    Gfx::BindGroupLayout layout;
    std::vector<Gfx::Buffer> buffers;
    uint32_t curIndex = 0;

    inline Gfx::Buffer GetBuffer(Gfx::Device& device, void* data, size_t size){
        if(buffers.size() <= curIndex){
            auto buffer = device.CreateBuffer(size, Gfx::BufferUsage::Uniform, Gfx::BufferMemory::CPUToGPU);
            Assert(buffer != Gfx::InvalidID);
            buffers.push_back(buffer);
        }

        auto buffer = buffers[curIndex];
        device.UpdatedBuffer(buffer, data, size);
        curIndex += 1;
        return buffer;
    }

    inline Gfx::BindGroup GetBindGroup(Gfx::Device& device, void* data, size_t size){
        auto buffer = GetBuffer(device, data, size);

        Gfx::BindGroupInfo bindGroupInfo = {};
        bindGroupInfo.layout = layout;
        Gfx::BindingEntry bindGroupEntries[1];
        bindGroupEntries[0].binding = 0;
        bindGroupEntries[0].buffer = buffer;
        bindGroupEntries[0].size = size;
        bindGroupEntries[0].dynamicOffset = false;
        bindGroupInfo.entriesCount = 1;
        bindGroupInfo.entries = bindGroupEntries;
        auto bindGroup = device.CreateFrameBindGroup(bindGroupInfo);
        return bindGroup;
    }
};

struct InstancingBufferPool{
    std::vector<Gfx::Buffer> buffers;
    uint32_t curIndex = 0;

    inline Gfx::Buffer GetBuffer(Gfx::Device& device, void* data, size_t size){
        Assert(size <= (sizeof(Matrix4) * MaxInstancesPerDraw));

        if(buffers.size() <= curIndex){
            // Instance transforms are rewritten every frame. Keep this test path
            // host-visible so UpdatedBuffer does not allocate a staging buffer
            // and submit an explicit upload for every instanced draw.
            auto buffer = device.CreateBuffer(sizeof(Matrix4) * MaxInstancesPerDraw, Gfx::BufferUsage::Vertex, Gfx::BufferMemory::CPUToGPU);
            Assert(buffer != Gfx::InvalidID);
            buffers.push_back(buffer);
        }

        auto out = buffers[curIndex];
        device.UpdatedBuffer(out, data, size);

        curIndex += 1;
        return out;
    }
};

UniformBufferPool camDataPool;
UniformBufferPool drawMeshPool;
UniformBufferPool drawMeshSkinnedPool;
InstancingBufferPool drawMeshInstancingPool;

Material* curMat = nullptr;
SubShader* curShader = nullptr;
Gfx::BindGroup curBindGroup;

struct CameraData{
    Matrix4 projection = Matrix4Identity;
    Matrix4 view = Matrix4Identity;
    Matrix4 invProjection = Matrix4Identity;
    Matrix4 inView = Matrix4Identity;
};

GraphicsDevice* Graphics::GetGraphicsDevice(){
    return graphicsDevice;
}

void Graphics::SelectGraphicsDevice(){
    #ifdef TestNewGPU_API
    graphicsDevice = new Gfx::VulkanGPUDevice(Gfx::VulkanPresentMode::Immediate);
    //graphicsDevice = new Gfx::OpenglGPUDevice();
    gfxDevice = dynamic_cast<Gfx::Device*>(graphicsDevice);
    return;
    #endif
    
    if(curGraphicsDevice >= supportedGraphicsDevices.size()){
        curGraphicsDevice = supportedGraphicsDevices.size() - 1;
    }  

    Assert(curGraphicsDevice >= 0);
    Assert(curGraphicsDevice < supportedGraphicsDevices.size());
    Assert(supportedGraphicsDevices.size() < 5);

    graphicsDevice = supportedGraphicsDevices[curGraphicsDevice]();

    /*#if defined(WEBGPU_SUPPORT)
    graphicsDevice = new WebGPUGraphicsDevice();
    #elif defined(OPENGL_SUPPORT) 
    graphicsDevice = new OpenGLGraphicsDevice();
    #endif*/
}

void Graphics::Initialize(){
    graphicsDevice->Initialize();

    RenderPassInfo renderPassInfo = {};
    renderPassInfo.colorAttachments.push_back({FramebufferTextureFormat::RGBA8} );
    renderPassInfo.depthAttachment = {FramebufferTextureFormat::DEPTH24_STENCIL8};
    renderPassInfo.createDepth = true;
    FramebufferRenderPass::RegisterRenderPass("DefaultWindows", renderPassInfo);

    #ifdef TestNewGPU_API
    CameraData camData;
    Matrix4 identity = Matrix4Identity;

    camBuffer = gfxDevice->CreateBuffer(sizeof(CameraData), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    gfxDevice->UpdatedBuffer(camBuffer, &camData, sizeof(CameraData));

    emptyModelBuffer = gfxDevice->CreateBuffer(sizeof(Matrix4), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    gfxDevice->UpdatedBuffer(emptyModelBuffer, &identity, sizeof(Matrix4));

    emptyInstacingVbo = gfxDevice->CreateBuffer(sizeof(Matrix4), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    gfxDevice->UpdatedBuffer(emptyInstacingVbo, &Matrix4Identity, sizeof(Matrix4));

    Gfx::BindGroupLayoutInfo bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entriesCount = 0;
    emptyLayout = gfxDevice->CreateBindGroupLayout(bindGroupLayoutInfo);

    bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entries[0] = {0, Gfx::BindingType::UniformBuffer, sizeof(CameraData), false};
    bindGroupLayoutInfo.entriesCount = 1;
    camGroupLayout = gfxDevice->CreateBindGroupLayout(bindGroupLayoutInfo);

    bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entries[0] = {0, Gfx::BindingType::UniformBuffer, sizeof(Matrix4), false};
    bindGroupLayoutInfo.entriesCount = 1;
    drawDrawMeshGroupLayout = gfxDevice->CreateBindGroupLayout(bindGroupLayoutInfo);

    bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entries[0] = {0, Gfx::BindingType::UniformBuffer, sizeof(Matrix4), false};
    bindGroupLayoutInfo.entries[1] = {1, Gfx::BindingType::UniformBuffer, sizeof(Matrix4) * MaxBonesPerDraw, false};
    bindGroupLayoutInfo.entriesCount = 2;
    drawMeshSkinnedGroupLayout = gfxDevice->CreateBindGroupLayout(bindGroupLayoutInfo);

    std::vector<Gfx::BindingEntry> bindGroupEntries;

    Gfx::BindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = emptyLayout;
    bindGroupInfo.entriesCount = 0;
    emptyBindGroup = gfxDevice->CreateBindGroup(bindGroupInfo);

    bindGroupInfo = {};
    bindGroupInfo.layout = camGroupLayout;
    bindGroupEntries.resize(1);
    bindGroupEntries[0].binding = 0;
    bindGroupEntries[0].buffer = camBuffer;
    bindGroupEntries[0].size = sizeof(CameraData);
    bindGroupInfo.entries = bindGroupEntries.data();
    bindGroupInfo.entriesCount = 1;
    camBindGroup = gfxDevice->CreateBindGroup(bindGroupInfo);

    bindGroupInfo = {};
    bindGroupInfo.layout = drawDrawMeshGroupLayout;
    bindGroupEntries.resize(1);
    bindGroupEntries[0].binding = 0;
    bindGroupEntries[0].buffer = emptyModelBuffer;
    bindGroupEntries[0].size = sizeof(Matrix4);
    bindGroupInfo.entriesCount = 1;
    bindGroupInfo.entries = bindGroupEntries.data();
    emptyModelBindGroup = gfxDevice->CreateBindGroup(bindGroupInfo);

    drawMeshPool.layout = drawDrawMeshGroupLayout;
    camDataPool.layout = camGroupLayout;

    defaultBuffer = gfxDevice->CreateBuffer(1024, Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    Assert(defaultBuffer != Gfx::InvalidID);
    std::vector<uint8_t> bufferData(1024, 0);
    gfxDevice->UpdatedBuffer(defaultBuffer, bufferData.data(), bufferData.size());

    std::vector<uint8_t> texPixels(static_cast<size_t>(256) * 256 * 4, 255);
    Gfx::Texture2DInfo texInfo{};
    texInfo.width = 256;
    texInfo.height = 256;
    texInfo.format = Gfx::ImageFormat::R8G8B8A8_SRGB;
    texInfo.mipmap = false;
    defaultTex = gfxDevice->CreateTexture2D(texInfo);
    Assert(defaultTex != Gfx::InvalidID);
    gfxDevice->UploadTexture2D(defaultTex, texPixels.data(), texPixels.size());

    std::vector<uint8_t> pixels(static_cast<size_t>(256) * 256 * 6 * 4, 255);
    Gfx::CubemapInfo cubeInfo{};
    cubeInfo.width = 256;
    cubeInfo.height = 256;
    cubeInfo.format = Gfx::ImageFormat::R8G8B8A8_SRGB;
    cubeInfo.mipmap = false;
    defaultCubemap = gfxDevice->CreateCubemap(cubeInfo);
    Assert(defaultCubemap != Gfx::InvalidID);
    gfxDevice->UploadCubemap(defaultCubemap, pixels.data(), pixels.size());

    fullScreenQuad = Mesh::FullScreenQuad();
    #endif
}

void Graphics::Shutdown(){
    Material::CleanGlobalUniformsData();
    
#ifdef TestNewGPU_API
    fullScreenQuad.reset();
#endif
    graphicsDevice->Shutdown();
    delete graphicsDevice;
    graphicsDevice = nullptr;
}

void Graphics::CreateLuaBind(sol::state& lua){
    lua.new_enum(
        "DepthTest",
        "DISABLE", DepthTest::DISABLE,
        "LESS", DepthTest::LESS,
        "LESS_EQUAL", DepthTest::LESS_EQUAL,
        "EQUAL", DepthTest::EQUAL,
        "GREATER", DepthTest::GREATER,
        "GREATER_EQUAL", DepthTest::GREATER_EQUAL,
        "DIFFERENT", DepthTest::DIFFERENT,
        "NEVER", DepthTest::NEVER,
        "ALWAYS", DepthTest::ALWAYS
    );
    lua.new_enum(
        "CullFace",
        "NONE", CullFace::NONE,
        "BACK", CullFace::BACK,
        "FRONT", CullFace::FRONT,
        "FRONT_AND_BACK", CullFace::FRONT_AND_BACK
    );
    lua.new_enum(
        "BlendMode",
        "ZERO", BlendMode::ZERO,
        "ONE", BlendMode::ONE,
        "SRC_COLOR", BlendMode::SRC_COLOR,
        "ONE_MINUS_SRC_COLOR", BlendMode::ONE_MINUS_SRC_COLOR,
        "DST_COLOR", BlendMode::DST_COLOR,
        "ONE_MINUS_DST_COLOR", BlendMode::ONE_MINUS_DST_COLOR,
        "SRC_ALPHA", BlendMode::SRC_ALPHA,
        "ONE_MINUS_SRC_ALPHA", BlendMode::ONE_MINUS_SRC_ALPHA,
        "DST_ALPHA", BlendMode::DST_ALPHA,
        "ONE_MINUS_DST_ALPHA", BlendMode::ONE_MINUS_DST_ALPHA,
        "CONSTANT_COLOR", BlendMode::CONSTANT_COLOR,
        "ONE_MINUS_CONSTANT_COLOR", BlendMode::ONE_MINUS_CONSTANT_COLOR,
        "CONSTANT_ALPHA", BlendMode::CONSTANT_ALPHA,
        "ONE_MINUS_CONSTANT_ALPHA", BlendMode::ONE_MINUS_CONSTANT_ALPHA
    );
    /*lua.new_enum(
        "GraphicsRenderMode",
        "SHADED", Graphics::RenderMode::SHADED,
        "WIREFRAME", Graphics::RenderMode::WIREFRAME
    );*/
    lua.new_usertype<Graphics>(
        "Graphics",
        //"GetDrawCallsCount", Graphics::GetDrawCallsCount,
        //"GetVerticesCount", Graphics::GetVerticesCount,
        //"GetTrisCount", Graphics::GetTrisCount,
        "Begin", Graphics::Begin,
        "End", Graphics::End,
        "HasBegin", Graphics::HasBegin,
        "Clean", Graphics::Clean,
        "SetCamera", Graphics::SetCamera,
        "GetCamera", Graphics::GetCamera,
        //"SetProjectionViewMatrix", Graphics::SetProjectionViewMatrix,
        //"SetModelMatrix", Graphics::SetModelMatrix,
        //"DrawMeshRaw", Graphics::DrawMeshRaw,
        //"DrawMeshInstancingRaw", Graphics::DrawMeshInstancingRaw,
        //"DrawMesh", Graphics::DrawMesh,
        //"DrawMeshInstancing", Graphics::DrawMeshInstancing,
        //"DrawModel", Graphics::DrawModel,
        "AddDrawLineCommand", Graphics::AddDrawLineCommand,
        "DrawLinesComamnd", Graphics::DrawLinesComamnd,
        "DrawLine", sol::overload(
            [](Vector3 start, Vector3 end, Vector3 color, int lineWidth){ Graphics::DrawLine(start, end, color, lineWidth);},
            [](Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth){ Graphics::DrawLine(model, start, end, color, lineWidth);}
        ),
        /*"DrawText", sol::overload(
            [](Font& f, SubShader& s, std::string text, Vector3 pos, float scale){ Graphics::DrawText(f, s, text, pos, scale); },
            [](Font& f, SubShader& s, std::string text, Matrix4 model){ Graphics::DrawText(f, s, text, model); }
        ),*/
        "SetViewport", Graphics::SetViewport,
        "GetViewport", Graphics::GetViewport
        //"SetRenderMode", Graphics::SetRenderMode,
        //"SetDepthMask", Graphics::SetDepthMask,
        //"SetDepthTest", Graphics::SetDepthTest,
        //"SetCullFace", Graphics::SetCullFace,
        //"SetBlend", Graphics::SetBlend,
        //"SetBlendFunc", Graphics::SetBlendFunc,
        //"BeginFramebuffer", Graphics::BeginFramebuffer,
        //"BlitQuadPostProcessing", Graphics::BlitQuadPostProcessing,
        //"BlitFramebuffer", Graphics::BlitFramebuffer
    );
}

GraphicsStats& Graphics::GetStats(){ 
    return graphicsDevice->GetStats(); 
}

GraphicsDebug& Graphics::GetGraphicsDebug(){
    return graphicsDevice->GetGraphicsDebug(); 
}

GPUMemoryStats& Graphics::GetMemoryStats(){
    return graphicsDevice->GetMemoryStats();
}

void Graphics::Begin(){ 
    graphicsDevice->Begin(); 
}

void Graphics::End(){ 
    graphicsDevice->End(); 
}

void Graphics::_Begin(){
    graphicsDevice->_Begin(); 

    drawMeshPool.curIndex = 0;
    drawMeshSkinnedPool.curIndex = 0;
    drawMeshInstancingPool.curIndex = 0;

    curFramebufferRenderPassIndex = -1;

    curMat = nullptr;
    curShader = nullptr;
    curBindGroup = emptyBindGroup;
    ResetBindingCache();
}

void Graphics::_End(){
    graphicsDevice->_End(); 
}

bool Graphics::HasBegin(){ 
    return graphicsDevice->HasBegin(); 
}

void Graphics::SetCamera(Camera& camera){ 
    #ifdef TestNewGPU_API
    CameraData data = {camera.projection, camera.view, math::inverse(camera.projection), math::inverse(camera.view)};

    /*if(graphicsDevice->GetInfo().apiName == "Vulkan"){
        data.projection[1][1] *= -1.0f;
    }*/

    gfxDevice->UpdatedBuffer(camBuffer, &data, sizeof(CameraData));
    curCameraBindGroup = camDataPool.GetBindGroup(*gfxDevice, &data, sizeof(CameraData));
    #else
    graphicsDevice->SetCamera(camera); 
    #endif
}

Camera Graphics::GetCamera(){ 
    return graphicsDevice->GetCamera(); 
}

void Graphics::BeginRenderToScreen(Vector4 clearColor){
    #ifdef TestNewGPU_API
    ResetBindingCache();
    curFramebufferRenderPassIndex = FramebufferRenderPass::GetRenderPassIndex("DefaultWindows");
    Assert(curFramebufferRenderPassIndex  != -1);

    gfxDevice->GetCommandBuffer()->BeginWindowFramebuffer();
    gfxDevice->GetCommandBuffer()->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{clearColor.x, clearColor.y, clearColor.z, clearColor.a}});
    #else
    graphicsDevice->BeginRenderToScreen(clearColor); 
    #endif
}

void Graphics::EndRenderToScreen(){
    curFramebufferRenderPassIndex = -1;

    #ifdef TestNewGPU_API
    gfxDevice->GetCommandBuffer()->EndFramebuffer();
    #else
    graphicsDevice->EndRenderToScreen(); 
    #endif
}

void Graphics::Clean(float r, float g, float b, float a){ 
    #ifdef TestNewGPU_API
    gfxDevice->GetCommandBuffer()->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{r, g, b, a}});
    #else
    graphicsDevice->Clean(r, g, b, a); 
    #endif
}

void Graphics::CleanColorOnly(float r, float g, float b, float a){
    graphicsDevice->CleanColorOnly(r, g, b, a);
}

void Graphics::CleanDepthOnly(){
    graphicsDevice->CleanDepthOnly();
}

void Graphics::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){ 
    #ifdef TestNewGPU_API
    gfxDevice->GetCommandBuffer()->Viewport(x, y, w, h);
    #else
    graphicsDevice->SetViewport(x, y, w, h); 
    #endif
}

void Graphics::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){ 
    graphicsDevice->GetViewport(x, y, w, h); 
}

void Graphics::EnableScissor(){
    graphicsDevice->EnableScissor();
}

void Graphics::DisableScissor(){
    graphicsDevice->DisableScissor();
}

void Graphics::Scissor(unsigned int x, unsigned int y, int w, int h){
    graphicsDevice->Scissor(x, y, w, h); 
}

Gfx::BindGroup Graphics::BindMaterial(Material& mat){
    static std::vector<Gfx::BindingEntry> bindGroupEntries(200);

    if(mat.shader->materialBindGroupLayout == emptyLayout){
        return emptyBindGroup;
    }

    for(const auto& i: mat.maps){
        const MaterialMap& map = i.second;
        if(map.hasBufferData == false) continue;
        if(map.type == MaterialMap::Type::None) continue;

        if(map.type == MaterialMap::Type::Int){
            Assert(map.bufferSize >= sizeof(int));
            memcpy((char*)mat.materialBufferData + map.bufferPos, &map.valueInt, sizeof(int));
        } else if(map.type == MaterialMap::Type::Float){
            Assert(map.bufferSize >= sizeof(float));
            memcpy((char*)mat.materialBufferData + map.bufferPos, &map.valueFloat, sizeof(float));
        } else if(map.type == MaterialMap::Type::Vector2){
            //#ifdef GLM_FORCE_ALIGNED
                Assert(map.bufferSize >= (sizeof(float) * 2));
                memcpy((char*)mat.materialBufferData + map.bufferPos, &map.vec.vector.x, sizeof(float) * 2);
            /*#else
                Assert(m.size >= sizeof(Vector2));
                memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector2));
            #endif*/
        } else if(map.type == MaterialMap::Type::Vector3){
            //#ifdef GLM_FORCE_ALIGNED
                Vector3 v = Vector3(map.vec.vector.x, map.vec.vector.y, map.vec.vector.z);
                if(map.vec.vectorIsColor) v = ToLinear(v);
                Assert(map.bufferSize >= (sizeof(float) * 3));
                memcpy((char*)mat.materialBufferData + map.bufferPos, &v.x, sizeof(float) * 3);
            /*#else
                Assert(m.size >= sizeof(Vector3));
                memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector3));
            #endif*/
        } else if(map.type == MaterialMap::Type::Vector4){
            Vector4 v = map.vec.vector;
            if(map.vec.vectorIsColor) v = ToLinear(v);
            Assert(map.bufferSize >= sizeof(Vector4));
            memcpy((char*)mat.materialBufferData + map.bufferPos, &v, sizeof(Vector4));
        } else if(map.type == MaterialMap::Type::Matrix4){
            Assert(map.bufferSize >= sizeof(Matrix4));
            memcpy((char*)mat.materialBufferData + map.bufferPos, &map.matrix, sizeof(Matrix4));
        } else if(map.type == MaterialMap::Type::FloatList){
            Assert(map.list != nullptr);
            Assert(map.listCount > 0);

            //Assert(m.size >= sizeof(float) * map.listCount);
            //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<float*>(map.list), sizeof(float) * map.listCount);
            int stride = map.bufferArrayStride > 0 ?map.bufferArrayStride: 16; // fallback seguro
            char* base = (char*)mat.materialBufferData + map.bufferPos;
            float* src = static_cast<float*>(map.list);
            for(int j = 0; j < map.listCount; ++j){
                memcpy(base + j * stride, &src[j], sizeof(float));
            }
        } else if(map.type == MaterialMap::Type::Vector4List){
            Assert(map.list != nullptr);
            Assert(map.listCount > 0);

            //Assert(m.size >= sizeof(Vector4) * map.listCount);
            //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Vector4*>(map.list), sizeof(Vector4) * map.listCount);
            int stride = map.bufferArrayStride > 0 ? map.bufferArrayStride : sizeof(Vector4);
            char* base = (char*)mat.materialBufferData + map.bufferPos;
            Vector4* src = static_cast<Vector4*>(map.list);
            for(int j = 0; j < map.listCount; ++j){
                memcpy(base + j * stride, &src[j], sizeof(Vector4));
            }
        } else if(map.type == MaterialMap::Type::Matrix4List){
            Assert(map.list != nullptr);
            Assert(map.listCount > 0);
            
            //Assert(m.size >= sizeof(Matrix4) * map.listCount);
            //memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Matrix4*>(map.list), sizeof(Matrix4) * map.listCount);
            int stride = map.bufferArrayStride > 0 ? map.bufferArrayStride : sizeof(Matrix4); // normalmente 64
            char* base = (char*)mat.materialBufferData + map.bufferPos;
            Matrix4* src = static_cast<Matrix4*>(map.list);
            for(int j = 0; j < map.listCount; ++j){
                memcpy(base + j * stride, &src[j], sizeof(Matrix4));
            }
        } else {
            Assert(false && "Type Not Supported in A UnifomBuffer");
        }
    }
    gfxDevice->UpdatedBuffer(mat.materialBuffer, mat.materialBufferData, mat.materialBufferSize);

    Assert(curMat->shader->materialBindGroupLayout == mat.shader->materialBindGroupLayout);

    Gfx::BindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = mat.shader->materialBindGroupLayout;
    for(auto& i: mat.shader->reflection.bindings){
        if(i.set != 0) continue;

        if(i.type == Gfx::BindingType::UniformBuffer){
            if(i.blockName == "Main"){
                Assert(mat.materialBuffer != Gfx::InvalidID);
                bindGroupEntries[bindGroupInfo.entriesCount] = {};
                bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                bindGroupEntries[bindGroupInfo.entriesCount].buffer = mat.materialBuffer;
                bindGroupEntries[bindGroupInfo.entriesCount].size = i.size;
                bindGroupInfo.entriesCount += 1;
            }  else if(mat.maps.count(i.blockName)){
                auto& m = mat.maps[i.blockName];
                //Assert(m.buffer != nullptr);
                bindGroupEntries[bindGroupInfo.entriesCount] = {};
                bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                bindGroupEntries[bindGroupInfo.entriesCount].buffer = m.buffer == nullptr ? defaultBuffer : m.buffer->buffer;
                bindGroupEntries[bindGroupInfo.entriesCount].size = i.size;
                bindGroupInfo.entriesCount += 1;
            } else if(mat.globalMaps.count(i.blockName)){
                auto& m = mat.globalMaps[i.blockName];
                //Assert(m.buffer != nullptr);
                bindGroupEntries[bindGroupInfo.entriesCount] = {};
                bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                bindGroupEntries[bindGroupInfo.entriesCount].buffer = m.buffer == nullptr ? defaultBuffer : m.buffer->buffer;
                bindGroupEntries[bindGroupInfo.entriesCount].size = i.size;
                bindGroupInfo.entriesCount += 1;
            } else {
                //Assert(false);
                bindGroupEntries[bindGroupInfo.entriesCount] = {};
                bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                bindGroupEntries[bindGroupInfo.entriesCount].buffer = defaultBuffer;
                bindGroupEntries[bindGroupInfo.entriesCount].size = i.size;
                bindGroupInfo.entriesCount += 1;
            }
        }

        if(i.type == Gfx::BindingType::Texture2D){
            if(mat.maps.count(i.name)){
                auto& m = mat.maps[i.name];

                if(m.type == MaterialMap::Type::Texture){
                    //Assert(m.texture != nullptr);
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].texture = m.texture == nullptr ? defaultTex : m.texture->tex;// mat.maps[i.name].texture->tex;
                    bindGroupInfo.entriesCount += 1;
                } else if(m.type == MaterialMap::Type::Framebuffer){
                    if(m.framebuffer != nullptr){
                    Assert(m.framebuffer != nullptr);
                    Assert(m.framebuffer->framebuffer != Gfx::InvalidID);
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebuffer = m.framebuffer->framebuffer;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferAttacement = m.framebufferAttachment;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferLayer = 0;
                    bindGroupInfo.entriesCount += 1;
                    } else {
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].texture = defaultTex;
                    bindGroupInfo.entriesCount += 1;
                    }
                } else {
                    Assert(false);
                }
            } else if(mat.globalMaps.count(i.name)){
                auto& m = mat.globalMaps[i.name];

                if(m.type == MaterialMap::Type::Texture){
                    //Assert(m.texture != nullptr);
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].texture = m.texture == nullptr ? defaultTex : m.texture->tex;// mat.maps[i.name].texture->tex;
                    bindGroupInfo.entriesCount += 1;
                } else if(m.type == MaterialMap::Type::Framebuffer){
                    Assert(m.framebuffer != nullptr);
                    Assert(m.framebuffer->framebuffer != Gfx::InvalidID);
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebuffer = m.framebuffer->framebuffer;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferAttacement = m.framebufferAttachment;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferLayer = 0;
                    bindGroupInfo.entriesCount += 1;
                } else {
                    Assert(false);
                }
            } else {
                Assert(false);
            }
        }

        if(i.type == Gfx::BindingType::TextureCube){
            if(mat.maps.count(i.name)){
                auto& m = mat.maps[i.name];

                if(m.type == MaterialMap::Type::Cubemap){
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].cubemap = m.cubemap == nullptr ? defaultCubemap : m.cubemap->tex;// mat.maps[i.name].texture->tex;
                    bindGroupInfo.entriesCount += 1;
                } else if(m.type == MaterialMap::Type::Framebuffer){
                    Assert(m.framebuffer != nullptr);
                    Assert(m.framebuffer->framebuffer != Gfx::InvalidID);
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebuffer = m.framebuffer->framebuffer;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferAttacement = m.framebufferAttachment;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferLayer = 0;
                    bindGroupInfo.entriesCount += 1;
                } else {
                    Assert(false);
                }
            } else if(mat.globalMaps.count(i.name)){
                auto& m = mat.globalMaps[i.name];

                if(m.type == MaterialMap::Type::Cubemap){
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].cubemap = m.cubemap == nullptr ? defaultCubemap : m.cubemap->tex;
                    bindGroupInfo.entriesCount += 1;
                } else if(m.type == MaterialMap::Type::Framebuffer){
                    Assert(m.framebuffer != nullptr);
                    Assert(m.framebuffer->framebuffer != Gfx::InvalidID);
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebuffer = m.framebuffer->framebuffer;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferAttacement = m.framebufferAttachment;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferLayer = 0;
                    bindGroupInfo.entriesCount += 1;
                } else {
                    //Assert(false);
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].cubemap = defaultCubemap;
                    bindGroupInfo.entriesCount += 1;
                }
            } else {
                //Assert(false);
                bindGroupEntries[bindGroupInfo.entriesCount] = {};
                bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                bindGroupEntries[bindGroupInfo.entriesCount].cubemap = defaultCubemap;
                bindGroupInfo.entriesCount += 1;
            }
        }

        if(i.type == Gfx::BindingType::Texture2DArray){
            //Assert(false);

            if(mat.globalMaps.count(i.name)){
                auto& m = mat.globalMaps[i.name];

                if(m.type == MaterialMap::Type::Framebuffer){
                    Assert(m.framebuffer != nullptr);
                    Assert(m.framebuffer->framebuffer != Gfx::InvalidID);
                    bindGroupEntries[bindGroupInfo.entriesCount] = {};
                    bindGroupEntries[bindGroupInfo.entriesCount].binding = i.binding;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebuffer = m.framebuffer->framebuffer;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferAttacement = m.framebufferAttachment;
                    bindGroupEntries[bindGroupInfo.entriesCount].framebufferLayer = 0;
                    bindGroupInfo.entriesCount += 1;
                } else {
                    Assert(false);
                }
            } else {
                Assert(false);
            }
        }

        if(i.type == Gfx::BindingType::TextureCubeArray){
            Assert(false);
        }

        if(i.type == Gfx::BindingType::StorageBuffer){
            Assert(false);
        }


        /*if(i.type == Gfx::BindingType::UniformBuffer && i.blockName == "Main"){
            bindGroupInfo.entries[bindGroupInfo.entriesCount] = {};
            bindGroupInfo.entries[bindGroupInfo.entriesCount].binding = i.binding;
            bindGroupInfo.entries[bindGroupInfo.entriesCount].buffer = mat.materialBuffer;
            bindGroupInfo.entries[bindGroupInfo.entriesCount].size = i.size;
            bindGroupInfo.entriesCount += 1;
        }

        if(i.type == Gfx::BindingType::UniformBuffer && i.blockName != "Main" && mat.globalMaps.count(i.blockName)){
            auto& m = mat.globalMaps[i.blockName];

            bindGroupInfo.entries[bindGroupInfo.entriesCount] = {};
            bindGroupInfo.entries[bindGroupInfo.entriesCount].binding = i.binding;
            bindGroupInfo.entries[bindGroupInfo.entriesCount].buffer = m.buffer->buffer;
            bindGroupInfo.entries[bindGroupInfo.entriesCount].size = i.size;
            bindGroupInfo.entriesCount += 1;
        }

        if(i.type == Gfx::BindingType::Texture2D && mat.maps.count(i.name)){
            auto& m = mat.maps[i.name];

            if(m.type == MaterialMap::Type::Texture){
                bindGroupInfo.entries[bindGroupInfo.entriesCount] = {};
                bindGroupInfo.entries[bindGroupInfo.entriesCount].binding = i.binding;
                bindGroupInfo.entries[bindGroupInfo.entriesCount].texture = m.texture->tex;// mat.maps[i.name].texture->tex;
                bindGroupInfo.entriesCount += 1;
            }

            if(m.type == MaterialMap::Type::Framebuffer){
                Assert(m.framebuffer->framebuffer != Gfx::InvalidID);
                bindGroupInfo.entries[bindGroupInfo.entriesCount] = {};
                bindGroupInfo.entries[bindGroupInfo.entriesCount].binding = i.binding;
                bindGroupInfo.entries[bindGroupInfo.entriesCount].framebuffer = m.framebuffer->framebuffer;
                bindGroupInfo.entries[bindGroupInfo.entriesCount].framebufferAttacement = m.framebufferAttachment;
                bindGroupInfo.entriesCount += 1;
            }
        }*/
    }

    bindGroupInfo.entries = bindGroupEntries.data();
    Assert(mat.shader->materialBindGroupLayoutInfo.entriesCount >= bindGroupInfo.entriesCount);
    return gfxDevice->CreateFrameBindGroup(bindGroupInfo);
}

void Graphics::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix, PerDrawData* perDrawData){ 
    #ifdef TestNewGPU_API
    //Assert(false);
    auto perDrawBindGroup = drawMeshPool.GetBindGroup(*gfxDevice, &modelMatrix, sizeof(Matrix4));

    if(curMat != &mat || curMat->isDirty || curMat->currentShader[curMat->currentPass].drawTypes[(int)Shader::DrawType::DefaultDraw].get() != curShader){
        curMat = &mat;
        curMat->isDirty = false;
        curShader = curMat->currentShader[curMat->currentPass].drawTypes[(int)Shader::DrawType::DefaultDraw].get();
        curBindGroup = BindMaterial(*curMat);
    }

    Assert(mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::DefaultDraw]->_pipelines[curFramebufferRenderPassIndex] != Gfx::InvalidID);

    auto* cmd = gfxDevice->GetCommandBuffer();
    SetPipelineCached(cmd, mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::DefaultDraw]->_pipelines[curFramebufferRenderPassIndex]);
    SetBindGroupCached(cmd, 0, curBindGroup);
    SetBindGroupCached(cmd, 1, perDrawBindGroup);
    SetBindGroupCached(cmd, 2, curCameraBindGroup);

    SetVertexBufferCached(cmd, 0, mesh.vertexVbo);
    SetVertexBufferCached(cmd, 1, mesh.uvVbo);
    SetVertexBufferCached(cmd, 2, mesh.normalVbo);
    SetVertexBufferCached(cmd, 3, mesh.colorVbo);
    SetVertexBufferCached(cmd, 4, mesh.tangentVbo);
    SetVertexBufferCached(cmd, 5, mesh.influencesVbo);
    SetVertexBufferCached(cmd, 6, mesh.weightsVbo);
    SetVertexBufferCached(cmd, 7, emptyInstacingVbo);

    if(mesh.indiceCount == 0){ //mesh.ebo == INVALID_ID){
        cmd->Draw(mesh.vertexCount);
    } else {
        SetIndexBufferCached(cmd, mesh.ebo);
        cmd->DrawIndexed(mesh.indiceCount);
    }
    #else
    graphicsDevice->DrawMesh(mesh, mat, modelMatrix, perDrawData); 
    #endif
}

void Graphics::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, Matrix4* animMatrix, int count, PerDrawData* perDrawData){ 
    #ifdef TestNewGPU_API
    Assert(count >= 0 && count <= MaxBonesPerDraw);

    Matrix4 bones[MaxBonesPerDraw]{};
    for(int i = 0; i < count; i++){
        bones[i] = animMatrix[i];
    }

    auto modelBuffer = drawMeshPool.GetBuffer(*gfxDevice, &model, sizeof(Matrix4));
    auto bonesBuffer = drawMeshSkinnedPool.GetBuffer(*gfxDevice, bones, sizeof(bones));

    Gfx::BindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = drawMeshSkinnedGroupLayout;
    Gfx::BindingEntry bindGroupEntries[2];
    bindGroupEntries[0].binding = 0;
    bindGroupEntries[0].buffer = modelBuffer;
    bindGroupEntries[0].size = sizeof(Matrix4);
    bindGroupEntries[1].binding = 1;
    bindGroupEntries[1].buffer = bonesBuffer;
    bindGroupEntries[1].size = sizeof(bones);
    bindGroupInfo.entriesCount = 2;
    bindGroupInfo.entries = bindGroupEntries;
    auto perDrawBindGroup = gfxDevice->CreateFrameBindGroup(bindGroupInfo);

    if(curMat != &mat || curMat->isDirty || curMat->currentShader[curMat->currentPass].drawTypes[(int)Shader::DrawType::SkinnedDraw].get() != curShader){
        curMat = &mat;
        curMat->isDirty = false;
        curShader = curMat->currentShader[curMat->currentPass].drawTypes[(int)Shader::DrawType::SkinnedDraw].get();
        curBindGroup = BindMaterial(*curMat);
    }

    Assert(mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::SkinnedDraw]->_pipelines[curFramebufferRenderPassIndex] != Gfx::InvalidID);

    auto* cmd = gfxDevice->GetCommandBuffer();
    SetPipelineCached(cmd, mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::SkinnedDraw]->_pipelines[curFramebufferRenderPassIndex]);
    SetBindGroupCached(cmd, 0, curBindGroup);
    SetBindGroupCached(cmd, 1, perDrawBindGroup);
    SetBindGroupCached(cmd, 2, curCameraBindGroup);

    SetVertexBufferCached(cmd, 0, mesh.vertexVbo);
    SetVertexBufferCached(cmd, 1, mesh.uvVbo);
    SetVertexBufferCached(cmd, 2, mesh.normalVbo);
    SetVertexBufferCached(cmd, 3, mesh.colorVbo);
    SetVertexBufferCached(cmd, 4, mesh.tangentVbo);
    SetVertexBufferCached(cmd, 5, mesh.influencesVbo);
    SetVertexBufferCached(cmd, 6, mesh.weightsVbo);
    SetVertexBufferCached(cmd, 7, emptyInstacingVbo);

    if(mesh.indiceCount == 0){
        cmd->Draw(mesh.vertexCount);
    } else {
        SetIndexBufferCached(cmd, mesh.ebo);
        cmd->DrawIndexed(mesh.indiceCount);
    }
    #else
    graphicsDevice->DrawMeshSkinned(mesh, mat, model, animMatrix, count, perDrawData); 
    #endif
}

void Graphics::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, UniformBuffer* data, int count, PerDrawData* perDrawData){
    #ifdef TestNewGPU_API
    Assert(data != nullptr);
    Assert(count >= 0 && count <= MaxBonesPerDraw);

    auto modelBuffer = drawMeshPool.GetBuffer(*gfxDevice, &model, sizeof(Matrix4));

    Gfx::BindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = drawMeshSkinnedGroupLayout;
    Gfx::BindingEntry bindGroupEntries[2];
    bindGroupEntries[0].binding = 0;
    bindGroupEntries[0].buffer = modelBuffer;
    bindGroupEntries[0].size = sizeof(Matrix4);
    bindGroupEntries[1].binding = 1;
    bindGroupEntries[1].buffer = data->buffer;
    bindGroupEntries[1].size = sizeof(Matrix4) * MaxBonesPerDraw;
    bindGroupInfo.entriesCount = 2;
    bindGroupInfo.entries = bindGroupEntries;
    auto perDrawBindGroup = gfxDevice->CreateFrameBindGroup(bindGroupInfo);

    if(curMat != &mat || curMat->isDirty || curMat->currentShader[curMat->currentPass].drawTypes[(int)Shader::DrawType::SkinnedDraw2].get() != curShader){
        curMat = &mat;
        curMat->isDirty = false;
        curShader = curMat->currentShader[curMat->currentPass].drawTypes[(int)Shader::DrawType::SkinnedDraw2].get();
        curBindGroup = BindMaterial(*curMat);
    }

    auto* shader = mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::SkinnedDraw2].get();
    Assert(shader != nullptr);
    Assert(shader->_pipelines[curFramebufferRenderPassIndex] != Gfx::InvalidID);

    auto* cmd = gfxDevice->GetCommandBuffer();
    SetPipelineCached(cmd, shader->_pipelines[curFramebufferRenderPassIndex]);
    SetBindGroupCached(cmd, 0, curBindGroup);
    SetBindGroupCached(cmd, 1, perDrawBindGroup);
    SetBindGroupCached(cmd, 2, curCameraBindGroup);

    SetVertexBufferCached(cmd, 0, mesh.vertexVbo);
    SetVertexBufferCached(cmd, 1, mesh.uvVbo);
    SetVertexBufferCached(cmd, 2, mesh.normalVbo);
    SetVertexBufferCached(cmd, 3, mesh.colorVbo);
    SetVertexBufferCached(cmd, 4, mesh.tangentVbo);
    SetVertexBufferCached(cmd, 5, mesh.influencesVbo);
    SetVertexBufferCached(cmd, 6, mesh.weightsVbo);
    SetVertexBufferCached(cmd, 7, emptyInstacingVbo);

    if(mesh.indiceCount == 0){
        cmd->Draw(mesh.vertexCount);
    } else {
        SetIndexBufferCached(cmd, mesh.ebo);
        cmd->DrawIndexed(mesh.indiceCount);
    }
    #else
    graphicsDevice->DrawMeshSkinned(mesh, mat, model, data, count, perDrawData); 
    #endif
}

void Graphics::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count){ 
    #ifdef TestNewGPU_API
    //Assert(false);

    auto DrawMeshInstancingInternal = [&](Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count){
        Assert(count <= MaxInstancesPerDraw);
        auto intacingBuffer = drawMeshInstancingPool.GetBuffer(*gfxDevice, animMatrixs, sizeof(Matrix4) * count);

        if(curMat != &mat || curMat->isDirty || curMat->currentShader[curMat->currentPass].drawTypes[(int)Shader::DrawType::InstancingDraw].get() != curShader){
            curMat = &mat;
            curMat->isDirty = false;
            curShader = curMat->currentShader[curMat->currentPass].drawTypes[(int)Shader::DrawType::DefaultDraw].get();
            curBindGroup = BindMaterial(*curMat);
        }

        Assert(mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::InstancingDraw]->_pipelines[curFramebufferRenderPassIndex] != Gfx::InvalidID);

        auto* cmd = gfxDevice->GetCommandBuffer();
        SetPipelineCached(cmd, mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::InstancingDraw]->_pipelines[curFramebufferRenderPassIndex]);
        SetBindGroupCached(cmd, 0, curBindGroup);
        SetBindGroupCached(cmd, 1, emptyModelBindGroup);
        SetBindGroupCached(cmd, 2, curCameraBindGroup);

        SetVertexBufferCached(cmd, 0, mesh.vertexVbo);
        SetVertexBufferCached(cmd, 1, mesh.uvVbo);
        SetVertexBufferCached(cmd, 2, mesh.normalVbo);
        SetVertexBufferCached(cmd, 3, mesh.colorVbo);
        SetVertexBufferCached(cmd, 4, mesh.tangentVbo);
        SetVertexBufferCached(cmd, 5, mesh.influencesVbo);
        SetVertexBufferCached(cmd, 6, mesh.weightsVbo);
        SetVertexBufferCached(cmd, 7, intacingBuffer);

        if(mesh.ebo == INVALID_ID){
            cmd->DrawInstanced(mesh.vertexCount, count);
        } else {
            SetIndexBufferCached(cmd, mesh.ebo);
            cmd->DrawIndexedInstanced(mesh.indiceCount, count);
        }
    };

    int offset = 0;

    while(offset < count){
        int batchCount = std::min(MaxInstancesPerDraw, count - offset);
        Matrix4* batchMatrices = animMatrixs + offset;
        DrawMeshInstancingInternal(mesh, mat, batchMatrices, batchCount);
        offset += batchCount;
    }

    #else
    graphicsDevice->DrawMeshInstancing(mesh, mat, animMatrixs, count); 
    #endif
}

void Graphics::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4x3* animMatrixs, int count){
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    graphicsDevice->DrawMeshInstancing(mesh, mat, animMatrixs, count); 
    #endif
}

void Graphics::DrawMeshInstancing(Mesh& mesh, Material& mat, InstancingBuffer& buffer, int count){
    #ifdef TestNewGPU_API
    //Assert(false);
    if(curMat != &mat || curMat->isDirty){
        curMat = &mat;
        curMat->isDirty = false;
        curBindGroup = BindMaterial(*curMat);
    }

    Assert(mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::InstancingDraw]->_pipelines[curFramebufferRenderPassIndex] != Gfx::InvalidID);

    auto* cmd = gfxDevice->GetCommandBuffer();
    SetPipelineCached(cmd, mat.currentShader[mat.currentPass].drawTypes[(int)Shader::DrawType::InstancingDraw]->_pipelines[curFramebufferRenderPassIndex]);
    SetBindGroupCached(cmd, 0, curBindGroup);
    SetBindGroupCached(cmd, 1, emptyModelBindGroup);
    SetBindGroupCached(cmd, 2, curCameraBindGroup);

    SetVertexBufferCached(cmd, 0, mesh.vertexVbo);
    SetVertexBufferCached(cmd, 1, mesh.uvVbo);
    SetVertexBufferCached(cmd, 2, mesh.normalVbo);
    SetVertexBufferCached(cmd, 3, mesh.colorVbo);
    SetVertexBufferCached(cmd, 4, mesh.tangentVbo);
    SetVertexBufferCached(cmd, 5, mesh.influencesVbo);
    SetVertexBufferCached(cmd, 6, mesh.weightsVbo);
    SetVertexBufferCached(cmd, 10, buffer.buffer);

    if(mesh.ebo == INVALID_ID){
        cmd->DrawInstanced(mesh.vertexCount, count);
    } else {
        SetIndexBufferCached(cmd, mesh.ebo);
        cmd->DrawIndexedInstanced(mesh.indiceCount, count);
    }
    #else
    graphicsDevice->DrawMeshInstancing(mesh, mat, buffer, count);
    #endif 
}

void Graphics::DrawModel(Model& model, Matrix4 modelMatrix){ 
    #ifdef TestNewGPU_API
    int index = 0;
    for(auto i: model.renderTargets){
        Ref<Material> targetMaterial = model.materials[i.materialIndex];
        Ref<Mesh> targetMesh = model.meshs[i.meshIndex];
        Matrix4 targetMatrix =  modelMatrix * model.skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
        DrawMesh(*targetMesh, *targetMaterial, targetMatrix);
    }
    #else
    graphicsDevice->DrawModel(model, modelMatrix); 
    #endif
}

void Graphics::AddDrawLineCommand(Vector3 start, Vector3 end){ 
    graphicsDevice->AddDrawLineCommand(start, end); 
}

void Graphics::DrawLinesComamnd(Vector3 color, int lineWidth){ 
    graphicsDevice->DrawLinesComamnd(color, lineWidth); 
}

void Graphics::DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth){ 
    graphicsDevice->DrawLine(start, end, color, lineWidth); 
}

void Graphics::DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth){ 
    graphicsDevice->DrawLine(model, start, end, color, lineWidth); 
}

void Graphics::DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){ 
    graphicsDevice->DrawWireCube(modelMatrix, color, lineWidth); 
}

void Graphics::DrawText(Font& f, Material& s, std::string text, Matrix4 model, bool alignWithTop, const TextParams& params){
    graphicsDevice->DrawText(f, s, text, model, alignWithTop, params); 
}

void Graphics::DrawFullScreenQuad(Material& mat, Matrix4 modelMatrix){
    #ifdef TestNewGPU_API
    Assert(fullScreenQuad != nullptr);
    DrawMesh(*fullScreenQuad, mat, modelMatrix);
    #else
    graphicsDevice->DrawFullScreenQuad(mat, modelMatrix); 
    #endif
}

void Graphics::DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& mat, int pass){ 
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    graphicsDevice->DrawQuadPostProcessing(src, dst, mat, pass); 
    #endif

}

void Graphics::DrawQuadPostProcessing(Framebuffer* dst, Material& mat, int pass){ 
    #ifdef TestNewGPU_API
    Assert(false);
    #else
    graphicsDevice->DrawQuadPostProcessing(dst, mat, pass); 
    #endif
}

void Graphics::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){ 
    #ifdef TestNewGPU_API
    Assert(src != nullptr);
    Assert(src->framebuffer != Gfx::InvalidID);
    if(dst != nullptr) Assert(dst->framebuffer != Gfx::InvalidID);
    gfxDevice->GetCommandBuffer()->BlitFramebuffer(
        src->framebuffer,
        dst == nullptr ? Gfx::InvalidID : dst->framebuffer,
        srcPass
    );
    #else
    graphicsDevice->BlitFramebuffer(src, dst, srcPass); 
    #endif
}

void Graphics::BeginFramebuffer(Framebuffer& frambuffer, bool clean, Vector4 clearColor, int layer, int mip){ 
    #ifdef TestNewGPU_API
    Assert(frambuffer.framebuffer != Gfx::InvalidID);

    ResetBindingCache();

    curFramebufferRenderPassIndex = frambuffer.passIndex;
    curFramebufferRenderPassName = frambuffer.passName;
    Assert(curFramebufferRenderPassIndex  != -1);

    gfxDevice->GetCommandBuffer()->BeginFramebuffer(
        frambuffer.framebuffer,
        layer,
        mip,
        clean,
        Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth,
        {{clearColor.x, clearColor.y, clearColor.z, clearColor.a}}
    );
    #else
    graphicsDevice->BeginFramebuffer(frambuffer, clean, clearColor, layer, mip); 
    #endif
}

void Graphics::EndFramebuffer(){ 
    #ifdef TestNewGPU_API
    gfxDevice->GetCommandBuffer()->EndFramebuffer();
    #else
    graphicsDevice->EndFramebuffer(); 
    #endif
}

void Graphics::BeginGPUTime(){
    graphicsDevice->BeginGPUTime();
}

double Graphics::EndGPUTime(){
    return graphicsDevice->EndGPUTime();
}

}
