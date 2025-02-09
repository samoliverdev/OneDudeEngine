#ifdef OPENGL_SUPPORT
#include "OpenGLGraphicsDevice.h"
#include "GL.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Camera.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Font.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Model.h"
#include "OD/Graphics/SubShader.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Font.h"

namespace OD{

const GraphicsStats& OpenGLGraphicsDevice::GetStats(){ 
    return stats; 
}

void OpenGLGraphicsDevice::Initialize(){

}

void OpenGLGraphicsDevice::Shutdown(){

}

void OpenGLGraphicsDevice::Begin(){
    
}

void OpenGLGraphicsDevice::End(){

}

void OpenGLGraphicsDevice::_Begin(){

}

void OpenGLGraphicsDevice::_End(){

}

bool OpenGLGraphicsDevice::HasBegin(){
    return false;
}

void OpenGLGraphicsDevice::Clean(float r, float g, float b, float a){
     
}

void OpenGLGraphicsDevice::SetCamera(Camera& inCamera){
    
}

Camera OpenGLGraphicsDevice::GetCamera(){
    return camera;
}

void OpenGLGraphicsDevice::BindMaterial(Material& mat){
    
}  

void OpenGLGraphicsDevice::DrawMesh(Mesh& mesh, Matrix4 modelMatrix){
    
}

void OpenGLGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Matrix4 modelMatrix, Matrix4* animMatrixs, int count){
    
}

void OpenGLGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count){
    
}

void OpenGLGraphicsDevice::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix){
    
}

void OpenGLGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 modelMatrix, Matrix4* animMatrixs, int count){
   
}

void OpenGLGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* modelMatrixs, int count){
    
}

void OpenGLGraphicsDevice::DrawModel(Model& model, Matrix4 modelMatrix){
    
}

void OpenGLGraphicsDevice::AddDrawLineCommand(Vector3 start, Vector3 end){
    
}

void OpenGLGraphicsDevice::DrawLinesComamnd(Vector3 color, int lineWidth){
    
}

void OpenGLGraphicsDevice::DrawLine(Vector3 start, Vector3 end, Vector3 color, int width){
    
}

void OpenGLGraphicsDevice::DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int width){
    
}

void OpenGLGraphicsDevice::DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){
    
}

void OpenGLGraphicsDevice::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
    
}

void OpenGLGraphicsDevice::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){
   
}

void OpenGLGraphicsDevice::BeginFramebuffer(Framebuffer* framebuffer){

}

void OpenGLGraphicsDevice::DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& mat, int pass){

}

void OpenGLGraphicsDevice::DrawQuadPostProcessing(Framebuffer* dst, Material& mat, int pass){

}

void OpenGLGraphicsDevice::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){

}

bool OpenGLGraphicsDevice::MeshCreateOrSubmit(
    Mesh& mesh,
    std::vector<unsigned int>* indices,
    std::vector<Vector3>* vertices,
    std::vector<Vector3>* uv,
    std::vector<Vector3>* normals,
    std::vector<Vector4>* colors,
    std::vector<Vector3>* tangents,
    std::vector<Vector4>* weights,
    std::vector<IVector4>* influences
){
    return false;
}

void OpenGLGraphicsDevice::MeshSubmitInstancingModelMatrixs(Mesh& mesh){

}

void OpenGLGraphicsDevice::MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count){

}

void OpenGLGraphicsDevice::MeshDestroy(Mesh& mesh){

}

void OpenGLGraphicsDevice::BeginFramebuffer(Framebuffer& frambuffer, int layer){

}

void OpenGLGraphicsDevice::EndFramebuffer(){

}

bool OpenGLGraphicsDevice::FramebufferCreate(Framebuffer& frambuffer, FrameBufferSpecification specification){
    return false;
}

void OpenGLGraphicsDevice::FramebufferDestroy(Framebuffer& frambuffer){

}

int OpenGLGraphicsDevice::FramebufferReadPixel(Framebuffer& frambuffer, int attachmentIndex, int x, int y){
    return 0;
}

bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, const std::string path, Texture2DSetting settings){
    return false;
}

bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, Texture2DSetting settings){
    return false;
}

bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings){
    return false;
}

void OpenGLGraphicsDevice::Texture2DDestroy(Texture2D& tex){

}

bool OpenGLGraphicsDevice::Texture2DArrayCreate(Texture2DArray& tex, const std::vector<std::string>& filePaths){
    return false;
}

void OpenGLGraphicsDevice::Texture2DArrayDestroy(Texture2DArray& tex){
    
}

bool OpenGLGraphicsDevice::CubemapCreateFromFile(
    Cubemap& cubemap,
    const char* right, const char* left, const char* top,
    const char* bottom, const char* front, const char* back
){
    return false;
}

void OpenGLGraphicsDevice::CubemapDestroy(Cubemap& cubemap){

}

bool OpenGLGraphicsDevice::SubShaderCreateFromBaseSource(
    SubShader& shader,
    std::string& source, 
    std::vector<std::string>& keyworlds,
    ShaderPipeline pipeline, 
    std::vector<std::string>& errors
){
    return false;
}

void OpenGLGraphicsDevice::SubShaderDestroy(SubShader& shader){

}

void OpenGLGraphicsDevice::SubShaderBind(SubShader& shader){

}

bool OpenGLGraphicsDevice::ShaderCreate(Shader& shader, std::string path){
    return false;
}

void OpenGLGraphicsDevice::ShaderDestroy(Shader& shader){

}

bool OpenGLGraphicsDevice::MaterialCreate(Material& shader){
    return false;
}

void OpenGLGraphicsDevice::MaterialDestroy(Material& shader){

}

}
#endif