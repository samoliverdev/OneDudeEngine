#include "StaticRendererClusterComponent.h"
#include "OD/Core/ImGui.h"

namespace OD{

void StaticRendererClusterComponent::OnGui(Entity& e, Scene& scene){
    StaticRendererClusterComponent& c = scene.GetComponent<StaticRendererClusterComponent>(e);

    if(ImGui::DragInt3("chunkCounts", &c.chunkCounts.x)){
        c.Create(c.chunkCounts, c.subchunkCounts, c.gridSize);
    }
    if(ImGui::DragInt3("subchunkCounts", &c.subchunkCounts.x)){
        c.Create(c.chunkCounts, c.subchunkCounts, c.gridSize);
    }
    if(ImGui::DragFloat("gridSize", &c.gridSize)){
        c.Create(c.chunkCounts, c.subchunkCounts, c.gridSize);
    }
}

StaticRendererClusterComponent::StaticRendererClusterComponent(){
    Create({5, 1, 5}, {4, 2, 4}, 100);
}

void StaticRendererClusterComponent::Create(glm::ivec3 inchunkCounts, glm::ivec3 insubchunkCounts, float ingridSize){
    chunkCounts = inchunkCounts;
    subchunkCounts = insubchunkCounts; 
    gridSize = ingridSize;

    chunks.resize(chunkCounts.x * chunkCounts.y * chunkCounts.z);

    glm::vec3 chunkSize(gridSize);
    subchunkSize = chunkSize / glm::vec3(subchunkCounts);

    // pre-allocate chunks + subchunks and assign AABBs
    for (int cz = 0; cz < chunkCounts.z; cz++) {
        for (int cy = 0; cy < chunkCounts.y; cy++) {
            for (int cx = 0; cx < chunkCounts.x; cx++) {
                glm::ivec3 cCoord(cx, cy, cz);
                Chunk& c = chunks[ChunkIndex(cCoord)];
                c.coord = cCoord;

                // chunk center in world space
                glm::vec3 cCenter = (glm::vec3(cCoord) + 0.5f) * chunkSize;
                glm::vec3 cHalfExtents = chunkSize * 0.5f;
                c.bounds = AABB(cCenter - cHalfExtents, cCenter + cHalfExtents);

                c.subchunks.resize(subchunkCounts.x * subchunkCounts.y * subchunkCounts.z);

                for (int sz = 0; sz < subchunkCounts.z; sz++) {
                    for (int sy = 0; sy < subchunkCounts.y; sy++) {
                        for (int sx = 0; sx < subchunkCounts.x; sx++) {
                            glm::ivec3 sCoord(sx, sy, sz);
                            SubChunk& s = c.subchunks[SubChunkIndex(sCoord)];
                            s.coord = sCoord;

                            glm::vec3 sCenter = (glm::vec3(sCoord) + 0.5f) * subchunkSize
                                                + glm::vec3(cCoord) * chunkSize;
                            glm::vec3 sHalfExtents = subchunkSize * 0.5f;
                            s.bounds = AABB(sCenter - sHalfExtents, sCenter + sHalfExtents);

                            s.renderBounds = {};
                        }
                    }
                }
            }
        }
    }
}

bool StaticRendererClusterComponent::IsValidPos(const Vector3& pos){
    glm::ivec3 chunkCoord = glm::floor(pos / gridSize);
    if(
        chunkCoord.x < 0 || chunkCoord.y < 0 || chunkCoord.z < 0 ||
        chunkCoord.x >= chunkCounts.x || chunkCoord.y >= chunkCounts.y || chunkCoord.z >= chunkCounts.z
    ){
        return false; // outside grid
    }

    return true;
}

bool StaticRendererClusterComponent::AddModel(const Ref<Mesh>& mesh, const Ref<Material>& material, const Vector3& pos, const Matrix4& transform, const AABB& modelBounds){
    if(!mesh) return false;

    // Compute chunk position
    glm::ivec3 chunkCoord = glm::floor(pos / gridSize);
    if(chunkCoord.x < 0 || chunkCoord.y < 0 || chunkCoord.z < 0 ||
        chunkCoord.x >= chunkCounts.x || chunkCoord.y >= chunkCounts.y || chunkCoord.z >= chunkCounts.z) {
        return false; // outside grid
    }

    Chunk& chunk = chunks[ChunkIndex(chunkCoord)];

    // Compute subchunk position inside chunk
    glm::vec3 localPos = pos - glm::vec3(chunkCoord) * gridSize;
    glm::ivec3 subCoord = glm::ivec3(
        glm::clamp((int)glm::floor(localPos.x / subchunkSize.x), 0, subchunkCounts.x - 1),
        glm::clamp((int)glm::floor(localPos.y / subchunkSize.y), 0, subchunkCounts.y - 1),
        glm::clamp((int)glm::floor(localPos.z / subchunkSize.z), 0, subchunkCounts.z - 1)
    );

    SubChunk& sub = chunk.subchunks[SubChunkIndex(subCoord)];

    sub.targets.push_back({
        mesh, material, transform, modelBounds
    });

    sub.renderBounds.Encapsulate(modelBounds);
    chunk.renderBounds.Encapsulate(modelBounds);

    return true;
}

StaticRendererClusterComponent::SubChunk* StaticRendererClusterComponent::GetSubChunkAtPos(const glm::vec3& pos){
    glm::ivec3 chunkCoord = glm::floor(pos / gridSize);
    if (chunkCoord.x < 0 || chunkCoord.y < 0 || chunkCoord.z < 0 ||
        chunkCoord.x >= chunkCounts.x || chunkCoord.y >= chunkCounts.y || chunkCoord.z >= chunkCounts.z) {
        return nullptr; // outside grid
    }

    Chunk& chunk = chunks[ChunkIndex(chunkCoord)];

    glm::vec3 localPos = pos - glm::vec3(chunkCoord) * gridSize;
    glm::ivec3 subCoord = glm::ivec3(
        glm::clamp((int)glm::floor(localPos.x / subchunkSize.x), 0, subchunkCounts.x - 1),
        glm::clamp((int)glm::floor(localPos.y / subchunkSize.y), 0, subchunkCounts.y - 1),
        glm::clamp((int)glm::floor(localPos.z / subchunkSize.z), 0, subchunkCounts.z - 1)
    );

    return &chunk.subchunks[SubChunkIndex(subCoord)];
}

}