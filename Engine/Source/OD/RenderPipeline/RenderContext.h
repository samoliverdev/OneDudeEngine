#pragma once
#include "OD/Defines.h"
#include "OD/Core/AlignedAllocator.h"
#include "OD/Graphics/Framebuffer.h"
#include "ChunkedVector.h"
#include "RenderData.h"
#include "RendererFeature.h"
#include "PassRenderSettings.h"
#include "RendererList.h"
#include "LightComponent.h"
#include "PostFX.h"
#include "RendererFeature.h"
#include "RenderingPath.h"
#include <vector>
#include <functional>

namespace OD{

class Scene;
class UniformBuffer;
class InstancingBuffer;
class RendererFeature;
class ComputeShader;
class RenderContext;

struct OD_API CameraRenderPass {
    Camera camera;
    Ref<Framebuffer> target = nullptr;           // nullptr = default backbuffer
    int targetFace = 0;                      // for cubemaps
    RenderingPath renderingPath;
    uint32_t cullingMask = ~0u;
    int renderOrder = 0;
    PassRenderSettings settings = {};
    PassCollectSettings collectSettings = {}; 
    bool isReflectionProbePass = false;
    
    // Optional: custom environment settings, quality preset, etc.
};

enum class SortType{None, CommonOpaque, CommonTransparent};
enum class RenderQueueRange{All, Opaue, Transparent};

struct OD_API DrawingSettings{
    RenderQueueRange renderQueueRange;
    SortType sortType;
    bool decalTarget = false;
    bool enableIntancing = true;
    std::vector<uint32_t> requiredTags; //TODO: Experimental, Messure the performace Later
    std::vector<uint32_t> excludedTags; //TODO: Experimental, Messure the performace Later 
};

struct OD_API ShadowDrawingSettings{
    LightComponent light;
    Transform tranform;

    IVector4 viewport;

    RenderQueueRange renderQueueRange;
    SortType sortType;

    bool enableIntancing = true;
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

struct OD_API RenderContextSettings{
    bool enableGizmos = true;
    bool enableGizmosRuntime = false;
    bool enableWireframe = false;
};

//INFO: Need sync with UniformsDef.glsl
#define MAX_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_OTHER_LIGHT_COUNT 16
#define MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT 1
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

struct alignas(16) ShadowData{
    Matrix4 lightSpaceMatrix;
};

//TODO: Move this to new RendererFeature
/*class OD_API RenderFeature{
public:
    virtual ~RenderFeature() = default;
    Scene* scene = nullptr;
    virtual void OnCollectRenderData(const Camera& cam, std::vector<RenderData>& outRenderData){}
    virtual void OnRenderUI(const Camera& cam){}
};*/

struct OD_API SSS_Settings{
    float surfaceThickness = 0.005f;
    float bilinearThreshold = 0.02f;
    float shadowContrast = 4;
};

struct OD_API RendererFeatureContext: public IRenderer{
    RendererFeatureContext();
    virtual ~RendererFeatureContext();

    template<typename T>
    static void RegisterRenderFeature(){
        _AddRenderFeatures().push_back([&](RendererFeatureContext& c){
            c.localRenderFeatures.push_back(new T());
        });
    }

    inline const std::vector<RendererFeature*>& RenderFeatures(){ return localRenderFeatures; }

    inline void ClearRenderPasses(){
        for(auto& i: renderPasses) i.clear();
    }

    inline void AddPass(RenderPass* pass) override {
        renderPasses[(int)pass->event].push_back(pass);
    }

    void SetupFeatures(std::vector<RendererFeature*>& features);
    void FeaturesRunAddRenderPasses(RenderContext& context);

    inline std::array<std::vector<RenderPass*>, (int)RenderPassEvent::Count>& GetRenderPasses(){ return renderPasses; }

    std::array<std::vector<RenderPass*>, (int)RenderPassEvent::Count> renderPasses;
    static std::vector<std::function<void(RendererFeatureContext&)>>& _AddRenderFeatures();
    std::vector<RendererFeature*> localRenderFeatures;
    std::vector<RendererFeature*> cachedRenderFeatures;
};

class OD_API RenderContext{
public:
    friend class CameraRenderer;

    RenderContext();
    ~RenderContext();
    RenderContext(const RenderContext& other) = delete;
    RenderContext& operator=(const RenderContext& other) = delete;

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
    void RenderDataLoop(Scene& scene, std::function<void(RenderData&)> onReciveRenderData);
    void RenderDataLoop2(Scene& scene, std::function<void(RenderData&)> onReciveRenderData);

    void UpdateRenderData(Scene& scene, RendererFeatureContext& passCtx);
    void RenderDataLoopNew(std::function<void(RenderData&)> onReciveRenderData);

    template<typename Func>
    void RenderDataLoopNew2(Func&& func){
        for(int i = 0; i < renderData.chunk_count(); i++){
            for(auto& rd: renderData[i]){
                //func(rd);
                std::forward<Func>(func)(rd);
            }
        }
    }

    void BeginDrawEntityIds();
    void EndDrawEntityIds();
    void DrawEntityIds(RendererList& commandBuffer);
    int ReadPixeIntFromEntityIdsFramebuffer(int x, int y);

    void BeginUIPass();
    void EndUIPass();
    
    void BeginDrawToScreen();
    void EndDrawToScreen();

    void BeginDrawToScreenNew();
    void EndDrawToScreenNew();

    void DrawCompose(std::vector<CameraRenderPass>& passes, int width, int height);

    inline void SetCustomFinalColor(Ref<Framebuffer> f, int slice){
        customFinalColor = f;
        customFinalColorIndex = slice; 
    }

    void BeginForwardPass(bool clean = true);
    void EndForwardPass();

    void BeginDeferredPass(bool clean = true);
    void EndDeferredPass();

    void DeferredCopyToForwardPass();
    void DrawDeferredLight(int index = -1, bool combinedIndirect = false);
    void DrawDeferredLightOther(int index, Vector3 pos, Vector3 dir, float size, bool isCone);

    void CleanSSS();
    void DrawSSS(Vector3 lightDir, SSS_Settings settings = {});

    void EndDeferredPassAndCopyToForwardPass();
    
    void ScreenClean();

    void RenderSkyboxLater(Scene& scene);
    //void DrawRenderers(const std::vector<DrawingTarget*>& targets);
    void DrawGizmos(Scene& scene);
    void DrawPostFXs(std::vector<PostFX*>& postFXs);

    void DrawPostFXs(Scene& scene, RendererFeatureContext& passCtx, RenderFrameData& data, RenderPass* last = nullptr, RenderPassEvent pass = RenderPassEvent::PostProcess);

    void AddDrawRenderers(RenderData& renderData, DrawingSettings& settings, RendererList& target);
    void DrawRenderersBuffer(RendererList& commandBuffer, bool sort = false, bool deferred = false, bool isDecal = false);
    void DrawZPreePassRenderersBuffer(RendererList& commandBuffer, bool sort = false, bool post = false);

    void CleanShadow(Framebuffer& shadowMap, int layer = 0);
    void BeginDrawShadow(Framebuffer& shadowMap, int layer = 0);
    void EndDrawShadow();
    void AddDrawShadow(RenderData& renderData, ShadowDrawingSettings& settings, RendererList& target);
    void DrawShadows(RendererList& targets, ShadowSplitData& splitData, Ref<Material>& shadowPass);

    void CopyDeffered();

    //inline Scene* GetScene(){ return scene; }
    inline Ref<Framebuffer> GetFinalColor(){ return finalColor; }
    inline Camera GetCamera(){ return cam; }

    static RenderContextSettings& GetSettings();

    //-------Settings---------
    Ref<Material> skyMaterial = nullptr;
    Ref<Framebuffer> overrideFramebuffer = nullptr;
    bool isDeferred = false;    

    PipelineData pipelineData;
    Ref<UniformBuffer> pipelineDataBuffer;

    ShadowData shadowData;
    Ref<UniformBuffer> shadowDataBuffer;

    inline Ref<Framebuffer> GetForwardFramebuffer(){ return forwardOutColor; }
    inline Ref<Framebuffer> GetDeferredFramebuffer(){ return deferredOutColor; }
    inline Ref<Framebuffer> GetDeferredCopyFramebuffer(){ return deferredOutColorCopy; }

    inline Ref<Framebuffer> GetPostFXSrc(){ return step == false ? postFx1 : postFx2; }
    inline Ref<Framebuffer> GetPostFXDest(){ return step == false ? postFx2 : postFx1; }

    inline Ref<Framebuffer>& GetDirectionalShadowAtlas(){ return directionalShadowAtlas; }
    inline Ref<Framebuffer>& GetOtherShadowAtlas(){ return otherShadowAtlas; }

private:
    Ref<Framebuffer> entityIdOutColor;
    Ref<Framebuffer> deferredOutColor;
    Ref<Framebuffer> deferredOutColorCopy;
    Ref<Framebuffer> forwardOutColor;
    Ref<Framebuffer> finalColor;
    Ref<Framebuffer> postFx1;
    Ref<Framebuffer> postFx2;

    Ref<Framebuffer> directionalShadowAtlas;
    Ref<Framebuffer> otherShadowAtlas;

    Ref<Framebuffer> customFinalColor;
    int customFinalColorIndex;

    Ref<Material> entityIdShader;

    Ref<Material> blitShader;
    Ref<Material> deferredGBufferShader;
    Ref<Material> deferredLightPassShader;
    Ref<Material> deferredLightPass;
    Ref<Material> deferredLightDirSinglePass;
    Ref<Material> deferredLightDirSingleOtherPass;
    Ref<Mesh> skyboxMesh;
    Ref<Mesh> spriteMesh;
    Ref<Mesh> fullScreenQuad;

    Ref<Material> screenSpaceShadow2 = nullptr;

    Ref<Model> decalMesh;

    Ref<Model> sphereMesh;
    Ref<Model> coneMesh;
    
    Camera cam;
    //Scene* scene;

    ChunkedVector<RenderData> renderData;

    Ref<ComputeShader> screenSpaceShadow = nullptr;
    Ref<UniformBuffer> screenSpaceShadowData = nullptr;
    Ref<Framebuffer> screenSpaceShadowOutput = nullptr;

    Ref<Framebuffer> finalFramebuffer = nullptr;

    bool step = false;

    //entt::view<entt::get_t<MeshRendererComponent, TransformComponent>> meshView;
    //entt::view<entt::get_t<ModelRendererComponent, TransformComponent>> meshRenderView;

    //void SetupDrawTarget(CommandBaseData& cmd, DrawingTarget& target);
    //void SetupShadowDrawTarget(CommandBaseData& cmd, ShadowDrawingTarget& target);
    //static void SetStandUniforms(Camera& cam, SubShader& shader);
};

}