#pragma once

#include <vector>
#include "OD/Scene/Scene.h"
#include "OD/Scene/BaseRenderPipeline.h"
#include "OD/Renderer/Culling.h"
#include "OD/Renderer/Shader.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Renderer/Framebuffer.h"
#include "OD/Renderer/Material.h"
#include "OD/Renderer/Cubemap.h"

#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "LightComponent.h"
#include "EnvironmentComponent.h"

#include "CommandBuffer.h"
#include "RenderContext.h"

namespace OD{

struct ShadowSettings{
    enum class TextureSize{
        _256 = 256, _512 = 512, _1024 = 1024,
        _2048 = 2048, _4096 = 4096, _8192 = 8192
    };

    //[Min(0.001f)]
    float maxDistance = 500.0f;
    
    //[Range(0.001f, 1f)]
	float distanceFade = 0.1f;

    struct Directional{
        TextureSize altasSize;
        int cascadeCount = 4; 
        float cascadeRatio1 = 0.1f;
        float cascadeRatio2 = 0.25f;
        float cascadeRatio3 = 0.5f;

        //[Range(0.001f, 1f)]
		float cascadeFade = 0.1f;

        float shadowBias = 0.001f;
    };

    Directional directional{TextureSize::_2048};
};

class Shadows{
    friend class Lighting;
public:
    Shadows();

    void Setup(RenderContext* context, ShadowSettings settings, Camera cam);
    void OnSetupLoop(RenderData& data);
    void Render();

    Vector2 ReserveDirectionalShadows(LightComponent light, Transform trans);
    
private:
    void RenderDirectionalShadows();

    RenderContext* context;
    ShadowSettings settings;
    Camera cam;

    inline static const int maxShadowedDirectionalLightCount = 4;
    inline static const int maxCascades = 4;
    
    int shadowedDirectionalLightCount;

    Framebuffer* directionalShadowAtlas;
    Ref<Material> shadowPass;

    CommandBuffer shadowDirectionalLightsBuffers[maxShadowedDirectionalLightCount * maxCascades];
    ShadowSplitData shadowDirectionalLightsSplits[maxShadowedDirectionalLightCount * maxCascades];

    inline static const char* dirShadowAtlasId = "_DirectionalShadowAtlas";
    inline static const char* dirShadowMatricesId = "_DirectionalShadowMatrices";
    inline static const char* cascadeCountId = "_CascadeCount";
	inline static const char* cascadeCullingSpheresId = "_CascadeCullingSpheres";
    //inline static const char* cascadeDataId = "_CascadeData";
    inline static const char* shadowDistanceId = "_ShadowDistance";
    inline static const char* shadowDistanceFadeId = "_ShadowDistanceFade";

	inline static Matrix4 dirShadowMatrices[maxShadowedDirectionalLightCount * maxCascades];
    inline static float cascadeCullingSpheres[maxShadowedDirectionalLightCount * maxCascades];
    //inline static Vector4 cascadeData[maxShadowedDirectionalLightCount * maxCascades];
};

class Lighting{
public:
    void Setup(RenderContext* context, Shadows* shadow, ShadowSettings shadowSettings);
	void SetupDirectionalLight();
    void UpdateGlobalShaders();
private:
    RenderContext* context;
    Shadows* shadows;

    int currentLightsCount;

    inline static const int maxDirLightCount = 4;

    inline static const char* dirLightCountId = "_DirectionalLightCount";
    inline static const char* dirLightColorsId = "_DirectionalLightColors";
	inline static const char* dirLightDirectionsId = "_DirectionalLightDirections";
    inline static const char* dirLightShadowDataId = "_DirectionalLightShadowData";

    inline static Vector4 dirLightColors[maxDirLightCount];
	inline static Vector4 dirLightDirections[maxDirLightCount];
    inline static Vector4 dirLightShadowData[maxDirLightCount];
};

class CameraRenderer{
public:
    Camera camera;
    RenderContext* context;
    void Render(Camera cam, RenderContext* renderContext, ShadowSettings shadowSettings);
    inline Lighting& GetLighting(){ return lighting; }
private:
    Shadows shadows;
    Lighting lighting;

    CommandBuffer opaqueDrawTarget;
    DrawingSettings opaqueDrawSettings;

    CommandBuffer blendDrawTarget;
    DrawingSettings blendDrawSettings;
    //std::vector<DrawingTarget*> drawTargets{&opaqueDrawTarget, &blendDrawTarget};

    void Setup();
    void DrawShadows();
    void DrawVisibleGeometry();
    void OnSetupLoop(RenderData& data);
};

class StandRenderPipeline2: public BaseRenderPipeline{
public:
    StandRenderPipeline2(Scene* _scene);
    ~StandRenderPipeline2();
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

    Camera* overrideCamera = nullptr;
    Transform overrideCameraTrans;
};

}