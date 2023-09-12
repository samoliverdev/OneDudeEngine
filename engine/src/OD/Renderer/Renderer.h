#pragma once

#include "OD/Core/Math.h"
#include "Mesh.h"
#include "Model.h"
#include "Shader.h"
#include "Camera.h"
#include "Framebuffer.h"

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

class Renderer {
    friend class Application;
public:
    enum class RenderMode{SHADED, WIREFRAME};

    static void Begin();
    static void End();

    static void Clean(float r, float g, float b, float a);

    static void SetCamera(Camera& camera);

    static void DrawMeshRaw(Mesh& mesh);
    static void DrawMesh(Mesh& mesh, Matrix4 modelMatrix, Shader& shader);
    static void DrawMesh(Mesh& mesh, Matrix4 modelMatrix, Material& shader);
    static void DrawModel(Model& model, Matrix4 modelMatrix, int subMeshIndex = -1, Ref<Material> materialOverride = nullptr);

    static void DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth);

    static void SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    static void GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h);

    static void SetRenderMode(RenderMode mode);
    static void SetDepthTest(DepthTest depthTest);
    static void SetCullFace(CullFace cullFace);

    static void BeginFramebuffer(Framebuffer* framebuffer);
    static void Blit(Framebuffer* src, Framebuffer* dst, Shader& shader, int pass = 0);

private:
    static void _Initialize();
    static void _Shutdown();
};

}