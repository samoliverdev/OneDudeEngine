#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Texture.h"
#include "Cubemap.h"
#include "Framebuffer.h"
#include "UniformBuffer.h"
#include "RendererTypes.h"
#include <algorithm>

namespace OD {

class OD_API Shader: public Asset{
    friend class Graphics;
public:
    static Ref<Shader> CreateFromFile(const std::string& filepath);
    static Ref<Shader> CreateFromFile(const std::string& filepath, std::vector<std::string>& keyworlds);

    bool LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;

    static void Destroy(Shader& shader);
    static void Bind(Shader& shader);
    static void Unbind();

    virtual ~Shader();

    bool IsValid();

    void OnGui() override;

    void Reload() override;

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
    void SetTexture2DArray(const char* name, Texture2DArray& value, int index);
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

    inline bool ContainUniformName(const std::string& name){ return std::find(_uniforms.begin(), _uniforms.end(), name) != _uniforms.end(); }

    inline std::vector<std::vector<std::string>>& Properties(){ return properties; }
    inline std::vector<std::vector<std::string>>& Pragmas(){ return pragmas; }

private:
    bool supportInstancing = false;
    
    CullFace cullFace = CullFace::BACK;
    DepthTest depthTest = DepthTest::LESS;
    bool depthMask = true;
    bool blend = false;
    BlendMode srcBlend;
    BlendMode dstBlend;

    std::vector<std::string> enabledKeyworlds;
    
    unsigned int rendererId = 0;
    std::unordered_map<std::string, int> uniforms;
    std::vector<std::string> _uniforms;
    std::vector<std::vector<std::string>> properties;
    std::vector<std::vector<std::string>> pragmas;
    
    bool Create(const std::string& filepath, std::vector<std::string>& keyworlds);
    std::string load(std::string path);
    int GetLocation(const char* name);
	std::unordered_map<unsigned int, std::string> PreProcess(const std::string& source, std::vector<std::string>& keyworlds);
	void Compile(const std::unordered_map<unsigned int, std::string>& shaderSources);
};

}