#include "AssimpLoader.h"
#include "OD/Core/Asset.h"

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
    Model* model;
    const aiScene* scene;
    std::string directory;

    //std::map<int, Ref<Mesh>> meshDb;
    //std::map<int, Ref<Material>> materialDb;

    Pose bindPose;
    std::vector<std::string> names;

    std::vector<aiBone*> bones;
    std::vector<aiNode*> nodes;
    std::vector<aiMesh*> meshs;
    std::vector<aiMaterial*> materials;

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

int GetNodeIndex(LoadData& loadData, std::string name){
    for(int i = 0; i < loadData.names.size(); i++){
        if(name == loadData.names[i]) return i;
    }
    return -1;
}

int GetNodeIndex(LoadData& loadData, aiNode* node){
    for(int i = 0; i < loadData.nodes.size(); i++){
        if(node == loadData.nodes[i]) return i;
    }
    return -1;
}

void ReadSkeleton(LoadData& loadData, aiNode* node, int parent = -1){
    Transform t(AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation));

    LogInfo("Skeleton Bones -> Name: %s, Id: %d: Parent: %d", node->mName.C_Str(), (int)loadData.bindPose.Size(), parent);

    int boneId = loadData.names.size();

    if(loadData.bindPose.Size() < boneId+1){
        loadData.bindPose.Resize(boneId+1);
    }

    loadData.bindPose.SetLocalTransform(boneId, t);
    loadData.bindPose.SetParent(boneId, parent);
    loadData.names.push_back(node->mName.C_Str()); 
    loadData.nodes.push_back(node);

    for(int i = 0; i < node->mNumChildren; i++){
        ReadSkeleton(loadData, node->mChildren[i], boneId);
    }
}

void LoadSkeleton(LoadData& data, aiNode* root){
    ReadSkeleton(data, root);
    data.model->skeleton.Set(data.bindPose, data.bindPose, data.names);
}

void LoadInvBindPose(LoadData& data){
    if(data.model->skeleton.GetInvBindPose().size() != data.bones.size()) return;

    //Assert(data.model->skeleton.GetInvBindPose().size() == data.bones.size());

    for(int i = 0; i < data.bones.size(); i++){
        Matrix4 m = Matrix4Identity;
        if(data.bones[i] != nullptr){
            m = AssimpGLMHelpers::ConvertMatrixToGLMFormat(data.bones[i]->mOffsetMatrix);
        }

        data.model->skeleton.GetInvBindPose()[i] = m;
    }
}

int AddToBindPose(LoadData& loadData, aiBone* bone){
    int boneID = loadData.boneCounter;
    loadData.boneCounter += 1;
 
    if(loadData.names.size() < loadData.boneCounter){
        loadData.names.resize(loadData.boneCounter);
        loadData.bindPose.Resize(loadData.boneCounter);
        loadData.bones.resize(loadData.boneCounter);
    }

    auto m = AssimpGLMHelpers::ConvertMatrixToGLMFormat(bone->mNode->mTransformation);

    loadData.names[boneID] = std::string(bone->mName.C_Str());
    loadData.bones[boneID] = bone;
    loadData.bindPose.SetLocalTransform(boneID, Transform(m));

    return boneID;
}

void ExtractBoneWeightForVertices(LoadData& skeletonData, Ref<Mesh> _mesh, aiMesh* mesh){
    _mesh->weights.resize(_mesh->vertices.size());
    std::fill(_mesh->weights.begin(), _mesh->weights.end(), Vector4(0,0,0,0));
    _mesh->influences.resize(_mesh->vertices.size());
    std::fill(_mesh->influences.begin(), _mesh->influences.end(), Vector4(-1,-1,-1,-1));

    for(int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex){
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        boneID = GetNodeIndex(skeletonData, boneName);

        //if(skeletonData.bones.size() < boneID+1){
        //    skeletonData.bones.resize(boneID+1);
        //}
        if(skeletonData.bones.size() < skeletonData.bindPose.Size()){
            skeletonData.bones.resize(skeletonData.bindPose.Size());
        }
        skeletonData.bones[boneID] = mesh->mBones[boneIndex];

        Assert(boneID != -1);
        if(boneID == -1){
            boneID = AddToBindPose(skeletonData, mesh->mBones[boneIndex]);
        }

        LogInfo("Bone -> Name: %s, Id: %d", boneName.c_str(), boneID);

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

        const aiTexture* paiTexture = loadData.scene->GetEmbeddedTexture(str.C_Str());

        Assert(mat->Get(AI_MATKEY_TEXTURE(type, i), str) == AI_SUCCESS);

        if(paiTexture){
            Ref<Texture2D> texture = CreateRef<Texture2D>(
                (void*)paiTexture->pcData, 
                (size_t)paiTexture->mWidth, 
                Texture2DSetting{TextureFilter::Linear, TextureWrapping::Repeat, true}
            );
            Assert(texture->IsValid() != false);
            
            textures.push_back(texture);
        } else {
            std::string filename = loadData.directory + '/' + std::string(str.C_Str());
            //LogWarningExtra("AssimpTexture: %s", filename.c_str());
            Ref<Texture2D> texture = AssetManager::Get().LoadAsset<Texture2D>(filename.c_str());
            textures.push_back(texture);
        }
    }

    /*if(textures.empty()){
        textures.push_back(AssetManager::Get().LoadAsset<Texture2D>(
            "res/Engine/Textures/White.jpg"
        ));
    }*/

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

/*Ref<Material> processMaterial(LoadData& loadData, aiMesh *mesh, const aiScene *scene, Ref<Shader> customShader){
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex]; 

    //int meshIndex = getMaterialIndex(material, scene);
    //if(meshIndex != -1 && loadData.materialDb.count(meshIndex)) return loadData.materialDb[meshIndex];

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

    
    //std::vector<Texture2D> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    //textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    /qstd::vector<Texture2D> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    //textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    

   //if(meshIndex != -1) loadData.materialDb[meshIndex] = out;

    return out;
}*/

Ref<Material> LoadMaterial(LoadData& data, aiMaterial* material, Ref<Shader> customShader){
    Ref<Material> out = CreateRef<Material>();

    if(customShader == nullptr){
        out->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Model.glsl"));
    } else {
        out->SetShader(customShader);
    }

    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    std::vector<Ref<Texture2D>> diffuseMaps = loadMaterialTextures(data, material, aiTextureType_DIFFUSE, "texture_diffuse");
    if(diffuseMaps.size() > 0){
        out->SetTexture("mainTex", diffuseMaps[0]);
        out->SetVector4("color", Vector4(1, 1, 1, 1));
    }

    std::vector<Ref<Texture2D>> normalMaps = loadMaterialTextures(data, material, aiTextureType_NORMALS, "texture_normal");
    if(normalMaps.size() > 0){
        out->SetTexture("normal", normalMaps[0]);
    }
    std::vector<Ref<Texture2D>> normalMaps2 = loadMaterialTextures(data, material, aiTextureType_NORMAL_CAMERA, "texture_normal");
    if(normalMaps2.size() > 0){
        out->SetTexture("normal", normalMaps2[0]);
    }
    std::vector<Ref<Texture2D>> normalMaps3 = loadMaterialTextures(data, material, aiTextureType_HEIGHT, "texture_normal");
    if(normalMaps3.size() > 0){
        out->SetTexture("normal", normalMaps3[0]);
    }

    /*
    std::vector<Texture2D> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    std::vector<Texture2D> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    */

   //if(meshIndex != -1) loadData.materialDb[meshIndex] = out;

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

/*Ref<Mesh> processMesh(LoadData& loadData, aiMesh *mesh, const aiScene *scene){
    //int meshIndex = getMeshIndex(mesh, scene);
    //if(meshIndex != -1 && loadData.meshDb.count(meshIndex)) return loadData.meshDb[meshIndex];

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
            
            //vector.x = mesh->mBitangents[i].x;
            //vector.y = mesh->mBitangents[i].y;
            //vector.z = mesh->mBitangents[i].z;
            //vertex.Bitangent = vector;
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
        ExtractBoneWeightForVertices(loadData, out, mesh);
    }

    out->UpdateMesh();

    //if(meshIndex != -1) loadData.meshDb[meshIndex] = out;

    return out;
}*/

/*void processNode(LoadData& loadData, aiNode *node, const aiScene *scene, Ref<Shader> customShader){

    for(unsigned int i = 0; i < node->mNumMeshes; i++){
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        loadData.model->meshs.push_back(processMesh(loadData, mesh, scene));
        loadData.model->materials.push_back(processMaterial(loadData, mesh, scene, customShader));
        
        Matrix4 m = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node[i].mTransformation);
        loadData.model->matrixs.push_back(m);
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++){
        processNode(loadData, node->mChildren[i], scene, customShader);
    }
}*/

void LoadAnimation(LoadData& loadData, aiAnimation* animation, Clip& outClip){
    outClip.SetName(animation->mName.C_Str());
    //outClip.SetDuration(animation->mDuration);
    //outClip.SetTicksPerSecond(animation->mTicksPerSecond);
    
    
    for(int i = 0; i < animation->mNumChannels; i++){
        Interpolation interpolation = Interpolation::Linear;
        bool isCubic = interpolation == Interpolation::Cubic;

        aiNodeAnim* channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        int nodeId = GetNodeIndex(loadData, boneName);
        LogInfo("Channel -> BoneName: %s BoneId: %d", channel->mNodeName.C_Str(), nodeId);
        Assert(nodeId != -1);

        VectorTrack& posTrack = outClip[nodeId].GetPositionTrack();
        posTrack.Resize(channel->mNumPositionKeys);
        posTrack.SetInterpolation(interpolation);
        for(int j = 0; j < channel->mNumPositionKeys; j++){
            Vector3 pos = AssimpGLMHelpers::GetGLMVec(channel->mPositionKeys[j].mValue);
            //pos = loadData.bindPose.GetLocalTransform(nodeId).localPosition();

            posTrack[j].time = channel->mPositionKeys[j].mTime / animation->mTicksPerSecond;
            //posTrack[j].value = pos;
            posTrack[j].value[0] = pos.x;
            posTrack[j].value[1] = pos.y;
            posTrack[j].value[2] = pos.z;

            posTrack[j].in[0] = isCubic ? pos.x : 0.0f;
            posTrack[j].in[1] = isCubic ? pos.y : 0.0f;
            posTrack[j].in[2] = isCubic ? pos.z : 0.0f;

            posTrack[j].out[0] = isCubic ? pos.x : 0.0f;
            posTrack[j].out[1] = isCubic ? pos.y : 0.0f;
            posTrack[j].out[2] = isCubic ? pos.z : 0.0f;
        }

        VectorTrack& scaleTrack = outClip[nodeId].GetScaleTrack();
        scaleTrack.SetInterpolation(interpolation);
        scaleTrack.Resize(channel->mNumScalingKeys);
        for(int j = 0; j < channel->mNumScalingKeys; j++){
            Vector3 scale = AssimpGLMHelpers::GetGLMVec(channel->mScalingKeys[j].mValue);
            //scale = loadData.bindPose.GetLocalTransform(nodeId).localScale();

            scaleTrack[j].time = channel->mScalingKeys[j].mTime / animation->mTicksPerSecond;
            scaleTrack[j].value[0] = scale.x;
            scaleTrack[j].value[1] = scale.y;
            scaleTrack[j].value[2] = scale.z;

            scaleTrack[j].in[0] = isCubic ? scale.x : 0.0f;
            scaleTrack[j].in[1] = isCubic ? scale.y : 0.0f;
            scaleTrack[j].in[2] = isCubic ? scale.z : 0.0f;

            scaleTrack[j].out[0] = isCubic ? scale.x : 0.0f;
            scaleTrack[j].out[1] = isCubic ? scale.y : 0.0f;
            scaleTrack[j].out[2] = isCubic ? scale.z : 0.0f;
        }

        QuaternionTrack& rotTrack = outClip[nodeId].GetRotationTrack();
        rotTrack.SetInterpolation(interpolation);
        rotTrack.Resize(channel->mNumRotationKeys);
        for(int j = 0; j < channel->mNumRotationKeys; j++){
            Quaternion rot = AssimpGLMHelpers::GetGLMQuat(channel->mRotationKeys[j].mValue);
            //rot = loadData.bindPose.GetLocalTransform(nodeId).localRotation();

            rotTrack[j].time = channel->mRotationKeys[j].mTime / animation->mTicksPerSecond;
            rotTrack[j].value[0] = rot.x;
            rotTrack[j].value[1] = rot.y;
            rotTrack[j].value[2] = rot.z;
            rotTrack[j].value[3] = rot.w;

            rotTrack[j].in[0] = isCubic ? rot.x : 0.0f;
            rotTrack[j].in[1] = isCubic ? rot.y : 0.0f;
            rotTrack[j].in[2] = isCubic ? rot.z : 0.0f;
            rotTrack[j].in[3] = isCubic ? rot.w : 0.0f;

            rotTrack[j].out[0] = isCubic ? rot.x : 0.0f;
            rotTrack[j].out[1] = isCubic ? rot.y : 0.0f;
            rotTrack[j].out[2] = isCubic ? rot.z : 0.0f;
            rotTrack[j].out[3] = isCubic ? rot.w : 0.0f;
        }
        
    }

    outClip.RecalculateDuration();
}

Ref<Mesh> LoadMesh(LoadData& data, aiMesh* mesh){
    Ref<Mesh> out = CreateRef<Mesh>();

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
        ExtractBoneWeightForVertices(data, out, mesh);
    }

    out->Submit();

    return out;
}

void LoadRenderTargets(LoadData& data, const aiScene* scene, aiNode* node){
    for(unsigned int i = 0; i < node->mNumMeshes; i++){
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).

        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        Model::RenderTarget renderTarget;
        renderTarget.meshIndex = node->mMeshes[i];
        renderTarget.materialIndex = mesh->mMaterialIndex;
        renderTarget.bindPoseIndex = GetNodeIndex(data, node);

        data.model->renderTargets.push_back(renderTarget);
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++){
        LoadRenderTargets(data, scene, node->mChildren[i]);
    }
}

bool AssimpLoadModel(Model& out, std::string const &path, Ref<Shader> customShader, std::vector<Clip>* outClips){
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path, 
        aiProcess_Triangulate | 
        aiProcess_GenSmoothNormals | 
        //aiProcess_FlipUVs | 
        aiProcess_CalcTangentSpace |
        aiProcess_PopulateArmatureData
        //| aiProcess_GlobalScale 
        //| aiProcess_OptimizeGraph 
    );

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        LogError("ERROR::ASSIMP:: %s", importer.GetErrorString());
        return false;
    }
    
    LoadData loadData;
    loadData.model = &out;
    loadData.model->SetPath(path);
    loadData.directory = path.substr(0, path.find_last_of('/'));
    loadData.scene = scene;

    LoadSkeleton(loadData, scene->mRootNode);

    for(int i = 0; i < scene->mNumMeshes; i++){
        Ref<Mesh> mesh = LoadMesh(loadData, scene->mMeshes[i]);
        loadData.model->meshs.push_back(mesh);
        loadData.meshs.push_back(scene->mMeshes[i]);
    }

    for(int i = 0; i < scene->mNumMaterials; i++){
        Ref<Material> m = LoadMaterial(loadData, scene->mMaterials[i], customShader);
        loadData.model->materials.push_back(m);
        loadData.materials.push_back(scene->mMaterials[i]);
    }

    for(int i = 0; i < scene->mNumAnimations; i++){
        Ref<Clip> out = CreateRef<Clip>();
        LoadAnimation(loadData, scene->mAnimations[i], *out);
        loadData.model->animationClips.push_back(out);
    }

    LoadRenderTargets(loadData, scene, scene->mRootNode);
    LoadInvBindPose(loadData);

    for(auto i: loadData.model->renderTargets){
        Assert(i.meshIndex != -1);
        Assert(i.meshIndex < loadData.model->meshs.size());

        Assert(i.materialIndex != -1);
        Assert(i.materialIndex < loadData.model->materials.size());

        Assert(i.bindPoseIndex != -1);
        Assert(i.bindPoseIndex < loadData.model->skeleton.GetBindPose().Size());
    }

    return true;
}

}