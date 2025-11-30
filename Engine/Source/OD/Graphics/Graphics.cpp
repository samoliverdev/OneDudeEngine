#include "Graphics.h"
#include "GraphicsDevice.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "Font.h"
#include "Mesh.h"
#include "Model.h"
#include "SubShader.h"
#include "Font.h"
#include "OD/Defines.h"
#include "OD/Core/Lua.h"

//#define ENGINE_RESOURCE_PATH "res/Engine/"

#include "OD/Platform/Headless/HeadlessGraphicsDevice.h"

#include "OD/Platform/OpenGL/OpenGLGraphicsDevice.h"
#if defined(WEBGPU_SUPPORT)
#include "OD/Platform/WebGPU/WebGPUGraphicsDevice.h"
#endif

#include <functional>

namespace OD{

void GraphicsModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Material>(".material", [](const std::string& path){ return AssetManager::Get().LoadAsset<Material>(path); });
    
    AssetTypesDB::Get().RegisterAssetType<Model>(".model", [](const std::string& path){ return AssetManager::Get().LoadAsset<Model>(path); });
    AssetTypesDB::Get().RegisterAssetType<Model>(".obj", [](const std::string& path){ return AssetManager::Get().LoadAsset<Model>(path); });
    AssetTypesDB::Get().RegisterAssetType<Model>(".glb", [](const std::string& path){ return AssetManager::Get().LoadAsset<Model>(path); });
    AssetTypesDB::Get().RegisterAssetType<Model>(".gltf", [](const std::string& path){ return AssetManager::Get().LoadAsset<Model>(path); });
    AssetTypesDB::Get().RegisterAssetType<Model>(".fbx", [](const std::string& path){ return AssetManager::Get().LoadAsset<Model>(path); });

    AssetTypesDB::Get().RegisterAssetType<Shader>(".glsl", [](const std::string& path){ return AssetManager::Get().LoadAsset<Shader>(path); });
    AssetTypesDB::Get().RegisterAssetType<Shader>(".shader", [](const std::string& path){ return AssetManager::Get().LoadAsset<Shader>(path); });

    AssetTypesDB::Get().RegisterAssetType<Mesh>(".bin", [](const std::string& path){ return AssetManager::Get().LoadAsset<Mesh>(path); });

    AssetTypesDB::Get().RegisterAssetType<Font>(".ttf", [](const std::string& path){ return AssetManager::Get().LoadAsset<Font>(path); });

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

void Graphics::SelectGraphicsDevice(){
    
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

void Graphics::Begin(){ 
    graphicsDevice->Begin(); 
}

void Graphics::End(){ 
    graphicsDevice->End(); 
}

void Graphics::_Begin(){
    graphicsDevice->_Begin(); 
}

void Graphics::_End(){
    graphicsDevice->_End(); 
}

bool Graphics::HasBegin(){ 
    return graphicsDevice->HasBegin(); 
}

void Graphics::SetCamera(Camera& camera){ 
    graphicsDevice->SetCamera(camera); 
}

Camera Graphics::GetCamera(){ 
    return graphicsDevice->GetCamera(); 
}

void Graphics::BeginRenderToScreen(Vector4 clearColor){
    graphicsDevice->BeginRenderToScreen(clearColor); 
}

void Graphics::EndRenderToScreen(){
    graphicsDevice->EndRenderToScreen(); 
}

void Graphics::Clean(float r, float g, float b, float a){ 
    graphicsDevice->Clean(r, g, b, a); 
}

void Graphics::CleanColorOnly(float r, float g, float b, float a){
    graphicsDevice->CleanColorOnly(r, g, b, a);
}

void Graphics::CleanDepthOnly(){
    graphicsDevice->CleanDepthOnly();
}

void Graphics::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){ 
    graphicsDevice->SetViewport(x, y, w, h); 
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

void Graphics::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix, PerDrawData* perDrawData){ 
    graphicsDevice->DrawMesh(mesh, mat, modelMatrix, perDrawData); 
}

void Graphics::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, Matrix4* animMatrix, int count, PerDrawData* perDrawData){ 
    graphicsDevice->DrawMeshSkinned(mesh, mat, model, animMatrix, count, perDrawData); 
}

void Graphics::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count){ 
    graphicsDevice->DrawMeshInstancing(mesh, mat, animMatrixs, count); 
}

void Graphics::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4x3* animMatrixs, int count){
    graphicsDevice->DrawMeshInstancing(mesh, mat, animMatrixs, count); 
}

void Graphics::DrawMeshInstancing(Mesh& mesh, Material& mat, InstancingBuffer& buffer, int count){
    graphicsDevice->DrawMeshInstancing(mesh, mat, buffer, count); 
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

}