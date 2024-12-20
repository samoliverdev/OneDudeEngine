#include "Terrain1.h"
#include "ProceduralTerrain/MeshGenerator.h"
#include "ProceduralTerrain/Noise.h"

float _Remap(float In, Vector2 InMinMax, Vector2 OutMinMax){
    return OutMinMax.x + (In - InMinMax.x) * (OutMinMax.y - OutMinMax.x) / (InMinMax.y - InMinMax.x);
}

Ref<Mesh> GenerateTerrainMesh1(int width, int height, int levelOfDetail, MeshBorderColaps toColaps){
    float topLeftX = (width - 1) / -2.0f;
    float topLeftZ = (height - 1) / 2.0f;

    int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
    int verticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

    Ref<MeshData> meshData = CreateRef<MeshData>(verticesPerLine, verticesPerLine);
    int vertexIndex = 0;

    for(int y = 0; y < height; y += meshSimplificationIncrement){
        for(int x = 0; x < width; x += meshSimplificationIncrement){
            //meshData->mesh->vertices[vertexIndex] = Vector3(x+topLeftX, 0, -(y-topLeftZ));
            int _x = x;
            int _y = y;
            if(toColaps.left && y % 2 != 0 && x == 0) _y -= 1;
            if(toColaps.right && y % 2 != 0 && x == (width - 1)) _y -= 1;
            if(toColaps.bottom && x % 2 != 0 && y == 0) _x -= 1;
            if(toColaps.top && x % 2 != 0 && y == (height-1)) _x -= 1;

            meshData->mesh->vertices[vertexIndex] = Vector3(_x+topLeftX, 0, -(_y-topLeftZ));
            //meshData->mesh->uv[vertexIndex] = Vector3((float)_x/(float)width, (float)_y/(float)height, 0);
            meshData->mesh->uv[vertexIndex] = Vector3(
                math::clamp(_Remap(_x, Vector2(0, width-1), Vector2(0, 1)), 0.0f, 1.0f), 
                math::clamp(_Remap(_y, Vector2(0, height-1), Vector2(0, 1)), 0.0f, 1.0f), 
                0
            );
            
            if(x < width-1 && y < height-1){
                meshData->AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
                meshData->AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
            }

            vertexIndex += 1;
        }
    }

    meshData->shapeData = CreateMeshShapeData(meshData->mesh->vertices, meshData->mesh->indices);
    meshData->mesh->CalculateNormals();
    meshData->mesh->Submit();

    return meshData->mesh;
}

void Combine2(std::vector<std::vector<std::string>> terms, std::string accum, std::vector<std::string>& combinations){
    bool last = (terms.size() == 1);
    int n = terms[0].size();
    for(int i = 0; i < n; i++){
        std::string item = accum + terms[0][i];
        if(last){
            combinations.push_back(item);
        } else{
            auto newTerms = terms;
            newTerms.erase(newTerms.begin());
            Combine2(newTerms, item, combinations);
        }
    }
}

Terrain1::TerrainLod Terrain1::GetTerrainLod(int chunkSize, int lod){
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

void Terrain1::OnStart(){
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

    terrainMaterial = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    terrainMaterial->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png"));

    meshTest = GenerateTerrainMesh1(mapChunkSize, mapChunkSize, 0, MeshBorderColaps{false, false, false, false});

    int startLoadRange = 1;
    for(int x = -startLoadRange; x <= startLoadRange; x++){
        for(int y = -startLoadRange; y <= startLoadRange; y++){
            LoadCood(IVector2(x, y));
        }
    }
}

void Terrain1::OnDestroy(){

}

void Terrain1::OnUpdate(){
    OD_PROFILE_SCOPE("Terrain1::OnUpdate");

    TransformComponent& camTrans = GetEntity().GetScene()->GetMainCamera().GetComponent<TransformComponent>();
    Vector2 viewPos = Vector2(camTrans.Position().x, -camTrans.Position().z);

    for(auto& i: loadedChunks){
        Vector3 pos(i.first.x * (float)chunkSize, 0, -(i.first.y * (float)chunkSize));
        i.second.lodInfo.lod = lods.size()-1;

        for(int j = 0; j < lods.size(); j++){
            if(math::distance(camTrans.Position(), pos) <= lods[j].visibleDstThreshold){
                i.second.lodInfo.lod = j;
                break;
            }
        }

        //loadedCoords[i.first].GetComponent<MeshRendererComponent>().mesh = lodsMesh[i.second.lod].mesh;
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

        if(loadedChunks.count(i.first + IVector2(1, 0))){
            loadedChunks[i.first].entity.GetComponent<MeshRendererComponent>().material->SetTexture(
                "heightMapWest", loadedChunks[i.first + IVector2(1, 0)].heightmap
            );
        }
        if(loadedChunks.count(i.first + IVector2(0, 1))){
            loadedChunks[i.first].entity.GetComponent<MeshRendererComponent>().material->SetTexture(
                "heightMapNorth", loadedChunks[i.first + IVector2(0, 1)].heightmap
            );
        }

        //MeshBorderColaps t = i.second.borders;
        //LogInfo("%d %d %d %d", t.left ? 1 : 0, t.right ? 1 : 0, t.top ? 1 : 0, t.bottom ? 1 : 0);

        loadedChunks[i.first].entity.GetComponent<MeshRendererComponent>().mesh = lodsMesh[lod].meshs[borders];
    }
}

void Terrain1::LoadCood(IVector2 coord){
    ChunkData chunkData;
    chunkData.lodInfo = LodInfo();

    chunkData.entity = this->GetEntity().GetScene()->AddEntity("Chunk");
    this->GetEntity().GetScene()->SetParent(this->GetEntity(), chunkData.entity);

    Vector3 pos(coord.x * (float)chunkSize, 0, -(coord.y * (float)chunkSize));
    chunkData.entity.GetComponent<TransformComponent>().LocalPosition(pos);
    //terrain.GetComponent<TransformComponent>().LocalScale(Vector3(256, 1, 256));
    //chunkData.entity.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(0, 90, 0));

    int heightmapSize = mapChunkSize;
    int heightmapSize2 = heightmapSize-1;
    Ref<NoiseData> dataTest = Noise::GenerateNoiseMap(heightmapSize, heightmapSize, 50, 0.25f/1, 4, 0.5f, 2.0f, Vector2(coord.x * heightmapSize2, coord.y * heightmapSize2));
    //dataTest->Invert();
    chunkData.heightmap = CreateRef<Texture2D>(
        (void*)&dataTest->data[0],
        (size_t)(dataTest->data.size() * sizeof(float)),
        heightmapSize, heightmapSize,
        TextureDataType::Float,
        Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
    );

    MeshRendererComponent& terrainMeshRenderer = chunkData.entity.AddComponent<MeshRendererComponent>();
    terrainMeshRenderer.mesh = meshTest;
    terrainMeshRenderer.UpdateAABB();
    //terrainMeshRenderer.material = terrainMaterial;
    terrainMeshRenderer.material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Sandbox/Shaders/TerrainClipmapMesh.glsl"));
    terrainMeshRenderer.material->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png"));
    terrainMeshRenderer.material->SetVector4("color", Vector4(1, 1, 1, 1));
    terrainMeshRenderer.material->SetTexture("heightMap", chunkData.heightmap);
    terrainMeshRenderer.material->SetFloat("heightScale", 100);

    HeightmapColliderComponent& heightmapCollider = chunkData.entity.AddComponent<HeightmapColliderComponent>();
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
    }

    loadedChunks[coord] = chunkData;
}