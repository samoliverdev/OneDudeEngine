#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "OD/Animation/Skeleton.h"
#include "OD/Animation/Clip.h"
#include "Mesh.h"
#include "Material.h"
#include "Culling.h"

namespace cereal{
    class BinaryOutputArchive;
    class BinaryInputArchive;
}

namespace OD{

struct ModelLoadSettings{
    Ref<Shader> customShader = nullptr;
    float scale = 1.0f;
    bool generateColliderData = false; //true;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, scale);
        ArchiveDumpNVP(ar, generateColliderData);

        AssetRefSerialize<Shader> shaderRef(customShader);
        ArchiveDumpNVP(ar, shaderRef);
    }
};

class OD_API Model: public Asset{
public:
    struct MaterialTarget{
        struct Tex{
            int texIndex;
            std::string extPath;
            bool isFromTexturesArray;

            template <class Archive>
            void serialize(Archive& ar){
                ar(
                    texIndex, 
                    extPath, 
                    isFromTexturesArray
                );
            }
        };  

        std::vector<std::string> argNames;
        std::vector<Tex> texs;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDump(ar, argNames);
            ArchiveDump(ar, texs);
        }
    };

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
    std::vector<Ref<Texture2D>> textures;
    std::vector<MaterialTarget> materialTargets;
    std::vector<Ref<Material>> materials;
    std::vector<Matrix4> matrixs;
    std::vector<Ref<ClipT>> animationClips;
    Skeleton skeleton;
    ModelLoadSettings settings;

    Ref<class MeshShapeData> modelShapeData = nullptr;

    Ref<ClipT> FindClipByName(const std::string& name);
    void OnGui() override;
    void SetPath(const std::string& inPath);
    bool LoadFromFile(const std::string& path) override;
    bool LoadFromPackage(const std::string& path, Package& package) override;
    bool Save(const std::string& outPath, SaveType type) override;
    std::vector<std::string> GetFileAssociations() override;
    void SetShader(Ref<Shader> customShader);
    void Reload() override;

    static bool CreateFromFile(Model& model, std::string const &path, ModelLoadSettings loadSettings = {});
    static bool CreateFromPackage(Model& model, std::string const &path, Package& package, ModelLoadSettings loadSettings = {});

    static AABB GenerateAABB(Model& model);
    static Sphere GenerateSphereBV(Model& model);

private: 
    void CreateMaterialsFromTargets();

    void SaveTo(cereal::BinaryOutputArchive& ar);
    void LoadFrom(cereal::BinaryInputArchive& ar);

    void Clear();
};

}