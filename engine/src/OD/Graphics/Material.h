#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Texture.h"
#include "OD/Graphics/Cubemap.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

struct MaterialMap{
    enum class Type{
        Float, Vector2, Vector3, Vector4, Matrix4, Texture, Cubemap, Framebuffer
    };

    Type type;

    Ref<Texture2D> texture;
    Ref<Cubemap> cubemap;
    Framebuffer* framebuffer;
    Vector4 vector;
    
    bool hidden = false;
    bool vectorIsColor = false;

    float value;
    float valueMin;
    float valueMax;

    Matrix4 matrix;

    template<class Archive> void save(Archive& ar) const;
    template<class Archive> void load(Archive& ar);
};

class Material: public Asset{
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

    void SetFloat(const char* name, float value, float min = 0.0f, float max = 0.0f);

    void SetVector2(const char* name, Vector2 value);
    void SetVector3(const char* name, Vector3 value, bool isColor = false);
    void SetVector4(const char* name, Vector4 value, bool isColor = false);
    void SetMatrix4(const char* name, Matrix4 value);
    void SetTexture(const char* name, Ref<Texture2D> tex);
    void SetTexture(const char* name, Framebuffer* tex);
    void SetCubemap(const char* name, Ref<Cubemap> tex);

    void CleanData();
    void UpdateDatas();
    void ApplyUniformTo(Shader& shader);

    void Save(std::string& path);

    void OnGui() override;

    static Ref<Material> CreateFromFile(std::string const &path);

    friend class cereal::access;
    template<class Archive> void save(Archive& ar) const;
    template<class Archive> void load(Archive& ar);

private:
    bool enableInstancing = false;

    Ref<Shader> shader;
    std::unordered_map<std::string, MaterialMap> maps;

    uint32_t id;
    static uint32_t baseId;

    void UpdateMaps();
};

////////////////////////////////////////

template<class Archive>
void MaterialMap::save(Archive& ar) const{
    std::string texPath = texture == nullptr ? "" : texture->Path();
    ar(
        CEREAL_NVP(type),
        CEREAL_NVP(texPath),
        CEREAL_NVP(vector),
        CEREAL_NVP(vectorIsColor),
        CEREAL_NVP(value),
        CEREAL_NVP(valueMin),
        CEREAL_NVP(valueMax)
    );
}

template<class Archive>
void MaterialMap::load(Archive& ar){
    std::string texPath;
    ar(
        CEREAL_NVP(type),
        CEREAL_NVP(texPath),
        CEREAL_NVP(vector),
        CEREAL_NVP(vectorIsColor),
        CEREAL_NVP(value),
        CEREAL_NVP(valueMin),
        CEREAL_NVP(valueMax)
    );

    if(texPath.empty() == false){
        texture = AssetManager::Get().LoadTexture2D(texPath);
    }
}

template<class Archive>
void Material::save(Archive& ar) const{
    std::string shaderPath = shader == nullptr ? "" : shader->Path();
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
        shader = AssetManager::Get().LoadShaderFromFile(shaderPath);
    }
}

}