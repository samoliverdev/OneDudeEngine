#pragma once
#include "OD/Defines.h"
#include "OD/Core/AlignedAllocator.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Culling.h"

namespace OD{

class Mesh;
class UniformBuffer;
class Material;
class InstancingBuffer;

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
    PerDrawData perDrawData;
    AABB aabb;
    AlignedVector<Matrix4>* posePalette = nullptr;
    UniformBuffer* skinnedBuffer = nullptr;
    Material* targetMaterial;
    Material* customShadowPass = nullptr;
    InstancingBuffer* instancingBuffer = nullptr;
    Mesh* targetMesh;
    float distance;
    uint32_t flags = Flag::RenderShadow | Flag::IsValid;//  0;//INFO: This very simple otimization give 2x more performace!!!!!!!!!!!!!!!!!!
    int layer = 0;
    /*bool awalsDraw = false;
    bool renderShadow = true;
    bool isDecal = false;
    bool isValid = true;*/

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