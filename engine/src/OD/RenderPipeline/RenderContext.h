#pragma once
#include "OD/Defines.h"
#include "CommandBuffer.h"
#include "LightComponent.h"

namespace OD{

class Scene;

enum class SortType{None, CommonOpaque, CommonTransparent};
enum class RenderQueueRange{All, Opaue, Transparent};

class OD_API PostFX{
public:
    virtual void OnSetup(){}
    virtual void OnRenderImage(Framebuffer* src, Framebuffer* dst){}
    bool enable = true;
};

struct OD_API DrawingSettings{
    bool enableIntancing = true;
    RenderQueueRange renderQueueRange;
    SortType sortType;
};

struct OD_API ShadowDrawingSettings{
    bool enableIntancing = true;
    RenderQueueRange renderQueueRange;
    SortType sortType;

    LightComponent light;
    Transform tranform;

    IVector4 viewport;
};

struct OD_API ShadowSplitData{
    Matrix4 projViewMatrix;
    //float splitDistance;
    Frustum frustum;
    
    static void SetupCascade(ShadowSplitData* splitData, int count, Camera& cam, Transform& light, std::vector<float>& shadowCascadeLevels);
    static void ComputeSpotShadowData(ShadowSplitData* splitData, LightComponent& light, Transform& lightTrans);
    static void ComputePointShadowData(ShadowSplitData* splitData, LightComponent& light, Transform& lightTrans);
};

struct OD_API CommandBaseData{
    Ref<Material> targetMaterial;
    Ref<Mesh> targetMesh;
    Matrix4 targetMatrix;
    std::vector<Matrix4>* posePalette = nullptr;
    float distance;
};

struct OD_API RenderData{
    Ref<Material> targetMaterial;
    Ref<Mesh> targetMesh;
    Matrix4 targetMatrix;
    std::vector<Matrix4>* posePalette = nullptr;
    float distance;
    Transform transform;
    AABB aabb;
};

struct OD_API RenderContextSettings{
    bool enableGizmos = true;
    bool enableGizmosRuntime = false;
    bool enableWireframe = false;
};

class OD_API RenderContext{
public:
    RenderContext(Scene* scene);
    ~RenderContext();

    void Begin();
    void End();

    //void SetupRenderers(const std::vector<DrawingTarget*>& targets, const std::vector<ShadowDrawingTarget*>& shadowTargets);
    void SetupCameraProperties(Camera cam);
    void SetupLoop(std::function<void(RenderData&)> onReciveRenderData);
    
    void BeginDrawToScreen();
    void EndDrawToScreen();
    void ScreenClean();

    void RenderSkybox();
    void RenderSkybox(Ref<Cubemap>& skyTexture);
    //void DrawRenderers(const std::vector<DrawingTarget*>& targets);
    void DrawGizmos();
    void DrawPostFXs(std::vector<PostFX*>& postFXs);

    void AddDrawRenderers(RenderData& renderData, DrawingSettings& settings, CommandBuffer& target);
    void DrawRenderersBuffer(CommandBuffer& commandBuffer);

    void CleanShadow(Framebuffer* shadowMap, int layer = 0);
    void BeginDrawShadow(Framebuffer* shadowMap, int layer = 0);
    void EndDrawShadow();
    void AddDrawShadow(RenderData& renderData, ShadowDrawingSettings& settings, CommandBuffer& target);
    void DrawShadows(CommandBuffer& targets, ShadowSplitData& splitData, Ref<Material>& shadowPass);

    inline Scene* GetScene(){ return scene; }
    inline Framebuffer* GetFinalColor(){ return finalColor; }
    inline Camera GetCamera(){ return cam; }

    static RenderContextSettings& GetSettings();

    //-------Settings---------
    Ref<Material> skyMaterial;
    Framebuffer* overrideFramebuffer = nullptr;

private:
    Framebuffer* outColor;
    Framebuffer* finalColor;
    Framebuffer* postFx1;
    Framebuffer* postFx2;

    Ref<Shader> blitShader;
    Ref<Mesh> skyboxMesh;
    Camera cam;

    Scene* scene;

    //void SetupDrawTarget(CommandBaseData& cmd, DrawingTarget& target);
    //void SetupShadowDrawTarget(CommandBaseData& cmd, ShadowDrawingTarget& target);
    static void SetStandUniforms(Camera& cam, Shader& shader);
};


}