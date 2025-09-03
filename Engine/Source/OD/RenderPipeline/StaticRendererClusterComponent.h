#pragma once
#include "OD/Defines.h"
#include "OD/Graphics/Culling.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Model.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Scene.h"
#include "OD/RenderPipeline/RendererList.h"

namespace OD{

//TODO: Check if this is make dont have any individual culling check
struct OD_API StaticRendererClusterComponent{
    friend class StandRenderPipeline;
    friend class RenderContext;

    struct RenderTarget{
        Ref<Mesh> targetMesh;
        Ref<Material> targetMaterial;
        Matrix4 targetMatrix;
        AABB aabb;
    };

    struct SubChunk{
        std::vector<RenderTarget> targets;
        CommandBucket4<Material*, Mesh*, DrawInstancingCommand2> drawIntancingCommands;

        AABB bounds;       // full subchunk region (static, from grid definition)
        AABB renderBounds; // tight bounds from contained models

        IVector3 coord;  // local subchunk coordinate inside chunk

        SubChunk() : bounds({}), renderBounds({}), coord(0) {}

        void RecalculateRenderBounds() {
            renderBounds = {};
            /*for(size_t i = 0; i < models.size(); i++) {
                if (!models[i]) continue;
                AABB modelBounds = models[i]->GetAABB().Transform(modelMatrices[i]);
                renderBounds.Encapsulate(modelBounds);
            }*/
        }

        template <class Archive>
        void serialize(Archive& ar){
            //ArchiveDumpNVP(ar, modelMatrices);
            ArchiveDumpNVP(ar, bounds);
            ArchiveDumpNVP(ar, renderBounds);
            ArchiveDumpNVP(ar, coord);
        }
    };

    struct Chunk {
        AABB bounds;       // full chunk region
        AABB renderBounds; // tight bounds from contained models
        IVector3 coord;  // chunk coordinate in grid
        std::vector<SubChunk> subchunks;

        Chunk():bounds({}), coord(0) {}

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, subchunks);
            ArchiveDumpNVP(ar, bounds);
            ArchiveDumpNVP(ar, coord);
        }
    };

    bool autoCollectChildRenderers = true;
    bool genInstancingCommands = true;

    StaticRendererClusterComponent();

    void Create(glm::ivec3 inchunkCounts, glm::ivec3 insubchunkCounts, float ingridSize);

    bool IsValidPos(const Vector3& pos);

    bool AddModel(const Ref<Mesh>& mesh, const Ref<Material>& material, const Vector3& pos, const Matrix4& transform, const AABB& modelBounds);

    inline int ChunkIndex(glm::ivec3 c) const {
        return (c.z * chunkCounts.y + c.y) * chunkCounts.x + c.x;
    }

    inline int SubChunkIndex(glm::ivec3 sc) const {
        return (sc.z * subchunkCounts.y + sc.y) * subchunkCounts.x + sc.x;
    }

    SubChunk* GetSubChunkAtPos(const glm::vec3& pos);

    void CreateIntancingCommands();

    static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, chunkCounts);
        ArchiveDumpNVP(ar, subchunkCounts);
        ArchiveDumpNVP(ar, gridSize);
        ArchiveDumpNVP(ar, subchunkSize);
        ArchiveDumpNVP(ar, chunks);
    }
private:
    IVector3 chunkCounts;     // number of chunks in x,y,z
    IVector3 subchunkCounts;  // number of subchunks per chunk
    float gridSize;             // size of each chunk in meters

    Vector3 subchunkSize;     // derived size (chunkSize / subchunkCounts)

    std::vector<Chunk> chunks;

    Vector3 posTest;

    bool started = false;
};

}