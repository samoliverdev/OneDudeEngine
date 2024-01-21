#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Texture.h"
#include "Cubemap.h"
#include "Framebuffer.h"
#include "UniformBuffer.h"
#include "RendererTypes.h"

namespace OD {

class Shader: public Asset{
    friend class Graphics;
public:
    static Ref<Shader> CreateFromFile(const std::string& filepath);
    static Ref<Shader> CreateFromFile(const std::string& filepath, std::vector<std::string>& keyworlds);

    static void Destroy(Shader& shader);
    static void Bind(Shader& shader);
    static void Unbind();

    virtual ~Shader();

    bool IsValid();

    void SetFloat(const char* name, float value);
    void SetFloat(const char* name, float* value, int count);
    void SetInt(const char* name, int value);
    void SetVector2( const char* name, Vector2 value);
    void SetVector3(const char* name, Vector3 value);
    void SetVector4(const char* name, Vector4 value);
    void SetVector4(const char* name, Vector4* value, int count);
    void SetMatrix4(const char* name, Matrix4 value);
    void SetMatrix4(const char* name, std::vector<Matrix4>& value);
    void SetMatrix4(const char* name, Matrix4* value, int count);
    void SetTexture2D(const char* name, Texture2D& value, int index);
    void SetCubemap(const char* name, Cubemap& value, int index);
    void SetUniforBuffer(const char* name, UniformBuffer& buffer, int index);
    void SetFramebuffer(const char* name, Framebuffer& framebuffer, int index, int colorAttachmentId);

    inline unsigned int RendererId(){ return rendererId; }

    inline bool SupportInstancing(){ return supportInstancing; }
    inline CullFace GetCullFace(){ return cullFace; }
    inline DepthTest GetDepthTest(){ return depthTest; }
    inline bool IsDepthMask(){ return depthMask; }
    inline bool IsBlend(){ return blend; }
    inline BlendMode GetSrcBlend(){ return srcBlend; }
    inline BlendMode GetDstBlend(){ return dstBlend; }

    inline std::vector<std::vector<std::string>>& Properties(){ return properties; }

private:
    bool supportInstancing = false;
    
    CullFace cullFace = CullFace::BACK;
    DepthTest depthTest = DepthTest::LESS;
    bool depthMask = true;
    bool blend = false;
    BlendMode srcBlend;
    BlendMode dstBlend;
    
    unsigned int rendererId;
    std::unordered_map<std::string, GLint> uniforms;
    std::vector<std::vector<std::string>> properties;
    
    bool Create(const std::string& filepath, std::vector<std::string>& keyworlds);
    std::string load(std::string path);
    GLint GetLocation(const char* name);
	std::unordered_map<GLenum, std::string> PreProcess(const std::string& source, std::vector<std::string>& keyworlds);
	void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);
};

}