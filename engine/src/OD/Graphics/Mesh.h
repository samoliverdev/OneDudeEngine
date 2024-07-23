#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"

namespace OD {

#define MAX_BONE_INFLUENCE 4

struct OD_API BoneData{
    IVector4 boneId;
    Vector4 weight;
};

class Graphics;

enum class MeshDrawMode{
    TRIANGLES = 0,
    LINES,
    POINTS,
    QUADS
};

class OD_API Mesh: public Asset{
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

    MeshDrawMode drawMode = MeshDrawMode::TRIANGLES;
    
    Mesh();
    Mesh(const Mesh& mesh);
    ~Mesh() override;

    void CalculateNormals();

    void UpdateMesh();
    void UpdateMeshInstancingModelMatrixs();
    void UpdateMeshInstancingCustomModelMatrixs(Matrix4* modelMatrixs, int count);

    bool IsValid();
    void Destroy();

    inline bool IsReadable(){ return isReadable; }
    inline int VertexCount(){ return vertexCount; }
    inline int IndiceCount(){ return ebo != 0 ? indiceCount : vertexCount / 3; }

    static Ref<Mesh> FullScreenQuad();
    static Ref<Mesh> SkyboxCube();
    static Ref<Mesh> CenterQuad(bool useIndices);

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