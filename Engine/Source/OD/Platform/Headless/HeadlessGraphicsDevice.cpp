#include "HeadlessGraphicsDevice.h"

namespace OD{

void HeadlessGraphicsDevice::LoadContext(void* data){}

GraphicsStats& HeadlessGraphicsDevice::GetStats(){ return stats; }
GraphicsDeviceInfo HeadlessGraphicsDevice::GetInfo(){ return info; }

void HeadlessGraphicsDevice::Begin(){}
void HeadlessGraphicsDevice::End(){}
bool HeadlessGraphicsDevice::HasBegin(){ return false; }
void HeadlessGraphicsDevice::SetCamera(Camera& camera){}
Camera HeadlessGraphicsDevice::GetCamera(){ return camera; }
void HeadlessGraphicsDevice::BeginRenderToScreen(Vector4 clearColor){}
void HeadlessGraphicsDevice::EndRenderToScreen(){}
void HeadlessGraphicsDevice::Clean(float r, float g, float b, float a){}
void HeadlessGraphicsDevice::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){}
void HeadlessGraphicsDevice::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){}

void HeadlessGraphicsDevice::DrawMesh(Mesh& mesh, Material& shader, Matrix4 modelMatrix, PerDrawData* perDrawData){}
void HeadlessGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Material& shader, Matrix4 model, Matrix4* animMatrix, int count, PerDrawData* perDrawData){}
void HeadlessGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Material& shader, Matrix4* animMatrixs, int count){}
void HeadlessGraphicsDevice::DrawModel(Model& model, Matrix4 modelMatrix){}

void HeadlessGraphicsDevice::AddDrawLineCommand(Vector3 start, Vector3 end){}
void HeadlessGraphicsDevice::DrawLinesComamnd(Vector3 color, int lineWidth){}

void HeadlessGraphicsDevice::DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth){}
void HeadlessGraphicsDevice::DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth){}
void HeadlessGraphicsDevice::DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){}

void HeadlessGraphicsDevice::DrawFullScreenQuad(Material& mat, Matrix4 modelMatrix){}

void HeadlessGraphicsDevice::DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& shader, int pass){}
void HeadlessGraphicsDevice::DrawQuadPostProcessing(Framebuffer* dst, Material& shader, int pass){}
void HeadlessGraphicsDevice::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){}

bool HeadlessGraphicsDevice::MeshCreateOrSubmit(
    Mesh& mesh,
    std::vector<unsigned int>* indices,
    std::vector<Vector3>* vertices,
    std::vector<Vector3>* uv,
    std::vector<Vector3>* normals,
    std::vector<Vector4>* colors,
    std::vector<Vector3>* tangents ,
    std::vector<Vector4>* weights,
    std::vector<IVector4>* influences
){ return false; }
void HeadlessGraphicsDevice::MeshSubmitInstancingModelMatrixs(Mesh& mesh){}
void HeadlessGraphicsDevice::MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count){}
void HeadlessGraphicsDevice::MeshDestroy(Mesh& mesh){}
bool HeadlessGraphicsDevice::MeshIsValid(Mesh& mesh){ return false; }

void HeadlessGraphicsDevice::BeginFramebuffer(Framebuffer& frambuffer, bool clean, Vector4 clearColor, int layer, int mip){}
void HeadlessGraphicsDevice::EndFramebuffer(){}
bool HeadlessGraphicsDevice::FramebufferCreate(Framebuffer& frambuffer){ return false; }
void HeadlessGraphicsDevice::FramebufferDestroy(Framebuffer& frambuffer){}
bool HeadlessGraphicsDevice::FramebufferIsValid(Framebuffer& frambuffer){ return false; }
void* HeadlessGraphicsDevice::FramebufferColorAttachmentId(Framebuffer& framebuffer, int index){ return nullptr; }
void* HeadlessGraphicsDevice::FramebufferDepthAttachmentId(Framebuffer& framebuffer){ return nullptr; }
int HeadlessGraphicsDevice::FramebufferReadPixel(Framebuffer& frambuffer, int attachmentIndex, int x, int y){ return 0; }

bool HeadlessGraphicsDevice::Texture2DCreate(Texture2D& tex, const std::string path){ return false; }
bool HeadlessGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size){ return false; }
bool HeadlessGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, int width, int height, TextureDataType dataType){ return false; }
void HeadlessGraphicsDevice::Texture2DDestroy(Texture2D& tex){}
bool HeadlessGraphicsDevice::Texture2DIsValid(Texture2D& tex){ return false; }
void* HeadlessGraphicsDevice::Texture2DRenderId(Texture2D& tex){ return nullptr; }

bool HeadlessGraphicsDevice::Texture2DArrayCreate(Texture2DArray& tex, const std::vector<std::string>& filePaths){ return false; }
void HeadlessGraphicsDevice::Texture2DArrayDestroy(Texture2DArray& tex){}
bool HeadlessGraphicsDevice::Texture2DArrayIsValid(Texture2DArray& tex){ return false; }

bool HeadlessGraphicsDevice::CubemapCreateFromFile(
    Cubemap& cubemap,
    const char* right, const char* left, const char* top,
    const char* bottom, const char* front, const char* back
){ return false; }
void HeadlessGraphicsDevice::CubemapDestroy(Cubemap& cubemap){}
bool HeadlessGraphicsDevice::CubemapIsValid(Cubemap& tex){ return false; }

bool HeadlessGraphicsDevice::SubShaderCreateFromBaseSource(
    SubShader& shader,
    std::string& source, 
    std::vector<std::string>& keyworlds,
    ShaderPipeline pipeline, 
    std::vector<std::string>& errors
){ return false; }
void HeadlessGraphicsDevice::SubShaderDestroy(SubShader& shader){}
bool HeadlessGraphicsDevice::SubShaderIsValid(SubShader& shader){ return false; }
void HeadlessGraphicsDevice::SubShaderBind(SubShader& shader){}

bool HeadlessGraphicsDevice::ShaderCreate(Shader& shader, std::string path){ return false; }
void HeadlessGraphicsDevice::ShaderDestroy(Shader& shader){}

bool HeadlessGraphicsDevice::MaterialCreate(Material& shader){ return false; }
void HeadlessGraphicsDevice::MaterialDestroy(Material& shader){}
void HeadlessGraphicsDevice::MaterialOnSetShader(Material& shader){}
void HeadlessGraphicsDevice::MaterialOnUnsetShader(Material& shader){}

void HeadlessGraphicsDevice::Initialize(){}
void HeadlessGraphicsDevice::Shutdown(){}
void HeadlessGraphicsDevice::_Begin(){}
void HeadlessGraphicsDevice::_End(){}

bool HeadlessGraphicsDevice::ImGuiSupport(){ return false; }
void HeadlessGraphicsDevice::ImGuiNewFrame(){}
void HeadlessGraphicsDevice::ImGuiRenderDrawData(unsigned int x, unsigned int y, unsigned int w, unsigned int h){}

}