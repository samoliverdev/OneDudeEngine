#pragma once

#include <vector>
#include "OD/Scene/Scene.h"
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

struct StandRendererSystem: public OD::System{
    StandRendererSystem();
    ~StandRendererSystem();

    SystemType Type() override { return SystemType::Renderer; }
    void Update() override;

    inline void SetOutFrameBuffer(Framebuffer* out){ _outFramebuffer = out; }
    inline void overrideCamera(Camera* cam, Transform trans){ _overrideCamera = cam; _overrideCameraTrans = trans;} 

    inline Framebuffer* finalColor(){ return _finalColor; }
    inline Framebuffer* objectsId(){ return _objectsId; }
    //inline Framebuffer* finalColor2(){ return _finalColor2; }

    inline void AddPostProcessingPass(PostProcessingPass* pass){ _ppPass.push_back(pass); }

private:
    Ref<Shader> _spriteShader;
    Mesh _spriteMesh;

    Framebuffer* _objectsId;

    Framebuffer* _shadowMap;
    Ref<Shader> _shadowMapShader;
    Ref<Shader> _postProcessingShader;
    Ref<Shader> _blitShader;

    Matrix4 _lightSpaceMatrix;

    Framebuffer* _outFramebuffer = nullptr;
    
    Framebuffer* _pp1;
    Framebuffer* _pp2;
    Framebuffer* _finalColor;
    Framebuffer* _finalColor2;

    Camera* _overrideCamera = nullptr;
    Transform _overrideCameraTrans;

    std::vector<PostProcessingPass*> _ppPass;

    //Ref<Shader> _skyboxShader;
    Mesh _skyboxMesh;
    Ref<Cubemap> _skyboxCubemap;

    void SetStandUniforms(Vector3 viewPos, Shader& material);
    void RenderScene(Camera& camera, bool isMain, Vector3 camPOs);
    void RenderSceneShadow(LightComponent& light, TransformComponent& transform);
    void ClearSceneShadow();
    void UpdateCurrentLight();
};

};