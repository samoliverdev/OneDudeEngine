#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "OD/Platform/OpenGL/GL.h"
#include "OD/Platform/WebGPU/WebGPU.h"

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
    QUADS,
    TRIANGLES_STRIP
};

class OD_API Mesh: public Asset{
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
    friend class WebGPUGraphicsDevice;
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

    inline void AppedFrom(Mesh& mesh){
        unsigned int vertexOffset = static_cast<unsigned int>(vertices.size());

        vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
        uv.insert(uv.end(), mesh.uv.begin(), mesh.uv.end());
        normals.insert(normals.end(), mesh.normals.begin(), mesh.normals.end());
        colors.insert(colors.end(), mesh.colors.begin(), mesh.colors.end());
        tangents.insert(tangents.end(), mesh.tangents.begin(), mesh.tangents.end());
        weights.insert(weights.end(), mesh.weights.begin(), mesh.weights.end());
        influences.insert(influences.end(), mesh.influences.begin(), mesh.influences.end());
        //indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());

        for(unsigned int index : mesh.indices){
            indices.push_back(index + vertexOffset);
        }
    }

    inline void ClearRuntimeData(){
        vertices.clear();
        uv.clear();
        normals.clear();
        colors.clear();
        tangents.clear();
        weights.clear();
        influences.clear();
        indices.clear();
    }
    
    Mesh();
    Mesh(const Mesh& mesh);
    ~Mesh() override;

    //Mesh& operator=(const Mesh& other) = delete;
    //Mesh(const Mesh& other) = delete;

    bool LoadFromFile(const std::string& path) override;
    bool Save(const std::string& outPath, SaveType type) override;
    void OnGui() override;

    void CalculateNormals();
    void CalculateTangent();

    void Submit();
    void Submit(
        std::vector<unsigned int>* indices,
        std::vector<Vector3>* vertices,
        std::vector<Vector3>* uv = nullptr,
        std::vector<Vector3>* normals = nullptr,
        std::vector<Vector4>* colors = nullptr,
        std::vector<Vector3>* tangents = nullptr,
        std::vector<Vector4>* weights = nullptr,
        std::vector<IVector4>* influences = nullptr
    );
    void SubmitInstancingModelMatrixs();
    void SubmitInstancingCustomModelMatrixs(Matrix4* modelMatrixs, int count);

    inline bool IsReadable(){ return isReadable; }
    inline int VertexCount(){ return vertexCount; }
    inline int IndiceCount(){ return indiceCount; }

    static Ref<Mesh> FullScreenQuad();
    static Ref<Mesh> SkyboxCube();
    static Ref<Mesh> CenterQuad(bool useIndices);

    //inline unsigned int RendererId(){ return vao; }

    inline int Id(){ return id; }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, vertices);
        ArchiveDumpNVP(ar, uv);
        ArchiveDumpNVP(ar, normals);
        ArchiveDumpNVP(ar, colors);
        ArchiveDumpNVP(ar, tangents);
        ArchiveDumpNVP(ar, weights);
        ArchiveDumpNVP(ar, influences);
        ArchiveDumpNVP(ar, indices);
        ArchiveDumpNVP(ar, drawMode);

        if constexpr (Archive::is_loading()){
            Submit();
        }
    }

private:
    int id;

    bool isReadable = false;
    unsigned int vertexCount = 0;
    unsigned int indiceCount = 0;
    MeshDataGL;
    MeshDataWG;

    void Bind();
};

}