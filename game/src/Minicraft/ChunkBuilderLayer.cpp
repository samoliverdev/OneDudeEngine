#include "ChunkBuilderLayer.h"
#include "Ultis/Ultis.h"
#include "WorldManagerSystem.h"

enum class BlockMaterialType{
    Air, Opaque, Transparent, Grass, Water
};

enum class BlockClassType{
    Air, Solid, Tree, Water
};

struct BlockInfo{
    int CubeVertexs[6] = { 0, 0, 0, 0, 0, 0};
    BlockMaterialType blockType;
    BlockClassType classType;

    BlockInfo(int all, BlockMaterialType inBlockType, BlockClassType inClassType){
        CubeVertexs[0] = all;
        CubeVertexs[1] = all;
        CubeVertexs[2] = all;
        CubeVertexs[3] = all;
        CubeVertexs[4] = all;
        CubeVertexs[5] = all;
        blockType = inBlockType;
        classType = inClassType;
    }

    BlockInfo(int front, int back, int left, int right, int up, int down, BlockMaterialType inBlockType, BlockClassType inClassType){
        CubeVertexs[0] = front;
        CubeVertexs[1] = back;
        CubeVertexs[2] = left;
        CubeVertexs[3] = right;
        CubeVertexs[4] = up;
        CubeVertexs[5] = down;
        blockType = inBlockType;
        classType = inClassType;
    }
};

struct MeshBuildHelper{
    int triangleIndex;
    Vector3 normal;
};

const std::vector<std::string> texturePaths = {
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/gravel_stone.png", //0
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/dirt.png", //1
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/dirt_grass.png", //2
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/leaves.png", //3
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/water.png", //4
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/trunk_side.png", //5
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/trunk_top.png", //6
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/leaves.png", //7
};

const BlockInfo blockTextureLookups[] = {
    BlockInfo(0, BlockMaterialType::Air, BlockClassType::Air), // air-0
    BlockInfo(0, BlockMaterialType::Opaque, BlockClassType::Solid), // Stone-1
    BlockInfo(2, 2, 2, 2, 3, 1, BlockMaterialType::Opaque, BlockClassType::Solid), // DirtGrass-2
    BlockInfo(4, BlockMaterialType::Water, BlockClassType::Water), // Water-3
    BlockInfo(5, 5, 5, 5, 6, 6, BlockMaterialType::Opaque, BlockClassType::Tree), // Trunk-4
    BlockInfo(7, BlockMaterialType::Opaque, BlockClassType::Tree), // Leaves-5
};

float Remap(float In, Vector2 InMinMax, Vector2 OutMinMax){
    return OutMinMax.x + (In - InMinMax.x) * (OutMinMax.y - OutMinMax.x) / (InMinMax.y - InMinMax.x);
}

ChunkBuilderLayer::ChunkBuilderLayer(WorldManagerSystem* inWorldManagerSystem){
    texture = CreateRef<Texture2DArray>(texturePaths);

    materialOpaque = CreateRef<Material>(AssetManager::Get().LoadShaderFromFile("res/Game/Shaders/Minicraft.glsl"));
    materialOpaque->SetTexture("mainTex", texture);

    materialWater = CreateRef<Material>(AssetManager::Get().LoadShaderFromFile("res/Game/Shaders/MinicraftWater.glsl"));
    materialWater->SetTexture("mainTex", texture);
    materialWater->SetVector4("color", Vector4{1, 1, 1, 0.85f});
    materialWater->SetFloat("smoothness", 0.55f);

    noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_VALUE;

    world = inWorldManagerSystem;
}

void AddFace(unsigned short blockId, Vector3 pos, int dirIndex, Mesh& mesh){
    FaceTriangles tris = FacesTriangles[dirIndex];
    Vector3 normal = ChunkVoxelNormals[dirIndex];

    //LogInfo("BlockID: %d", blockId); 
    //blockTextureLookups[blockId].CubeVertexs[dirIndex]

    // Add First Triangle
    for(int i = 0; i < 6; i++){
        mesh.vertices.push_back(CubeVertexs[tris.tri[i]] + pos);
        mesh.uv.push_back( Vector3(FaceUvs[i].x, FaceUvs[i].y, blockTextureLookups[blockId].CubeVertexs[dirIndex]));
        mesh.normals.push_back(normal);
        //mesh.indices.push_back(mesh.vertices.size()-1);
    }
}

void AddFace(unsigned short blockId, Vector3 pos, int dirIndex, Mesh& mesh, std::unordered_map<Vector3, MeshBuildHelper>& indexHelper){
    FaceTriangles tris = FacesTriangles[dirIndex];
    Vector3 normal = ChunkVoxelNormals[dirIndex];

    //LogInfo("BlockID: %d", blockId); 
    //blockTextureLookups[blockId].CubeVertexs[dirIndex]

    // Add First Triangle
    for(int i = 0; i < 6; i++){
        bool hasAddVertice = false;
        MeshBuildHelper meshBuildHelper;
        if(indexHelper.count(CubeVertexs[tris.tri[i]] + pos) > 0){
            hasAddVertice = true;
            meshBuildHelper = indexHelper[CubeVertexs[tris.tri[i]] + pos];
        }

        if(hasAddVertice && meshBuildHelper.normal == normal){
            mesh.indices.push_back(meshBuildHelper.triangleIndex);
        } else {
            mesh.vertices.push_back(CubeVertexs[tris.tri[i]] + pos);
            mesh.uv.push_back( Vector3(FaceUvs[i].x, FaceUvs[i].y, blockTextureLookups[blockId].CubeVertexs[dirIndex]));
            mesh.normals.push_back(normal);
            mesh.indices.push_back(mesh.vertices.size()-1);
            indexHelper[CubeVertexs[tris.tri[i]] + pos] = MeshBuildHelper{(int)mesh.vertices.size()-1, normal};
        }
    }
}

void SpawnTree(IVector3 localCoord, ChunkDataHolder& data){
    const IVector2 minMaxTreeTrunkHeight = {5, 8};
    const IVector2 minMaxLeafLayer = {3, 5};

    int treeTrunkHeight = random(minMaxTreeTrunkHeight.x, minMaxTreeTrunkHeight.y);
    int leafLayer = random(minMaxLeafLayer.x, minMaxLeafLayer.y);

    for(int i = 0; i < treeTrunkHeight; i++){
        if(data.chunkData->IsOffChunk(localCoord.x, localCoord.y+i, localCoord.z)) continue;
        data.chunkData->SetVoxel(localCoord.x, localCoord.y+i, localCoord.z, Voxel{4});
    }

    for(int i = 0; i < leafLayer; i++){
        for(int x = localCoord.x-i; x <= localCoord.x+i; x++){
            for(int z = localCoord.z-i; z <= localCoord.z+i; z++){
                IVector3 targetCood = {x, localCoord.y+(treeTrunkHeight-i), z};
                if(data.chunkData->IsOffChunk(targetCood.x, targetCood.y, targetCood.z)) continue;
                data.chunkData->SetVoxel(targetCood.x, targetCood.y, targetCood.z, Voxel{5});
            }
        }
    }
}

void ChunkBuilderLayer::BuildData(IVector3 coord, ChunkDataHolder& data){
    for(int z = 0; z < data.chunkData->GetSize(); z++){
        for(int x = 0; x < data.chunkData->GetSize(); x++){
            const int baseY = 192;
            int noiseHeight = 32;
            const float noiseScale = 1;
            const float treeNoiseScale = 50;
            const float treeNoiseOffset = 2000;

            int _x = x + coord.x * data.chunkData->GetSize();
            int _z = z + coord.z * data.chunkData->GetSize();

            //LogInfo("X: %d Y: %d", _x, _z);

            float noiseValue = fnlGetNoise2D(&noise, _x*noiseScale, _z*noiseScale);
            int targetY = noiseValue * noiseHeight + baseY;

            float treeNoiseValue = fnlGetNoise2D(&noise, (_x+treeNoiseOffset)*treeNoiseScale, (_z+treeNoiseOffset)*treeNoiseScale);

            //LogInfo("Target Y: %d NoiseValue: %f", targetY, noiseValue);

            for(int y = 0; y < data.chunkData->GetHeight(); y++){
                if(data.chunkData->GetVoxel(x, y, z).id != 0) continue;

                Voxel targetVoxel = y <= targetY ? Voxel{2} : Voxel{0};
                
                if(targetVoxel.id != 0){
                    if(y < 132){
                        targetVoxel.id = 2;
                    } else if(y < targetY){
                        targetVoxel.id = 1;
                    }
                } else {
                    if(y < 200){
                        targetVoxel.id = 3;
                    } else {
                        if(treeNoiseValue > 0.98f && (y-1) >=0 
                            && blockTextureLookups[data.chunkData->GetVoxel(x, y-1, z).id].classType == BlockClassType::Solid 
                        ){
                            targetVoxel.id = 4;
                            SpawnTree({x, y, z}, data);
                        }
                    }
                }
                
                data.chunkData->SetVoxel(x, y, z, targetVoxel);
                //chunkData.SetVoxel(x, y, z, Voxel{1});
            }
        }
    }
}

void ChunkBuilderLayer::BuildMesh(ChunkDataHolder& data){
    data.opaqueMesh->vertices.clear();
    data.opaqueMesh->uv.clear();
    data.opaqueMesh->normals.clear();
    data.opaqueMesh->indices.clear();

    for(int z = 0; z < data.chunkData->GetSize(); z++){
        for(int x = 0; x < data.chunkData->GetSize(); x++){
            for(int y = 0; y < data.chunkData->GetHeight(); y++){
                if(data.chunkData->GetVoxel(x, y, z).id == 0) continue;

                for(int d = 0; d < 6; d++){
                    IVector3 curCoord(x, y, z);
                    IVector3 dirCoodr = curCoord + ChunkVoxelNeighbors[d];

                    if(
                        data.chunkData->IsOffChunk(dirCoodr.x, dirCoodr.y, dirCoodr.z) == false
                        && data.chunkData->GetVoxel(dirCoodr.x, dirCoodr.y, dirCoodr.z).id == 0
                    ){
                        AddFace(data.chunkData->GetVoxel(x, y, z).id, Vector3(x, y, -z), d, *data.opaqueMesh);
                    }
                }
            }
        }
    }
}

void ChunkBuilderLayer::BuildMeshOpaque(ChunkDataHolder& data){
    data.opaqueMesh->vertices.clear();
    data.opaqueMesh->uv.clear();
    data.opaqueMesh->normals.clear();
    data.opaqueMesh->indices.clear();
    std::unordered_map<Vector3, MeshBuildHelper> indexHelper;

    for(int z = 0; z < data.chunkData->GetSize(); z++){
        for(int x = 0; x < data.chunkData->GetSize(); x++){
            for(int y = 0; y < data.chunkData->GetHeight(); y++){
                if(data.chunkData->GetVoxel(x, y, z).id == 0) continue;
                if(blockTextureLookups[data.chunkData->GetVoxel(x, y, z).id].blockType != BlockMaterialType::Opaque) continue;

                for(int d = 0; d < 6; d++){
                    IVector3 curCoord(x, y, z);
                    IVector3 dirCoodr = curCoord + ChunkVoxelNeighbors[d];
                    bool neighborsIsAir = false;

                    if(data.chunkData->IsOffChunk(dirCoodr.x, dirCoodr.y, dirCoodr.z) == true){
                        int nextVoxelId = world->CheckForVoxel((Vector3(dirCoodr)) + data.chunkData->CoordPos(), false);
                        if(nextVoxelId == 0 || nextVoxelId == 3){
                            neighborsIsAir = true;
                        }
                    } else if(data.chunkData->GetVoxel(dirCoodr.x, dirCoodr.y, dirCoodr.z).id == 0 || data.chunkData->GetVoxel(dirCoodr.x, dirCoodr.y, dirCoodr.z).id == 3){
                        neighborsIsAir = true;
                    }

                    if(neighborsIsAir) AddFace(data.chunkData->GetVoxel(x, y, z).id, Vector3(x, y, -z), d, *data.opaqueMesh);
                }
            }
        }
    }
}

void ChunkBuilderLayer::BuildMeshWater(ChunkDataHolder& data){
    data.waterMesh->vertices.clear();
    data.waterMesh->uv.clear();
    data.waterMesh->normals.clear();
    data.waterMesh->indices.clear();
    std::unordered_map<Vector3, MeshBuildHelper> indexHelper;

    for(int z = 0; z < data.chunkData->GetSize(); z++){
        for(int x = 0; x < data.chunkData->GetSize(); x++){
            for(int y = 0; y < data.chunkData->GetHeight(); y++){
                if(data.chunkData->GetVoxel(x, y, z).id == 0) continue;
                if(blockTextureLookups[data.chunkData->GetVoxel(x, y, z).id].blockType != BlockMaterialType::Water) continue;

                for(int d = 0; d < 6; d++){
                    IVector3 curCoord(x, y, z);
                    IVector3 dirCoodr = curCoord + ChunkVoxelNeighbors[d];

                    bool neighborsIsAir = false;

                    if(data.chunkData->IsOffChunk(dirCoodr.x, dirCoodr.y, dirCoodr.z) == true){
                        if(world->CheckForVoxelIsEmpty(Vector3(dirCoodr) + data.chunkData->CoordPos(), false)){
                            neighborsIsAir = true;
                        }
                    } else if(data.chunkData->GetVoxel(dirCoodr.x, dirCoodr.y, dirCoodr.z).id == 0){
                        neighborsIsAir = true;
                    }

                    if(neighborsIsAir) AddFace(data.chunkData->GetVoxel(x, y, z).id, Vector3(x, y, -z), d, *data.waterMesh);
                }
            }
        }
    }
}

void ChunkBuilderLayer::BuildMesh2(ChunkDataHolder& data){
    BuildMeshOpaque(data);
    BuildMeshWater(data);

    /*
    data.opaqueMesh->vertices.clear();
    data.opaqueMesh->uv.clear();
    data.opaqueMesh->normals.clear();
    data.opaqueMesh->indices.clear();

    for(int z = 0; z < data.chunkData->GetSize(); z++){
        for(int x = 0; x < data.chunkData->GetSize(); x++){
            for(int y = 0; y < data.chunkData->GetHeight(); y++){
                if(data.chunkData->GetVoxel(x, y, z).id == 0) continue;

                for(int d = 0; d < 6; d++){
                    IVector3 curCoord(x, y, z);
                    IVector3 dirCoodr = curCoord + ChunkVoxelNeighbors[d];

                    bool neighborsIsAir = false;

                    if(data.chunkData->IsOffChunk(dirCoodr.x, dirCoodr.y, dirCoodr.z) == true){
                        if(world->CheckForVoxelIsEmpty(Vector3(dirCoodr) + data.chunkData->CoordPos(), false)){
                            neighborsIsAir = true;
                        }
                    } else if(data.chunkData->GetVoxel(dirCoodr.x, dirCoodr.y, dirCoodr.z).id == 0){
                        neighborsIsAir = true;
                    }

                    if(neighborsIsAir) AddFace(data.chunkData->GetVoxel(x, y, z).id, Vector3(x, y, -z), d, *data.opaqueMesh);
                }
            }
        }
    }*/
}
