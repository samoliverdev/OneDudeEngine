#pragma once
#include <OD/OD.h>
#include "Terrain1.h"

using namespace OD;

struct NoiseData;

class Terrain2: public Script{
public:
    struct TerrainLod{
        Vector3 scale;
        std::unordered_map<MeshBorderColaps, Ref<Mesh>> meshs; 
    };

    struct LodInfo{
        int lod = 0;
        MeshBorderColaps borders;
    };

    struct LODDef {
        int lod;
        float visibleDstThreshold;
    };

    struct ChunkData{
        Entity entity;
        LodInfo lodInfo;
        Ref<Texture2D> heightmap;
    };

    inline static const int mapChunkSize = (128*1) + 1;
    inline static const int lodCounts = 4;
    inline static const int chunkWidthCount = 4;
    TransformComponent* viewer;

    virtual void OnStart() override;
    virtual void OnDestroy() override;
    virtual void OnUpdate() override;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, heightmap);
        ArchiveDumpNVP(ar, terrainWidth);
        ArchiveDumpNVP(ar, terrainLength);
        ArchiveDumpNVP(ar, terrainHeight);
    }

    static inline void OnGui(Entity e, Scene& scene){}

private:
    Ref<NoiseData> heightmap;
    Ref<Texture2D> heightmapTex;
    float terrainWidth = 1000/2;
    float terrainLength = 1000/2;
    float terrainHeight = 500/2;

    int chunkSize;
    std::unordered_map<IVector2, ChunkData> loadedChunks;
    std::vector<LODDef> lods;
    std::vector<TerrainLod> lodsMesh;

    void LoadCood(IVector2 coor);
    TerrainLod GetTerrainLod(int chunkSize, int lod);
};
