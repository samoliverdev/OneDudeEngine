#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Graphics/SubShader.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Texture.h"
#include "OD/Graphics/Cubemap.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Platform/OpenGL/GL.h"

namespace sol{ class state; }

namespace OD{

struct OD_API MaterialMap{
    enum class OD_API_IMPORT Type{
        None = 0, Int, Float, Vector2, Vector3, Vector4, Matrix4, Texture, TextureArray, Cubemap, Framebuffer, FloatList, Vector4List, Matrix4List
    };

    Type type;

    //Dont call destructor if has inside the union
    Ref<Texture2D> texture;
    Ref<Texture2DArray> textureArray;
    Ref<Cubemap> cubemap;

    struct Vec{
        Vector4 vector;
        bool vectorIsColor;
    };

    union{
        /*struct{
            Vector4 vector;
            bool vectorIsColor;
        };*/
        Vec vec;

        Matrix4 matrix;

        /*Ref<Texture2D> texture;
        Ref<Texture2DArray> textureArray;
        Ref<Cubemap> cubemap;*/
        
        struct{
            Framebuffer* framebuffer;
            int framebufferAttachment;
        };
        
        int valueInt;

        struct{
            float valueFloat;
            float valueFloatMin;
            float valueFloatMax;
        };

        struct{
            void* list;
            int listCount;
        };
    };

    MaterialMap(){ memset(this, 0, sizeof(MaterialMap)); }
    MaterialMap(const MaterialMap& other){ memcpy(this, &other, sizeof(MaterialMap)); }
    MaterialMap& operator=(const MaterialMap& other){ memcpy(this, &other, sizeof(MaterialMap)); return *this; }
    ~MaterialMap(){
        /*if(type == MaterialMap::Type::Vector2) vector.~Vector4();
        if(type == MaterialMap::Type::Vector3) vector.~Vector4();
        if(type == MaterialMap::Type::Vector4) vector.~Vector4();
        if(type == MaterialMap::Type::Matrix4) matrix.~Matrix4();
        if(type == MaterialMap::Type::Texture) texture.~Ref<Texture2D>();
        if(type == MaterialMap::Type::TextureArray) textureArray.~Ref<Texture2DArray>();
        if(type == MaterialMap::Type::Framebuffer) framebuffer->~Framebuffer();
        if(type == MaterialMap::Type::Cubemap) cubemap.~Ref<Cubemap>();*/
    };

    template<class Archive> void save(Archive& ar) const;
    template<class Archive> void load(Archive& ar);

private:
    void OnLoad(std::string& texPath);
};

class OD_API Material: public Asset{
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
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
    void SetFloat(const char* name, float value);
    void SetFloat(const char* name, float* value, int count);
    void SetVector2(const char* name, Vector2 value);
    void SetVector3(const char* name, Vector3 value);
    void SetVector4(const char* name, Vector4 value);
    void SetVector4(const char* name, Vector4* value, int count);
    void SetMatrix4(const char* name, Matrix4 value);
    void SetMatrix4(const char* name, Matrix4* value, int count);
    void SetTexture(const char* name, Ref<Texture2D> tex);
    void SetTexture(const char* name, Ref<Texture2DArray> tex);
    void SetTexture(const char* name, Framebuffer* tex, int attachment);
    void SetCubemap(const char* name, Ref<Cubemap> tex);

    static void SetGlobalInt(const char* name, int value);
    static void SetGlobalFloat(const char* name, float value);
    static void SetGlobalFloat(const char* name, float* value, int count);
    static void SetGlobalVector2(const char* name, Vector2 value);
    static void SetGlobalVector3(const char* name, Vector3 value);
    static void SetGlobalVector4(const char* name, Vector4 value);
    static void SetGlobalVector4(const char* name, Vector4* value, int count);
    static void SetGlobalMatrix4(const char* name, Matrix4 value);
    static void SetGlobalMatrix4(const char* name, Matrix4* value, int count);
    static void SetGlobalTexture(const char* name, Ref<Texture2D> tex);
    static void SetGlobalTexture(const char* name, Framebuffer* tex, int attachment);
    static void SetGlobalCubemap(const char* name, Ref<Cubemap> tex);

    void DisableKeyword(std::string keyword);
    void EnableKeyword(std::string keyword);

    void CleanData();
    //void UpdateDatas();

    //static void SubmitGraphicDatas(Material& material);
    static void CleanGlobalUniformsData();

    void Save(std::string& path);

    void OnGui() override;

    bool LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;
    
    //static Ref<Material> CreateFromFile(std::string const &path);

    static void CreateLuaBind(sol::state& lua);

    friend class cereal::access;
    template<class Archive> void save(Archive& ar) const;
    template<class Archive> void load(Archive& ar);

    inline void SetEnableInstancing(bool enable){
        enableInstancing = enable;
    }

private:
    bool enableInstancing = false;
    int currentTextureSlot = 0;

    std::set<std::string> enabledKeywords;

    Ref<Shader> shader;
    std::vector<std::string> properties;
    std::unordered_map<std::string, MaterialMap> maps;
    static std::unordered_map<std::string, MaterialMap> globalMaps;

    int currentPass = 0;
    Ref<SubShader> currentShader;

    uint32_t id;
    static uint32_t baseId;

    bool isDirty = true;
    
    bool isComplete = false;
    MaterialDataGL;

    void SetFloat(const char* name, float value, float min, float max);
    void SetColor3(const char* name, Vector3 value);
    void SetColor4(const char* name, Vector4 value);

    void UpdateMaps();
    //static void ApplyUniformTo(Material& material, SubShader& shader, std::unordered_map<std::string, MaterialMap>& maps);

    std::string GetKey(const std::set<std::string>& keyworlds);
    std::set<std::string> GetEnabledKeywords();
    void UpdateCurrentShader();
};

////////////////////////////////////////

template<class Archive>
void MaterialMap::save(Archive& ar) const{
    ArchiveDump(ar, CEREAL_NVP(type));
    if(type == MaterialMap::Type::Int){
        ArchiveDump(ar, CEREAL_NVP(valueInt));
    }
    if(type == MaterialMap::Type::Float){
        ArchiveDump(ar, CEREAL_NVP(valueFloat));
        ArchiveDump(ar, CEREAL_NVP(valueFloatMin));
        ArchiveDump(ar, CEREAL_NVP(valueFloatMax));
    }
    if(type == MaterialMap::Type::Vector2){
        ArchiveDump(ar, CEREAL_NVP(vec.vector));
        ArchiveDump(ar, CEREAL_NVP(vec.vectorIsColor));
    }
    if(type == MaterialMap::Type::Vector3){
        ArchiveDump(ar, CEREAL_NVP(vec.vector));
        ArchiveDump(ar, CEREAL_NVP(vec.vectorIsColor));
    }
    if(type == MaterialMap::Type::Vector4){
        ArchiveDump(ar, CEREAL_NVP(vec.vector));
        ArchiveDump(ar, CEREAL_NVP(vec.vectorIsColor));
    }
    if(type == MaterialMap::Type::Texture){
        std::string texPath = texture == nullptr ? "" : texture->Path();
        ArchiveDump(ar, CEREAL_NVP(texPath));
    }
}

template<class Archive>
void MaterialMap::load(Archive& ar){
    ArchiveDump(ar, CEREAL_NVP(type));
    if(type == MaterialMap::Type::Int){
        ArchiveDump(ar, CEREAL_NVP(valueInt));
    }
    if(type == MaterialMap::Type::Float){
        ArchiveDump(ar, CEREAL_NVP(valueFloat));
        ArchiveDump(ar, CEREAL_NVP(valueFloatMin));
        ArchiveDump(ar, CEREAL_NVP(valueFloatMax));
    }
    if(type == MaterialMap::Type::Vector2){
        ArchiveDump(ar, CEREAL_NVP(vec.vector));
        ArchiveDump(ar, CEREAL_NVP(vec.vectorIsColor));
    }
    if(type == MaterialMap::Type::Vector3){
        ArchiveDump(ar, CEREAL_NVP(vec.vector));
        ArchiveDump(ar, CEREAL_NVP(vec.vectorIsColor));
    }
    if(type == MaterialMap::Type::Vector4){
        ArchiveDump(ar, CEREAL_NVP(vec.vector));
        ArchiveDump(ar, CEREAL_NVP(vec.vectorIsColor));
    }
    if(type == MaterialMap::Type::Texture){
        std::string texPath = texture == nullptr ? "" : texture->Path();
        ArchiveDump(ar, CEREAL_NVP(texPath));
        OnLoad(texPath);
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
        SetShader(AssetManager::Get().LoadAsset<Shader>(shaderPath));
    }
}

}