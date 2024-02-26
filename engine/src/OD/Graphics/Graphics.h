#pragma once

#include "OD/Core/Math.h"
#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "Font.h"
#include "RendererTypes.h"

namespace OD {

class OD_API Graphics {
    friend class Application;
public:
    enum class RenderMode{SHADED, WIREFRAME};

    static int GetDrawCallsCount();
    static int GetVerticesCount();
    static int GetTrisCount();

    static void Begin();
    static void End();

    static void Clean(float r, float g, float b, float a);

    static void SetCamera(Camera& camera);
    static Camera GetCamera();

    static void SetDefaultShaderData(Shader& shader, Matrix4 modelMatrix, bool instancing = false);

    static void DrawMesh(Mesh& mesh);
    static void DrawMeshInstancing(Mesh& mesh, int count);

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
    static void Initialize();
    static void Shutdown();
};

}