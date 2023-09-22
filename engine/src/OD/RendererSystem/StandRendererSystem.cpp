#include "OD/Renderer/Renderer.h"
#include "StandRendererSystem.h"
#include "SpriteComponent.h"
#include "MeshRendererComponent.h"
#include "EnvironmentComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "OD/PhysicsSystem/PhysicsSystem.h"
#include <unordered_map>
#include <map>

namespace OD{

class TestPP1: public PostProcessingPass{
public:
    TestPP1(int option):_option(option){
        _ppShader = Shader::CreateFromFile("res/Builtins/Shaders/BasicPostProcessing.glsl");
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        _ppShader->Bind();
        _ppShader->SetFloat("option", _option);
        Renderer::Blit(src, dst, *_ppShader);
    }

private:
    int _option;
    Ref<Shader> _ppShader;
};

struct RenderGroupTarget{
    Ref<Mesh> mesh;
    Matrix4 trans;
};

EnvironmentSettings environmentSettings;

void StandRendererSystem::SetStandUniforms(Shader& shader){
    shader.Bind();

    shader.SetVector3("ambientLight", environmentSettings.ambient);
    shader.SetFramebuffer("shadowMap", *_shadowMap, 1, -1);
    shader.SetMatrix4("lightSpaceMatrix", _lightSpaceMatrix);

    std::vector<EntityId> pointLights;
    bool hasDirectionalLight = false;

    auto view = scene()->GetRegistry().view<LightComponent, TransformComponent>();
    for(auto e: view){
        auto& light = view.get<LightComponent>(e);
        auto& transform = view.get<TransformComponent>(e);

        if(light.type == LightComponent::Type::Directional){
            shader.SetVector3("directionalLightColor", light.color * light.intensity);
            shader.SetVector3("directionalLightDir", -transform.forward());
            hasDirectionalLight = true;
        }

        if(light.type == LightComponent::Type::Point){
            pointLights.push_back(e);
        }
    }

    if(hasDirectionalLight == false){
        shader.SetVector3("directionalLightDir", Vector3::zero);
        shader.SetVector3("directionalLightColor", Vector3::zero);
    }

    const int maxPointLight = 4;
    char buff[200];

    for(int i = 0; i < maxPointLight; i++){
        if(i < pointLights.size()){
            LightComponent& l = scene()->GetRegistry().get<LightComponent>(pointLights[i]);
            TransformComponent& t = scene()->GetRegistry().get<TransformComponent>(pointLights[i]);

            sprintf(buff, "pointLights[%d].position", i);
            shader.SetVector3(buff, t.position());

            sprintf(buff, "pointLights[%d].color", i);
            shader.SetVector3(buff, l.color * l.intensity);

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

void StandRendererSystem::RenderSceneShadow(LightComponent& light, TransformComponent& transform){
    OD_PROFILE_SCOPE("StandRendererSystem::RenderSceneShadow");

    float near_plane = 0.05f, far_plane = 1000;
    float lightBoxHalfExtend = 50;
    Matrix4 lightProjection = Matrix4::Ortho(-lightBoxHalfExtend, lightBoxHalfExtend, -lightBoxHalfExtend, lightBoxHalfExtend, near_plane, far_plane);
    
    Vector3 lightCenter = transform.position();
    Vector3 lightEye = lightCenter + (-(transform.forward() * lightBoxHalfExtend));
    Matrix4 lightView = Matrix4::LookAt(lightEye, lightCenter, Vector3(0.0f, 1.0f,  0.0f));  
    //lightView = directionalLight->entity->transform().globalModelMatrix().inverse();
    _lightSpaceMatrix = lightProjection * lightView; 

    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(CullFace::FRONT);
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);

    auto view = scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view){
        auto& c = view.get<MeshRendererComponent>(_entity);
        auto& t = view.get<TransformComponent>(_entity);

        for(Ref<Mesh> m: c.model()->meshs){
            _shadowMapShader->Bind();
            _shadowMapShader->SetMatrix4("lightSpaceMatrix", _lightSpaceMatrix);
            Renderer::DrawMesh(*m, t.globalModelMatrix(), *_shadowMapShader);
        }
    }
}

void StandRendererSystem::ClearSceneShadow(){
    OD_PROFILE_SCOPE("StandRendererSystem::ClearSceneShadow");
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);
}

void StandRendererSystem::RenderScene(Camera& camera, bool isMain, Vector3 camPos){
    OD_PROFILE_SCOPE("StandRendererSystem::RenderScene");

    if(isMain){
        Renderer::SetCamera(_overrideCamera != nullptr ? *_overrideCamera : camera);
    } else {
        Renderer::SetCamera(camera);
    }
    
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(CullFace::BACK);
    Renderer::SetBlend(false);
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);

    std::unordered_map< Ref<Material>, std::vector<RenderGroupTarget> > groups; 
    std::map<float, std::unordered_map<Ref<Material>,std::vector<RenderGroupTarget>> >groupsBlend; 

    auto view1 = scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view1){
        auto& c = view1.get<MeshRendererComponent>(_entity);
        auto& t = view1.get<TransformComponent>(_entity);

        int index = 0;
        for(Ref<Material> i: c.model()->materials){
            Ref<Material> targetMaterial = i;
            if(c.materialsOverride()[index] != nullptr) targetMaterial = c.materialsOverride()[index];

            if(targetMaterial->isBlend){
                float distance = Vector3::Distance(camPos, t.position());
                groupsBlend[distance][targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix()});
            } else {
                groups[targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix()});
            }
                
            index += 1;
        }
        //Renderer::DrawModel(*c.model(), t.globalModelMatrix(), c.subMeshIndex(), &c.materialsOverride());
    }

    for(auto i: groups){
        SetStandUniforms(*i.first->shader);
        i.first->UpdateUniforms();
        for(auto j: i.second){
            Renderer::DrawMesh(*j.mesh, j.trans, *i.first->shader);
        }
    }
    
    Renderer::SetCullFace(CullFace::NONE);
    Renderer::SetBlend(true);
    Renderer::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);

    for(auto it = groupsBlend.rbegin(); it != groupsBlend.rend(); it++){
    for(auto i: it->second){
        SetStandUniforms(*i.first->shader);
        i.first->UpdateUniforms();
        for(auto j: i.second){
            Renderer::DrawMesh(*j.mesh, j.trans, *i.first->shader);
        }
    }
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
    _spriteShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/Sprite.glsl");

    FrameBufferSpecification specification;
    specification.width = 1024 * 4;
    specification.height = 1024 * 4;
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT, false};
    _shadowMap = new Framebuffer(specification);

    _shadowMapShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/ShadowMap.glsl");
    _postProcessingShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/BasicPostProcessing.glsl");
    _blitShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/Blit.glsl");

    FrameBufferSpecification framebufferSpecification = {Application::screenWidth(), Application::screenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGB}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8, true};
    
    _finalColor = new Framebuffer(framebufferSpecification);
    _finalColor->Invalidate();

    _pp1 = new Framebuffer(framebufferSpecification);
    _pp1->Invalidate();

    _pp2 = new Framebuffer(framebufferSpecification);
    _pp2->Invalidate();

    _ppPass.push_back(new TestPP1(0));
    _ppPass.push_back(new TestPP1(3));
}

StandRendererSystem::~StandRendererSystem(){
    for(auto i: _ppPass){
        delete i;
    }

    delete _shadowMap;
    delete _finalColor;
    delete _pp1;
    delete _pp2;
}

void StandRendererSystem::Update(){
    OD_PROFILE_SCOPE("StandRendererSystem::Update");

    // ---------- Setups ---------- 
    environmentSettings = EnvironmentSettings();
    auto _view = scene()->GetRegistry().view<EnvironmentComponent>();
    for(auto entity: _view){
        environmentSettings = _view.get<EnvironmentComponent>(entity).settings;
        break;
    }

    // ---------- Render Shadows ---------- 
    UpdateCurrentLight();

    bool hasShadow = false;
    auto view = scene()->GetRegistry().view<LightComponent, TransformComponent>();
    for(auto entity: view){
        auto& light = view.get<LightComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        if(light.type != LightComponent::Type::Directional) continue;
        if(light.renderShadow == false) continue;

        _shadowMap->Bind();
        Renderer::SetViewport(0, 0, _shadowMap->width(), _shadowMap->height());
        RenderSceneShadow(light, transform);
        _shadowMap->Unbind();

        hasShadow = true;
        break;
    }
    if(hasShadow == false){
        _shadowMap->Bind();
        ClearSceneShadow();
        _shadowMap->Unbind();
    }
    
    // ---------- Render Cameras ---------- 

    int width = Application::screenWidth();
    int height = Application::screenHeight();

    if(_outFramebuffer != nullptr){
        _outFramebuffer->Bind();
        width = _outFramebuffer->width();
        height = _outFramebuffer->height();
    }

    _finalColor->Resize(width, height);
    _pp1->Resize(width, height);
    _pp2->Resize(width, height);
    
    _finalColor->Bind();

    Renderer::SetViewport(0, 0, width, height);
    auto view2 = scene()->GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto entity: view2){
        auto& c = view2.get<CameraComponent>(entity);
        auto& t = view2.get<TransformComponent>(entity);

        c.UpdateCameraData(t, width, height);
        if(c.isMain){
            RenderScene(c.camera(), true, _overrideCamera != nullptr ? _overrideCameraTrans.localPosition() : t.position());
            break;
        }
    }
    scene()->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    bool step = false;
    #define SRC_FRAMEBUFFER() step == false ? _finalColor : _pp1

    for(auto i: _ppPass){
        i->OnRenderImage(
            step == false ? _finalColor : _pp1, 
            step == false ? _pp1 : _finalColor
        );
        step = !step;
    }

    /*if(_outFramebuffer != nullptr){
        _postProcessingShader->Bind();
        
        _postProcessingShader->SetFloat("option", 0);
        Renderer::Blit(_finalColor, _pp1, *_postProcessingShader);

        _postProcessingShader->SetFloat("option", 3);
        Renderer::Blit(_pp1, _finalColor, *_postProcessingShader);

        _postProcessingShader->SetFloat("option", 5);
        Renderer::Blit(_finalColor, _outFramebuffer, *_postProcessingShader);
    }*/

    if(_outFramebuffer != nullptr){
        Renderer::Blit( _finalColor, _outFramebuffer, *_blitShader);
    } else {
        Renderer::Blit( _finalColor, nullptr, *_blitShader);
    }

    _finalColor->Unbind();
    _pp1->Unbind();
    _pp2->Unbind();
    if(_outFramebuffer != nullptr) _outFramebuffer->Unbind();
}

}