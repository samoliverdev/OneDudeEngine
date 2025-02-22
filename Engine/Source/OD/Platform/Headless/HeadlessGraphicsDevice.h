#pragma once
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"

namespace OD{

class HeadlessGraphicsDevice: public GraphicsDevice{
public:
    virtual void LoadContext(void* data) override;

    virtual GraphicsStats& GetStats() override;
    virtual GraphicsDeviceInfo GetInfo() override;

    virtual void Begin() override;
    virtual void End() override;
    virtual bool HasBegin() override;

    virtual void SetCamera(Camera& camera) override;
    virtual Camera GetCamera() override;

    virtual void Clean(float r, float g, float b, float a) override;
    virtual void SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h) override;
    virtual void GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h) override;

    virtual void BindMaterial(Material& mat) override;
    virtual void DrawMesh(Mesh& mesh, Matrix4 modelMatrix) override;
    virtual void DrawMeshSkinned(Mesh& mesh, Matrix4 model, Matrix4* animMatrix, int count) override;
    virtual void DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count) override;

    virtual void DrawMesh(Mesh& mesh, Material& shader, Matrix4 modelMatrix) override;
    virtual void DrawMeshSkinned(Mesh& mesh, Material& shader, Matrix4 model, Matrix4* animMatrix, int count) override;
    virtual void DrawMeshInstancing(Mesh& mesh, Material& shader, Matrix4* animMatrixs, int count) override;
    virtual void DrawModel(Model& model, Matrix4 modelMatrix) override;

    virtual void AddDrawLineCommand(Vector3 start, Vector3 end) override;
    virtual void DrawLinesComamnd(Vector3 color, int lineWidth) override;

    virtual void DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth) override;
    virtual void DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth) override;
    virtual void DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth) override;

    virtual void DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& shader, int pass = 0) override;
    virtual void DrawQuadPostProcessing(Framebuffer* dst, Material& shader, int pass = 0) override;
    virtual void BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass = 0) override;

    virtual bool MeshCreateOrSubmit(
        Mesh& mesh,
        std::vector<unsigned int>* indices,
        std::vector<Vector3>* vertices,
        std::vector<Vector3>* uv = nullptr,
        std::vector<Vector3>* normals = nullptr,
        std::vector<Vector4>* colors = nullptr,
        std::vector<Vector3>* tangents = nullptr,
        std::vector<Vector4>* weights = nullptr,
        std::vector<IVector4>* influences = nullptr
    ) override;
    virtual void MeshSubmitInstancingModelMatrixs(Mesh& mesh) override;
    virtual void MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count) override;
    virtual void MeshDestroy(Mesh& mesh) override;
    virtual bool MeshIsValid(Mesh& mesh) override;

    virtual void BeginFramebuffer(Framebuffer& frambuffer, int layer) override;
    virtual void EndFramebuffer() override;
    virtual bool FramebufferCreate(Framebuffer& frambuffer, FrameBufferSpecification specification) override;
    virtual void FramebufferDestroy(Framebuffer& frambuffer) override;
    virtual bool FramebufferIsValid(Framebuffer& frambuffer) override;
    virtual void* FramebufferColorAttachmentId(Framebuffer& framebuffer, int index) override;
    virtual void* FramebufferDepthAttachmentId(Framebuffer& framebuffer) override;
    virtual int FramebufferReadPixel(Framebuffer& frambuffer, int attachmentIndex, int x, int y) override;

    virtual bool Texture2DCreate(Texture2D& tex, const std::string path, Texture2DSetting settings) override;
    virtual bool Texture2DCreate(Texture2D& tex, void* data, size_t size, Texture2DSetting settings) override;
    virtual bool Texture2DCreate(Texture2D& tex, void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings) override;
    virtual void Texture2DDestroy(Texture2D& tex) override;
    virtual bool Texture2DIsValid(Texture2D& tex) override;
    virtual void* Texture2DRenderId(Texture2D& tex) override;

    virtual bool Texture2DArrayCreate(Texture2DArray& tex, const std::vector<std::string>& filePaths) override; 
    virtual void Texture2DArrayDestroy(Texture2DArray& tex) override;
    virtual bool Texture2DArrayIsValid(Texture2DArray& tex) override;

    virtual bool CubemapCreateFromFile(
        Cubemap& cubemap,
        const char* right, const char* left, const char* top,
        const char* bottom, const char* front, const char* back
    ) override; 
    virtual void CubemapDestroy(Cubemap& cubemap) override;
    virtual bool CubemapIsValid(Cubemap& tex) override;

    virtual bool SubShaderCreateFromBaseSource(
        SubShader& shader,
        std::string& source, 
        std::vector<std::string>& keyworlds,
        ShaderPipeline pipeline, 
        std::vector<std::string>& errors
    ) override;
    virtual void SubShaderDestroy(SubShader& shader) override;
    virtual bool SubShaderIsValid(SubShader& shader) override;
    virtual void SubShaderBind(SubShader& shader) override;

    virtual bool ShaderCreate(Shader& shader, std::string path) override;
    virtual void ShaderDestroy(Shader& shader) override;

    virtual bool MaterialCreate(Material& shader) override;
    virtual void MaterialDestroy(Material& shader) override;

    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void _Begin() override;
    virtual void _End() override;

    virtual bool ImGuiSupport() override;
    virtual void ImGuiNewFrame() override;
    virtual void ImGuiRenderDrawData(unsigned int x, unsigned int y, unsigned int w, unsigned int h) override;

    GraphicsDeviceInfo info;
    GraphicsStats stats;
    Camera camera;
};

}