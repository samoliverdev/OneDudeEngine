#pragma once
#include "OD/Defines.h"
#include "OD/Graphics/Framebuffer.h"
#include "RendererList.h"
#include "LightComponent.h"

namespace OD{

class Scene;

enum class SortType{None, CommonOpaque, CommonTransparent};
enum class RenderQueueRange{All, Opaue, Transparent};

class OD_API PostFX{
public:
    bool enable = true;

    virtual ~PostFX(){}
    virtual void OnSetup(){}
    virtual void OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context){}
    virtual void OnGui(){}
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
    Matrix4 targetMatrix;
    AABB aabb;
    Material* targetMaterial;
    Material* customShadowPass = nullptr;
    Mesh* targetMesh;
    std::vector<Matrix4>* posePalette = nullptr;
    float distance;
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
    void RenderDataLoop(std::function<void(RenderData&)> onReciveRenderData);
    
    void BeginDrawToScreen();
    void EndDrawToScreen();

    void BeginForwardPass();
    void EndForwardPass();

    void BeginDeferredPass();
    void EndDeferredPass();

    void EndDeferredPassAndCopyToForwardPass();
    
    void ScreenClean();

    void RenderSkyboxLater();
    //void DrawRenderers(const std::vector<DrawingTarget*>& targets);
    void DrawGizmos();
    void DrawPostFXs(std::vector<PostFX*>& postFXs);

    void AddDrawRenderers(RenderData& renderData, DrawingSettings& settings, RendererList& target);
    void DrawRenderersBuffer(RendererList& commandBuffer, bool sort = false, bool deferred = false);
    void DrawZPreePassRenderersBuffer(RendererList& commandBuffer, bool sort = false, bool post = false);

    void CleanShadow(Framebuffer* shadowMap, int layer = 0);
    void BeginDrawShadow(Framebuffer* shadowMap, int layer = 0);
    void EndDrawShadow();
    void AddDrawShadow(RenderData& renderData, ShadowDrawingSettings& settings, RendererList& target);
    void DrawShadows(RendererList& targets, ShadowSplitData& splitData, Ref<Material>& shadowPass);

    inline Scene* GetScene(){ return scene; }
    inline Framebuffer* GetFinalColor(){ return finalColor; }
    inline Camera GetCamera(){ return cam; }

    static RenderContextSettings& GetSettings();

    //-------Settings---------
    Ref<Material> skyMaterial = nullptr;
    Framebuffer* overrideFramebuffer = nullptr;
    bool isDeferred = false;

    inline Framebuffer* GetForwardFramebuffer(){ return forwardOutColor; }
    inline Framebuffer* GetDeferredFramebuffer(){ return deferredOutColor; }

private:
    Framebuffer* deferredOutColor;
    Framebuffer* forwardOutColor;
    Framebuffer* finalColor;
    Framebuffer* postFx1;
    Framebuffer* postFx2;

    Ref<Material> blitShader;
    Ref<Material> deferredGBufferShader;
    Ref<Material> deferredLightPassShader;
    Ref<Material> deferredLightPass;
    Ref<Mesh> skyboxMesh;
    Ref<Mesh> spriteMesh;
    Ref<Mesh> fullScreenQuad;
    
    Camera cam;
    Scene* scene;

    //entt::view<entt::get_t<MeshRendererComponent, TransformComponent>> meshView;
    //entt::view<entt::get_t<ModelRendererComponent, TransformComponent>> meshRenderView;

    //void SetupDrawTarget(CommandBaseData& cmd, DrawingTarget& target);
    //void SetupShadowDrawTarget(CommandBaseData& cmd, ShadowDrawingTarget& target);
    //static void SetStandUniforms(Camera& cam, SubShader& shader);
};


}