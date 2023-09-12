#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace OD{

struct BoneInfo{
    int id; //id is index in finalBoneMatrices
    Matrix4 offset; //offset matrix transforms vertex from model space to bone space
};

class Animation;

class Model: public Asset{
public:
    std::vector<Ref<Mesh>> meshs;
    std::vector<Ref<Material>> materials;
    std::vector<Ref<Animation>> animations;
    
    inline int& boneCounter(){ return _boneCounter; }
    inline std::map<std::string, BoneInfo>& boneInfoMap() { return _boneInfoMap; }

    static Ref<Model> CreateFromFile(std::string const &path, Ref<Shader> customShader = nullptr);

private:
    void processNode(aiNode *node, const aiScene *scene, Ref<Shader> customShader);
    Ref<Mesh> processMesh(aiMesh *mesh, const aiScene *scene);
    Ref<Material> processMaterial(aiMesh *mesh, const aiScene *scene, Ref<Shader> customShader);
    std::vector<Ref<Texture2D>> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    
    void ExtractBoneWeightForVertices(Ref<Mesh> _mesh, aiMesh* mesh, const aiScene* scene);
    void AddBoneData(BoneData& boneData, unsigned int boneId, float weight);

    std::string _directory;
    std::map<std::string, BoneInfo> _boneInfoMap;
    int _boneCounter = 0;
};


}