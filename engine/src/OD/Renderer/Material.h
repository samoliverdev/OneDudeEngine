#pragma once

#include "OD/Defines.h"
#include "OD/Renderer/Shader.h"
#include "OD/Renderer/Texture.h"
#include "OD/Renderer/Cubemap.h"

namespace OD{

struct MaterialMap{
    enum class Type{
        Texture, Float, Vector2, Vector3, Vector4 
    };

    Type type;

    Ref<Texture2D> texture;
    
    Vector4 vector;
    bool vectorIsColor = false;

    float value;
    bool isDirt = true;
};

class Material: public Asset{
public:
    Material(){}
    Material(Ref<Shader> s){
        _shader = s; 
        UpdateMaps();
    }

    inline Ref<Shader> shader(){ return _shader; }
    inline void shader(Ref<Shader> s){ _shader = s; UpdateMaps(); }
    
    inline bool isBlend(){
        if(_shader == nullptr) return false;
        return _shader->blendMode() == Shader::BlendMode::Blend;
    }

    inline bool enableInstancing(){ return _enableInstancing; }
    inline bool supportInstancing(){ return _shader != nullptr && _shader->supportInstancing(); }

    void SetFloat(const char* name, float value);
    void SetVector2(const char* name, Vector2 value);
    void SetVector3(const char* name, Vector3 value, bool isColor = false);
    void SetVector4(const char* name, Vector4 value, bool isColor = false);
    void SetTexture(const char* name, Ref<Texture2D> tex);

    void UpdateUniforms();

    void Save(std::string& path);

    void OnGui() override;

    static Ref<Material> CreateFromFile(std::string const &path);

private:
    bool _enableInstancing = false;
    bool _isDirt = true;

    Ref<Shader> _shader;
    std::unordered_map<std::string, MaterialMap> _maps;

    void UpdateMaps();
};

}