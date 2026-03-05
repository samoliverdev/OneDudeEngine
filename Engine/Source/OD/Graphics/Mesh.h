#pragma once
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
    Mesh(const std::string& label);
    Mesh(const Mesh& mesh);
    ~Mesh() override;

    void AppedFrom(const Mesh& mesh);

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

    virtual size_t RamUsage() override { return ramUsage; }
    virtual size_t VRamUsage() override { return vramUsage; }

private:
    int id;

    bool isReadable = false;
    unsigned int vertexCount = 0;
    unsigned int indiceCount = 0;
    size_t ramUsage;
    size_t vramUsage;
    MeshDataGL;
    MeshDataWG;

    void Bind();
    size_t CalculateRamUsage();
    size_t CalculateVRamUsage();
};

}