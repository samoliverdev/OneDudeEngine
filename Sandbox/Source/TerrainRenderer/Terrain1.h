#pragma once
#include <OD/OD.h>

using namespace OD;

struct MeshBorderColaps{
    bool left = false;
    bool right = false;
    bool top = false;
    bool bottom = false;

    MeshBorderColaps(){
        left = false;
        right = false;
        top = false;
        bottom = false;
    }

    MeshBorderColaps(bool a, bool b, bool c, bool d){
        left = a;
        right = b;
        top = c;
        bottom = d;
    }

    MeshBorderColaps(const std::string& p){
        left = p[0] == '1';
        right = p[1] == '1';
        top = p[2] == '1';
        bottom = p[3] == '1';
    }

    bool operator==(const MeshBorderColaps& rhs) const{
        return left == rhs.left && right == rhs.right && top == rhs.top && bottom == rhs.bottom;
    }
};

template <>
class std::hash<MeshBorderColaps>{
public:
    size_t operator()(const MeshBorderColaps &k) const{
        using std::size_t;
        using std::hash;
        using std::vector;
        return hash<vector<bool>>()(
            vector<bool>{k.left, k.right, k.top, k.bottom}
        ); 
    }
};

class Terrain1: public Script{
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
    TransformComponent* viewer;

    template <class Archive>
    void serialize(Archive& ar){
        //ArchiveDumpNVP(ar, viewerPosition);
    }

    virtual void OnStart() override;
    virtual void OnDestroy() override;
    virtual void OnUpdate() override;

private:
    int chunkSize;

    Ref<Material> terrainMaterial;
    std::unordered_map<IVector2, ChunkData> loadedChunks;
    
    std::vector<LODDef> lods;
    std::vector<TerrainLod> lodsMesh;

    Ref<Mesh> meshTest;

    void LoadCood(IVector2 coor);

    TerrainLod GetTerrainLod(int chunkSize, int lod);
};
