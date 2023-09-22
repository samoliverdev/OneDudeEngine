#pragma once

#include <vector>
#include "OD/Scene/Scene.h"
#include "OD/Renderer/Shader.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Renderer/Framebuffer.h"
#include "OD/Renderer/Material.h"

namespace OD{

struct SpriteComponent;
struct MeshRendererComponent;
struct LightComponent;

class PostProcessingPass{
public:
    virtual void OnRenderImage(Framebuffer* src, Framebuffer* dst){}
};

struct StandRendererSystem: public OD::System{
    StandRendererSystem();
    ~StandRendererSystem();

    SystemType Type() override { return SystemType::Renderer; }
    void Update() override;

    inline void SetOutFrameBuffer(Framebuffer* out){ _outFramebuffer = out; }
    inline void overrideCamera(Camera* cam, Transform trans){ _overrideCamera = cam; _overrideCameraTrans = trans;} 

    inline Framebuffer& finalColor(){ return *_finalColor; }

    inline void AddPostProcessingPass(PostProcessingPass* pass){ _ppPass.push_back(pass); }

private:
    Ref<Shader> _spriteShader;
    Mesh _spriteMesh;

    Framebuffer* _shadowMap;
    Ref<Shader> _shadowMapShader;
    Ref<Shader> _postProcessingShader;
    Ref<Shader> _blitShader;

    Matrix4 _lightSpaceMatrix;

    Framebuffer* _outFramebuffer;
    
    Framebuffer* _pp1;
    Framebuffer* _pp2;
    Framebuffer* _finalColor;

    Camera* _overrideCamera = nullptr;
    Transform _overrideCameraTrans;

    std::vector<PostProcessingPass*> _ppPass;

    void SetStandUniforms(Shader& material);
    void RenderScene(Camera& camera, bool isMain, Vector3 camPOs);
    void RenderSceneShadow(LightComponent& light, TransformComponent& transform);
    void ClearSceneShadow();
    void UpdateCurrentLight();
};

};