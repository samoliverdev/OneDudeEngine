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
        enable = false;
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        _ppShader->Bind();
        Renderer::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    Ref<Shader> _ppShader;
};

struct RenderTarget{
    Ref<Mesh> mesh;
    Matrix4 trans;
};

struct RenderTargetInstancing{
    Ref<Material> material;
    Ref<Mesh> mesh;

    bool operator==(const RenderTargetInstancing& p) const{
        return material->shader()->rendererId() == p.material->shader()->rendererId() && 
                mesh->rendererId() == p.mesh->rendererId();
    }
};

struct InstancingKeyHasher{
    std::size_t operator()(const RenderTargetInstancing& k) const{
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

    directinallightShadowPass._shadowMap = new Framebuffer(specification);
    for(int i = 0; i < MAX_SPOTLIGHT_SHADOWS; i++){
        spotlightShadowPass[i]._shadowMap = new Framebuffer(specification);
    }

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

    _skyboxMesh = Mesh::SkyboxCube();

    //_ppPass.push_back(new GameCorrectionPP());
    _ppPass.push_back(new ToneMappingPP());
}

StandRendererSystem::~StandRendererSystem(){
    for(auto i: _ppPass){
        delete i;
    }

    delete directinallightShadowPass._shadowMap;
    for(int i = 0; i < MAX_SPOTLIGHT_SHADOWS; i++){
        delete spotlightShadowPass[i]._shadowMap;
    }

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
        EnvironmentComponent& environmentComponent = _view.get<EnvironmentComponent>(entity);
        if(environmentComponent._inited == false) environmentComponent.Init();

        /*if(environmentComponent.settings.msaaQuality != environmentSettings.msaaQuality && environmentComponent.settings.antiAliasing != environmentSettings.antiAliasing){
            FrameBufferSpecification fs1 = _finalColor->specification();
            fs1.sample = 1;
            if(environmentComponent.settings.antiAliasing == AntiAliasing::MSAA){
                fs1.sample = MSAAQualityLookup[(int)environmentComponent.settings.msaaQuality];
            }
            _finalColor->Reload(fs1);
        }*/

        if(environmentComponent.settings.shadowQuality != environmentSettings.shadowQuality){
            int size = ShadowQualityLookup[(int)environmentComponent.settings.shadowQuality];
            directinallightShadowPass._shadowMap->Resize(size, size);
            for(int i = 0; i < MAX_SPOTLIGHT_SHADOWS; i++){
                spotlightShadowPass[i]._shadowMap->Resize(size, size);
            }
        }

        if(environmentComponent.settings.colorCorrection != environmentSettings.colorCorrection){
            if(environmentComponent.settings.colorCorrection == ColorCorrection::ColorCorrection){
                _ppPass[_ppPass.size()-1]->enable = true;
            } else {
                _ppPass[_ppPass.size()-1]->enable = false;
            }
        }

        environmentSettings = environmentComponent.settings;

        break;
    }

    // ---------- Render Shadows ---------- 

    spotlightShadowPassCount = 0;
    bool hasShadow = false;
    auto view = scene()->GetRegistry().view<LightComponent, TransformComponent>();
    for(auto entity: view){
        auto& light = view.get<LightComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        if(light.type == LightComponent::Type::Directional && light.renderShadow == true){
            //RenderSceneShadow(light, transform);
            directinallightShadowPass.Render(light, transform, *this);
            hasShadow = true;
        }

        if(light.type == LightComponent::Type::Spot && light.renderShadow == true && spotlightShadowPassCount < MAX_SPOTLIGHT_SHADOWS){
            spotlightShadowPass[spotlightShadowPassCount].Render(light, transform, *this);
            spotlightShadowPassCount += 1;
        }
    }
    if(hasShadow == false){
        directinallightShadowPass.Clean(*this);
        /*_shadowMap->Bind();
        ClearSceneShadow();
        _shadowMap->Unbind();*/
    }
    
    // ---------- Render Cameras ---------- 

    int width = Application::screenWidth();
    int height = Application::screenHeight();

    if(_outFramebuffer != nullptr){
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
            Camera cam = c.camera();
            RenderScene(cam, true, _overrideCamera != nullptr ? _overrideCameraTrans.localPosition() : t.position());
            break;
        }
    }
    scene()->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    //LogInfo("ReadPixel(1): %d", _finalColor->ReadPixel(1, 50, 50));

    Renderer::BlitFramebuffer(_finalColor, _pp1);
    Renderer::BlitFramebuffer(_finalColor, _objectsId, 1);
    //Renderer::BlitQuadPostProcessing(_finalColor, _pp1, *_blitShader);

    bool step = false;
    Framebuffer* finalFramebuffer = _pp1;

    for(auto i: _ppPass){
        finalFramebuffer = step == false ? _pp2 : _pp1;

        if(i->enable){
            i->OnRenderImage(
                step == false ? _pp1 : _pp2, 
                step == false ? _pp2 : _pp1
            );
        } else {
            Renderer::BlitFramebuffer(
                step == false ? _pp1 : _pp2,
                step == false ? _pp2 : _pp1
            );
        }

        step = !step;
    }

    if(_outFramebuffer != nullptr){
        Renderer::BlitQuadPostProcessing(finalFramebuffer, _outFramebuffer, *_blitShader);
    } else {
        Renderer::BlitQuadPostProcessing(finalFramebuffer, nullptr, *_blitShader);
    }

    _finalColor->Unbind();
    _pp1->Unbind();
    _pp2->Unbind();
    if(_outFramebuffer != nullptr) _outFramebuffer->Unbind();
}

/////////////////////////////////////////////////

void StandRendererSystem::SetStandUniforms(Vector3 viewPos, Shader& shader){
    int baseShadowMapIndex = 2;

    shader.Bind();

    shader.SetVector3("viewPos", viewPos);

    shader.SetVector3("ambientLight", environmentSettings.ambient);

    shader.SetFloat("shadowBias", environmentSettings.shadowBias);
    
    shader.SetFramebuffer("shadowMap", *directinallightShadowPass._shadowMap, baseShadowMapIndex, -1);
    shader.SetMatrix4("lightSpaceMatrix", directinallightShadowPass._lightSpaceMatrix);

    shader.SetInt("spotlightShadowCount", spotlightShadowPassCount);
    for(int i = 0; i < spotlightShadowPassCount; i++){
        std::string setMat = "spotlightSpaceMatrixs[" + std::to_string(i) + "]";
        shader.SetMatrix4(setMat.c_str(), spotlightShadowPass[i]._lightSpaceMatrix);

        std::string setFram = "spotlightShadowMaps[" + std::to_string(i) + "]";
        shader.SetFramebuffer(setFram.c_str(), *spotlightShadowPass[i]._shadowMap, baseShadowMapIndex+(i+1), -1);
    }

    std::vector<EntityId> lights;
    bool hasDirectionalLight = false;

    const int maxLights = 12;

    auto view = scene()->GetRegistry().view<LightComponent, TransformComponent>();
    for(auto e: view){
        auto& light = view.get<LightComponent>(e);
        auto& transform = view.get<TransformComponent>(e);

        if(light.type == LightComponent::Type::Directional){
            shader.SetVector3("directionalLightColor", light.color * light.intensity);
            shader.SetVector3("directionalLightDir", -transform.forward());
            shader.SetFloat("directionalLightspecular", light.specular);
            hasDirectionalLight = true;
        }

        if(lights.size() > maxLights) continue;

        if(light.type == LightComponent::Type::Point || light.type == LightComponent::Type::Spot){
            lights.push_back(e);
        }
    }

    if(hasDirectionalLight == false){
        shader.SetVector3("directionalLightDir", Vector3::zero);
        shader.SetVector3("directionalLightColor", Vector3::zero);
    }

    shader.SetInt("lightsCount", lights.size());

    char buff[200];
    for(int i = 0; i < lights.size(); i++){
        LightComponent& l = scene()->GetRegistry().get<LightComponent>(lights[i]);
        TransformComponent& t = scene()->GetRegistry().get<TransformComponent>(lights[i]);

        sprintf(buff, "lights[%d].type", i);
        shader.SetFloat(buff, (int)l.type);

        sprintf(buff, "lights[%d].pos", i);
        shader.SetVector3(buff, t.position());

        sprintf(buff, "lights[%d].dir", i);
        shader.SetVector3(buff, t.forward());

        sprintf(buff, "lights[%d].color", i);
        shader.SetVector3(buff, l.color * l.intensity);

        sprintf(buff, "lights[%d].specular", i);
        shader.SetFloat(buff, l.specular);

        sprintf(buff, "lights[%d].falloff", i);
        shader.SetFloat(buff, l.falloff);

        if(l.type == LightComponent::Type::Point){
            sprintf(buff, "lights[%d].radius", i);
            shader.SetFloat(buff, l.radius);
        }

        if(l.type == LightComponent::Type::Spot){
            sprintf(buff, "lights[%d].radius", i);
            shader.SetFloat(buff, l.radius);

            if(l.coneAngleInner > l.coneAngleOuter) l.coneAngleInner = l.coneAngleOuter;

            sprintf(buff, "lights[%d].coneAngleOuter", i);
            shader.SetFloat(buff, Mathf::Cos(Mathf::Deg2Rad(l.coneAngleOuter)));

            sprintf(buff, "lights[%d].coneAngleInner", i);
            shader.SetFloat(buff, Mathf::Cos(Mathf::Deg2Rad(l.coneAngleInner)));
        }
    }
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
    Renderer::Clean(environmentSettings.cleanColor.x, environmentSettings.cleanColor.y, environmentSettings.cleanColor.z, 1);

    #pragma region RenderSkyPass
    // --------- Render Sky --------- 
    if(environmentSettings.sky != nullptr){
        Assert(environmentSettings.sky->shader() != nullptr);

        environmentSettings.sky->UpdateUniforms();

        Renderer::SetCullFace(CullFace::BACK);
        Renderer::SetDepthMask(false);
        environmentSettings.sky->shader()->Bind();
        //environmentSettings.sky->shader()->SetCubemap("mainTex", *_skyboxCubemap, 0);
        environmentSettings.sky->shader()->SetMatrix4("projection", Renderer::GetCamera().projection);
        Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(Renderer::GetCamera().view)));
        environmentSettings.sky->shader()->SetMatrix4("view", skyboxView);
        Renderer::DrawMeshRaw(_skyboxMesh);
        Renderer::SetDepthMask(true);
    }
    #pragma endregion 

    // --------- Render Targets --------- 
    std::unordered_map< Ref<Material>, std::vector<RenderTarget> > renderTargetsOpaques; 
    std::unordered_map< RenderTargetInstancing, std::vector<Matrix4>, InstancingKeyHasher > renderTargetOpaquesInstancing; 
    std::map<float, std::unordered_map<Ref<Material>,std::vector<RenderTarget>> >renderTargetsBlend; 
    
    // --------- Setup Render Opaques --------- 
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(CullFace::BACK);

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
                renderTargetsBlend[distance][targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix()});
            } else {
                if(targetMaterial->enableInstancing() && targetMaterial->supportInstancing()){
                    renderTargetOpaquesInstancing[{targetMaterial, c.model()->meshs[index]}].push_back(t.globalModelMatrix());
                } else {
                    renderTargetsOpaques[targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix()});
                }

                //renderTargets[targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix()});
                //renderTargetInstancing[{targetMaterial, c.model()->meshs[index]}].push_back(t.globalModelMatrix());
            }
                
            index += 1;
        }
        //Renderer::DrawModel(*c.model(), t.globalModelMatrix(), c.subMeshIndex(), &c.materialsOverride());
    }

    // --------- Render Opaques Instancing --------- 
    for(auto& i: renderTargetOpaquesInstancing){
        SetStandUniforms(camPos, *i.first.material->shader());
        i.first.material->UpdateUniforms();

        i.first.mesh->instancingModelMatrixs.clear();
        for(auto j: i.second){
            i.first.mesh->instancingModelMatrixs.push_back(j);
        }
        i.first.mesh->UpdateMeshInstancingModelMatrixs();

        Renderer::DrawMeshInstancing(*i.first.mesh, *i.first.material->shader(), i.second.size());
        //LogInfo("Instancing count: %zd ShaderPath: %s", i.second.size(), i.first.material->shader->path().c_str());
    }
    //LogInfo("renderTargetOpaquesInstancing count: %zd", renderTargetOpaquesInstancing.size());

    // --------- Render Opaques --------- 
    for(auto i: renderTargetsOpaques){
        SetStandUniforms(camPos, *i.first->shader());
        i.first->UpdateUniforms();
        for(auto j: i.second){
            Renderer::DrawMesh(*j.mesh, j.trans, *i.first->shader());
        }
    }
    //LogInfo("renderTargetsOpaques count: %zd", renderTargetsOpaques.size());
    
    // --------- Render Blends --------- 
    Renderer::SetCullFace(CullFace::NONE);
    Renderer::SetBlend(true);
    Renderer::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);

    for(auto it = renderTargetsBlend.rbegin(); it != renderTargetsBlend.rend(); it++){
        for(auto i: it->second){
            SetStandUniforms(camPos, *i.first->shader());
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

void StandRendererSystem::ShadowRenderPass::Clean(StandRendererSystem& root){
    OD_PROFILE_SCOPE("StandRendererSystem::ShadowRenderPass::Clean");
    _shadowMap->Bind();
    Renderer::SetViewport(0, 0, _shadowMap->width(), _shadowMap->height());
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);
    _shadowMap->Unbind();
}

void StandRendererSystem::ShadowRenderPass::Render(LightComponent& light, TransformComponent& transform, StandRendererSystem& root){
    OD_PROFILE_SCOPE("StandRendererSystem::ShadowRenderPass::Render");

    _shadowMap->Bind();
    Renderer::SetViewport(0, 0, _shadowMap->width(), _shadowMap->height());

    Matrix4 lightProjection = Matrix4::identity;
    Matrix4 lightView = Matrix4::identity;

    if(light.type == LightComponent::Type::Spot){
        lightProjection = Matrix4::Perspective(Mathf::Deg2Rad(light.coneAngleOuter*2), 1, 0.1f, light.radius);
        lightView = Matrix4::LookAt(transform.position(), transform.position() - (-transform.forward()), Vector3::up);
    }

    if(light.type == LightComponent::Type::Directional){
        float near_plane = 0.05f, far_plane = 1000;
        float lightBoxHalfExtend = 50;
        lightProjection = Matrix4::Ortho(-lightBoxHalfExtend, lightBoxHalfExtend, -lightBoxHalfExtend, lightBoxHalfExtend, near_plane, far_plane);
        Vector3 lightCenter = transform.position();
        Vector3 lightEye = lightCenter + (-(transform.forward() * lightBoxHalfExtend));
        lightView = Matrix4::LookAt(lightEye, lightCenter, Vector3(0.0f, 1.0f,  0.0f));
    }

    _lightSpaceMatrix = lightProjection * lightView; 

    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(environmentSettings.shadowBackFaceRender == false ? CullFace::FRONT : CullFace::BACK);
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);

    auto view = root.scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view){
        auto& c = view.get<MeshRendererComponent>(_entity);
        auto& t = view.get<TransformComponent>(_entity);

        for(Ref<Mesh> m: c.model()->meshs){
            root._shadowMapShader->Bind();
            root._shadowMapShader->SetMatrix4("lightSpaceMatrix", _lightSpaceMatrix);
            Renderer::DrawMesh(*m, t.globalModelMatrix(), *root._shadowMapShader);
        }
    }

    _shadowMap->Unbind();
}

}