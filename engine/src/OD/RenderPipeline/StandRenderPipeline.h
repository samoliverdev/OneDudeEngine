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

#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "LightComponent.h"
#include "EnvironmentComponent.h"

#include "CommandBuffer.h"
#include "RenderContext.h"

namespace OD{

class OD_API PostFXTest: public PostFX{
public:
    PostFXTest(int option):_option(option){
        _ppShader = Shader::CreateFromFile("res/Engine/Shaders/BasicPostProcessing.glsl");
        Assert(_ppShader != nullptr);
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        Shader::Bind(*_ppShader);
        _ppShader->SetFloat("option", _option);
        Graphics::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    int _option;
    Ref<Shader> _ppShader;
};

enum class ShadowTextureSize{
    _256 = 256, _512 = 512, _1024 = 1024,
    _2048 = 2048, _4096 = 4096, _8192 = 8192
};

struct OD_API ShadowSettings{
    //[Min(0.001f)]
    float maxDistance = 500.0f;
    
    //[Range(0.001f, 1f)]
	float distanceFade = 0.1f;

    enum class FilterMode{
		PCF2x2, PCF3x3, PCF5x5, PCF7x7
	};

    struct Directional{
        ShadowTextureSize altasSize;
        FilterMode filter = FilterMode::PCF2x2;

        int cascadeCount = 4; 
        float cascadeRatio1 = 0.1f;
        float cascadeRatio2 = 0.25f;
        float cascadeRatio3 = 0.5f;
        float cascadeRatio4 = 1.0f;

        //[Range(0.001f, 1f)]
		float cascadeFade = 0.1f;

        float shadowBias = 0.001f;
    };

    struct Other{
        ShadowTextureSize altasSize;
        FilterMode filter = FilterMode::PCF2x2;
    };

    Directional directional{ShadowTextureSize::_2048};
    Other other{ShadowTextureSize::_2048};
};

class Shadows{
    friend class Lighting;
public:
    Shadows();

    void Setup(RenderContext* context, ShadowSettings settings, Camera cam);
    void OnSetupLoop(RenderData& data);
    void Render();

    Vector2 ReserveDirectionalShadows(LightComponent light, Transform trans);
    Vector4 ReserveOtherShadows(LightComponent light, Transform trans);
    
private:
    void RenderDirectionalShadows();
    void RenderOtherShadows();
    void RenderSpotShadows(int index, int split, int tileSize);
    void RenderPointShadows(int index, int split, int tileSize);

    std::vector<float> shadowCascadeLevels;

    RenderContext* context;
    ShadowSettings settings;
    Camera cam;

    inline static const int maxShadowedDirectionalLightCount = 4;
    inline static const int maxShadowedOtherLightCount = 16;
    inline static const int maxCascades = 4;
    
    int shadowedDirectionalLightCount;
    int shadowedOtherLightCount;

    Framebuffer* directionalShadowAtlas;
    Framebuffer* otherShadowAtlas;
    Ref<Material> shadowPass;

    CommandBuffer shadowDirectionalLightsBuffers[maxShadowedDirectionalLightCount * maxCascades];
    ShadowSplitData shadowDirectionalLightsSplits[maxShadowedDirectionalLightCount * maxCascades];

    CommandBuffer shadowOtherLightsBuffers[maxShadowedOtherLightCount];
    ShadowSplitData shadowOtherLightsSplits[maxShadowedOtherLightCount];

    inline static const char* dirShadowAtlasId = "_DirectionalShadowAtlas";
    inline static const char* dirShadowMatricesId = "_DirectionalShadowMatrices";
    inline static const char* cascadeCountId = "_CascadeCount";
	inline static const char* cascadeCullingSpheresId = "_CascadeCullingSpheres";
    //inline static const char* cascadeDataId = "_CascadeData";
    inline static const char* shadowDistanceId = "_ShadowDistance";
    inline static const char* shadowAtlasSizeId = "_ShadowAtlasSize";
    inline static const char* shadowDistanceFadeId = "_ShadowDistanceFade";

	inline static Matrix4 dirShadowMatrices[maxShadowedDirectionalLightCount * maxCascades];
    inline static float cascadeCullingSpheres[maxCascades];
    //inline static Vector4 cascadeData[maxShadowedDirectionalLightCount * maxCascades];

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

class Lighting{
public:
    void Setup(RenderContext* context, Shadows* shadow, ShadowSettings shadowSettings, EnvironmentSettings inEnvironmentSettings);
	void SetupDirectionalLight();
    void UpdateGlobalShaders();
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

class CameraRenderer{
public:
    Camera camera;
    RenderContext* context;
    CameraRenderer();
    void Render(Camera cam, RenderContext* renderContext, ShadowSettings shadowSettings, EnvironmentSettings environmentSettings);
    inline Lighting& GetLighting(){ return lighting; }
private:
    Shadows shadows;
    Lighting lighting;

    CommandBuffer opaqueDrawTarget;
    DrawingSettings opaqueDrawSettings;

    CommandBuffer blendDrawTarget;
    DrawingSettings blendDrawSettings;

    PostFXTest* postFXTest;

    void RunSetupLoop();
    void OnSetupLoop(RenderData& data);
    void RenderVisibleGeometry(EnvironmentSettings& environmentSettings);
    void RenderUI();
    
    std::vector<PostFX*> GetPostFXs(EnvironmentSettings& environmentSettings);
};

class StandRenderPipeline: public BaseRenderPipeline{
public:
    StandRenderPipeline(Scene* _scene);
    ~StandRenderPipeline();
    System* Clone(Scene* inScene) const override;

    void SetOverrideFrameBuffer(Framebuffer* out) override;
    void SetOverrideCamera(Camera* cam, Transform trans) override;
    Framebuffer* FinalColor() override;

    SystemType Type() override { return SystemType::Renderer; }
    void Update() override;

private:
    ShadowSettings shadow;

    RenderContext* renderContext;
    CameraRenderer cameraRenderer;
    EnvironmentSettings environmentSettings;

    EnvironmentSettings defaultEnvironmentSettings;

    Camera* overrideCamera = nullptr;
    Transform overrideCameraTrans;
};

void StandRenderPipelineModuleInit();

}