#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/BaseRenderPipeline.h"
#include "OD/Graphics/Culling.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Cubemap.h"
#include "OD/Graphics/Graphics.h"
#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "LightComponent.h"
#include "EnvironmentComponent.h"
#include "RendererList.h"
#include "RenderContext.h"

#define UseAsGlobal 1

namespace OD{

class ComputeShader;

class OD_API PostFXTest: public PostFX{
public:
    PostFXTest(int option):_option(option){
        _ppShader = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/BasicPostProcessing.glsl"));

        Assert(_ppShader != nullptr);
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context) override {
        _ppShader->SetFloat("option", _option);
        Graphics::DrawQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    int _option;
    Ref<Material> _ppShader;
};

class OD_API GamaCorrectionPP: public PostFX{
public:
    GamaCorrectionPP(){
        gamaCorrection = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/GamaCorrectionPP.glsl"));
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context) override{
        //Graphics::DrawQuadPostProcessing(src, dst, *gamaCorrection);

        Graphics::BeginFramebuffer(*dst);
        gamaCorrection->SetTexture("mainTex", src, 0);
        Graphics::DrawFullScreenQuad(*gamaCorrection, Matrix4Identity);
        Graphics::EndFramebuffer();
    }

private:
    Ref<Material> gamaCorrection = nullptr;
};

enum class ShadowTextureSize{
    _256 = 256, _512 = 512, _1024 = 1024,
    _2048 = 2048, _4096 = 4096, _8192 = 8192
};

struct OD_API ShadowSettings{
    float maxDistance = 500.0f; //[Min(0.001f)]
	float distanceFade = 0.1f; //[Range(0.001f, 1f)]

    /*enum class FilterMode{
		PCF2x2, PCF3x3, PCF5x5, PCF7x7
	};*/

    struct Directional{
        ShadowTextureSize altasSize;
        //FilterMode filter = FilterMode::PCF2x2;

        int cascadeCount = 4; 
        float cascadeRatio1 = 0.1f;
        float cascadeRatio2 = 0.25f;
        float cascadeRatio3 = 0.5f;
        float cascadeRatio4 = 1.0f;

		float cascadeFade = 0.1f; //[Range(0.001f, 1f)]
        float shadowBias = 0.001f;
    };

    struct Other{
        ShadowTextureSize altasSize;
        //FilterMode filter = FilterMode::PCF2x2;
    };

    Directional directional{ShadowTextureSize::_2048};
    Other other{ShadowTextureSize::_2048};
};

class OD_API IRenderPass{
public:
    virtual ~IRenderPass(){}
    virtual void OnRender(Scene& scene, const Camera& cam){}
};

enum class RenderStage{
    UI = 0,
    Count = 1
};

struct OD_API RenderStagePasses{
    std::vector<IRenderPass*> renderPass[(int)RenderStage::Count];
};

class OD_API Shadows{
    friend class Lighting;
public:
    Shadows();
    ~Shadows();

    void Setup(RenderContext* context, ShadowSettings settings, Camera cam);
    void AddRunComputeRenderList();
    void AddRenderData(RenderData& data);
    void Render();

    Vector2 ReserveDirectionalShadows(LightComponent light, Transform trans);
    Vector4 ReserveOtherShadows(LightComponent light, Transform trans);
    
    inline Framebuffer* GetDirectionalShadowAtlas(){ return directionalShadowAtlas; };
    inline Framebuffer* GetOtherShadowAtlas(){ return otherShadowAtlas; };

private:
    void RenderDirectionalShadows();
    void RenderOtherShadows();
    void RenderSpotShadows(int index, int split, int tileSize);
    void RenderPointShadows(int index, int split, int tileSize);

    std::vector<float> shadowCascadeLevels;

    RenderContext* context;
    ShadowSettings settings;
    Camera cam;
    ShadowDrawingSettings drawSettings;

    inline static const int maxShadowedDirectionalLightCount = 1; //2;
    inline static const int maxShadowedOtherLightCount = 6*3;
    inline static const int maxCascades = 4;
    
    int shadowedDirectionalLightCount;
    int shadowedOtherLightCount;

    Framebuffer* directionalShadowAtlas;
    Framebuffer* otherShadowAtlas;
    Ref<Material> shadowPass;

    RendererList shadowDirectionalLightsBuffers[maxShadowedDirectionalLightCount * maxCascades];
    ShadowSplitData shadowDirectionalLightsSplits[maxShadowedDirectionalLightCount * maxCascades];

    RendererList shadowOtherLightsBuffers[maxShadowedOtherLightCount];
    ShadowSplitData shadowOtherLightsSplits[maxShadowedOtherLightCount];

    inline static const char* dirShadowAtlasId = "_DirectionalShadowAtlas";
    inline static const char* dirShadowMatricesId = "_DirectionalShadowMatrices";
    inline static const char* cascadeCountId = "_CascadeCount";
	inline static const char* cascadeCullingSpheresId = "_CascadeCullingSpheres";
    inline static const char* shadowDistanceId = "_ShadowDistance";
    inline static const char* shadowAtlasSizeId = "_ShadowAtlasSize";
    inline static const char* shadowDistanceFadeId = "_ShadowDistanceFade";

	inline static Matrix4 dirShadowMatrices[maxShadowedDirectionalLightCount * maxCascades];
    inline static float cascadeCullingSpheres[maxCascades];

    inline static const char* otherShadowAltasId = "_OtherShadowAtlas";
    inline static const char* otherShadowMatricesId = "_OtherShadowMatrices";

    inline static Matrix4 otherShadowMatrices[maxShadowedOtherLightCount];

    struct ShadowedOtherLight{
        int visibleLightIndex;
        float slopeScaleBias;
        float normalBias;
        bool isPoint;
    };

    inline static ShadowedOtherLight shadowedOtherLights[maxShadowedOtherLightCount]; 
};

class OD_API Lighting{
    friend class CameraRenderer;
public:
    void Setup(RenderContext* context, Shadows* shadow, ShadowSettings shadowSettings, EnvironmentSettings inEnvironmentSettings);
	void SetupDirectionalLight();
    void UpdateGlobalShaders();

    Vector3 mainDirectionalLightDir = Vector3Zero;

private:
    RenderContext* context;
    Shadows* shadows;
    EnvironmentSettings environmentSettings;

    int curDirLightsCount;
    int curOtherLightsCount;

    inline static const int maxDirLightCount = 4;
    inline static const int maxOtherLightCount = 64;

    inline static const char* ambientLightId = "_AmbientLight";

    inline static const char* dirLightCountId = "_DirectionalLightCount";       
    inline static const char* dirLightColorsId = "_DirectionalLightColors";
	inline static const char* dirLightDirectionsId = "_DirectionalLightDirections";
    inline static const char* dirLightShadowDataId = "_DirectionalLightShadowData";

    inline static Vector4 dirLightColors[maxDirLightCount];
	inline static Vector4 dirLightDirections[maxDirLightCount];
    inline static Vector4 dirLightShadowData[maxDirLightCount];

    inline static const char* otherLightCountId = "_OtherLightCount";
    inline static const char* otherLightColorsId = "_OtherLightColors";
    inline static const char* otherLightPositionsId = "_OtherLightPositions";
    inline static const char* otherLightDirectionId = "_OtherLightDirections";
    inline static const char* otherLightSpotAnglesId = "_OtherLightSpotAngles";
    inline static const char* otherLightShadowDataId = "_OtherLightShadowData";

    inline static Vector4 otherLightColors[maxOtherLightCount];
	inline static Vector4 otherLightPositions[maxOtherLightCount];
    inline static Vector4 otherLightDirections[maxOtherLightCount];
    inline static Vector4 otherLightSpotAngles[maxOtherLightCount];
    inline static Vector4 otherLightShadowData[maxOtherLightCount];
};

class OD_API CameraRenderer{
public:
    enum class RenderingPath{
        Forward,
        Deferred
    };

    Camera camera;
    RenderStagePasses* renderStagePasses;
    RenderContext* context;
    RenderingPath renderingPath;
    
    CameraRenderer();
    ~CameraRenderer();
    void Render(Camera cam, RenderContext* renderContext, ShadowSettings shadowSettings, EnvironmentSettings& environmentSettings, RenderingPath renderingPath = RenderingPath::Forward);
    inline Lighting& GetLighting(){ return lighting; }
    inline Shadows& GetShadows(){ return shadows; }

    void RenderEntityIds(Camera cam, RenderContext* renderContext);

private:
    Shadows shadows;
    Lighting lighting;

    RendererList opaqueDrawTarget;
    DrawingSettings opaqueDrawSettings;

    RendererList blendDrawTarget;
    DrawingSettings blendDrawSettings;

    RendererList decalDrawTarget;
    DrawingSettings decalDrawSettings;

    RendererList entityIdDrawTarget;
    DrawingSettings entityIdDrawSettings;

    Ref<Material> blitPass;

    //PostFXTest* postFXTest;

    Ref<Material> cubemapSkyMaterial = nullptr;
    Ref<Texture2D> brdfLUT = nullptr;
    Ref<Mesh> cubeMesh = nullptr;

    Ref<Mesh> spriteMesh = nullptr;
    Ref<Material> spriteMaterial = nullptr;

    Ref<Font> font = nullptr;
    Ref<Material> fontMaterial = nullptr;
    
    GamaCorrectionPP* gamaCorrectionPP = nullptr;

    void RunRenderDataLoop();
    void AddRenderData(RenderData& data);
    void RenderVisibleGeometry(EnvironmentSettings& environmentSettings);
    void RenderSprites();
    void RenderUI();
    
    std::vector<PostFX*> GetPostFXs(EnvironmentSettings& environmentSettings);
};

class OD_API StandRenderPipeline: public BaseRenderPipeline{
public:
    StandRenderPipeline(){ name = "StandRenderPipeline"; }

    void OnInit(Scene& scene);
    void OnEnd(Scene& scene);
    //~StandRenderPipeline(){}
    //System* Clone(Scene* inScene) const override;

    void SetOverrideFrameBuffer(Framebuffer* out) override;
    void SetOverrideCamera(Camera* cam, Transform trans) override;
    Framebuffer* FinalColor() override;

    inline bool ExecuteAlways() override { return true; }

    int Type() override { return SystemType::Renderer | SystemType::Stand | SystemType::Late; }
    void Update(Scene& scene) override;
    void LateUpdate(Scene& scene) override;
    void Render(Scene& scene) override;

    void OnDrawGizmos(Scene& scene, Camera& cam) override;
    void OnDrawGizmosSelected(Scene& scene, Camera& cam, Entity entity) override;

    int ReadEntityId(int x, int y) override;

    inline CameraRenderer& GetCameraRenderer(){ return cameraRenderer; }
    inline RenderStagePasses& GetRenderStagePasses(){ return renderStagePasses; }

    void SaveScreenshot(const std::string& filename);

    int ExecutionSortPriority(SystemType type) override; 

private:
    ShadowSettings shadow;

    RenderContext* renderContext;
    CameraRenderer cameraRenderer;
    //EnvironmentSettings environmentSettings;

    EnvironmentSettings defaultEnvironmentSettings;

    Camera* overrideCamera = nullptr;
    Transform overrideCameraTrans;

    RenderStagePasses renderStagePasses;
};

void StandRenderPipelineModuleInit();

}