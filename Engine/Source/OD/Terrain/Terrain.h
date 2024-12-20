#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Scene.h"
#include "Heightmap.h"

struct OD_API MeshBorders{
    bool left = false;
    bool right = false;
    bool top = false;
    bool bottom = false;

    MeshBorders(){
        left = false;
        right = false;
        top = false;
        bottom = false;
    }

    MeshBorders(bool a, bool b, bool c, bool d){
        left = a;
        right = b;
        top = c;
        bottom = d;
    }

    MeshBorders(const std::string& p){
        left = p[0] == '1';
        right = p[1] == '1';
        top = p[2] == '1';
        bottom = p[3] == '1';
    }

    bool operator==(const MeshBorders& rhs) const{
        return left == rhs.left && right == rhs.right && top == rhs.top && bottom == rhs.bottom;
    }
};

template <>
class std::hash<MeshBorders>{
public:
    size_t operator()(const MeshBorders &k) const{
        using std::size_t;
        using std::hash;
        using std::vector;
        return hash<vector<bool>>()(
            vector<bool>{k.left, k.right, k.top, k.bottom}
        ); 
    }
};

namespace OD{

class OD_API QuadTree{
public:    
    struct OD_API Node{
        std::vector<Ref<Node>> childen;
        int depth = 0;
        
        inline void Split(){
            auto a = CreateRef<Node>();
            auto b = CreateRef<Node>();
            auto c = CreateRef<Node>();
            auto d = CreateRef<Node>();

            a->depth = depth + 1;
            b->depth = depth + 1;
            c->depth = depth + 1;
            d->depth = depth + 1;

            childen.push_back(a);
            childen.push_back(b);
            childen.push_back(c);
            childen.push_back(d);
        }
    };

    Ref<Node> root = nullptr;
};

struct OD_API QuadTreeTerrainComponent{
    friend class QuadTreeTerrainSystem;

    static inline void OnGui(Entity& e){}
    template <class Archive> void serialize(Archive& ar){}

private:
    float terrainWidth = 1000;
    float terrainLength = 1000;
    float terrainHeight = 500;

    QuadTree quadtree;
};

class OD_API QuadTreeTerrainSystem: public System{
public:
    QuadTreeTerrainSystem(Scene* scene);
    ~QuadTreeTerrainSystem() override;

    virtual SystemType Type() override { return SystemType::Physics; }
    virtual void Update() override;
    virtual void OnDrawGizmos() override;
};

struct OD_API TerrainComponent{
    friend class TerrainSystem;

    float lodBias = 1;
    float terrainWidth = 1000/2;
    float terrainLength = 1000/2;
    float terrainHeight = 500/2;

    Ref<Texture2D> splatmap = nullptr;
    Ref<Texture2D> layer0 = nullptr;
    Ref<Texture2D> layer1 = nullptr;
    Ref<Texture2D> layer2 = nullptr;
    Ref<Texture2D> layer3 = nullptr;
    Ref<Texture2D> layer4 = nullptr;

    void SetHeightmap(Ref<Heightmap> heightmap);
    void SubmitHeightmap();

    static void OnGui(Entity e);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, heightmap);
        ArchiveDumpNVP(ar, lodBias);
        ArchiveDumpNVP(ar, terrainWidth);
        ArchiveDumpNVP(ar, terrainLength);
        ArchiveDumpNVP(ar, terrainHeight);
    }
private:
    
    struct TerrainLod{
        Vector3 scale;
        std::unordered_map<MeshBorders, Ref<Mesh>> meshs; 
    };

    struct LodInfo{
        int lod = 0;
        MeshBorders borders;
    };

    struct LODDef {
        int lod;
        float visibleDstThreshold;
    };

    struct ChunkData{
        Entity entity;
        LodInfo lodInfo;
    };

    int mapChunkSize = (128*1) + 1;
    int lodCounts = 4;
    int chunkWidthCount = 4*2;

    Ref<Heightmap> heightmap = nullptr;
    Ref<Texture2D> heightmapTex = nullptr;
    
    Entity meshsRoot;
    Entity collider;

    int chunkSize;
    std::unordered_map<IVector2, ChunkData> loadedChunks;
    std::vector<LODDef> lods;
    std::vector<TerrainLod> lodsMesh;
};

class OD_API TerrainSystem: public System{
public:
    TerrainSystem(Scene* scene);
    ~TerrainSystem() override;

    virtual SystemType Type() override { return SystemType::Physics; }
    virtual void Update() override;

private:
    void CreateTerrain(TerrainComponent& terrain, EntityId e);
    void UpdateTerrain(TerrainComponent& terrain);
    void LoadCood(TerrainComponent& terrain, IVector2 coor);
    TerrainComponent::TerrainLod GetTerrainLod(int chunkSize, int lod);
};

void TerrainModuleInit();

}