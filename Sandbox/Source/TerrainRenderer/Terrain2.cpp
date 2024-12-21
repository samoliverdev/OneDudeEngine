#include "Terrain2.h"
#include "ProceduralTerrain/MeshGenerator.h"
#include "ProceduralTerrain/Noise.h"

extern float _Remap(float In, Vector2 InMinMax, Vector2 OutMinMax);
extern Ref<Mesh> GenerateTerrainMesh1(int width, int height, int levelOfDetail, MeshBorderColaps toColaps);
extern void Combine2(std::vector<std::vector<std::string>> terms, std::string accum, std::vector<std::string>& combinations);

Terrain2::TerrainLod Terrain2::GetTerrainLod(int chunkSize, int lod){
    TerrainLod out;

    int divider = lod + 1; 
    int width = (chunkSize / divider) + 1;
    int height = (chunkSize / divider) + 1;

    out.scale = Vector3(
        ((float)chunkSize / (float)(chunkSize/divider)), 
        1, 
        ((float)chunkSize / (float)(chunkSize/divider))
    );
    //out.scale = Vector3(1.0f * (lod + 1.0f));

    //LogInfo("-------------BoolVariantCreateTest------------");
    std::vector<std::vector<std::string>> multCompile{
        std::vector<std::string>{ "0", "1"},
        std::vector<std::string>{ "0", "1"},
        std::vector<std::string>{ "0", "1"},
        std::vector<std::string>{ "0", "1"},
    };
    std::vector<std::string> combinations;
    Combine2(multCompile, std::string(""), combinations);
    for(std::string s: combinations){
        //LogInfo("%s", s.c_str());

        MeshBorderColaps mbc(s);
        out.meshs[mbc] = GenerateTerrainMesh1(width, height, 0, mbc);
    }

    return out;
}

void Terrain2::OnStart(){
    chunkSize = mapChunkSize - 1;
    lods = std::vector<LODDef>{
        {0, 100}, 
        {1, 200}, 
        {3, 300},
        {7, 400},
        //{15, 500},
        //{31, 600},
        //{63, 700}
    };

    for(int i = 0; i < lods.size(); i++){
        lodsMesh.push_back(GetTerrainLod(chunkSize, lods[i].lod));
    }

    int heightmapSize = 1024;
    int heightmapSize2 = heightmapSize-1;
    heightmap = Noise::GenerateNoiseMap(heightmapSize, heightmapSize, 50, 0.25f/4, 4, 0.5f, 2.0f, Vector2(0, 0));
    heightmapTex = Texture2D::CreateFromRaw( //CreateRef<Texture2D>(
        (void*)&heightmap->data[0],
        (size_t)(heightmap->data.size() * sizeof(float)),
        heightmapSize, heightmapSize,
        TextureDataType::Float,
        Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
    );

    Entity collider = GetEntity().GetScene()->AddEntity("Collider");
    this->GetEntity().GetScene()->SetParent(this->GetEntity(), collider);

    float terrainMeshWidth = (float)(chunkSize * chunkWidthCount);
    TransformComponent& colliderTrans = collider.GetComponent<TransformComponent>();
    colliderTrans.LocalScale(Vector3(
        terrainMeshWidth / (float)heightmapSize,
        terrainHeight, 
        terrainMeshWidth / (float)heightmapSize
    ));
    colliderTrans.LocalPosition(
        Vector3(
            terrainMeshWidth / 2.0f,
            terrainHeight/2,
            -(terrainMeshWidth / 2.0f)
        )
    );
    
    HeightmapColliderComponent& heightmapCollider = collider.AddComponent<HeightmapColliderComponent>();
    heightmapCollider.width = heightmapSize;
    heightmapCollider.length = heightmapSize;
    heightmapCollider.scale = heightmapSize;
    heightmapCollider.minHeight = 0;
    heightmapCollider.maxHeight = 1; //terrainHeight;
    heightmap->TransposeTo(heightmapCollider.heights);
    //heightmapCollider.offset = Vector3(0, 100/2, 0);
    //heightmap->Invert();
    /*for(int i = 0; i < heightmap->data.size(); i++){
        heightmapCollider.heights.push_back(heightmap->data[i] * 1); //terrainHeight
    }*/
    /*heightmapCollider.heights.resize(heightmap->data.size());
    for(int i = 0, j = heightmap->data.size() - heightmapSize; i < heightmap->data.size(); i += heightmapSize, j -= heightmapSize) {
        for(int k = 0; k < heightmapSize; ++k) {
            heightmapCollider.heights[i + k] = heightmap->data[j + k];// * terrainHeight;
            Assert(heightmapCollider.heights[i + k] >= 0);
            Assert(heightmapCollider.heights[i + k] <= terrainHeight);
            //if(heightmapCollider.heights[i + k] < 0) heightmapCollider.heights[i + k] = 0;
        }
    }*/

    for(int x = 0; x < chunkWidthCount; x++){
        for(int y = 0; y < chunkWidthCount; y++){
            LoadCood(IVector2(x, y));
        }
    }
}

void Terrain2::OnDestroy(){

}

void Terrain2::OnUpdate(){
    OD_PROFILE_SCOPE("Terrain1::OnUpdate");

    TransformComponent& camTrans = GetEntity().GetScene()->GetMainCamera().GetComponent<TransformComponent>();
    Vector2 viewPos = Vector2(camTrans.Position().x, -camTrans.Position().z);

    for(auto& i: loadedChunks){
        TransformComponent& trans = i.second.entity.GetComponent<TransformComponent>();

        //Vector3 pos(i.first.x * (float)chunkSize, 0, -(i.first.y * (float)chunkSize));
        Vector3 pos = trans.Position();
        i.second.lodInfo.lod = lods.size()-1;

        for(int j = 0; j < lods.size(); j++){
            if(math::distance(camTrans.Position(), pos) <= lods[j].visibleDstThreshold){
                i.second.lodInfo.lod = j;
                break;
            }
        }

        loadedChunks[i.first].entity.GetComponent<TransformComponent>().LocalScale(
            lodsMesh[i.second.lodInfo.lod].scale
        );
    }  

    for(auto& i: loadedChunks){
        MeshBorderColaps& borders = i.second.lodInfo.borders;
        int& lod = i.second.lodInfo.lod;

        borders = MeshBorderColaps();

        if(loadedChunks.count(i.first + IVector2(-1, 0))) borders.left = lod < loadedChunks[i.first + IVector2(-1, 0)].lodInfo.lod;
        if(loadedChunks.count(i.first + IVector2(1, 0))) borders.right = lod < loadedChunks[i.first + IVector2(1, 0)].lodInfo.lod;
        if(loadedChunks.count(i.first + IVector2(0, 1))) borders.top = lod < loadedChunks[i.first + IVector2(0, 1)].lodInfo.lod;
        if(loadedChunks.count(i.first + IVector2(0, -1))) borders.bottom = lod < loadedChunks[i.first + IVector2(0, -1)].lodInfo.lod;

        MeshRendererComponent& meshComponent = loadedChunks[i.first].entity.GetComponent<MeshRendererComponent>();
        meshComponent.mesh = lodsMesh[lod].meshs[borders];
        meshComponent.boundingVolume = AABB(
            Vector3(0, terrainHeight/2, 0), 
            (chunkSize / lodsMesh[lod].scale.x) / 2, 
            terrainHeight/2, 
            (chunkSize / lodsMesh[lod].scale.z) / 2
        );
    }

    GetEntity().GetComponent<TransformComponent>().LocalScale(
        Vector3(
            terrainWidth / (float)(chunkSize * chunkWidthCount),
            1, 
            terrainLength / (float)(chunkSize * chunkWidthCount)
        )
    );
}

void Terrain2::LoadCood(IVector2 coord){
    ChunkData chunkData;
    chunkData.lodInfo = LodInfo();

    chunkData.entity = this->GetEntity().GetScene()->AddEntity("Chunk");
    this->GetEntity().GetScene()->SetParent(this->GetEntity(), chunkData.entity);

    Vector3 pos(coord.x * (float)chunkSize, 0, -(coord.y * (float)chunkSize));
    pos += Vector3((float)chunkSize/2.0f, 0, -(chunkSize/2.0f));
    chunkData.entity.GetComponent<TransformComponent>().LocalPosition(pos);

    InfoComponent& info = chunkData.entity.GetComponent<InfoComponent>();
    //info.hidden = true;

    MeshRendererComponent& terrainMeshRenderer = chunkData.entity.AddComponent<MeshRendererComponent>();
    terrainMeshRenderer.UpdateAABB();
    terrainMeshRenderer.material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Sandbox/Shaders/TerrainClipmapMesh.glsl"));
    terrainMeshRenderer.material->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png"));
    terrainMeshRenderer.material->SetVector4("color", Vector4(1, 1, 1, 1));

    terrainMeshRenderer.material->SetTexture("heightMap", heightmapTex);
    float offset = 1.0f / (float)chunkWidthCount;
    terrainMeshRenderer.material->SetVector2("heightmapTilling", Vector2(offset, offset));
    terrainMeshRenderer.material->SetVector2("heightmapOffset", Vector2(coord.x * offset, coord.y * offset));
    terrainMeshRenderer.material->SetFloat("heightScale", terrainHeight);

    /*HeightmapColliderComponent& heightmapCollider = chunkData.entity.AddComponent<HeightmapColliderComponent>();
    heightmapCollider.width = heightmapSize;
    heightmapCollider.length = heightmapSize;
    heightmapCollider.scale = heightmapSize;
    heightmapCollider.minHeight = 0;
    heightmapCollider.maxHeight = 100;
    heightmapCollider.offset = Vector3(0, 100/2, 0);
    for(int i = 0; i < dataTest->data.size(); i++){
        heightmapCollider.heights.push_back(dataTest->data[i] * 100);
    }
    heightmapCollider.heights.resize(heightmapSize*heightmapSize);
    for(int i = 0, j = dataTest->data.size() - heightmapSize; i < dataTest->data.size(); i += heightmapSize, j -= heightmapSize) {
        for(int k = 0; k < heightmapSize; ++k) {
            heightmapCollider.heights[i + k] = dataTest->data[j + k] * 100;
            Assert(heightmapCollider.heights[i + k] >= 0);
            Assert(heightmapCollider.heights[i + k] <= 100);
            //if(heightmapCollider.heights[i + k] < 0) heightmapCollider.heights[i + k] = 0;
        }
    }*/

    loadedChunks[coord] = chunkData;
}