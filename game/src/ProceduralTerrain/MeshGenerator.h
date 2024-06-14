#pragma once
#include "Noise.h"

struct MeshData{
    std::vector<Vector3> vertices;
    std::vector<Vector3> uvs;
    std::vector<unsigned int> triangles;

    int triangleIndex = 0;

    MeshData(int meshWidth, int meshHeight){
        vertices.resize(meshWidth * meshHeight);
        uvs.resize(meshWidth * meshHeight);
        triangles.resize((meshWidth-1)*(meshHeight-1)*6);
    }

    void AddTriangle(int a, int b, int c){
        triangles[triangleIndex] = a;
        triangles[triangleIndex+1] = b;
        triangles[triangleIndex+2] = c;
        triangleIndex += 3;
    }

    Ref<Mesh> CreateMesh(){
        Ref<Mesh> mesh = CreateRef<Mesh>();
        mesh->vertices = vertices;
        mesh->uv = uvs;
        mesh->indices = triangles;
        mesh->CalculateNormals();
        mesh->UpdateMesh();
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
                meshData->vertices[vertexIndex] = Vector3(x+topLeftX, heightMap->Get(x, y)*heightMultiplier, -(y-topLeftZ));
                meshData->uvs[vertexIndex] = Vector3(x/(float)width, y/(float)height, 0);

                if(x < width-1 && y < height-1){
                    meshData->AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
                    meshData->AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
                }

                vertexIndex += 1;
            }
        }

        return meshData;
    }
}