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
        Renderer::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    int _option;
    Ref<Shader> _ppShader;
};

class GameCorrectionPP: public PostProcessingPass{
public:
    GameCorrectionPP(){
        _ppShader = Shader::CreateFromFile("res/Builtins/Shaders/GamaCorrectionPP.glsl");
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        _ppShader->Bind();
        Renderer::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    Ref<Shader> _ppShader;
};

class ToneMappingPP: public PostProcessingPass{
public:
    ToneMappingPP(){
        _ppShader = Shader::CreateFromFile("res/Builtins/Shaders/ToneMappingPP.glsl");
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        _ppShader->Bind();
        Renderer::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    Ref<Shader> _ppShader;
};

struct RenderGroupTarget{
    Ref<Mesh> mesh;
    Matrix4 trans;
};

struct InstancingKey{
    Ref<Material> material;
    Ref<Mesh> mesh;

    bool operator==(const InstancingKey& p) const{
        return material->shader()->rendererId() == p.material->shader()->rendererId() && 
                mesh->rendererId() == p.mesh->rendererId();
    }
};

struct InstancingKeyHasher{
    std::size_t operator()(const InstancingKey& k) const{
        return k.material->shader()->rendererId() + k.mesh->rendererId();
    }
};

EnvironmentSettings environmentSettings;

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
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGB16F}, {FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8, true};
    framebufferSpecification.sample = 8;
    _finalColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::None};
    framebufferSpecification.sample = 1;
    _objectsId = new Framebuffer(framebufferSpecification);

    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGB16F}};
    framebufferSpecification.sample = 1;
    _finalColor2 = new Framebuffer(framebufferSpecification);
    _pp1 = new Framebuffer(framebufferSpecification);
    _pp2 = new Framebuffer(framebufferSpecification);

    _skyboxShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkyboxCubemap.glsl");
    _skyboxMesh = Mesh::SkyboxCube();
    _skyboxCubemap = Cubemap::CreateFromFile(
        "res/Builtins/Textures/Skybox/right.jpg",
        "res/Builtins/Textures/Skybox/left.jpg",
        "res/Builtins/Textures/Skybox/top.jpg",
        "res/Builtins/Textures/Skybox/bottom.jpg",
        "res/Builtins/Textures/Skybox/front.jpg",
        "res/Builtins/Textures/Skybox/back.jpg"
    );

    //_ppPass.push_back(new TestPP1(0));
    //_ppPass.push_back(new TestPP1(3));

    //_ppPass.push_back(new GameCorrectionPP());
    _ppPass.push_back(new ToneMappingPP());
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
        //_outFramebuffer->Bind();
        width = _outFramebuffer->width();
        height = _outFramebuffer->height();
    }

    _finalColor->Resize(width, height);
    _pp1->Resize(width, height);
    _pp2->Resize(width, height);
    ////_finalColor2->Resize(width, height);
    
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

    //LogInfo("ReadPixel(1): %d",_finalColor->ReadPixel(1, 50, 50));

    Renderer::BlitFramebuffer(_finalColor, _pp1);
    Renderer::BlitFramebuffer(_finalColor, _objectsId, 1);
    //Renderer::BlitQuadPostProcessing(_finalColor, _pp1, *_blitShader);

    bool step = false;
    Framebuffer* finalFramebuffer = _pp1;

    for(auto i: _ppPass){
        finalFramebuffer = step == false ? _pp2 : _pp1;
        i->OnRenderImage(
            step == false ? _pp1 : _pp2, 
            step == false ? _pp2 : _pp1
        );
        step = !step;
    }

    if(_outFramebuffer != nullptr){
        Renderer::BlitQuadPostProcessing(finalFramebuffer, _outFramebuffer, *_blitShader);
        //Renderer::BlitQuadPostProcessing(finalFramebuffer, _finalColor2, *_blitShader);
    } else {
        Renderer::BlitQuadPostProcessing(finalFramebuffer, nullptr, *_blitShader);
    }

    _finalColor->Unbind();
    //_finalColor2->Unbind();
    _pp1->Unbind();
    _pp2->Unbind();
    if(_outFramebuffer != nullptr) _outFramebuffer->Unbind();
}

/////////////////////////////////////////////////

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

    // --------- Set Camera --------- 
    if(isMain){
        Renderer::SetCamera(_overrideCamera != nullptr ? *_overrideCamera : camera);
    } else {
        Renderer::SetCamera(camera);
    }

    // --------- Clean --------- 
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);

    // --------- Render Sky --------- 
    //Renderer::SetDepthTest(DepthTest::ALWAYS);
    Renderer::SetCullFace(CullFace::BACK);
    Renderer::SetDepthMask(false);
    _skyboxShader->Bind();
    _skyboxShader->SetCubemap("mainTex", *_skyboxCubemap, 0);
    _skyboxShader->SetMatrix4("projection", Renderer::GetCamera().projection);
    Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(Renderer::GetCamera().view)));
    _skyboxShader->SetMatrix4("view", skyboxView);
    Renderer::DrawMeshRaw(_skyboxMesh);
    Renderer::SetDepthMask(true);
    
    // --------- Render Opaques --------- 
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(CullFace::BACK);

    std::unordered_map< Ref<Material>, std::vector<RenderGroupTarget> > groups; 
    std::map<float, std::unordered_map<Ref<Material>,std::vector<RenderGroupTarget>> >groupsBlend; 

    std::unordered_map< InstancingKey, std::vector<Matrix4>, InstancingKeyHasher > groupsInstancing; 

    auto view1 = scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view1){
        auto& c = view1.get<MeshRendererComponent>(_entity);
        auto& t = view1.get<TransformComponent>(_entity);

        int index = 0;
        for(Ref<Material> i: c.model()->materials){
            Ref<Material> targetMaterial = i;
            if(c.materialsOverride()[index] != nullptr) targetMaterial = c.materialsOverride()[index];

            if(targetMaterial->isBlend()){
                float distance = Vector3::Distance(camPos, t.position());
                groupsBlend[distance][targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix()});
            } else {
                groups[targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix()});
                groupsInstancing[{targetMaterial, c.model()->meshs[index]}].push_back(t.globalModelMatrix());
            }
                
            index += 1;
        }
        //Renderer::DrawModel(*c.model(), t.globalModelMatrix(), c.subMeshIndex(), &c.materialsOverride());
    }

    bool instancing = false;

    if(instancing){
        for(auto& i: groupsInstancing){
            SetStandUniforms(*i.first.material->shader());
            i.first.material->UpdateUniforms();

            i.first.mesh->instancingModelMatrixs.clear();
            for(auto j: i.second){
                i.first.mesh->instancingModelMatrixs.push_back(j);
            }
            i.first.mesh->UpdateMeshInstancingModelMatrixs();

            Renderer::DrawMeshInstancing(*i.first.mesh, *i.first.material->shader(), i.second.size());
            //LogInfo("Instancing count: %zd ShaderPath: %s", i.second.size(), i.first.material->shader->path().c_str());
        }
        //LogInfo("GroupsInstancing count: %zd", groupsInstancing.size());
    } else {
        for(auto i: groups){
            SetStandUniforms(*i.first->shader());
            i.first->UpdateUniforms();
            for(auto j: i.second){
                Renderer::DrawMesh(*j.mesh, j.trans, *i.first->shader());
            }
        }
    }
    
    // --------- Render Blends --------- 
    Renderer::SetCullFace(CullFace::NONE);
    Renderer::SetBlend(true);
    Renderer::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);

    for(auto it = groupsBlend.rbegin(); it != groupsBlend.rend(); it++){
    for(auto i: it->second){
        SetStandUniforms(*i.first->shader());
        i.first->UpdateUniforms();
        for(auto j: i.second){
            Renderer::DrawMesh(*j.mesh, j.trans, *i.first->shader());
        }
    }
    }

    Renderer::SetBlend(false);

    // --------- Render Sprites --------- 
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
}