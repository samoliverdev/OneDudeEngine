#include "OD/Renderer/Renderer.h"
#include "StandRendererSystem.h"
#include "SpriteComponent.h"
#include "MeshRendererComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"

namespace OD{

void StandRendererSystem::SetStandUniforms(Shader& shader){
    shader.Bind();

    shader.SetVector3("ambientLight", sceneLightSettings.ambient);
    shader.SetFramebuffer("shadowMap", *_shadowMap, 1, -1);
    shader.SetMatrix4("lightSpaceMatrix", _lightSpaceMatrix);

    shader.SetVector3("directionalLightDir", Vector3::zero);
    shader.SetVector3("directionalLightColor", Vector3::zero);

    std::vector<EntityId> pointLights;

    auto view = scene()->GetRegistry().view<LightComponent, TransformComponent>();
    for(auto e: view){
        auto& light = view.get<LightComponent>(e);
        auto& transform = view.get<TransformComponent>(e);

        if(light.type == LightComponent::Type::Directional){
            shader.SetVector3("directionalLightColor", light.color);
            shader.SetVector3("directionalLightDir", -transform.forward());
        }

        if(light.type == LightComponent::Type::Point){
            pointLights.push_back(e);
        }
    }

    const int maxPointLight = 4;
    char buff[200];

    for(int i = 0; i < maxPointLight; i++){
        if(i < pointLights.size()){
            sprintf(buff, "pointLights[%d].position", i);

            LightComponent& l = scene()->GetRegistry().get<LightComponent>(pointLights[i]);
            TransformComponent& t = scene()->GetRegistry().get<TransformComponent>(pointLights[i]);

            shader.SetVector3(buff, t.position());

            sprintf(buff, "pointLights[%d].color", i);
            shader.SetVector3(buff, l.color);

            sprintf(buff, "pointLights[%d].radius", i);
            shader.SetFloat(buff, l.radius);
        } else {
            sprintf(buff, "pointLights[%d].position", i);
            shader.SetVector3(buff, Vector3::zero);

            sprintf(buff, "pointLights[%d].color", i);
            shader.SetVector3(buff, Vector3::zero);

            sprintf(buff, "pointLights[%d].radius", i);
            shader.SetFloat(buff, 5);
        }
    }
}

void StandRendererSystem::UpdateCurrentLight(){}

void StandRendererSystem::RenderSceneShadow(Matrix4 lightSpaceMatrix){
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(CullFace::FRONT);
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);

    auto view = scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view){
        auto& c = view.get<MeshRendererComponent>(_entity);
        auto& t = view.get<TransformComponent>(_entity);

        for(Ref<Mesh> m: c.mesh->meshs){
            _shadowMapShader->Bind();
            _shadowMapShader->SetMatrix4("lightSpaceMatrix", lightSpaceMatrix);
            Renderer::DrawMesh(*m, t.globalModelMatrix(), *_shadowMapShader);
        }
    }
}

void StandRendererSystem::RenderScene(Camera& camera){
    Renderer::SetCamera(camera);
    
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(CullFace::BACK);
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);

    auto view1 = scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view1){
        auto& c = view1.get<MeshRendererComponent>(_entity);
        auto& t = view1.get<TransformComponent>(_entity);
    
        for(Ref<Material> i: c.mesh->materials){
            SetStandUniforms(*i->shader);
        }
        Renderer::DrawModel(*c.mesh, t.globalModelMatrix(), c.subMeshIndex, c.materialOverride);
    }

    auto view2 = scene()->GetRegistry().view<SpriteComponent, TransformComponent>();
    for(auto _entity: view2){
        auto& c = view2.get<SpriteComponent>(_entity);
        auto& t = view2.get<TransformComponent>(_entity);

        if(c.texture->IsValid() == false) continue;
        
        _spriteShader->Bind();
        _spriteShader->SetTexture2D("texture", *c.texture, 0); 
        Renderer::DrawMesh(_spriteMesh, t.globalModelMatrix(), *_spriteShader);
    }
}

StandRendererSystem::StandRendererSystem(){
    //glEnable(GL_FRAMEBUFFER_SRGB); 

    _spriteMesh = Mesh::CenterQuad(true);
    _spriteShader = Shader::CreateFromFile("res/Builtins/Shaders/Sprite.glsl");
    //spriteShader = scene->GetAssetManager().LoadShaderFromFiles("sprite", "res/shaders/sprite.vert", "res/shaders/sprite.frag");

    FrameBufferSpecification specification;
    specification.width = 1024 * 8;
    specification.height = 1024 * 8;
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT, false};
    _shadowMap = new Framebuffer(specification);

    _shadowMapShader = Shader::CreateFromFile("res/Builtins/Shaders/ShadowMap.glsl");
    _postProcessingShader = Shader::CreateFromFile("res/Builtins/Shaders/BasicPostProcessing.glsl");
}

void StandRendererSystem::Update(){
    //Assert(CameraComponent::mainCamera != nullptr);

    UpdateCurrentLight();

    auto view = scene()->GetRegistry().view<LightComponent, TransformComponent>();
    for(auto entity: view){
        auto& light = view.get<LightComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        if(light.type != LightComponent::Type::Directional) continue;

        float near_plane = 0.05f, far_plane = 1000;
        float lightBoxHalfExtend = 50;
        Matrix4 lightProjection = Matrix4::Ortho(-lightBoxHalfExtend, lightBoxHalfExtend, -lightBoxHalfExtend, lightBoxHalfExtend, near_plane, far_plane);
        
        Vector3 lightCenter = transform.position();
        Vector3 lightEye = lightCenter + (-(transform.forward() * lightBoxHalfExtend));
        Matrix4 lightView = Matrix4::LookAt(lightEye, lightCenter, Vector3(0.0f, 1.0f,  0.0f));  
        //lightView = directionalLight->entity->transform().globalModelMatrix().inverse();
        _lightSpaceMatrix = lightProjection * lightView; 
    }
    
    _shadowMap->Bind();
    Renderer::SetViewport(0, 0, _shadowMap->width(), _shadowMap->height());
    RenderSceneShadow(_lightSpaceMatrix);

    _shadowMap->Unbind();
    Renderer::SetViewport(0, 0, Application::screenWidth(), Application::screenHeight());
    
    Camera cam;

    auto view2 = scene()->GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto entity: view2){
        auto& c = view2.get<CameraComponent>(entity);
        auto& t = view2.get<TransformComponent>(entity);

        c.UpdateCameraData(t);
        cam = c.camera();
    }

    RenderScene(cam);

    //Renderer::Blit(shadowMap, nullptr, *postProcessingShader, -1);
    
    ///*
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //Renderer::Clean(1,1,1,1);
    //Renderer::SetDepthTest(DepthTest::DISABLE); 
    //shadowMapShader->Bind();
    //shadowMapShader->SetFramebuffer("mainTex", *shadowMap, 0, -1);
    //Renderer::DrawMeshRaw(fullscreenQuad);
    //*/
}

}