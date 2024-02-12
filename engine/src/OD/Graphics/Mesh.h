#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"

namespace OD {

#define MAX_BONE_INFLUENCE 4

struct BoneData{
    IVector4 boneId;
    Vector4 weight;
};

class Graphics;

class Mesh: public Asset{
    friend class Graphics;
public:
    std::vector<Vector3> vertices;
    std::vector<Vector3> uv;
    std::vector<Vector3> normals;
    std::vector<Vector4> colors;
    std::vector<Vector3> tangents;
    std::vector<Vector4> weights;
	std::vector<IVector4> influences;
    std::vector<unsigned int> indices;

    std::vector<Matrix4> instancingModelMatrixs;
    
    Mesh();
    ~Mesh();

    void UpdateMesh();
    void UpdateMeshInstancingModelMatrixs();

    bool IsValid();
    void Destroy();

    inline bool IsReadable(){ return isReadable; }
    inline int VertexCount(){ return vertexCount; }
    inline int IndiceCount(){ return indiceCount; }

    static Mesh FullScreenQuad();
    static Mesh SkyboxCube();
    static Mesh CenterQuad(bool useIndices);

    inline unsigned int RendererId(){ return vao; }

private:
    bool isReadable = false;

    unsigned int vao = 0;

    unsigned int vertexVbo = 0;
    unsigned int uvVbo = 0;
    unsigned int normalVbo = 0;
    unsigned int colorVbo = 0;
    unsigned int tangentVbo = 0;

    unsigned int instancingModelMatrixsVbo = 0;
    
    unsigned int jointVbo = 0; 
    unsigned int weightsVbo = 0;  
    
    unsigned int ebo = 0;

    unsigned int vertexCount = 0;
    unsigned int indiceCount = 0;
};

}