#pragma once

#include "OD/Core/Math.h"
#include "Mesh.h"
#include "Model.h"
#include "Shader.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "Font.h"

namespace OD {

class Application;

enum class DepthTest{
    DISABLE         = 0,
    LESS            = 1,
    LESS_EQUAL      = 2,
    EQUAL           = 3,
    GREATER         = 4,
    GREATER_EQUAL   = 5,
    DIFFERENT       = 6,
    NEVER           = 7,
    ALWAYS          = 8
};

enum class CullFace{
    NONE            = 0,
    BACK            = 1,
    FRONT           = 2,
    FRONT_AND_BACK  = 3
};

enum class BlendMode{
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA	
};

class Renderer {
    friend class Application;
public:
    enum class RenderMode{SHADED, WIREFRAME};

    static int drawCalls;
    static int vertices;
    static int tris;

    static void Begin();
    static void End();

    static void Clean(float r, float g, float b, float a);

    static void SetCamera(Camera& camera);
    static Camera GetCamera();

    static void DrawMeshRaw(Mesh& mesh);
    static void DrawMesh(Mesh& mesh, Matrix4 modelMatrix, Shader& shader);
    static void DrawMesh(Mesh& mesh, Matrix4 modelMatrix, Material& shader);
    static void DrawMeshMVP(Mesh& mesh, Matrix4& modelMatrix, Shader& shader);
    static void DrawModel(Model& model, Matrix4 modelMatrix, int subMeshIndex = -1, std::vector<Ref<Material>>* materialsOverride = nullptr);

    static void DrawMeshInstancing(Mesh& mesh, Shader& shader, int count);

    static void DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth);

    static void DrawText(Font& f, Shader& s, std::string text, Vector3 pos, float scale, Vector3 color);

    static void SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    static void GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h);

    static void SetRenderMode(RenderMode mode);
    static void SetDepthMask(bool value);
    static void SetDepthTest(DepthTest depthTest);
    static void SetCullFace(CullFace cullFace);
    static void SetBlend(bool blend);
    static void SetBlendFunc(BlendMode sfactor, BlendMode dfactor);

    static void BeginFramebuffer(Framebuffer* framebuffer);
    static void BlitQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Shader& shader, int pass = 0);
    static void BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass = 0);

private:
    static void _Initialize();
    static void _Shutdown();
};

}