#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/Color.h"
#include "Camera.h"
#include "RendererTypes.h"
#include "Framebuffer.h"
#include "GraphicsDevice.h"

namespace sol{ class state; }

namespace OD {

class SubShader;
class Mesh;
class Model;
class Framebuffer;
class Font;
class Material;

struct GraphicsStats{
    int drawCalls;
    int vertices;
    int tris;
    int shaderBinds;
    int uniformSet;
    int materialSubmitDatas;
};

enum class OD_API_IMPORT RenderMode{SHADED, WIREFRAME};

class OD_API Graphics {
    friend class Application;
    friend class Material;
public:
    inline static const GraphicsStats& GetStats(){ return Device().GetStats(); }
    inline static void Begin(){ Device().Begin(); }
    inline static void End(){ Device().End(); }
    inline static bool HasBegin(){ return Device().HasBegin(); }

    inline static void SetCamera(Camera& camera){ Device().SetCamera(camera); }
    inline static Camera GetCamera(){ return Device().GetCamera(); }

    inline static void Clean(float r, float g, float b, float a){ Device().Clean(r, g, b, a); }
    inline static void SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){ Device().SetViewport(x, y, w, h); }
    inline static void GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){ Device().GetViewport(x, y, w, h); }

    inline static void BindMaterial(Material& mat){ Device().BindMaterial(mat); }
    inline static void DrawMesh(Mesh& mesh, Matrix4 modelMatrix){ Device().DrawMesh(mesh, modelMatrix); }
    inline static void DrawMeshSkinned(Mesh& mesh, Matrix4 model, Matrix4* animMatrix, int count){ Device().DrawMeshSkinned(mesh, model, animMatrix, count); }
    inline static void DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count){ Device().DrawMeshInstancing(mesh, modelMatrixs, count); }

    inline static void DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix){ Device().DrawMesh(mesh, mat, modelMatrix); }
    inline static void DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, Matrix4* animMatrix, int count){ Device().DrawMeshSkinned(mesh, mat, model, animMatrix, count); }
    inline static void DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count){ Device().DrawMeshInstancing(mesh, mat, animMatrixs, count); }
    inline static void DrawModel(Model& model, Matrix4 modelMatrix){ Device().DrawModel(model, modelMatrix); }

    inline static void AddDrawLineCommand(Vector3 start, Vector3 end){ Device().AddDrawLineCommand(start, end); }
    inline static void DrawLinesComamnd(Vector3 color, int lineWidth){ Device().DrawLinesComamnd(color, lineWidth); };

    inline static void DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth){ Device().DrawLine(start, end, color, lineWidth); };
    inline static void DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth){ Device().DrawLine(model, start, end, color, lineWidth); };
    inline static void DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){ Device().DrawWireCube(modelMatrix, color, lineWidth); };

    inline static void BeginFramebuffer(Framebuffer* framebuffer){ Device().BeginFramebuffer(framebuffer); };

    inline static void DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& mat, int pass = 0){ Device().DrawQuadPostProcessing(src, dst, mat, pass); };
    inline static void DrawQuadPostProcessing(Framebuffer* dst, Material& mat, int pass = 0){ Device().DrawQuadPostProcessing(dst, mat, pass); };
    inline static void BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass = 0){ Device().BlitFramebuffer(src, dst, srcPass); };

    inline static bool MeshCreateOrSubmit(
        Mesh& mesh,
        std::vector<unsigned int>* indices,
        std::vector<Vector3>* vertices,
        std::vector<Vector3>* uv = nullptr,
        std::vector<Vector3>* normals = nullptr,
        std::vector<Vector4>* colors = nullptr,
        std::vector<Vector3>* tangents = nullptr,
        std::vector<Vector4>* weights = nullptr,
        std::vector<IVector4>* influences = nullptr
    ){ 
        return Device().MeshCreateOrSubmit(
            mesh, 
            indices, 
            vertices, 
            uv, 
            normals,
            colors,
            tangents,
            weights
        ); 
    };

    inline static void MeshSubmitInstancingModelMatrixs(Mesh& mesh){ Device().MeshSubmitInstancingModelMatrixs(mesh); };
    inline static void MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count){ Device().MeshSubmitInstancingCustomModelMatrixs(mesh, modelMatrixs, count); };
    inline static void MeshDestroy(Mesh& mesh){ Device().MeshDestroy(mesh); };

    inline static void BeginFramebuffer(Framebuffer& frambuffer, int layer = 0){ Device().BeginFramebuffer(frambuffer, layer); };
    inline static void EndFramebuffer(){ Device().EndFramebuffer(); };
    inline static bool FramebufferCreate(Framebuffer& frambuffer, FrameBufferSpecification specification){ return Device().FramebufferCreate(frambuffer, specification); };
    inline static void FramebufferDestroy(Framebuffer& frambuffer){ Device().FramebufferDestroy(frambuffer); };
    inline static int FramebufferReadPixel(Framebuffer& frambuffer, int attachmentIndex, int x, int y){ return Device().FramebufferReadPixel(frambuffer, attachmentIndex, x, y); };

    inline static bool Texture2DCreate(Texture2D& tex, const std::string path, Texture2DSetting settings){ return Device().Texture2DCreate(tex, path, settings); };
    inline static bool Texture2DCreate(Texture2D& tex, void* data, size_t size, Texture2DSetting settings){ return Device().Texture2DCreate(tex, data, size, settings); };
    inline static bool Texture2DCreate(Texture2D& tex, void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings){ return Device().Texture2DCreate(tex, data, size, width, height, dataType, settings); };
    inline static void Texture2DDestroy(Texture2D& tex){ Device().Texture2DDestroy(tex); };

    inline static bool Texture2DArrayCreate(Texture2DArray& tex, const std::vector<std::string>& filePaths){ return Device().Texture2DArrayCreate(tex, filePaths); };
    inline static void Texture2DArrayDestroy(Texture2DArray& tex){ Device().Texture2DArrayDestroy(tex); }

    inline static bool CubemapCreateFromFile(
        Cubemap& cubemap,
        const char* right, const char* left, const char* top,
        const char* bottom, const char* front, const char* back
    ){ 
        return Device().CubemapCreateFromFile(
            cubemap,
            right, left, top,
            bottom, front, back
        ); 
    }

    inline static void CubemapDestroy(Cubemap& cubemap){ Device().CubemapDestroy(cubemap); }

    inline static bool SubShaderCreateFromBaseSource(
        SubShader& shader,
        std::string& source, 
        std::vector<std::string>& keyworlds,
        ShaderPipeline pipeline, 
        std::vector<std::string>& errors
    ){ 
        return Device().SubShaderCreateFromBaseSource(shader, source, keyworlds, pipeline, errors); 
    }

    inline static void SubShaderDestroy(SubShader& shader){ Device().SubShaderDestroy(shader); }
    inline static void SubShaderBind(SubShader& shader){ Device().SubShaderBind(shader); }

    inline static bool ShaderCreate(Shader& shader, std::string path){ return Device().ShaderCreate(shader, path); }
    inline static void ShaderDestroy(Shader& shader){ Device().ShaderDestroy(shader); }

    inline static bool MaterialCreate(Material& shader){ return Device().MaterialCreate(shader); }
    inline static void MaterialDestroy(Material& shader){ Device().MaterialDestroy(shader); }

    static void CreateLuaBind(sol::state& lua);

private:
    static inline std::vector<std::string> supportedGraphicsDevices = {
        #ifdef OPENGL_SUPPORT 
        "OpenGL" 
        #endif
    };

    static GraphicsDevice& Device();

    static void Initialize();
    static void Shutdown();
};

void GraphicsModuleInit();

}