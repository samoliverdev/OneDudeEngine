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

class Lighting{
public:
    void Setup(RenderContext* context);
	void SetupDirectionalLight();

private:
    RenderContext* context;

    inline static const int maxDirLightCount = 4;

    inline static const char* dirLightCountId = "_DirectionalLightCount";
    inline static const char* dirLightColorsId = "_DirectionalLightColors";
	inline static const char* dirLightDirectionsId = "_DirectionalLightDirections";

    inline static Vector4 dirLightColors[maxDirLightCount];
	inline static Vector4 dirLightDirections[maxDirLightCount];
};

class CameraRenderer{
public:
    Camera camera;
    RenderContext* context;
    void Render(Camera cam, RenderContext* renderContext);
private:
    DrawingTarget drawTargets[2];
    Lighting lighting;

    void Setup();
    void DrawVisibleGeometry();
    void Submit();
};

class StandRenderPipeline2: public BaseRenderPipeline{
public:
    StandRenderPipeline2(Scene* _scene);
    ~StandRenderPipeline2();

    void SetOverrideFrameBuffer(Framebuffer* out) override;
    void SetOverrideCamera(Camera* cam, Transform trans) override;
    Framebuffer* FinalColor() override;

    SystemType Type() override { return SystemType::Renderer; }
    void Update() override;

private:
    RenderContext* renderContext;
    CameraRenderer cameraRenderer;
    EnvironmentSettings environmentSettings;

    Camera* overrideCamera = nullptr;
    Transform overrideCameraTrans;
};

}