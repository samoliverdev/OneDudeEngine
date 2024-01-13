#pragma once
#include "CommandBuffer.h"
#include "LightComponent.h"
#include "OD/Scene/Scene.h"

namespace OD{

enum class SortType{None, CommonOpaque, CommonTransparent};
enum class RenderQueueRange{All, Opaue, Transparent};

struct DrawingSettings{
    bool enableIntancing = true;
    RenderQueueRange renderQueueRange;
    SortType sortType;
};

struct ShadowDrawingSettings{
    bool enableIntancing = true;
    RenderQueueRange renderQueueRange;
    SortType sortType;

    LightComponent light;
    Transform tranform;

    IVector4 viewport;
};

struct ShadowSplitData{
    Matrix4 projViewMatrix;
    float splitDistance;
    
    static void SetupCascade(ShadowSplitData* splitData, int count, Camera& cam, Transform& light);
};

struct CommandBaseData{
    Ref<Material> targetMaterial;
    Ref<Mesh> targetMesh;
    Matrix4 targetMatrix;
    std::vector<Matrix4>* posePalette = nullptr;
    float distance;
};

struct RenderData{
    Ref<Material> targetMaterial;
    Ref<Mesh> targetMesh;
    Matrix4 targetMatrix;
    std::vector<Matrix4>* posePalette = nullptr;
    float distance;
    Transform transform;
    AABB aabb;
};

class RenderContext{
public:
    RenderContext(Scene* scene);
    ~RenderContext();

    //void SetupRenderers(const std::vector<DrawingTarget*>& targets, const std::vector<ShadowDrawingTarget*>& shadowTargets);
    void SetupCameraProperties(Camera cam);
    void SetupLoop(std::function<void(RenderData&)> onReciveRenderData);
    
    void BeginDrawToScreen();
    void EndDrawToScreen();
    void Clean();

    void RenderSkybox();
    //void DrawRenderers(const std::vector<DrawingTarget*>& targets);
    void DrawGizmos();

    void AddDrawRenderers(RenderData& renderData, DrawingSettings& settings, CommandBuffer& target);
    void DrawRenderersBuffer(CommandBuffer& commandBuffer);

    void BeginDrawShadow(Framebuffer* shadowMap, int layer = 0);
    void EndDrawShadow();
    void AddDrawShadow(RenderData& renderData, ShadowDrawingSettings& settings, CommandBuffer& target);
    void DrawShadows(CommandBuffer& targets, ShadowSplitData& splitData, Ref<Material>& shadowPass);

    inline Scene* GetScene(){ return scene; }
    inline Framebuffer* GetFinalColor(){ return finalColor; }
    inline Camera GetCamera(){ return cam; }

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

    //void SetupDrawTarget(CommandBaseData& cmd, DrawingTarget& target);
    //void SetupShadowDrawTarget(CommandBaseData& cmd, ShadowDrawingTarget& target);
    static void SetStandUniforms(Camera& cam, Shader& shader);
};


}