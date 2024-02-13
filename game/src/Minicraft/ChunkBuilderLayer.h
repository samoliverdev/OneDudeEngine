#pragma once
#include <OD/OD.h>
#include "VextexData.h"
#include "ChunkData.h"

using namespace OD;

class ChunkBuilderLayer{
public:
    ChunkBuilderLayer(){
        Ref<Texture2D> tex = AssetManager::Get().LoadTexture2D("res/Game/Textures/floor.jpg");
        material = CreateRef<Material>(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl"));
        material->SetTexture("mainTex", tex);
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

    void BuildMesh(ChunkData& chunkData, MeshRendererComponent& mesh){
        LogWarning("Building Chunk Mesh");

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
                for(int y = 0; y < chunkData.GetSize(); y++){
                    if(chunkData.GetVoxel(x, y, z).id == 0) continue;

                    for(int d = 0; d < 6; d++){
                        IVector3 curCoord(x, y, z);
                        IVector3 dirCoodr = curCoord + ChunkVoxelNeighbors[d];

                        if(
                            chunkData.IsOffChunk(dirCoodr.x, dirCoodr.y, dirCoodr.z)
                            || chunkData.GetVoxel(dirCoodr.x, dirCoodr.y, dirCoodr.z).id == 0
                        ){
                            AddFace(Vector3(x, y, z), d, *mesh.mesh);
                        }
                    }
                }
            }
        }

        mesh.mesh->UpdateMesh();
    }

private:
    Ref<Material> material;
};