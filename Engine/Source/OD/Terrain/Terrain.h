#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Graphics/Texture.h"
#include "OD/Graphics/Material.h"
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

class Mesh;

struct OD_API TerrainComponent{
    friend class TerrainSystem;

    float lodBias = 1;
    float terrainWidth = 1000/2;
    float terrainLength = 1000/2;
    float terrainHeight = 500/2;

    Vector2 texTilling = {1, 1};

    Ref<Texture2D> splatmap = nullptr;
    Ref<Texture2D> layer0 = nullptr;
    Ref<Texture2D> layer0Normal = nullptr;
    Ref<Texture2D> layer1 = nullptr;
    Ref<Texture2D> layer2 = nullptr;
    Ref<Texture2D> layer3 = nullptr;
    Ref<Texture2D> layer4 = nullptr;

    int mapChunkSize = (128*1) + 1;
    int chunkWidthCount = 4*2;
    int meshToNavmeshLod = 8;
    int meshToNavmeshChunkXCount = 1;
    int meshToNavmeshChunkYCount = 1;
    
    inline Ref<Heightmap> GetHeightmap(){ return heightmap; }
    void SetHeightmap(Ref<Heightmap> heightmap);
    //void SubmitHeightmap();

    static void OnGui(Entity e, Scene& scene);

    template <class Archive>
    void serialize(Archive& ar){
        //ArchiveDumpNVP(ar, heightmap);
        ArchiveDumpNVP(ar, lodBias);
        ArchiveDumpNVP(ar, terrainWidth);
        ArchiveDumpNVP(ar, terrainLength);
        ArchiveDumpNVP(ar, terrainHeight);
        ArchiveDumpNVP(ar, texTilling);

        AssetRefSerialize<Heightmap> _heightmap(heightmap);
        ArchiveDump(ar, CEREAL_NVP(_heightmap));

        AssetRefSerialize<Texture2D> _splatmap(splatmap);
        ArchiveDump(ar, CEREAL_NVP(_splatmap));

        AssetRefSerialize<Texture2D> _layer0(layer0);
        ArchiveDump(ar, CEREAL_NVP(_layer0));
        AssetRefSerialize<Texture2D> _layer0Normal(layer0Normal);
        ArchiveDump(ar, CEREAL_NVP(_layer0Normal));
        AssetRefSerialize<Texture2D> _layer1(layer1);
        ArchiveDump(ar, CEREAL_NVP(_layer1));
        AssetRefSerialize<Texture2D> _layer2(layer2);
        ArchiveDump(ar, CEREAL_NVP(_layer2));
        AssetRefSerialize<Texture2D> _layer3(layer3);
        ArchiveDump(ar, CEREAL_NVP(_layer3));
        AssetRefSerialize<Texture2D> _layer4(layer4);
        ArchiveDump(ar, CEREAL_NVP(_layer4));
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

    struct ChunkData{
        Entity entity;
        LodInfo lodInfo;
    };

    Ref<Heightmap> heightmap = nullptr;
    Ref<Texture2D> heightmapTex = nullptr;
    Ref<Texture2D> normalTex = nullptr;
    
    Entity meshsRoot = EntityNull;
    Entity collider = EntityNull;
    Entity meshToNavmesh = EntityNull;

    Ref<Material> mat = nullptr;
    Ref<Material> matShadow = nullptr;

    std::vector<Entity> meshToNavmeshChunks;

    int chunkSize;
    std::unordered_map<IVector2, ChunkData> loadedChunks;
    std::vector<int> lods;
    std::vector<TerrainLod> lodsMesh;

    bool isDirt = true;
    bool heightMapIsDirt = true; //false;

    void CreateMeshToNavmesh(Scene& scene);
};

class OD_API TerrainSystem: public System{
public:
    void OnInit(Scene& scene);

    virtual int Type() override { return SystemType::Physics; }
    virtual void PhysicsUpdate(Scene& scene) override;

private:
    void DestroyTerrain(TerrainComponent& terrain);
    void CreateTerrain(TerrainComponent& terrain, Entity e);
    void UpdateTerrainData(TerrainComponent& terrain);
    void UpdateTerrain(TerrainComponent& terrain);
    void LoadCood(TerrainComponent& terrain, IVector2 coor);
    TerrainComponent::TerrainLod GetTerrainLod(int chunkSize, int lod);

    Scene* scene;
};

void TerrainModuleInit();

}