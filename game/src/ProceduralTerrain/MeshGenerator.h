#pragma once
#include "Noise.h"
#include "OD/Physics/PhysicsSystem.h"

struct MeshData{
    Ref<Mesh> mesh = CreateRef<Mesh>();
    Ref<MeshShapeData> shapeData;

    int triangleIndex = 0;

    MeshData(int meshWidth, int meshHeight){
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

namespace MeshGenerator{
    inline Ref<MeshData> GenerateTerrainMesh(Ref<NoiseData>& heightMap, float heightMultiplier, int levelOfDetail){
        int width = heightMap->width;
        int height = heightMap->height;
        float topLeftX = (width - 1) / -2.0f;
        float topLeftZ = (height - 1) / 2.0f;

        int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
        int verticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

        Ref<MeshData> meshData = CreateRef<MeshData>(verticesPerLine, verticesPerLine);
        int vertexIndex = 0;

        for(int y = 0; y < height; y += meshSimplificationIncrement){
            for(int x = 0; x < width; x += meshSimplificationIncrement){
                meshData->mesh->vertices[vertexIndex] = Vector3(x+topLeftX, heightMap->Get(x, y)*heightMultiplier, -(y-topLeftZ));
                meshData->mesh->uv[vertexIndex] = Vector3(x/(float)width, y/(float)height, 0);

                if(x < width-1 && y < height-1){
                    meshData->AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
                    meshData->AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
                }

                vertexIndex += 1;
            }
        }

        meshData->shapeData = CreateMeshShapeData(meshData->mesh->vertices, meshData->mesh->indices);
        meshData->mesh->CalculateNormals();

        return meshData;
    }
}