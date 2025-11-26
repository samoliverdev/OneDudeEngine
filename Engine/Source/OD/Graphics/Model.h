#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Mesh.h"
#include "Material.h"
#include "Culling.h"
#include "OD/Animation/Skeleton.h"
#include "OD/Animation/Clip.h"

namespace OD{

/*class Mesh{
    std::vector<Vector3> vertices;
    std::vector<Vector3> uv;
    std::vector<Vector3> normals;
    std::vector<Vector4> colors;
    std::vector<Vector3> tangents;
    std::vector<Vector4> weights;
    std::vector<IVector4> influences;
    std::vector<unsigned int> indices;
};

class Model{
public:
    struct RenderTarget{
        int meshIndex;
        int materialIndex;
        int bindPoseIndex;
    };

    std::vector<RenderTarget> renderTargets;
    std::vector<Ref<Mesh>> meshs;
    std::vector<Ref<Material>> materials;
    std::vector<Ref<Texture2D>> textures;
    std::vector<Matrix4> matrixs;
};*/

struct ModelLoadSettings{
    Ref<Shader> customShader = nullptr;
    float scale = 1.0f;
    bool generateColliderData = false; //true;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, scale);
        ArchiveDumpNVP(ar, generateColliderData);
    }
};

class OD_API Model: public Asset{
public:
    struct RenderTarget{
        int meshIndex;
        int materialIndex;
        int bindPoseIndex;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDump(ar, meshIndex);
            ArchiveDump(ar, materialIndex);
            ArchiveDump(ar, bindPoseIndex);
        }
    };

    std::vector<RenderTarget> renderTargets;
    std::vector<Ref<Mesh>> meshs;
    std::vector<Ref<Material>> materials;
    std::vector<Ref<Texture2D>> textures;
    std::vector<Matrix4> matrixs;
    Skeleton skeleton;
    std::vector<Ref<ClipT>> animationClips;
    Ref<class MeshShapeData> modelShapeData = nullptr;

    Ref<ClipT> FindClipByName(const std::string& name);
    void OnGui() override;
    void SetPath(const std::string& inPath);
    bool LoadFromFile(const std::string& path) override;
    bool Save(const std::string& outPath, SaveType type) override;
    std::vector<std::string> GetFileAssociations() override;
    void SetShader(Ref<Shader> customShader);
    void Reload() override;

    static bool CreateFromFile(Model& model, std::string const &path, ModelLoadSettings loadSettings = {});
    static AABB GenerateAABB(Model& model);
    static Sphere GenerateSphereBV(Model& model);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, renderTargets);
        ArchiveDumpNVP(ar, meshs);
        ArchiveDumpNVP(ar, matrixs);
        ArchiveDumpNVP(ar, skeleton);
    }

private: 
    ModelLoadSettings settings;

    void Clear();
};

}