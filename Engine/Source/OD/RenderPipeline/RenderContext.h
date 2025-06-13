#pragma once
#include "OD/Defines.h"
#include "OD/Graphics/Framebuffer.h"
#include "RendererList.h"
#include "LightComponent.h"
#include "PostFX.h"

namespace OD{

class Scene;

enum class SortType{None, CommonOpaque, CommonTransparent};
enum class RenderQueueRange{All, Opaue, Transparent};

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
    Matrix4 targetMatrix;
    Ref<Material> targetMaterial;
    Ref<Mesh> targetMesh;
    AlignedVector<Matrix4>* posePalette = nullptr;
    float distance;
};

struct OD_API alignas(16) RenderData{
    Matrix4 targetMatrix;
    AABB aabb;
    AlignedVector<Matrix4>* posePalette = nullptr;
    Material* targetMaterial;
    Material* customShadowPass = nullptr;
    Mesh* targetMesh;
    float distance;

    /*#if EnableExperimentalPerDrawCustomData
    bool useCustomData = false;
    Vector4 customData;
    #endif*/

    PerDrawData perDrawData;
};

struct OD_API RenderContextSettings{
    bool enableGizmos = true;
    bool enableGizmosRuntime = false;
    bool enableWireframe = false;
};

#define MAX_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_OTHER_LIGHT_COUNT 16
#define MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_SHADOWED_OTHER_LIGHT_COUNT 16
#define MAX_CASCADE_COUNT 4

struct alignas(16) PipelineData{
    Matrix4 _DirectionalShadowMatrices[MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT * MAX_CASCADE_COUNT];
    Matrix4 _OtherShadowMatrices[MAX_SHADOWED_OTHER_LIGHT_COUNT];

    Vector4 _DirectionalLightColors[MAX_DIRECTIONAL_LIGHT_COUNT];
    Vector4 _DirectionalLightDirections[MAX_DIRECTIONAL_LIGHT_COUNT];
    Vector4 _DirectionalLightShadowData[MAX_DIRECTIONAL_LIGHT_COUNT];
    Vector4 _OtherLightColors[MAX_OTHER_LIGHT_COUNT];
    Vector4 _OtherLightPositions[MAX_OTHER_LIGHT_COUNT];
    Vector4 _OtherLightDirections[MAX_OTHER_LIGHT_COUNT];
    Vector4 _OtherLightSpotAngles[MAX_OTHER_LIGHT_COUNT];
    Vector4 _OtherLightShadowData[MAX_OTHER_LIGHT_COUNT];
    Vector4 _CascadeCullingSpheres[MAX_CASCADE_COUNT];

    Vector4 _ShadowAtlasSize;
    Vector4 _ShadowDistanceFade;
    Vector4 _AmbientLight;   
    Vector4 _IrradianceMapScale;

    float _SkyLightIntensity;
    float _ShadowDistance;
    float _Pad0;
    float _Pad1;

    int _DirectionalLightCount;
    int _OtherLightCount;
    int _CascadeCount;
    int _Pad2;
};

class OD_API RenderContext{
public:
    RenderContext(Scene* scene);
    ~RenderContext();

    void Begin();
    void End();

    struct ComputeRenderListSettings{
        Frustum frustum;
        bool checkOnFrustum;
    };
    void RunComputeRenderList(ComputeRenderListSettings settings, DrawingSettings drawSettings, RendererList& renderList);
    void RunComputeRenderListShadow(ComputeRenderListSettings settings, ShadowDrawingSettings drawSettings, RendererList& renderList, Material* shadowPass);

    //void SetupRenderers(const std::vector<DrawingTarget*>& targets, const std::vector<ShadowDrawingTarget*>& shadowTargets);
    void SetupCameraProperties(Camera cam);
    void RenderDataLoop(std::function<void(RenderData&)> onReciveRenderData);
    void RenderDataLoop2(std::function<void(RenderData&)> onReciveRenderData);

    void BeginDrawEntityIds();
    void EndDrawEntityIds();
    void DrawEntityIds(RendererList& commandBuffer);
    int ReadPixeIntFromEntityIdsFramebuffer(int x, int y);
    
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

    PipelineData pipelineData;
    Ref<UniformBuffer> pipelineDataBuffer;

    inline Framebuffer* GetForwardFramebuffer(){ return forwardOutColor; }
    inline Framebuffer* GetDeferredFramebuffer(){ return deferredOutColor; }

private:
    Framebuffer* entityIdOutColor;
    Framebuffer* deferredOutColor;
    Framebuffer* forwardOutColor;
    Framebuffer* finalColor;
    Framebuffer* postFx1;
    Framebuffer* postFx2;

    Ref<Material> entityIdShader;

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