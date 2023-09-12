#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "OD/Platform/GL.h"
#include "Texture.h"
#include "Framebuffer.h"

namespace OD {

class Renderer;

class Shader: public Asset{
    friend class Renderer;
public:
    virtual ~Shader();

    static Ref<Shader> CreateFromFile(const std::string& filepath);

    void Destroy();
    bool IsValid();

    void Bind();
    void Unbind();

    void SetFloat(const char* name, float value);
    void SetInt(const char* name, int value);
    void SetVector2( const char* name, Vector2 value);
    void SetVector3(const char* name, Vector3 value);
    void SetVector4(const char* name, Vector4 value);
    void SetMatrix4(const char* name, Matrix4 value);
    void SetTexture2D(const char* name, Texture2D& value, int index);
    void SetFramebuffer(const char* name, Framebuffer& framebuffer, int index, int colorAttachmentId);

private:
    unsigned int _rendererId;
    std::unordered_map<std::string, GLint> _uniforms;
    
    GLint GetLocation(const char* name);
	std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
	void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);
};

}