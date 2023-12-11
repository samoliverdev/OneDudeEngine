#pragma once

#include <vector>
#include "OD/Scene/Scene.h"
#include "OD/Scene/Culling.h"
#include "OD/Renderer/Shader.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Renderer/Framebuffer.h"
#include "OD/Renderer/Material.h"
#include "OD/Renderer/Cubemap.h"

#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "LightComponent.h"
#include "EnvironmentComponent.h"

namespace OD{

class PostProcessingPass{
public:
    virtual void OnRenderImage(Framebuffer* src, Framebuffer* dst){}
    bool enable = true;
};

struct RenderCamera{
    Camera cam;
    Frustum frustum;
};

struct StandRendererSystem: public OD::System{
    StandRendererSystem();
    ~StandRendererSystem();

    SystemType Type() override { return SystemType::Renderer; }
    void Update() override;

    inline void SetOutFrameBuffer(Framebuffer* out){ outFramebuffer = out; }
    inline void GetOverrideCamera(RenderCamera* cam, Transform trans){ overrideCamera = cam; overrideCameraTrans = trans;} 

    inline Framebuffer* FinalColor(){ return finalColor; }
    inline Framebuffer* ObjectsId(){ return objectsId; }
    //inline Framebuffer* finalColor2(){ return _finalColor2; }

    inline void AddPostProcessingPass(PostProcessingPass* pass){ ppPass.push_back(pass); }

private:
    Ref<Shader> spriteShader;
    Mesh spriteMesh;

    Framebuffer* objectsId;

    Ref<Shader> shadowMapShader;
    Ref<Shader> postProcessingShader;
    Ref<Shader> blitShader;

    Matrix4 lightSpaceMatrix;

    Framebuffer* outFramebuffer = nullptr;
    
    Framebuffer* pp1;
    Framebuffer* pp2;
    Framebuffer* finalColor;
    Framebuffer* finalColor2;

    RenderCamera* overrideCamera = nullptr;
    Transform overrideCameraTrans;

    std::vector<PostProcessingPass*> ppPass;

    //Ref<Shader> _skyboxShader;
    Mesh skyboxMesh;
    Ref<Cubemap> skyboxCubemap;

    struct CommandBaseData{
        Ref<Material> targetMaterial;
        Ref<Mesh> targetMesh;
        Matrix4 targetMatrix;
        float distance;
    };

    void UpdateAllCommands(Vector3 viewPos);
    void UpdateOpaquesCommands(CommandBaseData& data);
    void UpdateOpaquesIntancingCommands(CommandBaseData& data);
    void UpdateBlendCommands(CommandBaseData& data);
    void UpdateCascadeShadowCommands(CommandBaseData& data);
    void UpdateCascadeShadowIntancingCommands(CommandBaseData& data);
    
    void SetStandUniforms(Vector3 viewPos, Shader& material);
    void RenderScene(RenderCamera& camera, bool isMain, Vector3 camPOs);

    struct ShadowRenderPass{
        Framebuffer* shadowMap;
        Matrix4 lightSpaceMatrix;

        void Clean(StandRendererSystem& root);
        void Render(LightComponent& light, TransformComponent& transform, StandRendererSystem& root);
    };

    ShadowRenderPass directinallightShadowPass;

    #define MAX_SPOTLIGHT_SHADOWS 5
    ShadowRenderPass spotlightShadowPass[MAX_SPOTLIGHT_SHADOWS];
    int spotlightShadowPassCount = 0;

    #define SHADOW_MAP_CASCADE_COUNT 4
    struct CascadeShadow{
        Matrix4 projViewMatrix;
        float splitDistance;

        Framebuffer* shadowMap;

        void Clean(StandRendererSystem& root);
        void Render(LightComponent& light, TransformComponent& transform, StandRendererSystem& root);

        static void UpdateCascadeShadow(CascadeShadow* cascadeShadows, Camera& cam, TransformComponent& light);
        static void UpdateCascadeShadow2(CascadeShadow* cascadeShadows, Camera& cam, TransformComponent& light);
    };
    CascadeShadow cascadeShadows[SHADOW_MAP_CASCADE_COUNT];

    Framebuffer* cascadeShadowMap;
    Ref<Shader> cascadeShadowMapShader;
    void RenderCascadeShadow(LightComponent& light, TransformComponent& transform, StandRendererSystem& root);
};

};