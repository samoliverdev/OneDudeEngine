#pragma once

#include "OD/Defines.h"
#include "OD/Renderer/Shader.h"
#include "OD/Renderer/Texture.h"

namespace OD{

struct MaterialMap{
    enum class Type{
        Texture, Float, Vector2, Vector3, Vector4 
    };

    Type type;

    Ref<Texture2D> texture;
    Vector4 vector;
    float value;
    bool isDirt = true;
};

class Material: public Asset{
public:
    Ref<Shader> shader;
    std::unordered_map<std::string, MaterialMap> maps;
    bool isBlend = false;

    void SetFloat(const char* name, float value);
    void SetVector2(const char* name, Vector2 value);
    void SetVector3(const char* name, Vector3 value);
    void SetVector4(const char* name, Vector4 value);
    void SetTexture(const char* name, Ref<Texture2D> tex);

    inline void BindGlobal(){ shader->Bind(); }
    inline void SetGlobalFloat(const char* name, float value){ shader->SetFloat(name, value); }
    inline void SetGlobalInt(const char* name, int value){ shader->SetInt(name, value); }
    inline void SetGlobalVector2( const char* name, Vector2 value){ shader->SetVector2(name, value); }
    inline void SetGlobalVector3(const char* name, Vector3 value){ shader->SetVector3(name, value); }
    inline void SetGlobaVector4(const char* name, Vector4 value){ shader->SetVector4(name, value); }
    inline void SetGlobalMatrix4(const char* name, Matrix4 value){ shader->SetMatrix4(name, value); }
    inline void SetGlobalTexture2D(const char* name, Texture2D& value, int index){ shader->SetTexture2D(name, value, index); }
    inline void SetGlobalFramebuffer(const char* name, Framebuffer& framebuffer, int index, int colorAttachmentId){ shader->SetFramebuffer(name, framebuffer, index, colorAttachmentId); }
    
    void UpdateUniforms();

    void Save(std::string& path);

    static Ref<Material> CreateFromFile(std::string const &path);

private:
    bool _isDirt = true;
};

}