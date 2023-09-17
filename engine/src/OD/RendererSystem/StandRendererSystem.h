#pragma once

#include <vector>
#include "OD/Scene/Scene.h"
#include "OD/Renderer/Shader.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Renderer/Framebuffer.h"

namespace OD{

struct SpriteComponent;
struct MeshRendererComponent;
struct LightComponent;

struct SceneLightSettings{
    Vector3 ambient = {0.1f, 0.1f, 0.1f};
};

struct StandRendererSystem: public OD::System{
    SceneLightSettings sceneLightSettings;

    SystemType Type() override { return SystemType::Renderer; }
    
    void Update() override;
    StandRendererSystem();

    inline void SetOutFrameBuffer(Framebuffer* out){ _outFramebuffer = out; }
    inline void overrideCamera(Camera* cam){ _overrideCamera = cam; } 

private:
    Ref<Shader> _spriteShader;
    Mesh _spriteMesh;

    Framebuffer* _shadowMap;
    Ref<Shader> _shadowMapShader;
    Ref<Shader> _postProcessingShader;

    Matrix4 _lightSpaceMatrix;

    Framebuffer* _outFramebuffer;

    Camera* _overrideCamera = nullptr;

    void SetStandUniforms(Shader& material);
    void RenderScene(Camera& camera);
    void RenderSceneShadow(Matrix4 lightSpaceMatrix);
    void UpdateCurrentLight();
};

};