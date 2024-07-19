#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/MultiCompileShader.h"
#include "OD/Graphics/Texture.h"
#include "OD/Graphics/Cubemap.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

struct OD_API MaterialMap{
    enum class OD_API_IMPORT Type{
        Int, Float, Vector2, Vector3, Vector4, Matrix4, Texture, TextureArray, Cubemap, Framebuffer, FloatList, Vector4List, Matrix4List
    };

    Type type;

    //union{
    Ref<Texture2D> texture;
    Ref<Texture2DArray> textureArray;
    Ref<Cubemap> cubemap;
    Framebuffer* framebuffer;
    
    //struct{
        int framebufferBind;
        int framebufferAttachment;
    //};
    
    bool hidden/* = false*/;
    
    //struct{
    Vector4 vector;
    bool vectorIsColor/* = false*/;
    //};

    int valueInt;

    //struct{
    float value;
    float valueMin;
    float valueMax;
    //};

    //struct{
    void* list;
    int listCount;
    //};
    //};

    Matrix4 matrix;

    template<class Archive> void save(Archive& ar) const;
    template<class Archive> void load(Archive& ar);

private:
    void OnLoad(std::string& texPath);
};

class OD_API Material: public Asset{
public:
    Material();
    Material(Ref<Shader> s);

    Ref<Shader> GetShader();
    void SetShader(Ref<Shader> s);

    uint32_t MaterialId();

    bool IsBlend();
    bool EnableInstancingValid();
    bool EnableInstancing();
    bool SupportInstancing();

    void SetInt(const char* name, int value);
    void SetFloat(const char* name, float value, float min = 0.0f, float max = 0.0f);
    void SetFloat(const char* name, float* value, int count);
    void SetVector2(const char* name, Vector2 value);
    void SetVector3(const char* name, Vector3 value);
    void SetVector4(const char* name, Vector4 value);
    void SetVector4(const char* name, Vector4* value, int count);
    void SetMatrix4(const char* name, Matrix4 value);
    void SetMatrix4(const char* name, Matrix4* value, int count);
    void SetTexture(const char* name, Ref<Texture2D> tex);
    void SetTexture(const char* name, Ref<Texture2DArray> tex);
    void SetTexture(const char* name, Framebuffer* tex, int bind, int attachment);
    void SetCubemap(const char* name, Ref<Cubemap> tex);

    static void SetGlobalInt(const char* name, int value);
    static void SetGlobalFloat(const char* name, float value, float min = 0.0f, float max = 0.0f);
    static void SetGlobalFloat(const char* name, float* value, int count);
    static void SetGlobalVector2(const char* name, Vector2 value);
    static void SetGlobalVector3(const char* name, Vector3 value);
    static void SetGlobalVector4(const char* name, Vector4 value);
    static void SetGlobalVector4(const char* name, Vector4* value, int count);
    static void SetGlobalMatrix4(const char* name, Matrix4 value);
    static void SetGlobalMatrix4(const char* name, Matrix4* value, int count);
    static void SetGlobalTexture(const char* name, Ref<Texture2D> tex);
    static void SetGlobalTexture(const char* name, Framebuffer* tex, int bind, int attachment);
    static void SetGlobalCubemap(const char* name, Ref<Cubemap> tex);

    void DisableKeyword(std::string keyword);
    void EnableKeyword(std::string keyword);

    void CleanData();
    //void UpdateDatas();

    static void SubmitGraphicDatas(Material& material);
    static void CleanGlobalUniformsData();

    void Save(std::string& path);

    void OnGui() override;

    bool LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;
    
    //static Ref<Material> CreateFromFile(std::string const &path);

    friend class cereal::access;
    template<class Archive> void save(Archive& ar) const;
    template<class Archive> void load(Archive& ar);

    inline void SetEnableInstancing(bool enable){
        enableInstancing = enable;
    }

private:
    bool enableInstancing = false;
    int currentTextureSlot = 0;

    //Ref<Shader> shader;
    Ref<MultiCompileShader> shaderHandler;
    std::vector<std::string> properties;
    std::unordered_map<std::string, MaterialMap> maps;
    static std::unordered_map<std::string, MaterialMap> globalMaps;

    uint32_t id;
    static uint32_t baseId;

    void SetColor3(const char* name, Vector3 value);
    void SetColor4(const char* name, Vector4 value);

    void UpdateMaps();
    static void ApplyUniformTo(Material& material, Shader& shader, std::unordered_map<std::string, MaterialMap>& maps);
};

////////////////////////////////////////

template<class Archive>
void MaterialMap::save(Archive& ar) const{
    /*std::string texPath = texture == nullptr ? "" : texture->Path();
    ar(
        CEREAL_NVP(type),
        CEREAL_NVP(texPath),
        CEREAL_NVP(vector),
        CEREAL_NVP(vectorIsColor),
        CEREAL_NVP(value),
        CEREAL_NVP(valueMin),
        CEREAL_NVP(valueMax)
    );*/

    ArchiveDump(ar, CEREAL_NVP(type));
    if(type == MaterialMap::Type::Int){
        ArchiveDump(ar, CEREAL_NVP(valueInt));
    }
    if(type == MaterialMap::Type::Float){
        ArchiveDump(ar, CEREAL_NVP(value));
        ArchiveDump(ar, CEREAL_NVP(valueMin));
        ArchiveDump(ar, CEREAL_NVP(valueMax));
    }
    if(type == MaterialMap::Type::Vector2){
        ArchiveDump(ar, CEREAL_NVP(vector));
        ArchiveDump(ar, CEREAL_NVP(vectorIsColor));
    }
    if(type == MaterialMap::Type::Vector3){
        ArchiveDump(ar, CEREAL_NVP(vector));
        ArchiveDump(ar, CEREAL_NVP(vectorIsColor));
    }
    if(type == MaterialMap::Type::Vector4){
        ArchiveDump(ar, CEREAL_NVP(vector));
        ArchiveDump(ar, CEREAL_NVP(vectorIsColor));
    }
    if(type == MaterialMap::Type::Texture){
        std::string texPath = texture == nullptr ? "" : texture->Path();
        ArchiveDump(ar, CEREAL_NVP(texPath));
    }
}

template<class Archive>
void MaterialMap::load(Archive& ar){
    /*std::string texPath;
    ar(
        CEREAL_NVP(type),
        CEREAL_NVP(texPath),
        CEREAL_NVP(vector),
        CEREAL_NVP(vectorIsColor),
        CEREAL_NVP(value),
        CEREAL_NVP(valueMin),
        CEREAL_NVP(valueMax)
    );
    OnLoad(texPath);*/

    ArchiveDump(ar, CEREAL_NVP(type));
    if(type == MaterialMap::Type::Int){
        ArchiveDump(ar, CEREAL_NVP(valueInt));
    }
    if(type == MaterialMap::Type::Float){
        ArchiveDump(ar, CEREAL_NVP(value));
        ArchiveDump(ar, CEREAL_NVP(valueMin));
        ArchiveDump(ar, CEREAL_NVP(valueMax));
    }
    if(type == MaterialMap::Type::Vector2){
        ArchiveDump(ar, CEREAL_NVP(vector));
        ArchiveDump(ar, CEREAL_NVP(vectorIsColor));
    }
    if(type == MaterialMap::Type::Vector3){
        ArchiveDump(ar, CEREAL_NVP(vector));
        ArchiveDump(ar, CEREAL_NVP(vectorIsColor));
    }
    if(type == MaterialMap::Type::Vector4){
        ArchiveDump(ar, CEREAL_NVP(vector));
        ArchiveDump(ar, CEREAL_NVP(vectorIsColor));
    }
    if(type == MaterialMap::Type::Texture){
        std::string texPath = texture == nullptr ? "" : texture->Path();
        ArchiveDump(ar, CEREAL_NVP(texPath));
        OnLoad(texPath);
    }
}

template<class Archive>
void Material::save(Archive& ar) const{
    std::string shaderPath = shaderHandler->GetCurrentShader() == nullptr ? "" : shaderHandler->GetCurrentShader()->Path();
    ar(
        CEREAL_NVP(enableInstancing),
        CEREAL_NVP(shaderPath),
        CEREAL_NVP(maps)
    );
}

template<class Archive>
void Material::load(Archive& ar){
    std::string shaderPath;
    ar(
        CEREAL_NVP(enableInstancing),
        CEREAL_NVP(shaderPath),
        CEREAL_NVP(maps)
    );

    if(shaderPath.empty() == false){
        SetShader(AssetManager::Get().LoadAsset<Shader>(shaderPath));
    }
}

}