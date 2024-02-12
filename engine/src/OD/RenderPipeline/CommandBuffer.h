#pragma once

#include "OD/Graphics/Material.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Culling.h"
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

    std::set<Ref<Material>> drawCommandsMaterials;

    bool setRenderTarget = false;
    Framebuffer* renderTarget = nullptr;
    bool clearRenderTarget = false;
    Vector3 clearColor;
    bool setViewport = false;
    IVector4 viewport;

    Ref<Material> overrideMaterial = nullptr;

    bool setCamera = false;
    Camera camera;
};

}