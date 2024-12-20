#include "Terrain.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"

namespace OD{

void TerrainModuleInit(){
    SceneManager::Get().RegisterCoreComponent<QuadTreeTerrainComponent>("QuadTreeTerrainComponent");
    SceneManager::Get().RegisterSystem<QuadTreeTerrainSystem>("QuadTreeTerrainSystem");

    SceneManager::Get().RegisterCoreComponent<TerrainComponent>("TerrainComponent");
    SceneManager::Get().RegisterSystem<TerrainSystem>("TerrainSystem");
}

/////////////////////////////////////////////////////////////////////////////

QuadTreeTerrainSystem::QuadTreeTerrainSystem(Scene* inScene):System(inScene){

}

QuadTreeTerrainSystem::~QuadTreeTerrainSystem(){

}

void SplitQuadTreeBaseOnPosition(Vector3 camPos, QuadTree::Node& node, Vector3 pos, int width, int length){
    float radius = ((width + length)/2)/2;
    radius *= 1.5f;
    int halfWidth2 = (width/2)/2;
    int halfLength2 = (length/2)/2;

    if(math::distance(pos, camPos) <= radius){
        node.Split();
        SplitQuadTreeBaseOnPosition(camPos, *node.childen[0], Vector3(pos.x-halfWidth2, 0, pos.z-halfLength2), width/2, length/2);
        SplitQuadTreeBaseOnPosition(camPos, *node.childen[1], Vector3(pos.x+halfWidth2, 0, pos.z-halfLength2), width/2, length/2);
        SplitQuadTreeBaseOnPosition(camPos, *node.childen[2], Vector3(pos.x-halfWidth2, 0, pos.z+halfLength2), width/2, length/2);
        SplitQuadTreeBaseOnPosition(camPos, *node.childen[3], Vector3(pos.x+halfWidth2, 0, pos.z+halfLength2), width/2, length/2);
    }
}

int GetMinDepth(QuadTree::Node& node){
    if(node.childen.size() > 0){
        std::vector<int> depths;
        depths.push_back(GetMinDepth(*node.childen[0]));
        depths.push_back(GetMinDepth(*node.childen[1]));
        depths.push_back(GetMinDepth(*node.childen[2]));
        depths.push_back(GetMinDepth(*node.childen[3]));
        int depth = 0;
        for(int i: depths){
            if(i > depth) depth = i;
        }
        return depth;
    }
    return node.depth;
}

void SplitQuadTreeBaseStillMaxDepth(QuadTree::Node& node, int maxDepth){
    if(node.depth >= (maxDepth-1)) return;

    for(auto& i: node.childen){
        if(i->childen.size() == 0){
            i->Split();
        } else {
            SplitQuadTreeBaseStillMaxDepth(*i, maxDepth);
        }
    }
}

void QuadTreeTerrainSystem::Update(){
    OD_PROFILE_SCOPE("QuadTreeTerrainSystem::Update");

    Entity cam = GetScene()->GetMainCamera();
    if(cam.IsValid() == false) return;

    TransformComponent& camTrans = cam.GetComponent<TransformComponent>();
    Vector2 viewPos = Vector2(camTrans.Position().x, -camTrans.Position().z);

    auto terrainView = GetScene()->GetRegistry().view<TransformComponent, QuadTreeTerrainComponent>();
    for(auto e: terrainView){
        TransformComponent& transform = terrainView.get<TransformComponent>(e);
        QuadTreeTerrainComponent& terrain = terrainView.get<QuadTreeTerrainComponent>(e);

        if(terrain.quadtree.root == nullptr){
            terrain.quadtree.root = CreateRef<QuadTree::Node>();
            /*terrain.quadtree.root->Split();
            terrain.quadtree.root->childen[0]->Split();
            terrain.quadtree.root->childen[0]->childen[0]->Split();
            terrain.quadtree.root->childen[0]->childen[0]->childen[0]->Split();*/
        } else {
            terrain.quadtree.root->childen.clear();
        }
        SplitQuadTreeBaseOnPosition(camTrans.Position(), *terrain.quadtree.root, transform.Position(), terrain.terrainWidth, terrain.terrainLength);
        //int maxDepth = GetMinDepth(*terrain.quadtree.root);
        //SplitQuadTreeBaseStillMaxDepth(*terrain.quadtree.root, maxDepth);
    }
}

void DrawQuadTreeNode(QuadTree::Node& node, int depth, int width, int length, Vector3 pos = Vector3Zero){
    int halfWidth2 = (width/2)/2;
    int halfLength2 = (length/2)/2;

    Transform gizmosTrans;
    gizmosTrans.LocalPosition(pos);
    gizmosTrans.LocalScale(Vector3(width, 0, length));

    Graphics::DrawWireCube(gizmosTrans.GetLocalModelMatrix(), Vector3(0, 1, 0), 1);

    if(node.childen.size() > 0){
        DrawQuadTreeNode(*node.childen[0], depth + 1, width/2, length/2, Vector3(pos.x-halfWidth2, 0, pos.z-halfLength2));
        DrawQuadTreeNode(*node.childen[1], depth + 1, width/2, length/2, Vector3(pos.x+halfWidth2, 0, pos.z-halfLength2));
        DrawQuadTreeNode(*node.childen[2], depth + 1, width/2, length/2, Vector3(pos.x-halfWidth2, 0, pos.z+halfLength2));
        DrawQuadTreeNode(*node.childen[3], depth + 1, width/2, length/2, Vector3(pos.x+halfWidth2, 0, pos.z+halfLength2));
    }
}

void QuadTreeTerrainSystem::OnDrawGizmos(){
    auto terrainView = GetScene()->GetRegistry().view<TransformComponent, QuadTreeTerrainComponent>();
    for(auto e: terrainView){
        TransformComponent& transform = terrainView.get<TransformComponent>(e);
        QuadTreeTerrainComponent& terrain = terrainView.get<QuadTreeTerrainComponent>(e);
        if(terrain.quadtree.root == nullptr) continue;

        DrawQuadTreeNode(*terrain.quadtree.root, 0, terrain.terrainWidth, terrain.terrainLength);
    }
}

/////////////////////////////////////////////////////////////////////////////

int ManhattanDistance(IVector2 a, IVector2 b){
    return math::abs(a.x - b.x) + math::abs(a.y - b.y);
}

int ManhattanDistance(IVector3 a, IVector3 b){
    return math::abs(a.x - b.x) + math::abs(a.y - b.y) + math::abs(a.z - b.z);
}

void TerrainComponent::OnGui(Entity e){
    TerrainComponent& terrain = e.GetComponent<TerrainComponent>();

    ImGui::DragFloat("lodBias", &terrain.lodBias);
    ImGui::DragFloat("terrainWidth", &terrain.terrainWidth);
    ImGui::DragFloat("terrainLength", &terrain.terrainLength);
    ImGui::DragFloat("terrainHeight", &terrain.terrainHeight);
}

void TerrainComponent::SetHeightmap(Ref<Heightmap> inHeightmap){
    heightmap = inHeightmap;
}

void TerrainComponent::SubmitHeightmap(){
    heightmapTex = CreateRef<Texture2D>(
        (void*)&heightmap->data[0],
        (size_t)(heightmap->data.size() * sizeof(float)),
        heightmap->width, heightmap->height,
        TextureDataType::Float,
        Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
    );

    for(auto& i: loadedChunks){
        IVector2 coord = i.first;
        MeshRendererComponent& terrainMeshRenderer =  i.second.entity.AddComponent<MeshRendererComponent>();
        terrainMeshRenderer.UpdateAABB();
        terrainMeshRenderer.material->SetVector4("color", Vector4(1, 1, 1, 1));
        terrainMeshRenderer.material->SetTexture("heightMap", heightmapTex);
        float offset = 1.0f / (float)chunkWidthCount;
        terrainMeshRenderer.material->SetVector2("heightmapTilling", Vector2(offset, offset));
        terrainMeshRenderer.material->SetVector2("heightmapOffset", Vector2(coord.x * offset, coord.y * offset));
        terrainMeshRenderer.material->SetFloat("heightScale", terrainHeight);
    }
}

TerrainSystem::TerrainSystem(Scene* inScene):System(inScene){

}

TerrainSystem::~TerrainSystem(){

}

void TerrainSystem::Update(){
    OD_PROFILE_SCOPE("TerrainSystem::Update");

    auto terrainView = GetScene()->GetRegistry().view<TransformComponent, TerrainComponent>();
    for(auto e: terrainView){
        TransformComponent& trans = terrainView.get<TransformComponent>(e);
        TerrainComponent& terrain = terrainView.get<TerrainComponent>(e);

        if(terrain.meshsRoot.IsValid() == false){
            CreateTerrain(terrain, e);
        } else {
            UpdateTerrain(terrain);
        }
    }
}

void TerrainSystem::CreateTerrain(TerrainComponent& terrain, EntityId e){
    terrain.meshsRoot = GetScene()->AddEntity("Root");
    Assert(terrain.meshsRoot.IsValid() == true);
    GetScene()->SetParent(e, terrain.meshsRoot.Id());

    terrain.chunkSize = terrain.mapChunkSize - 1;
    terrain.lods = std::vector<TerrainComponent::LODDef>{
        {0, 100}, 
        {1, 200}, 
        {3, 300},
        {7, 400},
        //{15, 500},
        //{31, 600},
        //{63, 700}
    };

    for(int i = 0; i < terrain.lods.size(); i++){
        terrain.lodsMesh.push_back(GetTerrainLod(terrain.chunkSize, terrain.lods[i].lod));
    }

    int heightmapSize = 1024;
    if(terrain.heightmap == nullptr) terrain.heightmap = CreateRef<Heightmap>(heightmapSize, heightmapSize);// Noise::GenerateNoiseMap(heightmapSize, heightmapSize, 50, 0.25f/4, 4, 0.5f, 2.0f, Vector2(0, 0));
    terrain.heightmapTex = CreateRef<Texture2D>(
        (void*)&terrain.heightmap->data[0],
        (size_t)(terrain.heightmap->data.size() * sizeof(float)),
        terrain.heightmap->width, terrain.heightmap->height,
        TextureDataType::Float,
        Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
    );

    terrain.collider = GetScene()->AddEntity("Collider");
    GetScene()->SetParent(terrain.meshsRoot, terrain.collider);

    float terrainMeshWidth = (float)(terrain.chunkSize * terrain.chunkWidthCount);
    TransformComponent& colliderTrans = terrain.collider.GetComponent<TransformComponent>();
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
    
    HeightmapColliderComponent& heightmapCollider = terrain.collider.AddComponent<HeightmapColliderComponent>();

    heightmapCollider.width = terrain.heightmap->width;
    heightmapCollider.length = terrain.heightmap->height;
    heightmapCollider.scale = heightmapSize;
    heightmapCollider.minHeight = 0;
    heightmapCollider.maxHeight = 1; //terrainHeight;
    terrain.heightmap->TransposeTo(heightmapCollider.heights);

    for(int x = 0; x < terrain.chunkWidthCount; x++){
        for(int y = 0; y < terrain.chunkWidthCount; y++){
            LoadCood(terrain, IVector2(x, y));
        }
    }
}

void TerrainSystem::UpdateTerrain(TerrainComponent& terrain){
    //TODO: Revise this design
    terrain.meshsRoot.scene = scene;
    terrain.collider.scene = scene;

    TransformComponent& camTrans = GetScene()->GetMainCamera().GetComponent<TransformComponent>();
    //Vector2 viewPos = Vector2(camTrans.Position().x, -camTrans.Position().z);
    Vector3 viewPos = Vector3(camTrans.Position().x, camTrans.Position().y, -camTrans.Position().z);
    auto currentCoord = IVector3(
        math::round(viewPos.x / terrain.chunkSize), 
        math::round(viewPos.y / terrain.chunkSize),
        math::round(viewPos.z / terrain.chunkSize) 
    );

    for(auto& i: terrain.loadedChunks){
        TransformComponent& trans = i.second.entity.GetComponent<TransformComponent>();

        //Vector3 pos(i.first.x * (float)chunkSize, 0, -(i.first.y * (float)chunkSize));
        Vector3 pos = trans.Position();
        i.second.lodInfo.lod = terrain.lods.size()-1;

        //int d = ManhattanDistance(currentCoord, IVector3(i.first.x, 0, i.first.y));
        //LogInfo("%d", d);

        for(int j = 0; j < terrain.lods.size(); j++){
            if(math::distance(camTrans.Position(), pos) <= terrain.lods[j].visibleDstThreshold){
                i.second.lodInfo.lod = j;
                break;
            }
        }
        auto p1 = camTrans.Position() / Vector3(terrain.chunkSize);
        auto p2 = pos / Vector3(terrain.chunkSize);
        //i.second.lodInfo.lod = math::clamp<int>(d, 0, terrain.lods.size()-1);
        i.second.lodInfo.lod = math::clamp<int>(
            //ManhattanDistance(IVector3(p1), IVector3(p2)) * terrain.lodBias, 
            math::distance(p1, p2) * terrain.lodBias, 
            0, 
            terrain.lods.size()-1
        );

        terrain.loadedChunks[i.first].entity.GetComponent<TransformComponent>().LocalScale(
            terrain.lodsMesh[i.second.lodInfo.lod].scale
        );
    }  

    for(auto& i: terrain.loadedChunks){
        MeshBorders& borders = i.second.lodInfo.borders;
        int& lod = i.second.lodInfo.lod;

        borders = MeshBorders();

        if(terrain.loadedChunks.count(i.first + IVector2(-1, 0))) borders.left = lod < terrain.loadedChunks[i.first + IVector2(-1, 0)].lodInfo.lod;
        if(terrain.loadedChunks.count(i.first + IVector2(1, 0))) borders.right = lod < terrain.loadedChunks[i.first + IVector2(1, 0)].lodInfo.lod;
        if(terrain.loadedChunks.count(i.first + IVector2(0, 1))) borders.top = lod < terrain.loadedChunks[i.first + IVector2(0, 1)].lodInfo.lod;
        if(terrain.loadedChunks.count(i.first + IVector2(0, -1))) borders.bottom = lod < terrain.loadedChunks[i.first + IVector2(0, -1)].lodInfo.lod;

        MeshRendererComponent& meshComponent = terrain.loadedChunks[i.first].entity.GetComponent<MeshRendererComponent>();
        meshComponent.mesh = terrain.lodsMesh[lod].meshs[borders];
        meshComponent.boundingVolume = AABB(
            Vector3(0, terrain.terrainHeight/2, 0), 
            (terrain.chunkSize / terrain.lodsMesh[lod].scale.x) / 2, 
            terrain.terrainHeight/2, 
            (terrain.chunkSize / terrain.lodsMesh[lod].scale.z) / 2
        );
        meshComponent.material->SetFloat("heightScale", terrain.terrainHeight);
    }

    terrain.meshsRoot.GetComponent<TransformComponent>().LocalScale(
        Vector3(
            terrain.terrainWidth / (float)(terrain.chunkSize * terrain.chunkWidthCount),
            1, 
            terrain.terrainLength / (float)(terrain.chunkSize * terrain.chunkWidthCount)
        )
    );
    float terrainMeshWidth = (float)(terrain.chunkSize * terrain.chunkWidthCount);
    TransformComponent& colliderTrans = terrain.collider.GetComponent<TransformComponent>();
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

void TerrainSystem::LoadCood(TerrainComponent& terrain, IVector2 coord){
    TerrainComponent::ChunkData chunkData;
    chunkData.lodInfo = TerrainComponent::LodInfo();

    chunkData.entity = GetScene()->AddEntity("Chunk");
    GetScene()->SetParent(terrain.meshsRoot, chunkData.entity);

    Vector3 pos(coord.x * (float)terrain.chunkSize, 0, -(coord.y * (float)terrain.chunkSize));
    pos += Vector3((float)terrain.chunkSize/2.0f, 0, -(terrain.chunkSize/2.0f));
    chunkData.entity.GetComponent<TransformComponent>().LocalPosition(pos);

    InfoComponent& info = chunkData.entity.GetComponent<InfoComponent>();
    //info.hidden = true;

    MeshRendererComponent& terrainMeshRenderer = chunkData.entity.AddComponent<MeshRendererComponent>();
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
    float offset = 1.0f / (float)terrain.chunkWidthCount;
    terrainMeshRenderer.material->SetVector2("heightmapTilling", Vector2(offset, offset));
    terrainMeshRenderer.material->SetVector2("heightmapOffset", Vector2(coord.x * offset, coord.y * offset));
    terrainMeshRenderer.material->SetFloat("heightScale", terrain.terrainHeight);

    terrain.loadedChunks[coord] = chunkData;
}

float _Remap(float In, Vector2 InMinMax, Vector2 OutMinMax){
    return OutMinMax.x + (In - InMinMax.x) * (OutMinMax.y - OutMinMax.x) / (InMinMax.y - InMinMax.x);
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

Ref<Mesh> GenerateTerrainMesh1(int width, int height, int levelOfDetail, MeshBorders toColaps){
    float topLeftX = (width - 1) / -2.0f;
    float topLeftZ = (height - 1) / 2.0f;

    int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
    int verticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

    Ref<TerrainMeshData> meshData = CreateRef<TerrainMeshData>(verticesPerLine, verticesPerLine);
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