#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/Color.h"
#include "Camera.h"
#include "RendererTypes.h"

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
};

class OD_API Graphics {
    friend class Application;
    friend class Material;
public:
    enum class OD_API_IMPORT RenderMode{SHADED, WIREFRAME};

    static const GraphicsStats& GetStats();

    static void Begin();
    static void End();
    static bool HasBegin();

    static void SetCamera(Camera& camera);
    static Camera GetCamera();

    static void Clean(float r, float g, float b, float a);

    static void SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    static void GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h);

    static void BindMaterial(Material& mat);
    static void DrawMesh(Mesh& mesh, Matrix4 modelMatrix);
    static void DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count);

    static void DrawMesh(Mesh& mesh, Material& shader, Matrix4 modelMatrix);
    static void DrawMeshSkinned(Mesh& mesh, Material& shader, Matrix4* modelMatrix, int count);
    static void DrawMeshInstancing(Mesh& mesh, Material& shader, Matrix4* modelMatrixs, int count);
    static void DrawModel(Model& model, Matrix4 modelMatrix);

    static void AddDrawLineCommand(Vector3 start, Vector3 end);
    static void DrawLinesComamnd(Vector3 color, int lineWidth);

    static void DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth);

    /*static void DrawText(Font& f, SubShader& s, std::string text, Vector3 pos, float scale);
    static void DrawText(Font& f, SubShader& s, std::string text, Matrix4 model);*/

    static void BeginFramebuffer(Framebuffer* framebuffer);

    static void BlitQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& shader, int pass = 0);
    static void BlitQuadPostProcessing(Framebuffer* dst, Material& shader, int pass = 0);

    static void BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass = 0);

    static void CreateLuaBind(sol::state& lua);

private:
    static void Initialize();
    static void Shutdown();

    static void SetColorMask(float r, float g, float b, float a);
    static void SetRenderMode(RenderMode mode);
    static void SetDepthMask(bool value);
    static void SetDepthTest(DepthTest depthTest);
    static void SetCullFace(CullFace cullFace);
    static void SetBlend(bool blend);
    static void SetBlendFunc(BlendMode sfactor, BlendMode dfactor);

    static void _Begin();
    static void _End();
};

void GraphicsModuleInit();

}