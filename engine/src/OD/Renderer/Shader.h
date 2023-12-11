#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "OD/Platform/GL.h"
#include "Texture.h"
#include "Cubemap.h"
#include "Framebuffer.h"

namespace OD {

class Renderer;

class Shader: public Asset{
    friend class Renderer;
public:
    enum class BlendMode{ OFF, Blend};

    static bool CreateFromFile(Shader& shader, const std::string& filepath);
    static void Destroy(Shader& shader);
    static void Bind(Shader& shader);
    static void Unbind();

    virtual ~Shader();

    bool IsValid();

    void SetFloat(const char* name, float value);
    void SetInt(const char* name, int value);
    void SetVector2( const char* name, Vector2 value);
    void SetVector3(const char* name, Vector3 value);
    void SetVector4(const char* name, Vector4 value);
    void SetMatrix4(const char* name, Matrix4 value);
    void SetMatrix4(const char* name, std::vector<Matrix4>& value);
    void SetTexture2D(const char* name, Texture2D& value, int index);
    void SetCubemap(const char* name, Cubemap& value, int index);
    void SetFramebuffer(const char* name, Framebuffer& framebuffer, int index, int colorAttachmentId);

    inline unsigned int RendererId(){ return rendererId; }

    inline BlendMode GetBlendMode(){ return blendMode; }
    inline bool SupportInstancing(){ return supportInstancing; }
    inline std::vector<std::vector<std::string>>& Properties(){ return properties; }

private:
    BlendMode blendMode = BlendMode::OFF;
    bool supportInstancing = false;

    unsigned int rendererId;
    std::unordered_map<std::string, GLint> uniforms;
    std::vector<std::vector<std::string>> properties;
    
    std::string load(std::string path);
    GLint GetLocation(const char* name);
	std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
	void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);
};

}