#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/Color.h"
#include "Camera.h"
#include "RendererTypes.h"
#include "Graphics.h"
#include "Framebuffer.h"
#include "Texture.h"
#include "SubShader.h"

namespace OD{

class SubShader;
class Mesh;
class Model;
class Framebuffer;
class Font;
class Material;
class Texture2D;
class Texture2DArray;
class Cubemap;
class Shader;
struct GraphicsStats;

struct GraphicsDeviceInfo{
    std::string apiName;
    int version;
};

class OD_API GraphicsDevice {
public:
    virtual GraphicsStats& GetStats() = 0;
    virtual GraphicsDeviceInfo GetInfo() = 0;

    virtual void LoadContext(void* data) = 0;

    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual bool HasBegin() = 0;

    virtual void SetCamera(Camera& camera) = 0;
    virtual Camera GetCamera() = 0;

    virtual void Clean(float r, float g, float b, float a) = 0;
    virtual void SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h) = 0;
    virtual void GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h) = 0;

    virtual void BindMaterial(Material& mat) = 0;
    virtual void DrawMesh(Mesh& mesh, Matrix4 modelMatrix) = 0;
    virtual void DrawMeshSkinned(Mesh& mesh, Matrix4 model, Matrix4* animMatrix, int count) = 0;
    virtual void DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count) = 0;

    virtual void DrawMesh(Mesh& mesh, Material& shader, Matrix4 modelMatrix) = 0;
    virtual void DrawMeshSkinned(Mesh& mesh, Material& shader, Matrix4 model, Matrix4* animMatrix, int count) = 0;
    virtual void DrawMeshInstancing(Mesh& mesh, Material& shader, Matrix4* animMatrixs, int count) = 0;
    virtual void DrawModel(Model& model, Matrix4 modelMatrix) = 0;

    virtual void AddDrawLineCommand(Vector3 start, Vector3 end) = 0;
    virtual void DrawLinesComamnd(Vector3 color, int lineWidth) = 0;

    virtual void DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth) = 0;
    virtual void DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth) = 0;
    virtual void DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth) = 0;

    virtual void BeginFramebuffer(Framebuffer& frambuffer, int layer) = 0;
    virtual void EndFramebuffer() = 0;
    virtual bool FramebufferCreate(Framebuffer& frambuffer, FrameBufferSpecification specification) = 0;
    virtual void FramebufferDestroy(Framebuffer& frambuffer) = 0;
    virtual bool FramebufferIsValid(Framebuffer& frambuffer) = 0;
    virtual void* FramebufferColorAttachmentId(Framebuffer& framebuffer, int index) = 0;
    virtual void* FramebufferDepthAttachmentId(Framebuffer& framebuffer) = 0;
    virtual int FramebufferReadPixel(Framebuffer& frambuffer, int attachmentIndex, int x, int y) = 0;
    virtual void BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass = 0) = 0;

    virtual void DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& shader, int pass = 0) = 0;
    virtual void DrawQuadPostProcessing(Framebuffer* dst, Material& shader, int pass = 0) = 0;

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
    ) = 0;
    virtual void MeshSubmitInstancingModelMatrixs(Mesh& mesh) = 0;
    virtual void MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count) = 0;
    virtual void MeshDestroy(Mesh& mesh) = 0;
    virtual bool MeshIsValid(Mesh& mesh) = 0;

    virtual bool Texture2DCreate(Texture2D& tex, const std::string path, Texture2DSetting settings) = 0;
    virtual bool Texture2DCreate(Texture2D& tex, void* data, size_t size, Texture2DSetting settings) = 0;
    virtual bool Texture2DCreate(Texture2D& tex, void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings) = 0;
    virtual void Texture2DDestroy(Texture2D& tex) = 0;
    virtual bool Texture2DIsValid(Texture2D& tex) = 0;

    virtual bool Texture2DArrayCreate(Texture2DArray& tex, const std::vector<std::string>& filePaths) = 0; 
    virtual void Texture2DArrayDestroy(Texture2DArray& tex) = 0;
    virtual bool Texture2DArrayIsValid(Texture2DArray& tex) = 0;

    virtual bool CubemapCreateFromFile(
        Cubemap& cubemap,
        const char* right, const char* left, const char* top,
        const char* bottom, const char* front, const char* back
    ) = 0; 
    virtual void CubemapDestroy(Cubemap& cubemap) = 0;
    virtual bool CubemapIsValid(Cubemap& tex) = 0;

    virtual bool SubShaderCreateFromBaseSource(
        SubShader& shader,
        std::string& source, 
        std::vector<std::string>& keyworlds,
        ShaderPipeline pipeline, 
        std::vector<std::string>& errors
    ) = 0;
    virtual void SubShaderDestroy(SubShader& shader) = 0;
    virtual bool SubShaderIsValid(SubShader& shader) = 0;
    virtual void SubShaderBind(SubShader& shader) = 0;

    virtual bool ShaderCreate(Shader& shader, std::string path) = 0;
    virtual void ShaderDestroy(Shader& shader) = 0;

    virtual bool MaterialCreate(Material& shader) = 0;
    virtual void MaterialDestroy(Material& shader) = 0;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void _Begin() = 0;
    virtual void _End() = 0;

    virtual void ImGuiNewFrame() = 0;
    virtual void ImGuiRenderDrawData(unsigned int x, unsigned int y, unsigned int w, unsigned int h) = 0;
};

}