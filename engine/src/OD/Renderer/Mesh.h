#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"

namespace OD {

#define MAX_BONE_INFLUENCE 4

struct BoneData{
    int boneId[MAX_BONE_INFLUENCE] = {0};
    float weight[MAX_BONE_INFLUENCE] = {0};
};

class Renderer;

class Mesh: public Asset{
    friend class Renderer;
public:
    std::vector<Vector3> vertices;
    std::vector<Vector3> uv;
    std::vector<Vector3> normals;
    std::vector<Vector4> colors;
    std::vector<Vector3> tangents;
    
    std::vector<BoneData> boneDatas;

    std::vector<unsigned int> indices;

    Mesh();
    ~Mesh();

    void UpdateMesh();

    bool IsValid();
    void Destroy();

    inline bool IsReadable(){ return _isReadable; }
    inline int VertexCount(){ return _vertexCount; }
    inline int IndiceCount(){ return _indiceCount; }

    static Mesh FullScreenQuad();
    static Mesh SkyboxCube();
    static Mesh CenterQuad(bool useIndices);

private:
    bool _isReadable = false;

    unsigned int _vao = 0;

    unsigned int _vertexVbo = 0;
    unsigned int _uvVbo = 0;
    unsigned int _normalVbo = 0;
    unsigned int _colorVbo = 0;
    unsigned int _tangentVbo = 0;
    
    //unsigned int jointVbo; 
    //unsigned int weightsVbo;  
    unsigned int _boneDataVbo;  
    
    unsigned int _ebo = 0;

    unsigned int _vertexCount = 0;
    unsigned int _indiceCount = 0;
};

}