#include "Terrain.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/Navmesh/Navmesh.h"
#include "OD/Core/Instrumentor.h"

namespace OD{

void TerrainModuleInit(){
    SceneManager::Get().RegisterCoreComponent<TerrainComponent>("TerrainComponent");
    SceneManager::Get().RegisterSystem<TerrainSystem>("TerrainSystem");
}

int ManhattanDistance(IVector2 a, IVector2 b){
    return math::abs(a.x - b.x) + math::abs(a.y - b.y);
}

int ManhattanDistance(IVector3 a, IVector3 b){
    return math::abs(a.x - b.x) + math::abs(a.y - b.y) + math::abs(a.z - b.z);
}

void TerrainComponent::OnGui(Entity e, Scene& scene){
    TerrainComponent& terrain = scene.GetComponent<TerrainComponent>(e);

    ImGui::DragFloat("lodBias", &terrain.lodBias);
    ImGui::DragFloat("terrainWidth", &terrain.terrainWidth);
    ImGui::DragFloat("terrainLength", &terrain.terrainLength);
    ImGui::DragFloat("terrainHeight", &terrain.terrainHeight);
    ImGui::DragInt("chunkWidthCount", &terrain.chunkWidthCount);

    if(ImGui::Button("Rebuild")) terrain.isDirt = true;
}

void TerrainComponent::SetHeightmap(Ref<Heightmap> inHeightmap){
    heightmap = inHeightmap;
    heightMapIsDirt = true;
    //isDirt = true;
    ///SubmitHeightmap();
}

/*void TerrainComponent::SubmitHeightmap(){
    Assert(false);
    
    int heightmapSize = 1024;
    if(heightmap == nullptr) heightmap = CreateRef<Heightmap>(heightmapSize, heightmapSize);
    heightmapTex = Texture2D::CreateFromRaw(
        (void*)&heightmap->data[0],
        (size_t)(heightmap->data.size() * sizeof(float)),
        heightmap->width, heightmap->height,
        TextureDataType::Float,
        Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
    );

    for(auto& i: loadedChunks){
        IVector2 coord = i.first;
        MeshRendererComponent& terrainMeshRenderer =  i.second.entity.AddComponent<MeshRendererComponent>();
        float offset = 1.0f / (float)chunkWidthCount;

        terrainMeshRenderer.UpdateAABB();
        terrainMeshRenderer.material->SetVector4("color", Vector4(1, 1, 1, 1));
        terrainMeshRenderer.material->SetTexture("heightMap", heightmapTex);
        //terrainMeshRenderer.material->SetTexture("heightMapNormal", normalTex);
        terrainMeshRenderer.material->SetVector2("heightmapTilling", Vector2(offset, offset));
        terrainMeshRenderer.material->SetVector2("heightmapOffset", Vector2(coord.x * offset, coord.y * offset));
        terrainMeshRenderer.material->SetFloat("heightScale", terrainHeight);

        terrainMeshRenderer.customShadowPass->SetTexture("heightMap", heightmapTex);
        terrainMeshRenderer.customShadowPass->SetVector2("heightmapTilling", Vector2(offset, offset));
        terrainMeshRenderer.customShadowPass->SetVector2("heightmapOffset", Vector2(coord.x * offset, coord.y * offset));
        terrainMeshRenderer.customShadowPass->SetFloat("heightScale", terrainHeight);
    }
}*/

TerrainSystem::TerrainSystem(Scene* inScene):System(inScene){

}

TerrainSystem::~TerrainSystem(){

}

void TerrainSystem::PhysicsUpdate(){
    OD_PROFILE_SCOPE("TerrainSystem::Update");
    //OD_LOG_PROFILE("TerrainSystem::Update");

    auto terrainView = GetScene()->GetRegistry().view<TransformComponent, TerrainComponent>();
    for(auto e: terrainView){
        TransformComponent& trans = terrainView.get<TransformComponent>(e);
        TerrainComponent& terrain = terrainView.get<TerrainComponent>(e);
        Assert(GetScene()->IsValid(EntityNull) == false);

        if(GetScene()->IsValid(terrain.meshsRoot) == false || terrain.isDirt == true){
            CreateTerrain(terrain, e);
        } else {
            if(terrain.heightMapIsDirt){
                terrain.heightMapIsDirt = false;
                UpdateTerrainData(terrain);
            }
            UpdateTerrain(terrain);
        }
    }
}

struct TerrainMeshData{
    Ref<Mesh> mesh = CreateRef<Mesh>();
    Ref<MeshShapeData> shapeData;

    int triangleIndex = 0;

    TerrainMeshData(int meshWidth, int meshHeight){
        mesh->vertices.resize(meshWidth * meshHeight);
        mesh->uv.resize(meshWidth * meshHeight);
        mesh->indices.resize((meshWidth-1)*(meshHeight-1)*6);
    }

    void AddTriangle(int a, int b, int c){
        mesh->indices[triangleIndex] = a;
        mesh->indices[triangleIndex+1] = b;
        mesh->indices[triangleIndex+2] = c;
        triangleIndex += 3;
    }

    Ref<Mesh> CreateMesh(){
        mesh->Submit();
        return mesh;
    }
};

struct TerrainMeshData2{
    std::vector<unsigned int> indices;
    std::vector<Vector3> vertices;
    std::vector<Vector3> uv;
    int triangleIndex = 0;
    bool useUv = true;

    void Reset(int meshWidth, int meshHeight){
        triangleIndex = 0;
        vertices.resize(meshWidth * meshHeight);
        if(useUv) uv.resize(meshWidth * meshHeight);
        indices.resize((meshWidth-1)*(meshHeight-1)*6);
    }

    void AddTriangle(int a, int b, int c){
        indices[triangleIndex] = a;
        indices[triangleIndex+1] = b;
        indices[triangleIndex+2] = c;
        triangleIndex += 3;
    }

    Ref<Mesh> CreateMesh(){
        Ref<Mesh> mesh = CreateRef<Mesh>();
        mesh->Submit(
            &indices, 
            &vertices,
            useUv ? &uv : nullptr
        );
        /*mesh->vertices = vertices;
        mesh->indices = indices;
        mesh->uv = uv;
        mesh->Submit();*/
        return mesh;
    }
};

float _Remap(float In, Vector2 InMinMax, Vector2 OutMinMax){
    return OutMinMax.x + (In - InMinMax.x) * (OutMinMax.y - OutMinMax.x) / (InMinMax.y - InMinMax.x);
}

TerrainMeshData2 meshGenData;

Ref<Mesh> GenerateTerrainMesh1(int width, int height, int levelOfDetail, MeshBorders toColaps){
    float topLeftX = (width - 1) / -2.0f;
    float topLeftZ = (height - 1) / 2.0f;

    int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
    int verticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

    //Ref<TerrainMeshData> meshData = CreateRef<TerrainMeshData>(verticesPerLine, verticesPerLine);
    auto meshData = &meshGenData;
    meshData->useUv = true;
    meshData->Reset(verticesPerLine, verticesPerLine);
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

            meshData->vertices[vertexIndex] = Vector3(_x+topLeftX, 0, -(_y-topLeftZ));
            //meshData->mesh->uv[vertexIndex] = Vector3((float)_x/(float)width, (float)_y/(float)height, 0);
            meshData->uv[vertexIndex] = Vector3(
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

    return meshData->CreateMesh();

    /*//meshData->shapeData = CreateMeshShapeData(meshData->mesh->vertices, meshData->mesh->indices);
    meshData->mesh->CalculateNormals();
    meshData->mesh->CalculateTangent();
    meshData->mesh->Submit();
    return meshData->mesh;*/
}

Ref<Mesh> GenerateTerrainFromHeightmap(Ref<Heightmap> heightmap, int levelOfDetail){
    int width = heightmap->width;
    int height = heightmap->height;

    float topLeftX = (width - 1) / -2.0f;
    float topLeftZ = (height - 1) / 2.0f;

    int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
    int verticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

    //Ref<TerrainMeshData> meshData = CreateRef<TerrainMeshData>(verticesPerLine, verticesPerLine);
    auto meshData = &meshGenData;
    meshData->useUv = false;
    meshData->Reset(verticesPerLine, verticesPerLine);
    int vertexIndex = 0;

    for(int y = 0; y < height; y += meshSimplificationIncrement){
        for(int x = 0; x < width; x += meshSimplificationIncrement){
            //meshData->mesh->vertices[vertexIndex] = Vector3(x+topLeftX, 0, -(y-topLeftZ));
            int _x = x;
            int _y = y;

            meshData->vertices[vertexIndex] = Vector3(_x+topLeftX, heightmap->Get(_x, _y), -(_y-topLeftZ));
            
            if(x < width-1 && y < height-1){
                meshData->AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
                meshData->AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
            }

            vertexIndex += 1;
        }
    }
    return meshData->CreateMesh();

    //meshData->shapeData = CreateMeshShapeData(meshData->mesh->vertices, meshData->mesh->indices);
    //meshData->mesh->CalculateNormals();
    //meshData->mesh->CalculateTangent();
    //meshData->mesh->Submit();
    //return meshData->mesh;
}

Ref<Mesh> GenerateTerrainFromHeightmap2(Ref<Heightmap> heightmap, int levelOfDetail){
    int width = heightmap->width;
    int height = heightmap->height;

    float topLeftX = (width - 1) / -2.0f;
    float topLeftZ = (height - 1) / 2.0f;

    int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
    int verticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

    Ref<Mesh> out = CreateRef<Mesh>();
    int triangleIndex = 0;
    bool useUv = false;

    auto Reset = [&](int meshWidth, int meshHeight){
        triangleIndex = 0;
        out->vertices.resize(meshWidth * meshHeight);
        if(useUv) out->uv.resize(meshWidth * meshHeight);
        out->indices.resize((meshWidth-1)*(meshHeight-1)*6);
    };

    auto AddTriangle = [&](int a, int b, int c){
        out->indices[triangleIndex] = a;
        out->indices[triangleIndex+1] = b;
        out->indices[triangleIndex+2] = c;
        triangleIndex += 3;
    };

    //Ref<TerrainMeshData> meshData = CreateRef<TerrainMeshData>(verticesPerLine, verticesPerLine);
    
    Reset(verticesPerLine, verticesPerLine);
    int vertexIndex = 0;

    for(int y = 0; y < height; y += meshSimplificationIncrement){
        for(int x = 0; x < width; x += meshSimplificationIncrement){
            //meshData->mesh->vertices[vertexIndex] = Vector3(x+topLeftX, 0, -(y-topLeftZ));
            int _x = x;
            int _y = y;

            out->vertices[vertexIndex] = Vector3(_x+topLeftX, heightmap->Get(_x, _y), -(_y-topLeftZ));
            
            if(x < width-1 && y < height-1){
                AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
                AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
            }

            vertexIndex += 1;
        }
    }
    out->Submit();
    return out;
}

std::vector<std::pair<Ref<Mesh>, Vector3>> GenerateTerrainChunksFromHeightmap(
    Ref<Heightmap> heightmap, int levelOfDetail, int chunkCountX, int chunkCountY)
{
    int width = heightmap->width;
    int height = heightmap->height;

    float topLeftX = (width - 1) / -2.0f;
    float topLeftZ = (height - 1) / 2.0f;

    int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
    int totalVerticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

    int chunkSizeX = width / chunkCountX;
    int chunkSizeY = height / chunkCountY;

    std::vector<std::pair<Ref<Mesh>, Vector3>> outMeshes;

    for (int cy = 0; cy < chunkCountY; ++cy) {
        for (int cx = 0; cx < chunkCountX; ++cx) {
            int startX = cx * chunkSizeX;
            int startY = cy * chunkSizeY;
            int endX = (cx == chunkCountX - 1) ? width : startX + chunkSizeX;
            int endY = (cy == chunkCountY - 1) ? height : startY + chunkSizeY;

            int chunkWidth = (endX - startX);
            int chunkHeight = (endY - startY);

            int verticesX = (chunkWidth - 1) / meshSimplificationIncrement + 1;
            int verticesY = (chunkHeight - 1) / meshSimplificationIncrement + 1;

            Ref<Mesh> mesh = CreateRef<Mesh>();
            mesh->vertices.resize(verticesX * verticesY);
            mesh->indices.resize((verticesX - 1) * (verticesY - 1) * 6);

            int triangleIndex = 0;
            auto AddTriangle = [&](int a, int b, int c) {
                mesh->indices[triangleIndex++] = a;
                mesh->indices[triangleIndex++] = b;
                mesh->indices[triangleIndex++] = c;
            };

            int vertexIndex = 0;
            for (int y = startY; y < endY; y += meshSimplificationIncrement) {
                for (int x = startX; x < endX; x += meshSimplificationIncrement) {
                    float vx = x + topLeftX;
                    float vz = -(y - topLeftZ);
                    float vy = heightmap->Get(x, y);
                    mesh->vertices[vertexIndex] = Vector3(vx, vy, vz);

                    int localX = (x - startX) / meshSimplificationIncrement;
                    int localY = (y - startY) / meshSimplificationIncrement;
                    if (localX < verticesX - 1 && localY < verticesY - 1) {
                        int a = vertexIndex;
                        int b = a + verticesX + 1;
                        int c = a + verticesX;
                        int d = a + 1;
                        AddTriangle(a, b, c);
                        AddTriangle(b, a, d);
                    }

                    vertexIndex++;
                }
            }

            mesh->Submit();

            // Local offset for this chunk (based on topLeftX/Z + offset in chunk grid)
            float chunkOffsetX = startX + topLeftX;
            float chunkOffsetZ = -(startY - topLeftZ);
            outMeshes.push_back({ mesh, Vector3(chunkOffsetX, 0.0f, chunkOffsetZ) });
        }
    }

    return outMeshes;
}

std::vector<std::pair<Ref<Mesh>, Vector3>> GenerateTerrainChunksFromHeightmap2( 
    Ref<Heightmap> heightmap, int levelOfDetail, int chunkCountX, int chunkCountY)
{
    int width = heightmap->width;
    int height = heightmap->height;

    float topLeftX = (width - 1) / -2.0f;
    float topLeftZ = (height - 1) / 2.0f;

    int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;

    int chunkSizeX = (width - 1) / chunkCountX;
    int chunkSizeY = (height - 1) / chunkCountY;

    std::vector<std::pair<Ref<Mesh>, Vector3>> outMeshes;

    for (int cy = 0; cy < chunkCountY; ++cy) {
        for (int cx = 0; cx < chunkCountX; ++cx) {
            int startX = cx * chunkSizeX;
            int startY = cy * chunkSizeY;

            // Include one extra vertex on non-edge chunks to stitch boundaries
            int endX = (cx == chunkCountX - 1) ? width : startX + chunkSizeX + meshSimplificationIncrement;
            int endY = (cy == chunkCountY - 1) ? height : startY + chunkSizeY + meshSimplificationIncrement;

            // Clamp to avoid reading outside heightmap
            endX = std::min(endX, width);
            endY = std::min(endY, height);

            int chunkWidth = endX - startX;
            int chunkHeight = endY - startY;

            int verticesX = (chunkWidth - 1) / meshSimplificationIncrement + 1;
            int verticesY = (chunkHeight - 1) / meshSimplificationIncrement + 1;

            Ref<Mesh> mesh = CreateRef<Mesh>();
            mesh->vertices.resize(verticesX * verticesY);
            mesh->indices.resize((verticesX - 1) * (verticesY - 1) * 6);

            int triangleIndex = 0;
            auto AddTriangle = [&](int a, int b, int c) {
                mesh->indices[triangleIndex++] = a;
                mesh->indices[triangleIndex++] = b;
                mesh->indices[triangleIndex++] = c;
            };

            int vertexIndex = 0;
            for (int y = startY; y < endY; y += meshSimplificationIncrement) {
                for (int x = startX; x < endX; x += meshSimplificationIncrement) {
                    float vx = x + topLeftX;
                    float vz = -(y - topLeftZ);
                    float vy = heightmap->Get(x, y);
                    mesh->vertices[vertexIndex] = Vector3(vx, vy, vz);

                    int localX = (x - startX) / meshSimplificationIncrement;
                    int localY = (y - startY) / meshSimplificationIncrement;
                    if (localX < verticesX - 1 && localY < verticesY - 1) {
                        int a = vertexIndex;
                        int b = a + verticesX + 1;
                        int c = a + verticesX;
                        int d = a + 1;
                        AddTriangle(a, b, c);
                        AddTriangle(b, a, d);
                    }

                    vertexIndex++;
                }
            }

            mesh->Submit();

            float chunkOffsetX = startX + topLeftX;
            float chunkOffsetZ = -(startY - topLeftZ);
            outMeshes.push_back({ mesh, Vector3(chunkOffsetX, 0.0f, chunkOffsetZ) });
        }
    }

    return outMeshes;
}

void TerrainComponent::CreateMeshToNavmesh(Scene& scene){
    for(auto i: meshToNavmeshChunks){
        scene.DestroyEntity(i);
    }
    meshToNavmeshChunks.clear();

    if(meshToNavmesh != EntityNull) scene.DestroyEntity(meshToNavmesh);

    float terrainMeshWidth = (float)(chunkSize * chunkWidthCount);

    auto chunks = GenerateTerrainChunksFromHeightmap2(heightmap, meshToNavmeshLod, 1, 1);
    for(auto& i: chunks){
        Entity e = scene.AddEntity("meshToNavmesh (" + std::to_string(i.second.x) +", " + std::to_string(i.second.z));
        scene.SetParent(meshsRoot, e);
        meshToNavmeshChunks.push_back(e);


        MeshRendererComponent& _meshToNavmesh = scene.AddComponent<MeshRendererComponent>(e);
        {
        OD_LOG_PROFILE("TerrainSystem::CreateTerrain::GenerateTerrainFromHeightmap");  
        //Ref<Mesh> m = CreateRef<Mesh>();
        _meshToNavmesh.mesh = i.first;
        _meshToNavmesh.UpdateAABB();
        }
        //_meshToNavmesh.material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
        TransformComponent& meshToNavmeshTrans = scene.GetComponent<TransformComponent>(e);
        meshToNavmeshTrans.LocalScale(Vector3(
            terrainMeshWidth / (float)heightmap->width,
            terrainHeight,  // vertical exaggeration
            terrainMeshWidth / (float)heightmap->height
        ));
    
        meshToNavmeshTrans.LocalPosition(Vector3(
            terrainMeshWidth / 2.0f,
            0,
            -(terrainMeshWidth / 2.0f)
        ));
    }

    //Create Mesh To Navmesh
    /*meshToNavmesh = scene.AddEntity("meshToNavmesh");
    scene.SetParent(meshsRoot, meshToNavmesh);
    MeshRendererComponent& _meshToNavmesh = scene.AddComponent<MeshRendererComponent>(meshToNavmesh);
    {
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::GenerateTerrainFromHeightmap");  
    //Ref<Mesh> m = CreateRef<Mesh>();
    _meshToNavmesh.mesh = GenerateTerrainFromHeightmap2(heightmap, meshToNavmeshLod);
    _meshToNavmesh.UpdateAABB();
    }
    _meshToNavmesh.material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    TransformComponent& meshToNavmeshTrans = scene.GetComponent<TransformComponent>(meshToNavmesh);
    meshToNavmeshTrans.LocalScale(Vector3(
        terrainMeshWidth / (float)heightmap->width,
        terrainHeight, 
        terrainMeshWidth / (float)heightmap->height
    ));
    meshToNavmeshTrans.LocalPosition(
        Vector3(
            terrainMeshWidth / 2.0f,
            0,
            -(terrainMeshWidth / 2.0f)
        )
    );*/
}

void TerrainSystem::DestroyTerrain(TerrainComponent& terrain){
    if(GetScene()->IsValid(terrain.meshsRoot)){
        scene->DestroyEntity(terrain.meshsRoot);
        terrain.meshsRoot = Entity();
        terrain.collider = Entity();
        terrain.meshToNavmesh = Entity();
        terrain.meshToNavmeshChunks.clear();
    }

    terrain.loadedChunks.clear();
    terrain.lods.clear();
    terrain.lodsMesh.clear();
}

void TerrainSystem::CreateTerrain(TerrainComponent& terrain, Entity e){
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain");

    DestroyTerrain(terrain);

    terrain.meshsRoot = GetScene()->AddEntity("Root");
    GetScene()->GetComponent<InfoComponent>(terrain.meshsRoot).hidden = false;//true;
    GetScene()->SetParent(e, terrain.meshsRoot);
    
    terrain.chunkSize = terrain.mapChunkSize - 1;
    terrain.lods = std::vector<int>{
        0, 
        1, 
        3,
        7,
        15,
        31,
        63
    };

    {
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::GetTerrainLod");    
    for(int i = 0; i < terrain.lods.size(); i++){
        terrain.lodsMesh.push_back(GetTerrainLod(terrain.chunkSize, terrain.lods[i]));
    }
    }

    
    // Create HeightmapTex
    int heightmapSize = 1024;
    if(terrain.heightmap == nullptr) terrain.heightmap = CreateRef<Heightmap>(heightmapSize, heightmapSize);

    {
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::GenHeightmapTex");    
    terrain.heightmapTex = Texture2D::CreateFromRaw( 
        (void*)&terrain.heightmap->data[0],
        (size_t)(terrain.heightmap->data.size() * sizeof(float)),
        terrain.heightmap->width, terrain.heightmap->height,
        TextureDataType::Float,
        Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
    );
    }

    /*auto GenerateVertex = [&](int x, int y){
        x = math::clamp<int>(x, 0, terrain.heightmap->width);
        y = math::clamp<int>(y, 0, terrain.heightmap->height);
        float h = terrain.heightmap->Get(x, y) * terrain.terrainHeight;
        return Vector3(x, h, -y);
    };
    auto GetNormalFromFace = [&](Vector3 pointA, Vector3 pointB, Vector3 pointC){
        Vector3 sideAB = pointB - pointA;
		Vector3 sideAC = pointC - pointA;
		return math::normalize(math::cross(sideAB, sideAC));
    };
    auto ToNormalMap = [](Vector3 normal){
        return normal * 0.5f + 0.5f;
    };
    Vector3* normal = new Vector3[terrain.heightmap->data.size()];
    for(int x = 0; x < terrain.heightmap->width; x++){
        for(int y = 0; y < terrain.heightmap->height; y++){
            //normal[heightmap->ToFlatCoord(x, y)] = Vector3Up;
            Vector3 a = GenerateVertex(x-1, y-1);
            Vector3 b = GenerateVertex(x, y);
            Vector3 c = GenerateVertex(x-1, y);
            normal[terrain.heightmap->ToFlatCoord(x, y)] = GetNormalFromFace(a, b, c);
            normal[terrain.heightmap->ToFlatCoord(x, y)] = ToNormalMap(GetNormalFromFace(a, b, c));
        }
    }
    terrain.normalTex = Texture2D::CreateFromRaw(
        (void*)normal,
        (size_t)(terrain.heightmap->data.size() * sizeof(Vector3)),
        terrain.heightmap->width, terrain.heightmap->height,
        TextureDataType::Float,
        Texture2DSetting{TextureFilter::Linear, TextureWrapping::Repeat, true, TextureFormat::RGB16F}
    );
    delete normal;*/

    //Create Collider
    terrain.collider = GetScene()->AddEntity("Collider");
    GetScene()->SetParent(terrain.meshsRoot, terrain.collider);
    float terrainMeshWidth = (float)(terrain.chunkSize * terrain.chunkWidthCount);
    TransformComponent& colliderTrans = GetScene()->GetComponent<TransformComponent>(terrain.collider);
    colliderTrans.LocalScale(Vector3(
        terrainMeshWidth / (float)terrain.heightmap->width,
        terrain.terrainHeight, 
        terrainMeshWidth / (float)terrain.heightmap->height
    ));
    colliderTrans.LocalPosition(
        Vector3(
            terrainMeshWidth / 2.0f,
            terrain.terrainHeight/2,
            -(terrainMeshWidth / 2.0f)
        )
    );
    HeightmapColliderComponent& heightmapCollider = GetScene()->AddComponent<HeightmapColliderComponent>(terrain.collider);
    heightmapCollider.width = terrain.heightmap->width;
    heightmapCollider.length = terrain.heightmap->height;
    heightmapCollider.scale = heightmapSize;
    heightmapCollider.minHeight = 0;
    heightmapCollider.maxHeight = 1; //terrainHeight;
    {
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::TransposeTo");    
    terrain.heightmap->TransposeTo(heightmapCollider.heights);
    }

    terrain.CreateMeshToNavmesh(*GetScene());

    //Create Mesh To Navmesh
    /*terrain.meshToNavmesh = GetScene()->AddEntity("meshToNavmesh");
    GetScene()->SetParent(terrain.meshsRoot, terrain.meshToNavmesh);
    MeshRendererComponent& meshToNavmesh = GetScene()->AddComponent<MeshRendererComponent>(terrain.meshToNavmesh);
    {
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::GenerateTerrainFromHeightmap");  
    //Ref<Mesh> m = CreateRef<Mesh>();
    meshToNavmesh.mesh = GenerateTerrainFromHeightmap2(terrain.heightmap, terrain.meshToNavmeshLod);
    meshToNavmesh.UpdateAABB();
    }
    //meshToNavmesh.material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    TransformComponent& meshToNavmeshTrans = GetScene()->GetComponent<TransformComponent>(terrain.meshToNavmesh);
    meshToNavmeshTrans.LocalScale(Vector3(
        terrainMeshWidth / (float)terrain.heightmap->width,
        terrain.terrainHeight, 
        terrainMeshWidth / (float)terrain.heightmap->height
    ));
    meshToNavmeshTrans.LocalPosition(
        Vector3(
            terrainMeshWidth / 2.0f,
            0,
            -(terrainMeshWidth / 2.0f)
        )
    );*/

    // Load Coords
    {
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::LoadCoods");  
    for(int x = 0; x < terrain.chunkWidthCount; x++){
        for(int y = 0; y < terrain.chunkWidthCount; y++){
            LoadCood(terrain, IVector2(x, y));
        }
    }
    }

    terrain.isDirt = false;
    terrain.heightMapIsDirt = false;
}

void TerrainSystem::UpdateTerrainData(TerrainComponent& terrain){
    int heightmapSize = terrain.heightmap->width;
    float terrainMeshWidth = (float)(terrain.chunkSize * terrain.chunkWidthCount);

    {
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::GenHeightmapTex");    
    terrain.heightmapTex = Texture2D::CreateFromRaw( 
        (void*)&terrain.heightmap->data[0],
        (size_t)(terrain.heightmap->data.size() * sizeof(float)),
        terrain.heightmap->width, terrain.heightmap->height,
        TextureDataType::Float,
        Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
    );
    }

    {
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::TransposeTo");  
    HeightmapColliderComponent& heightmapCollider = GetScene()->GetComponent<HeightmapColliderComponent>(terrain.collider);
    heightmapCollider.width = terrain.heightmap->width;
    heightmapCollider.length = terrain.heightmap->height;
    heightmapCollider.scale = heightmapSize;
    heightmapCollider.minHeight = 0;
    heightmapCollider.maxHeight = 1; //terrainHeight;
    terrain.heightmap->TransposeTo(heightmapCollider.heights);
    }

    /*{
    OD_LOG_PROFILE("TerrainSystem::CreateTerrain::GenerateTerrainFromHeightmap");  
    MeshRendererComponent& meshToNavmesh = GetScene()->GetComponent<MeshRendererComponent>(terrain.meshToNavmesh);
    //Ref<Mesh> m = CreateRef<Mesh>();
    meshToNavmesh.mesh = GenerateTerrainFromHeightmap2(terrain.heightmap, terrain.meshToNavmeshLod);
    meshToNavmesh.UpdateAABB();
    }*/

    terrain.CreateMeshToNavmesh(*GetScene());
}

void TerrainSystem::UpdateTerrain(TerrainComponent& terrain){
    OD_PROFILE_SCOPE("TerrainSystem::UpdateTerrain");

    //TODO: Revise this design
    //terrain.meshsRoot.scene = scene;
    //terrain.collider.scene = scene;
    //terrain.meshToNavmesh.scene = scene;

    TransformComponent& camTrans = GetScene()->GetComponent<TransformComponent>(GetScene()->GetMainCamera());

    Vector3 viewPos = Vector3(camTrans.Position().x, camTrans.Position().y, -camTrans.Position().z);
    auto currentCoord = IVector3(
        math::round(viewPos.x / terrain.chunkSize), 
        math::round(viewPos.y / terrain.chunkSize),
        math::round(viewPos.z / terrain.chunkSize) 
    );

    {
    OD_PROFILE_SCOPE("TerrainSystem::UpdateTerrain::1");
    for(auto& i: terrain.loadedChunks){
        TransformComponent& trans = GetScene()->GetComponent<TransformComponent>(i.second.entity);

        Vector3 pos = trans.Position();
        i.second.lodInfo.lod = terrain.lods.size()-1;

        auto p1 = camTrans.Position() / Vector3(terrain.chunkSize);
        auto p2 = pos / Vector3(terrain.chunkSize);

        i.second.lodInfo.lod = math::clamp<int>(
            math::distance(p1, p2) * terrain.lodBias, 
            0, 
            terrain.lods.size()-1
        );

        GetScene()->GetComponent<TransformComponent>(terrain.loadedChunks[i.first].entity).LocalScale(
            terrain.lodsMesh[i.second.lodInfo.lod].scale
        );
    }  
    }

    {
    OD_PROFILE_SCOPE("TerrainSystem::UpdateTerrain::2");
    for(auto& i: terrain.loadedChunks){
        MeshBorders& borders = i.second.lodInfo.borders;
        int& lod = i.second.lodInfo.lod;

        borders = MeshBorders();

        if(terrain.loadedChunks.count(i.first + IVector2(-1, 0))) borders.left = lod < terrain.loadedChunks[i.first + IVector2(-1, 0)].lodInfo.lod;
        if(terrain.loadedChunks.count(i.first + IVector2(1, 0))) borders.right = lod < terrain.loadedChunks[i.first + IVector2(1, 0)].lodInfo.lod;
        if(terrain.loadedChunks.count(i.first + IVector2(0, 1))) borders.top = lod < terrain.loadedChunks[i.first + IVector2(0, 1)].lodInfo.lod;
        if(terrain.loadedChunks.count(i.first + IVector2(0, -1))) borders.bottom = lod < terrain.loadedChunks[i.first + IVector2(0, -1)].lodInfo.lod;

        MeshRendererComponent& meshComponent = GetScene()->GetComponent<MeshRendererComponent>(terrain.loadedChunks[i.first].entity);
        meshComponent.mesh = terrain.lodsMesh[lod].meshs[borders];
        meshComponent.boundingVolume = AABB(
            Vector3(0, terrain.terrainHeight/2, 0), 
            (terrain.chunkSize / terrain.lodsMesh[lod].scale.x) / 2, 
            terrain.terrainHeight/2, 
            (terrain.chunkSize / terrain.lodsMesh[lod].scale.z) / 2
        );
        meshComponent.material->SetTexture("heightMap", terrain.heightmapTex);
        meshComponent.material->SetFloat("heightScale", terrain.terrainHeight);
        meshComponent.customShadowPass->SetTexture("heightMap", terrain.heightmapTex);
        meshComponent.customShadowPass->SetFloat("heightScale", terrain.terrainHeight);
    }

    GetScene()->GetComponent<TransformComponent>(terrain.meshsRoot).LocalScale(
        Vector3(
            terrain.terrainWidth / (float)(terrain.chunkSize * terrain.chunkWidthCount),
            1, 
            terrain.terrainLength / (float)(terrain.chunkSize * terrain.chunkWidthCount)
        )
    );
    }

    {
    OD_PROFILE_SCOPE("TerrainSystem::UpdateTerrain::3");
    float terrainMeshWidth = (float)(terrain.chunkSize * terrain.chunkWidthCount);
    TransformComponent& colliderTrans = GetScene()->GetComponent<TransformComponent>(terrain.collider);
    colliderTrans.LocalScale(Vector3(
        terrainMeshWidth / (float)terrain.heightmap->width,
        terrain.terrainHeight, 
        terrainMeshWidth / (float)terrain.heightmap->height
    ));
    colliderTrans.LocalPosition(
        Vector3(
            terrainMeshWidth / 2.0f,
            terrain.terrainHeight/2,
            -(terrainMeshWidth / 2.0f)
        )
    );
    }
}

void TerrainSystem::LoadCood(TerrainComponent& terrain, IVector2 coord){
    TerrainComponent::ChunkData chunkData;
    chunkData.lodInfo = TerrainComponent::LodInfo();

    chunkData.entity = GetScene()->AddEntity("Chunk");
    GetScene()->SetParent(terrain.meshsRoot, chunkData.entity);

    Vector3 pos(coord.x * (float)terrain.chunkSize, 0, -(coord.y * (float)terrain.chunkSize));
    pos += Vector3((float)terrain.chunkSize/2.0f, 0, -(terrain.chunkSize/2.0f));
    GetScene()->GetComponent<TransformComponent>(chunkData.entity).LocalPosition(pos);

    InfoComponent& info = GetScene()->GetComponent<InfoComponent>(chunkData.entity);
    info.hidden = true;

    GetScene()->AddComponent<NavmeshSkipTag>(chunkData.entity);

    float offset = 1.0f / (float)terrain.chunkWidthCount;

    MeshRendererComponent& terrainMeshRenderer = GetScene()->AddComponent<MeshRendererComponent>(chunkData.entity);
    terrainMeshRenderer.UpdateAABB();
    terrainMeshRenderer.material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Terrain.glsl"));
    terrainMeshRenderer.material->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png"));
    terrainMeshRenderer.material->SetTexture("splatmap", terrain.splatmap);
    terrainMeshRenderer.material->SetTexture("tex0", terrain.layer0);
    terrainMeshRenderer.material->SetTexture("tex1", terrain.layer1);
    terrainMeshRenderer.material->SetTexture("tex2", terrain.layer2);
    terrainMeshRenderer.material->SetTexture("tex3", terrain.layer3);
    terrainMeshRenderer.material->SetTexture("tex4", terrain.layer4);
    terrainMeshRenderer.material->SetVector4("color", Vector4(1, 1, 1, 1));

    terrainMeshRenderer.material->SetTexture("heightMap", terrain.heightmapTex);
    //terrainMeshRenderer.material->SetTexture("heightMapNormal", terrain.normalTex);
    terrainMeshRenderer.material->SetVector2("heightmapTilling", Vector2(offset, offset));
    terrainMeshRenderer.material->SetVector2("heightmapOffset", Vector2(coord.x * offset, coord.y * offset));
    terrainMeshRenderer.material->SetFloat("heightScale", terrain.terrainHeight);

    terrainMeshRenderer.customShadowPass = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/TerrainShadow.glsl"));
    terrainMeshRenderer.customShadowPass->SetTexture("heightMap", terrain.heightmapTex);
    terrainMeshRenderer.customShadowPass->SetVector2("heightmapTilling", Vector2(offset, offset));
    terrainMeshRenderer.customShadowPass->SetVector2("heightmapOffset", Vector2(coord.x * offset, coord.y * offset));
    terrainMeshRenderer.customShadowPass->SetFloat("heightScale", terrain.terrainHeight);

    terrain.loadedChunks[coord] = chunkData;
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

TerrainComponent::TerrainLod TerrainSystem::GetTerrainLod(int chunkSize, int lod){
    TerrainComponent::TerrainLod out;

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

        MeshBorders mbc(s);
        out.meshs[mbc] = GenerateTerrainMesh1(width, height, 0, mbc);
    }

    return out;
}

}