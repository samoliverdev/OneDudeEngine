#pragma once
#include "CommandBuffer.h"

namespace OD{

enum class SortType{None, CommonOpaque, CommonTransparent};
enum class RenderQueueRange{All, Opaue, Transparent};

struct DrawingSettings{
    bool enableIntancing = true;
    RenderQueueRange renderQueueRange;
    SortType sortType;
};

struct DrawingTarget{
    DrawingSettings settings;
    CommandBuffer commandBuffer;
};

struct CommandBaseData{
    Ref<Material> targetMaterial;
    Ref<Mesh> targetMesh;
    Matrix4 targetMatrix;
    float distance;
};

class RenderContext{
public:
    RenderContext(Scene* scene);
    ~RenderContext();

    void Begin();
    void End();

    void SetupCameraProperties(Camera cam);

    void SetupRenderers(DrawingTarget* targets, int count);
    void DrawRenderers(DrawingTarget* targets, int count);
    void DrawGizmos();

    void Clean();
    void RenderSkybox();
    void Submit();

    inline Scene* GetScene(){ return scene; }
    inline Framebuffer* GetFinalColor(){ return finalColor; }

    std::set<Ref<Shader>> globalShaders;

    //-------Settings---------
    Ref<Material> skyMaterial;
    Framebuffer* overrideFramebuffer = nullptr;

private:
    Framebuffer* outColor;
    Framebuffer* finalColor;

    Ref<Shader> blitShader;
    Mesh skyboxMesh;
    Camera cam;

    Scene* scene;

    void SetupDrawTarget(CommandBaseData& cmd, DrawingTarget& target);
    void SetStandUniforms(Camera& cam, Shader& shader);
};


}