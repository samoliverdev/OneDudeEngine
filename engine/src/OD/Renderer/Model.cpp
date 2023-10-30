#include "Model.h"
//#include "Animations.h"
#include "OD/Core/AssetManager.h"
#include "AssimpGLMHelpers.h"

namespace OD{

Ref<Model> Model::CreateFromFile(std::string const &path, Ref<Shader> customShader){
    Ref<Model> out = CreateRef<Model>();
    out->_path = path;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals /*| aiProcess_FlipUVs*/ | aiProcess_CalcTangentSpace);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        LogError("ERROR::ASSIMP:: %s", importer.GetErrorString());
        return nullptr;
    }
    // retrieve the directory path of the filepath
    out->_directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    out->processNode(scene->mRootNode, scene, customShader);

    //for(int i = 0; i < scene->mNumAnimations; i++){
    //    out->animations.push_back(CreateRef<Animation>(scene, scene->mAnimations[i], out.get()));
    //}

    return out;
}

void Model::processNode(aiNode *node, const aiScene *scene, Ref<Shader> customShader){
    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; i++){
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshs.push_back(processMesh(mesh, scene));
        materials.push_back(processMaterial(mesh, scene, customShader));
        
        Matrix4 m = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node[i].mTransformation);
        if(node[i].mParent != nullptr) m = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node[i].mParent->mTransformation) * m;

        matrixs.push_back(m);
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++){
        processNode(node->mChildren[i], scene, customShader);
    }
}

Ref<Mesh> Model::processMesh(aiMesh *mesh, const aiScene *scene){
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

    out->UpdateMesh();
    return out;
}

Ref<Material> Model::processMaterial(aiMesh *mesh, const aiScene *scene, Ref<Shader> customShader){
    Ref<Material> out = CreateRef<Material>();

    if(customShader == nullptr){
        out->shader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/Model.glsl"));
    } else {
        out->shader(customShader);
    }

    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    std::vector<Ref<Texture2D>> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
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

    return out;
}

std::vector<Ref<Texture2D>> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName){
    std::vector<Ref<Texture2D>> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++){
        aiString str;
        aiTextureMapping map;
        unsigned int uvIndex;
        ai_real blend;
        mat->GetTexture(type, i, &str, &map, &uvIndex, &blend);

        //LogInfo("Blende %f", (float)blend);

        std::string filename = this->_directory + '/' +std::string(str.C_Str());
        
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

}