#pragma once
#include "OD/Defines.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/InstancingBuffer.h"
#include "RendererList.h"
#include "LightComponent.h"
#include "PostFX.h"
#include <vector>
#include <functional>

namespace OD{

class Scene;

enum class SortType{None, CommonOpaque, CommonTransparent};
enum class RenderQueueRange{All, Opaue, Transparent};

struct OD_API DrawingSettings{
    RenderQueueRange renderQueueRange;
    SortType sortType;
    bool decalTarget = false;
    bool enableIntancing = true;
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

struct OD_API alignas(16) RenderData{
    enum Flag : uint32_t {
        AlwaysDraw        = 1 << 0,
        RenderShadow      = 1 << 1,
        IsDecal           = 1 << 2,
        IsValid           = 1 << 3
    };

    Matrix4 targetMatrix;
    AABB aabb;
    PerDrawData perDrawData;
    AlignedVector<Matrix4>* posePalette = nullptr;
    Material* targetMaterial;
    Material* customShadowPass = nullptr;
    InstancingBuffer* instancingBuffer = nullptr;
    Mesh* targetMesh;
    float distance;
    uint32_t flags = Flag::RenderShadow | Flag::IsValid;//  0;//INFO: This very simple otimization give 2x more performace!!!!!!!!!!!!!!!!!!
    /*bool awalsDraw = false;
    bool renderShadow = true;
    bool isDecal = false;
    bool isValid = true;*/

    inline void SetFlag(RenderData::Flag flag, bool enabled){
        if(enabled)
            flags |= flag;
        else
            flags &= ~flag;
    }

    inline bool HasFlag(RenderData::Flag flag) {
        return (flags & flag) != 0;
    }
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

template<typename T>
class ChunkedVector {
public:
    using Chunk = std::vector<T>;

    ChunkedVector():m_chunks(1){}

    ChunkedVector(size_t chunkCount)
        : m_chunks(chunkCount)
    {}
    
    // Access chunk by index
    Chunk& operator[](size_t chunkIndex) {
        assert(chunkIndex < m_chunks.size());
        return m_chunks[chunkIndex];
    }

    const Chunk& operator[](size_t chunkIndex) const {
        assert(chunkIndex < m_chunks.size());
        return m_chunks[chunkIndex];
    }

    T& GetNew(int chunkIndex){
        m_chunks[chunkIndex].emplace_back();
        return m_chunks[chunkIndex][m_chunks[chunkIndex].size()-1];
    }

    // Number of chunks
    size_t chunk_count() const {
        return m_chunks.size();
    }

private:
    std::vector<Chunk> m_chunks;
};

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

class OD_API RenderFeature{
public:
    Scene* scene = nullptr;
    virtual void OnCollectRenderData(const Camera& cam, std::vector<RenderData>& outRenderData){}
    virtual void OnRenderUI(const Camera& cam){}
};

class OD_API RenderContext{
public:
    friend class CameraRenderer;

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

    void UpdateRenderData();
    void RenderDataLoopNew(std::function<void(RenderData&)> onReciveRenderData);

    template<typename Func>
    void RenderDataLoopNew2(Func onReciveRenderData){
        for(int i = 0; i < renderData.chunk_count(); i++){
            for(auto& renderData: renderData[i]){
                onReciveRenderData(renderData);
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

    void BeginForwardPass();
    void EndForwardPass();

    void BeginDeferredPass();
    void EndDeferredPass();

    void DeferredCopyToForwardPass();
    void DrawDeferredLight(int index = -1, bool combinedIndirect = false);
    void DrawDeferredLightOther(int index, Vector3 pos, Vector3 dir, float size, bool isCone);

    void EndDeferredPassAndCopyToForwardPass();
    
    void ScreenClean();

    void RenderSkyboxLater();
    //void DrawRenderers(const std::vector<DrawingTarget*>& targets);
    void DrawGizmos();
    void DrawPostFXs(std::vector<PostFX*>& postFXs);

    void AddDrawRenderers(RenderData& renderData, DrawingSettings& settings, RendererList& target);
    void DrawRenderersBuffer(RendererList& commandBuffer, bool sort = false, bool deferred = false, bool isDecal = false);
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

    ShadowData shadowData;
    Ref<UniformBuffer> shadowDataBuffer;

    inline Framebuffer* GetForwardFramebuffer(){ return forwardOutColor; }
    inline Framebuffer* GetDeferredFramebuffer(){ return deferredOutColor; }

    template<typename T>
    static void RegisterRenderFeature(){
        _AddRenderFeatures().push_back([&](RenderContext& r){
            r.renderFeatures.push_back(new T());
        });
    }

    inline const std::vector<RenderFeature*>& RenderFeatures(){ return renderFeatures; }

private:
    static std::vector<std::function<void(RenderContext&)>>& _AddRenderFeatures();
    std::vector<RenderFeature*> renderFeatures;

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
    Ref<Material> deferredLightDirSinglePass;
    Ref<Material> deferredLightDirSingleOtherPass;
    Ref<Mesh> skyboxMesh;
    Ref<Mesh> spriteMesh;
    Ref<Mesh> fullScreenQuad;

    Ref<Model> decalMesh;

    Ref<Model> sphereMesh;
    Ref<Model> coneMesh;
    
    Camera cam;
    Scene* scene;

    ChunkedVector<RenderData> renderData;

    //entt::view<entt::get_t<MeshRendererComponent, TransformComponent>> meshView;
    //entt::view<entt::get_t<ModelRendererComponent, TransformComponent>> meshRenderView;

    //void SetupDrawTarget(CommandBaseData& cmd, DrawingTarget& target);
    //void SetupShadowDrawTarget(CommandBaseData& cmd, ShadowDrawingTarget& target);
    //static void SetStandUniforms(Camera& cam, SubShader& shader);
};

}