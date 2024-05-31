#include "ChunkBuilderLayer.h"
#include "Ultis/Ultis.h"
#include "WorldManagerSystem.h"

struct BlockTextureLookup{
    int CubeVertexs[6] = { 0, 0, 0, 0, 0, 0};
    BlockTextureLookup(int all){
        CubeVertexs[0] = all;
        CubeVertexs[1] = all;
        CubeVertexs[2] = all;
        CubeVertexs[3] = all;
        CubeVertexs[4] = all;
        CubeVertexs[5] = all;
    }
    BlockTextureLookup(int front, int back, int left, int right, int up, int down){
        CubeVertexs[0] = front;
        CubeVertexs[1] = back;
        CubeVertexs[2] = left;
        CubeVertexs[3] = right;
        CubeVertexs[4] = up;
        CubeVertexs[5] = down;
    }
};

const std::vector<std::string> texturePaths = {
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/gravel_stone.png", //0
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/dirt.png", //1
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/dirt_grass.png", //2
    "res/Game/Textures/kenney_voxel-pack/PNG/Tiles/leaves.png", //3
};

const BlockTextureLookup blockTextureLookups[] = {
    BlockTextureLookup(0), // air
    BlockTextureLookup(0), // Stone
    BlockTextureLookup(2, 2, 2, 2, 3, 1), // DirtGrass
};

float Remap(float In, Vector2 InMinMax, Vector2 OutMinMax){
    return OutMinMax.x + (In - InMinMax.x) * (OutMinMax.y - OutMinMax.x) / (InMinMax.y - InMinMax.x);
}

ChunkBuilderLayer::ChunkBuilderLayer(WorldManagerSystem* inWorldManagerSystem){
    texture = CreateRef<Texture2DArray>(texturePaths);
    material = CreateRef<Material>(AssetManager::Get().LoadShaderFromFile("res/Game/Shaders/Minicraft.glsl"));
    material->SetTexture("mainTex", texture);

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
        mesh.indices.push_back(mesh.vertices.size()-1);
    }
}

void ChunkBuilderLayer::BuildData(IVector3 coord, Ref<ChunkData> chunkData){
    for(int z = 0; z < chunkData->GetSize(); z++){
        for(int x = 0; x < chunkData->GetSize(); x++){
            const int baseY = 192;
            int noiseHeight = 32;
            const float noiseScale = 1;

            int _x = x + coord.x * chunkData->GetSize();
            int _z = z + coord.z * chunkData->GetSize();

            //LogInfo("X: %d Y: %d", _x, _z);

            float noiseValue = fnlGetNoise2D(&noise, _x*noiseScale, _z*noiseScale);
            int targetY = noiseValue * noiseHeight + baseY;

            //LogInfo("Target Y: %d NoiseValue: %f", targetY, noiseValue);

            for(int y = 0; y < chunkData->GetHeight(); y++){
                Voxel targetVoxel = y <= targetY ? Voxel{2} : Voxel{0};
                
                if(targetVoxel.id != 0){
                    if(y < 132){
                        targetVoxel.id = 2;
                    } else if(y < targetY){
                        targetVoxel.id = 1;
                    }
                }
                chunkData->SetVoxel(x, y, z, targetVoxel);
                //chunkData.SetVoxel(x, y, z, Voxel{1});
            }
        }
    }
}

void ChunkBuilderLayer::BuildMesh(ChunkData& chunkData, Ref<Mesh>& mesh){
    if(mesh == nullptr){
        mesh = CreateRef<Mesh>();
    }

    mesh->vertices.clear();
    mesh->uv.clear();
    mesh->normals.clear();
    mesh->indices.clear();

    for(int z = 0; z < chunkData.GetSize(); z++){
        for(int x = 0; x < chunkData.GetSize(); x++){
            for(int y = 0; y < chunkData.GetHeight(); y++){
                if(chunkData.GetVoxel(x, y, z).id == 0) continue;

                for(int d = 0; d < 6; d++){
                    IVector3 curCoord(x, y, z);
                    IVector3 dirCoodr = curCoord + ChunkVoxelNeighbors[d];

                    if(
                        chunkData.IsOffChunk(dirCoodr.x, dirCoodr.y, dirCoodr.z) == false
                        && chunkData.GetVoxel(dirCoodr.x, dirCoodr.y, dirCoodr.z).id == 0
                    ){
                        AddFace(chunkData.GetVoxel(x, y, z).id, Vector3(x, y, -z), d, *mesh);
                    }
                }
            }
        }
    }
}

void ChunkBuilderLayer::BuildMesh2(/*IVector3 coord,*/ ChunkData& chunkData, Ref<Mesh>& mesh/*, std::unordered_map<IVector3, Ref<ChunkData>>& loadedChunkDatas*/){
    mesh->vertices.clear();
    mesh->uv.clear();
    mesh->normals.clear();
    mesh->indices.clear();

    for(int z = 0; z < chunkData.GetSize(); z++){
        for(int x = 0; x < chunkData.GetSize(); x++){
            for(int y = 0; y < chunkData.GetHeight(); y++){
                if(chunkData.GetVoxel(x, y, z).id == 0) continue;

                for(int d = 0; d < 6; d++){
                    IVector3 curCoord(x, y, z);
                    IVector3 dirCoodr = curCoord + ChunkVoxelNeighbors[d];

                    bool neighborsIsAir = false;

                    if(chunkData.IsOffChunk(dirCoodr.x, dirCoodr.y, dirCoodr.z) == true){
                        /*
                        IVector3 neighborsGlobalCoord = coord + ChunkVoxelNeighbors[d];
                        if(loadedChunkDatas.count(neighborsGlobalCoord)){
                            IVector3 neighborsLocalCoord = {
                                chunkData.IsOffChunkX(dirCoodr.x) ? math::abs(math::abs(dirCoodr.x) - chunkData.GetSize()) : dirCoodr.x,
                                chunkData.IsOffChunkY(dirCoodr.y) ? math::abs(math::abs(dirCoodr.y) - chunkData.GetHeight()) : dirCoodr.y,
                                chunkData.IsOffChunkZ(dirCoodr.z) ? math::abs(math::abs(dirCoodr.z) - chunkData.GetSize()) : dirCoodr.z
                            };
                            if(loadedChunkDatas[neighborsGlobalCoord]->GetVoxel(neighborsLocalCoord.x, neighborsLocalCoord.y, neighborsLocalCoord.z).id == 0){
                                neighborsIsAir = true;
                            } else {
                                neighborsIsAir = false;
                            }
                        } else {
                            neighborsIsAir = false;
                        }
                        */

                        if(world->CheckForVoxelIsEmpty(Vector3(dirCoodr) + chunkData.CoordPos(), false)){
                            neighborsIsAir = true;
                        }
                    } else if(chunkData.GetVoxel(dirCoodr.x, dirCoodr.y, dirCoodr.z).id == 0){
                        neighborsIsAir = true;
                    }

                    if(neighborsIsAir) AddFace(chunkData.GetVoxel(x, y, z).id, Vector3(x, y, -z), d, *mesh);
                }
            }
        }
    }
}
