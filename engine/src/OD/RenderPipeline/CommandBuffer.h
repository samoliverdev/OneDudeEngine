#pragma once

#include "OD/Renderer/Material.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Renderer/Framebuffer.h"
#include "OD/Renderer/Renderer.h"
#include "OD/Renderer/Culling.h"
#include "OD/Scene/Scene.h"
#include "OD/Core/Math.h"
#include "RenderPipelineUtils.h"

namespace OD{

struct DrawCommand{
    Ref<Material> material;
    Ref<Mesh> meshs;
    Matrix4 trans;

    bool operator<(const DrawCommand& a) const{
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
    CommandBucket1<MaterialBind2, DrawCommand> drawCommands;
    CommandBucket4<Ref<Material>, Ref<Mesh>, DrawInstancingCommand> drawIntancingCommands;

    inline void Clean(){
        drawCommands.Clear();
        drawIntancingCommands.Clear();
    }

    inline void Sort(){
        drawCommands.Sort();
        drawIntancingCommands.Sort();
    }
};

}