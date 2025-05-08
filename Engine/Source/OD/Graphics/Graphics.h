#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/Color.h"
#include "Camera.h"
#include "RendererTypes.h"
#include "Framebuffer.h"
#include "GraphicsDevice.h"

#define EnableExperimentalPerDrawCustomData 1

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
    static GraphicsStats& GetStats();
    static void Begin();
    static void End();
    static bool HasBegin();

    static void SetCamera(Camera& camera);
    static Camera GetCamera();

    //static void SubmitGlobalData(void* data, size_t size);
    //static void SubmitRenderPipelineData(void* data, size_t size);

    static void BeginRenderToScreen(Vector4 clearColor = Vector4(0, 0, 0, 1));
    static void EndRenderToScreen();

    static void Clean(float r, float g, float b, float a);
    static void SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    static void GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h);

    static void BindMaterial(Material& mat);
    static void DrawMesh(Mesh& mesh, Matrix4 modelMatrix);
    static void DrawMesh(Mesh& mesh, Matrix4 modelMatrix, Vector4 customData);
    static void DrawMeshSkinned(Mesh& mesh, Matrix4 model, Matrix4* animMatrix, int count);
    static void DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count);

    static void DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix);
    static void DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, Matrix4* animMatrix, int count);
    static void DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count);
    static void DrawModel(Model& model, Matrix4 modelMatrix);

    static void AddDrawLineCommand(Vector3 start, Vector3 end);
    static void DrawLinesComamnd(Vector3 color, int lineWidth);

    static void DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth);

    static void DrawFullScreenQuad(Material& mat, Matrix4 modelMatrix);

    static void DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& mat, int pass = 0);
    static void DrawQuadPostProcessing(Framebuffer* dst, Material& mat, int pass = 0);
    static void BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass = 0);

    static void BeginFramebuffer(Framebuffer& frambuffer, Vector4 clearColor = Vector4(0, 0, 0, 1), int layer = 0, int mip = 0);
    static void EndFramebuffer();

    static void CreateLuaBind(sol::state& lua);

private:
    static void SelectGraphicsDevice();
    static void Initialize();
    static void Shutdown();
    static void _Begin();
    static void _End();
};

void GraphicsModuleInit();

}