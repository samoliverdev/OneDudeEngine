#pragma once

#include "OD/Renderer/Material.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Renderer/Framebuffer.h"
#include "OD/Renderer/Renderer.h"
#include "OD/Renderer/Culling.h"
#include "OD/Scene/Scene.h"
#include "OD/Core/Math.h"
#include "RenderPipelineUtils.h"
#include <functional>

namespace OD{

struct DrawCommand{
    Ref<Material> material;
    Ref<Mesh> meshs;
    Matrix4 trans;
    float distance;

    bool operator<(const DrawCommand& a) const{
        return material->MaterialId() < a.material->MaterialId();
    }
};

struct SkinnedDrawCommand{
    Ref<Material> material;
    Ref<Mesh> meshs;
    Matrix4 trans;
    std::vector<Matrix4>* posePalette;
    float distance;

    bool operator<(const SkinnedDrawCommand& a) const{
        return material->MaterialId() < a.material->MaterialId();
    }
};

struct DrawInstancingCommand{
    Ref<Material> material;
    Ref<Mesh> meshs;
    std::vector<Matrix4> trans;

    bool operator<(const DrawCommand& a) const{
        return material->MaterialId() < a.material->MaterialId();
    }
};

struct MaterialBind2{
    float distance;
    uint32_t materialId;

    bool operator<(const MaterialBind2& a) const{
        return materialId < a.materialId;
    }
};

struct CommandBuffer{
    friend class RenderContext;

    enum class SortType{None, CommonOpaque, CommonTransparent};

    std::string name = "CommandBuffer";
    std::function<void(Material& material)> onUpdateMaterial = nullptr;
    SortType sortType;

    void SetCamera(Camera inCamera);
    void CleanRenderTarget(Vector3 inClearColor);
    void SetRenderTarget(Framebuffer* framebuffer);
    void SetViewport(IVector4 inViewport);

    void AddGlobalShader(Ref<Shader> shader);
    void SetGlobalFloat(const char* name, float value);
    void SetGlobalVector2(const char* name, Vector2 value);
    void SetGlobalVector3(const char* name, Vector3 value);
    void SetGlobalVector4(const char* name, Vector4 value);
    void SetGlobalMatrix4(const char* name, Matrix4 value);
    void SetGlobalTexture(const char* name, Ref<Texture2D> tex);
    void SetGlobalTexture(const char* name, Framebuffer* tex);
    void SetGlobalCubemap(const char* name, Ref<Cubemap> tex);

    void SetOverrideMaterial(Ref<Material> shader);

    void AddDrawCommand(DrawCommand comand, float distance = 0);  
    void AddDrawInstancingCommand(DrawCommand comand);
    void AddSkinnedDrawCommand(SkinnedDrawCommand comand, float distance = 0); 
    
    void Clean();
    void Sort();
    void Submit();

private:
    CommandBucket0<DrawCommand> drawCommands;
    //CommandBucket1<MaterialBind2, DrawCommand> drawCommands;
    CommandBucket4<Ref<Material>, Ref<Mesh>, DrawInstancingCommand> drawIntancingCommands;
    CommandBucket1<MaterialBind2, SkinnedDrawCommand> skinnedDrawCommands;

    bool setRenderTarget = false;
    Framebuffer* renderTarget = nullptr;
    bool clearRenderTarget = false;
    Vector3 clearColor;
    bool setViewport = false;
    IVector4 viewport;
    Material globalMaterial;
    std::set<Ref<Shader>> globalShaders;

    Ref<Material> overrideMaterial = nullptr;

    bool setCamera = false;
    Camera camera;
};

}