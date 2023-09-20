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

struct StandRendererSystem: public OD::System{
    SystemType Type() override { return SystemType::Renderer; }
    
    void Update() override;
    StandRendererSystem();

    inline void SetOutFrameBuffer(Framebuffer* out){ _outFramebuffer = out; }
    inline void overrideCamera(Camera* cam){ _overrideCamera = cam; } 

    inline Framebuffer& finalColor(){ return *_finalColor; }

private:
    Ref<Shader> _spriteShader;
    Mesh _spriteMesh;

    Framebuffer* _shadowMap;
    Ref<Shader> _shadowMapShader;
    Ref<Shader> _postProcessingShader;

    Matrix4 _lightSpaceMatrix;

    Framebuffer* _outFramebuffer;

    Framebuffer* _finalColor;

    Camera* _overrideCamera = nullptr;

    void SetStandUniforms(Shader& material);
    void RenderScene(Camera& camera, bool isMain);
    void RenderSceneShadow(LightComponent& light, TransformComponent& transform);
    void ClearSceneShadow();
    void UpdateCurrentLight();
};

};