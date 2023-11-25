#include "AssimpLoader.h"
#include "OD/Core/AssetManager.h"

namespace OD{

namespace AssimpGLMHelpers{
	inline Matrix4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from){
		Matrix4 to;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		return to;
	}

	inline Vector3 GetGLMVec(const aiVector3D& vec){ 
		return Vector3(vec.x, vec.y, vec.z); 
	}

	inline Quaternion GetGLMQuat(const aiQuaternion& pOrientation){
		return Quaternion(pOrientation.x, pOrientation.y, pOrientation.z, pOrientation.w);
	}
};

struct LoadData{
    Ref<Model> model;
    aiScene* scene;
    std::string directory;

    std::map<int, Ref<Mesh>> meshDb;
    std::map<int, Ref<Material>> materialDb;

    Pose bindPose;
    std::vector<std::string> names;
    std::vector<aiBone*> bones;

    int boneCounter = 0;
};

void AddBoneData(Vector4& _weights, IVector4& _influences, unsigned int boneId, float weight){
    for(int i = 0; i < MAX_BONE_INFLUENCE; ++i){
        if(_influences[i] <= 0){
            _weights[i] = weight;
            _influences[i] = boneId;
            break;
        }
    }
}

int GetNodeIndex(LoadData& loadData, aiNode* target){
    if(target == nullptr) return -1;
    
    for(unsigned int i = 0; i < loadData.bones.size(); ++i){
        if(target == loadData.bones[i]->mNode) return (int)i;
    }

    return -1;
}

int AddToBindPose(LoadData& loadData, aiBone* bone, aiMesh* mesh){
    int boneID = loadData.boneCounter;
 
    if(loadData.names.size() < loadData.boneCounter+1){
        loadData.names.resize(loadData.boneCounter+1);
        loadData.bindPose.Resize(loadData.boneCounter+1);
        loadData.bones.resize(loadData.boneCounter+1);
    }

    auto m = AssimpGLMHelpers::ConvertMatrixToGLMFormat(bone->mNode->mTransformation);

    loadData.names[boneID] = std::string(bone->mName.C_Str());
    loadData.bones[boneID] = bone;
    loadData.bindPose.SetLocalTransform(boneID, Transform(m));
    
    int r = loadData.boneCounter;
    loadData.boneCounter += 1;
    return r;
}

void UpdateSkeleton(LoadData& loadData, const aiScene* scene){
    for(int i = 0; i < loadData.bones.size(); i++){
        aiBone* bone = loadData.bones[i];

        int parentID = -1;
        if(bone->mNode->mParent != nullptr){
            parentID = GetNodeIndex(loadData, bone->mNode->mParent);
            loadData.bindPose.SetParent(i, parentID);
        }

        LogInfo("Bone -> Name: %s, Id: %d: Parent: %d", bone->mName.C_Str(), i, parentID);
    }

    loadData.model->skeleton.Set(loadData.bindPose, loadData.bindPose, loadData.names);
}

int GetFromBindPose(LoadData& skeletonData, aiBone* bone){
    for(int i = 0; i < skeletonData.names.size(); i++){
        if(skeletonData.names[i] == std::string(bone->mName.C_Str())) return i;
    }
    return -1;
}

void ExtractBoneWeightForVertices(LoadData& skeletonData, Ref<Mesh> _mesh, aiMesh* mesh, const aiScene* scene){
    _mesh->weights.resize(_mesh->vertices.size());
    _mesh->influences.resize(_mesh->vertices.size());

    for(int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex){
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        //LogInfo("Bone -> Name: %s, Id: %d", boneName.c_str(), boneID);

        boneID = GetFromBindPose(skeletonData, mesh->mBones[boneIndex]);
        if(boneID == -1){
            boneID = AddToBindPose(skeletonData, mesh->mBones[boneIndex], mesh);
        } 

        assert(boneID != -1);
        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for(int weightIndex = 0; weightIndex < numWeights; ++weightIndex){
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;

            assert(vertexId <= _mesh->vertices.size());
            AddBoneData(_mesh->weights[vertexId], _mesh->influences[vertexId], boneID, weight);
        }
    }
}

std::vector<Ref<Texture2D>> loadMaterialTextures(LoadData& loadData, aiMaterial *mat, aiTextureType type, std::string typeName){
    std::vector<Ref<Texture2D>> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++){
        aiString str;
        aiTextureMapping map;
        unsigned int uvIndex;
        ai_real blend;
        mat->GetTexture(type, i, &str, &map, &uvIndex, &blend);

        //LogInfo("Blende %f", (float)blend);

        std::string filename = loadData.directory + '/' +std::string(str.C_Str());
        
        //Ref<Texture2D> texture = Texture2D::CreateFromFile(filename.c_str(), false, TextureFilter::Linear);
        Ref<Texture2D> texture = AssetManager::Get().LoadTexture2D(filename.c_str(), {TextureFilter::Linear, true});
        textures.push_back(texture);
    }

    if(textures.empty()){
        textures.push_back(AssetManager::Get().LoadTexture2D(
            "res/Builtins/Textures/White.jpg", 
            {OD::TextureFilter::Linear, false}
        ));
    }

    return textures;
}

int getMaterialIndex(aiMaterial *mesh, const aiScene *scene){
    int meshIndex = -1;
    for(int i = 0; i < scene->mNumMaterials; i++){
        if(mesh == scene->mMaterials[i]){
            meshIndex = i;
            break;
        }
    }
    return meshIndex;
}

Ref<Material> processMaterial(LoadData& loadData, aiMesh *mesh, const aiScene *scene, Ref<Shader> customShader){
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex]; 

    int meshIndex = getMaterialIndex(material, scene);
    if(meshIndex != -1 && loadData.materialDb.count(meshIndex)) return loadData.materialDb[meshIndex];

    Ref<Material> out = CreateRef<Material>();

    if(customShader == nullptr){
        out->shader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/Model.glsl"));
    } else {
        out->shader(customShader);
    }

    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    std::vector<Ref<Texture2D>> diffuseMaps = loadMaterialTextures(loadData, material, aiTextureType_DIFFUSE, "texture_diffuse");
    if(diffuseMaps.size() > 0){
        //MaterialMap map;
        //map.texture = diffuseMaps[0];
        //map.type = MaterialMap::Type::Texture;
        //out.maps["texture1"] = map;
        out->SetTexture("mainTex", diffuseMaps[0]);
        out->SetVector4("color", Vector4(1, 1, 1, 1));
    }

    /*
    std::vector<Texture2D> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    std::vector<Texture2D> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    */

   if(meshIndex != -1) loadData.materialDb[meshIndex] = out;

    return out;
}

int getMeshIndex(aiMesh *mesh, const aiScene *scene){
    int meshIndex = -1;
    for(int i = 0; i < scene->mNumMeshes; i++){
        if(mesh == scene->mMeshes[i]){
            meshIndex = i;
            break;
        }
    }
    return meshIndex;
}

Ref<Mesh> processMesh(LoadData& loadData, aiMesh *mesh, const aiScene *scene){
    int meshIndex = getMeshIndex(mesh, scene);
    if(meshIndex != -1 && loadData.meshDb.count(meshIndex)) return loadData.meshDb[meshIndex];

    Ref<Mesh> out = CreateRef<Mesh>();

    //LogInfo("Mesh Index: %d", meshIndex);

    // walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++){
        aiVector3D& pos = mesh->mVertices[i];
        out->vertices.push_back(Vector3(pos.x, pos.y, pos.z));
        
        // normals
        if (mesh->HasNormals()){
            aiVector3D& normal = mesh->mNormals[i];
            out->normals.push_back(Vector3(normal.x, normal.y, normal.z));
        }
        if(mesh->HasVertexColors(0)){
            aiColor4D& color = mesh->mColors[0][i];
            out->colors.push_back(Vector4(color.r, color.g, color.b, color.a));
        }

        if(mesh->HasTangentsAndBitangents()){
            aiVector3D& tangent = mesh->mTangents[i];
            out->tangents.push_back(Vector3(tangent.x, tangent.y, tangent.z));
            // bitangent
            /*
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
            */
        }

        // texture coordinates
        if(mesh->mTextureCoords[0]){ // does the mesh contain texture coordinates?
            aiVector3D& uv = mesh->mTextureCoords[0][i];
            out->uv.push_back(Vector3(uv.x, uv.y, 0)); 
        } else {
            out->uv.push_back({0, 0, 0});
        }
    }

    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for(unsigned int i = 0; i < mesh->mNumFaces; i++){
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            out->indices.push_back(face.mIndices[j]);        
    }

    if(mesh->HasBones()){
        ExtractBoneWeightForVertices(loadData, out, mesh, scene);
    }

    out->UpdateMesh();

    if(meshIndex != -1) loadData.meshDb[meshIndex] = out;

    return out;
}

void processNode(LoadData& loadData, aiNode *node, const aiScene *scene, Ref<Shader> customShader){
    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; i++){
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        loadData.model->meshs.push_back(processMesh(loadData, mesh, scene));
        loadData.model->materials.push_back(processMaterial(loadData, mesh, scene, customShader));
        
        Matrix4 m = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node[i].mTransformation);
        if(node[i].mParent != nullptr) m = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node[i].mParent->mTransformation) * m;

        loadData.model->matrixs.push_back(m);
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++){
        processNode(loadData, node->mChildren[i], scene, customShader);
    }
}

Ref<Model> AssimpLoadModel(std::string const &path, Ref<Shader> customShader){
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path, 
        aiProcess_Triangulate | 
        aiProcess_GenSmoothNormals | /*| aiProcess_FlipUVs*/ 
        aiProcess_CalcTangentSpace |
        aiProcess_PopulateArmatureData
    );

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        LogError("ERROR::ASSIMP:: %s", importer.GetErrorString());
        return nullptr;
    }
    
    LoadData loadData;
    loadData.model = CreateRef<Model>();
    loadData.model->path(path);
    loadData.directory = path.substr(0, path.find_last_of('/'));
   
    processNode(loadData, scene->mRootNode, scene, customShader);

    if(loadData.boneCounter > 0){
        UpdateSkeleton(loadData, scene);
    }

    return loadData.model;
}

}