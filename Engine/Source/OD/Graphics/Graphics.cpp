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

#include "OD/Platform/OpenGL/OpenGLGraphicsDevice.h"

namespace OD{

void GraphicsModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Material>(".material", [](const std::string& path){ return AssetManager::Get().LoadAsset<Material>(path); });

    LuaBindsDB::Get().RegisterLuaBind<Camera>();
    LuaBindsDB::Get().RegisterLuaBind<Cubemap>();
    LuaBindsDB::Get().RegisterLuaBind<Font>();
    LuaBindsDB::Get().RegisterLuaBind<Framebuffer>();
    LuaBindsDB::Get().RegisterLuaBind<Graphics>();
    LuaBindsDB::Get().RegisterLuaBind<Material>();
    LuaBindsDB::Get().RegisterLuaBind<Texture2D>();
}

GraphicsDevice* curDevice = nullptr;

GraphicsDevice& Graphics::Device(){ return *curDevice; }

void Graphics::Initialize(){
    #ifdef OPENGL_SUPPORT 
    curDevice = new OpenGLGraphicsDevice();
    #endif

    curDevice->Initialize();
}

void Graphics::Shutdown(){
    curDevice->Shutdown();
    delete curDevice;
    curDevice = nullptr;
}

void Graphics::CreateLuaBind(sol::state& lua){
    return;
    /*
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
    lua.new_enum(
        "GraphicsRenderMode",
        "SHADED", Graphics::RenderMode::SHADED,
        "WIREFRAME", Graphics::RenderMode::WIREFRAME
    );
    lua.new_usertype<Graphics>(
        "Graphics",
        "GetDrawCallsCount", Graphics::GetDrawCallsCount,
        "GetVerticesCount", Graphics::GetVerticesCount,
        "GetTrisCount", Graphics::GetTrisCount,
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
        "DrawMesh", Graphics::DrawMesh,
        "DrawMeshInstancing", Graphics::DrawMeshInstancing,
        "DrawModel", Graphics::DrawModel,
        "AddDrawLineCommand", Graphics::AddDrawLineCommand,
        "DrawLinesComamnd", Graphics::DrawLinesComamnd,
        "DrawLine", sol::overload(
            [](Vector3 start, Vector3 end, Vector3 color, int lineWidth){ Graphics::DrawLine(start, end, color, lineWidth);},
            [](Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth){ Graphics::DrawLine(model, start, end, color, lineWidth);}
        ),
        "DrawText", sol::overload(
            [](Font& f, SubShader& s, std::string text, Vector3 pos, float scale){ Graphics::DrawText(f, s, text, pos, scale); },
            [](Font& f, SubShader& s, std::string text, Matrix4 model){ Graphics::DrawText(f, s, text, model); }
        ),
        "SetViewport", Graphics::SetViewport,
        "GetViewport", Graphics::GetViewport,
        "SetRenderMode", Graphics::SetRenderMode,
        "SetDepthMask", Graphics::SetDepthMask,
        "SetDepthTest", Graphics::SetDepthTest,
        "SetCullFace", Graphics::SetCullFace,
        "SetBlend", Graphics::SetBlend,
        "SetBlendFunc", Graphics::SetBlendFunc,
        "BeginFramebuffer", Graphics::BeginFramebuffer,
        "BlitQuadPostProcessing", Graphics::BlitQuadPostProcessing,
        "BlitFramebuffer", Graphics::BlitFramebuffer
    );
    */
}

}