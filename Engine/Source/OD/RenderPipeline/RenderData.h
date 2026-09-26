#pragma once
#include "OD/Defines.h"
#include "OD/Core/AlignedAllocator.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Culling.h"
#include "OD/Core/Resource.h"
#include <vector>

namespace OD{

class Mesh;
class UniformBuffer;
class Material;
class InstancingBuffer;

struct RenderDataCullingView{
    Frustum frustum;
    uint32_t visibilityBit = 0;
};

inline uint32_t ComputeRenderDataCullingMask(
    AABB& bounds,
    const std::vector<RenderDataCullingView>& views,
    bool alwaysDraw
){
    uint32_t mask = 0;
    for(const RenderDataCullingView& view : views){
        if(alwaysDraw || bounds.isOnFrustum(view.frustum)){
            mask |= view.visibilityBit;
        }
    }
    return mask;
}

struct OD_API alignas(16) RenderData{
    enum Flag : uint32_t {
        AlwaysDraw        = 1 << 0,
        RenderShadow      = 1 << 1,
        IsDecal           = 1 << 2,
        IsValid           = 1 << 3,

        IsStatic = 1 << 4,
        IsParticle = 1 << 5,
        
        FromModel         = 1 << 6,
        FromMesh          = 1 << 7,
        FromSkinnedModel         = 1 << 8,
        FromSkinnedMesh          = 1 << 9,
        FromCluster          = 1 << 10,
    };

    Matrix4 targetMatrix;
    //TODO: Depreaced, remove later 
    //PerDrawData perDrawData; 
    AABB aabb;
    AlignedVector<Matrix4>* posePalette = nullptr;
    uint32_t skinnedBuffer = INVALID_RESOURCE_ID;
    uint32_t targetMaterial = INVALID_RESOURCE_ID;
    uint32_t customShadowPass = INVALID_RESOURCE_ID;
    uint32_t instancingBuffer = INVALID_RESOURCE_ID;
    uint32_t targetMesh = INVALID_RESOURCE_ID;
    float distance;
    uint32_t flags = Flag::RenderShadow | Flag::IsValid;//  0;//INFO: This very simple otimization give 2x more performace!!!!!!!!!!!!!!!!!!
    int layer = 0;

    inline void SetFlag(RenderData::Flag flag, bool enabled){
        if(enabled){
            flags |= flag;
        } else {
            flags &= ~flag;
        }
    }

    inline bool HasFlag(RenderData::Flag flag) const {
        return (flags & flag) != 0;
    }
};

}
