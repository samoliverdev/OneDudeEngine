#include "ChunkBuilderLayer.h"
#include "Ultis/Ultis.h"

float Remap(float In, Vector2 InMinMax, Vector2 OutMinMax){
    return OutMinMax.x + (In - InMinMax.x) * (OutMinMax.y - OutMinMax.x) / (InMinMax.y - InMinMax.x);
}

ChunkBuilderLayer::ChunkBuilderLayer(){
    Ref<Texture2D> tex = AssetManager::Get().LoadTexture2D("res/Game/Textures/floor.jpg");
    material = CreateRef<Material>(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl"));
    material->SetTexture("mainTex", tex);

    noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_VALUE;
}

void AddFace(Vector3 pos, int dirIndex, Mesh& mesh){
    FaceTriangles tris = FacesTriangles[dirIndex];
    Vector3 normal = ChunkVoxelNormals[dirIndex];

    // Add First Triangle
    for(int i = 0; i < 6; i++){
        mesh.vertices.push_back(CubeVertexs[tris.tri[i]] + pos);
        mesh.uv.push_back(FaceUvs[i]);
        mesh.normals.push_back(normal);
        mesh.indices.push_back(mesh.vertices.size()-1);
    }
}

void ChunkBuilderLayer::BuildData(IVector3 coord, ChunkData& chunkData){
    for(int z = 0; z < chunkData.GetSize(); z++){
        for(int x = 0; x < chunkData.GetSize(); x++){
            const int baseY = 192;
            int noiseHeight = 32;
            const float noiseScale = 1;

            int _x = x + coord.x * chunkData.GetSize();
            int _z = z + coord.z * chunkData.GetSize();

            //LogInfo("X: %d Y: %d", _x, _z);

            float noiseValue = fnlGetNoise2D(&noise, _x*noiseScale, _z*noiseScale);;
            int targetY = noiseValue * noiseHeight + baseY;

            //LogInfo("Target Y: %d NoiseValue: %f", targetY, noiseValue);

            for(int y = 0; y < chunkData.GetHeight(); y++){
                chunkData.SetVoxel(x, y, z, y <= targetY ? Voxel{1} : Voxel{0});
                //chunkData.SetVoxel(x, y, z, Voxel{1});
            }
        }
    }
}

void ChunkBuilderLayer::BuildMesh(ChunkData& chunkData, MeshRendererComponent& mesh){
    //LogWarning("Building Chunk Mesh");

    mesh.material = material;
    if(mesh.mesh == nullptr){
        mesh.mesh = CreateRef<Mesh>();
    }

    mesh.mesh->vertices.clear();
    mesh.mesh->uv.clear();
    mesh.mesh->normals.clear();
    mesh.mesh->indices.clear();

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
                        AddFace(Vector3(x, y, z), d, *mesh.mesh);
                    }
                }
            }
        }
    }

    mesh.mesh->UpdateMesh();
    mesh.UpdateAABB();
}
