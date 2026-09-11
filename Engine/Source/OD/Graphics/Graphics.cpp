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

int curGraphicsDevice = 1;
GraphicsDevice* graphicsDevice = nullptr;
Gfx::Device* gfxDevice = nullptr;

Gfx::BindGroupLayout emptyLayout;
Gfx::BindGroupLayout camGroupLayout;
Gfx::BindGroupLayout drawDrawMeshGroupLayout;

Gfx::BindGroup emptyBindGroup;
Gfx::BindGroup camBindGroup;
Gfx::BindGroup emptyModelBindGroup;

Gfx::Buffer camBuffer;
Gfx::Buffer emptyModelBuffer;

struct UniformBufferPool{
    Gfx::BindGroupLayout layout;
    std::vector<Gfx::Buffer> buffers;
    uint32_t curIndex = 0;

    inline Gfx::BindGroup GetBindGroup(Gfx::Device& device, void* data, size_t size){
        if(buffers.size() <= curIndex){
            auto buffer = device.CreateBuffer(size, Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
            Assert(buffer != Gfx::InvalidID);
            buffers.push_back(buffer);
        }

        device.UpdatedBuffer(buffers[curIndex], data, size);

        Gfx::BindGroupInfo bindGroupInfo = {};
        bindGroupInfo.layout = layout;
        bindGroupInfo.entries[0].buffer = buffers[curIndex];
        bindGroupInfo.entries[0].size = size;
        bindGroupInfo.entriesCount = 1;
        auto bindGroup = device.CreateFrameBindGroup(bindGroupInfo);
        curIndex += 1;
        return bindGroup;
    }
};

UniformBufferPool drawMeshPool;
UniformBufferPool drawMeshInstancingPool;

Material* curMat = nullptr;
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
    graphicsDevice = new Gfx::OpenglGPUDevice();
    //graphicsDevice = new Gfx::VulkanGPUDevice();
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

    #ifdef TestNewGPU_API
    CameraData camData;
    Matrix4 identity = Matrix4Identity;

    camBuffer = gfxDevice->CreateBuffer(sizeof(CameraData), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    gfxDevice->UpdatedBuffer(camBuffer, &camData, sizeof(CameraData));

    emptyModelBuffer = gfxDevice->CreateBuffer(sizeof(Matrix4), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    gfxDevice->UpdatedBuffer(emptyModelBuffer, &identity, sizeof(Matrix4));

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

    Gfx::BindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = emptyLayout;
    bindGroupInfo.entriesCount = 0;
    emptyBindGroup = gfxDevice->CreateBindGroup(bindGroupInfo);

    bindGroupInfo = {};
    bindGroupInfo.layout = camGroupLayout;
    bindGroupInfo.entries[0].binding = 0;
    bindGroupInfo.entries[0].buffer = camBuffer;
    bindGroupInfo.entries[0].size = sizeof(CameraData);
    bindGroupInfo.entriesCount = 1;
    camBindGroup = gfxDevice->CreateBindGroup(bindGroupInfo);

    bindGroupInfo = {};
    bindGroupInfo.layout = drawDrawMeshGroupLayout;
    bindGroupInfo.entries[0].binding = 0;
    bindGroupInfo.entries[0].buffer = emptyModelBuffer;
    bindGroupInfo.entries[0].size = sizeof(Matrix4);
    bindGroupInfo.entriesCount = 1;
    emptyModelBindGroup = gfxDevice->CreateBindGroup(bindGroupInfo);

    drawMeshPool.layout = drawDrawMeshGroupLayout;
    #endif
}

void Graphics::Shutdown(){
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

    curMat = nullptr;
    curBindGroup = emptyBindGroup;
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
    #else
    graphicsDevice->SetCamera(camera); 
    #endif
}

Camera Graphics::GetCamera(){ 
    return graphicsDevice->GetCamera(); 
}

void Graphics::BeginRenderToScreen(Vector4 clearColor){
    #ifdef TestNewGPU_API
    gfxDevice->GetCommandBuffer()->BeginWindowFramebuffer();
    gfxDevice->GetCommandBuffer()->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{clearColor.x, clearColor.y, clearColor.z, clearColor.a}});
    #else
    graphicsDevice->BeginRenderToScreen(clearColor); 
    #endif
}

void Graphics::EndRenderToScreen(){
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
    if(mat.shader->materialBindGroupLayout == emptyLayout) return emptyBindGroup;

    for(const auto& i: mat.maps){
        const MaterialMap& map = i.second;
        if(map.hasBufferData == false) continue;

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

    Gfx::BindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = mat.shader->materialBindGroupLayout;
    for(auto& i: mat.shader->reflection.bindings){
        if(i.set != 0) continue;

        if(i.type == Gfx::BindingType::UniformBuffer && i.blockName == "Main"){
            bindGroupInfo.entries[bindGroupInfo.entriesCount] = {};
            bindGroupInfo.entries[bindGroupInfo.entriesCount].binding = i.binding;
            bindGroupInfo.entries[bindGroupInfo.entriesCount].buffer = mat.materialBuffer;
            bindGroupInfo.entries[bindGroupInfo.entriesCount].size = i.size;
            bindGroupInfo.entriesCount += 1;
        }

        if(i.type == Gfx::BindingType::Texture2D && mat.maps.count(i.name)){
            bindGroupInfo.entries[bindGroupInfo.entriesCount] = {};
            bindGroupInfo.entries[bindGroupInfo.entriesCount].binding = i.binding;
            bindGroupInfo.entries[bindGroupInfo.entriesCount].texture = mat.maps[i.name].texture->tex;
            bindGroupInfo.entriesCount += 1;
        }
    }

    return gfxDevice->CreateFrameBindGroup(bindGroupInfo);
}

void Graphics::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix, PerDrawData* perDrawData){ 
    #ifdef TestNewGPU_API
    //Assert(false);
    auto perDrawBindGroup = drawMeshPool.GetBindGroup(*gfxDevice, &modelMatrix, sizeof(Matrix4));

    if(curMat != &mat || curMat->isDirty){
        curMat = &mat;
        curMat->isDirty = false;
        curBindGroup = BindMaterial(*curMat);
    }

    auto* cmd = gfxDevice->GetCommandBuffer();
    cmd->SetPipeline(mat.currentShader.drawTypes[0]->_pipeline);
    cmd->SetBindGroup(0, curBindGroup);
    cmd->SetBindGroup(1, perDrawBindGroup);
    cmd->SetBindGroup(2, camBindGroup);
    cmd->SetVertexBuffer(0, mesh.vertexVbo);
    cmd->SetVertexBuffer(1, mesh.uvVbo);
    cmd->SetVertexBuffer(2, mesh.normalVbo);
    cmd->SetVertexBuffer(4, mesh.tangentVbo);
    if(mesh.ebo == INVALID_ID){
        cmd->Draw(mesh.vertexCount);
    } else {
        cmd->SetIndexBuffer(mesh.ebo);
        cmd->DrawIndexed(mesh.indiceCount);
    }
    #else
    graphicsDevice->DrawMesh(mesh, mat, modelMatrix, perDrawData); 
    #endif
}

void Graphics::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, Matrix4* animMatrix, int count, PerDrawData* perDrawData){ 
    graphicsDevice->DrawMeshSkinned(mesh, mat, model, animMatrix, count, perDrawData); 
}

void Graphics::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, UniformBuffer* data, int count, PerDrawData* perDrawData){
    graphicsDevice->DrawMeshSkinned(mesh, mat, model, data, count, perDrawData); 
}

void Graphics::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count){ 
    #ifdef TestNewGPU_API
    Assert(false);

    auto DrawMeshInstancingInternal = [&](Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count){
        Assert(count <= 1000);
        auto perDrawBindGroup = drawMeshInstancingPool.GetBindGroup(*gfxDevice, animMatrixs, sizeof(Matrix4) * count);

        if(curMat != &mat || curMat->isDirty){
            curMat = &mat;
            curMat->isDirty = false;
            curBindGroup = BindMaterial(*curMat);
        }

        auto* cmd = gfxDevice->GetCommandBuffer();
        cmd->SetPipeline(mat.currentShader.drawTypes[0]->_pipeline);
        cmd->SetBindGroup(0, curBindGroup);
        cmd->SetBindGroup(1, perDrawBindGroup);
        cmd->SetBindGroup(2, camBindGroup);
        cmd->SetVertexBuffer(0, mesh.vertexVbo);
        cmd->SetVertexBuffer(1, mesh.uvVbo);
        cmd->SetVertexBuffer(2, mesh.normalVbo);
        cmd->SetVertexBuffer(4, mesh.tangentVbo);
        if(mesh.ebo == INVALID_ID){
            cmd->Draw(mesh.vertexCount);
        } else {
            cmd->SetIndexBuffer(mesh.ebo);
            cmd->DrawIndexed(mesh.indiceCount);
        }
    };

    constexpr int MaxInstancesPerDraw = 1000;
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
    Assert(false);
    #else
    graphicsDevice->DrawMeshInstancing(mesh, mat, buffer, count);
    #endif 
}

void Graphics::DrawModel(Model& model, Matrix4 modelMatrix){ 
    graphicsDevice->DrawModel(model, modelMatrix); 
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
    graphicsDevice->DrawFullScreenQuad(mat, modelMatrix); 
}

void Graphics::DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& mat, int pass){ 
    graphicsDevice->DrawQuadPostProcessing(src, dst, mat, pass); 
}

void Graphics::DrawQuadPostProcessing(Framebuffer* dst, Material& mat, int pass){ 
    graphicsDevice->DrawQuadPostProcessing(dst, mat, pass); 
}

void Graphics::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){ 
    graphicsDevice->BlitFramebuffer(src, dst, srcPass); 
}

void Graphics::BeginFramebuffer(Framebuffer& frambuffer, bool clean, Vector4 clearColor, int layer, int mip){ 
    graphicsDevice->BeginFramebuffer(frambuffer, clean, clearColor, layer, mip); 
}

void Graphics::EndFramebuffer(){ 
    graphicsDevice->EndFramebuffer(); 
}

void Graphics::BeginGPUTime(){
    graphicsDevice->BeginGPUTime();
}

double Graphics::EndGPUTime(){
    return graphicsDevice->EndGPUTime();
}

}