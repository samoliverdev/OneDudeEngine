#pragma once
#include "OD/Core/Asset.h"
#include "OD/Platform/OpenGL/GL.h"
#include <unordered_map>

namespace OD {

class Texture2D;
class Framebuffer;
class UniformBuffer;
class ComputeBuffer;

class OD_API ComputeShader : public Asset {
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
public:
    static Ref<ComputeShader> Create(const std::string& path);

    ComputeShader();
    ~ComputeShader();

    bool LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;

    void Dispatch(uint32_t x, uint32_t y, uint32_t z);

    void SetTexture(const char* name, Ref<Texture2D> tex);
    void SetTexture(const char* name, Framebuffer* tex, int attachment);

    void SetUniformBuffer(const char* name, Ref<UniformBuffer> buffer, int bind);
    void SetComputeBuffer(const char* name, Ref<ComputeBuffer> buffer, int bind);

    void SetInt(const char* name, int v);
    void SetFloat(const char* name, float v);
    void SetVector4(const char* name, Vector4 v);

    bool IsValid();

private:
    ComputeShaderDataGL;
};

}