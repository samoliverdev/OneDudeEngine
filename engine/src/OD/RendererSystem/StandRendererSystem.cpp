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
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};

    directinallightShadowPass._shadowMap = new Framebuffer(specification);
    for(int i = 0; i < MAX_SPOTLIGHT_SHADOWS; i++){
        spotlightShadowPass[i]._shadowMap = new Framebuffer(specification);
    }
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        cascadeShadows[i]._shadowMap = new Framebuffer(specification);
    }

    FrameBufferSpecification specificationC;
    specificationC.width = 1024 * 4;
    specificationC.height = 1024 * 4;
    specificationC.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    specificationC.sample = SHADOW_MAP_CASCADE_COUNT;
    specificationC.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
    _cascadeShadowMap = new Framebuffer(specificationC);

    _shadowMapShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/ShadowMap.glsl");
    _cascadeShadowMapShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/ShadowMapCascade.glsl");

    _postProcessingShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/BasicPostProcessing.glsl");
    _blitShader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/Blit.glsl");

    FrameBufferSpecification framebufferSpecification = {Application::screenWidth(), Application::screenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGB16F}, {FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
    framebufferSpecification.sample = 8;
    _finalColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::None};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.sample = 1;
    _objectsId = new Framebuffer(framebufferSpecification);

    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
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
    delete _cascadeShadowMap;

    for(int i = 0; i < MAX_SPOTLIGHT_SHADOWS; i++){
        delete spotlightShadowPass[i]._shadowMap;
    }

    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        delete cascadeShadows[i]._shadowMap;
    }

    delete _finalColor;
    delete _pp1;
    delete _pp2;
}

Vector3 Plane3Intersect(Plane p1, Plane p2, Plane p3){ //get the intersection point of 3 planes
    return ( ( -p1.distance * math::cross( p2.normal, p3.normal ) ) +
            ( -p2.distance * math::cross( p3.normal, p1.normal ) ) +
            ( -p3.distance * math::cross( p1.normal, p2.normal ) ) ) /
        ( math::dot( p1.normal, math::cross( p2.normal, p3.normal ) ) );
}

void DrawFrustum(Frustum frustum, Matrix4 model = Matrix4Identity){
    Vector3 nearCorners[4]; //Approx'd nearplane corners
    Vector3 farCorners[4]; //Approx'd farplane corners
    Plane camPlanes[6];
    camPlanes[0] = frustum.leftFace;
    camPlanes[1] = frustum.rightFace;
    camPlanes[2] = frustum.bottomFace;
    camPlanes[3] = frustum.topFace;
    camPlanes[4] = frustum.nearFace;
    camPlanes[5] = frustum.farFace;

    Plane temp = camPlanes[1]; camPlanes[1] = camPlanes[2]; camPlanes[2] = temp; //swap [1] and [2] so the order is better for the loop

    for(int i = 0; i < 4; i++){
        nearCorners[i] = Plane3Intersect(camPlanes[4], camPlanes[i], camPlanes[(i + 1) % 4]); //near corners on the created projection matrix
        farCorners[i] = Plane3Intersect(camPlanes[5], camPlanes[i], camPlanes[(i + 1) % 4]); //far corners on the created projection matrix
    }

    for(int i = 0; i < 4; i++){
        Renderer::DrawLine(model, nearCorners[i], nearCorners[( i + 1 ) % 4], Vector3(1,1,1), 1); //near corners on the created projection matrix
        Renderer::DrawLine(model, farCorners[i], farCorners[( i + 1 ) % 4], Vector3(1,1,1), 1); //far corners on the created projection matrix
        Renderer::DrawLine(model, nearCorners[i], farCorners[i], Vector3(1,1,1), 1); //sides of the created projection matrix
    }
}

void StandRendererSystem::Update(){
    OD_PROFILE_SCOPE("StandRendererSystem::Update");

    int width = Application::screenWidth();
    int height = Application::screenHeight();

    if(_outFramebuffer != nullptr){
        width = _outFramebuffer->width();
        height = _outFramebuffer->height();
    }

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
            _cascadeShadowMap->Resize(size, size);
            for(int i = 0; i < MAX_SPOTLIGHT_SHADOWS; i++){
                spotlightShadowPass[i]._shadowMap->Resize(size, size);
            }
            for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
                cascadeShadows[i]._shadowMap->Resize(size, size);
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

    bool hasMainCamera = false;
    Camera mainCamera;
    
    auto cameraView = scene()->GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto e: cameraView){
        CameraComponent& c = cameraView.get<CameraComponent>(e);
        TransformComponent& t = cameraView.get<TransformComponent>(e);
        if(c.isMain){
            c.UpdateCameraData(t, width, height);
            mainCamera = c.camera();
            hasMainCamera = true;
            break;
        }
    }
    if(_overrideCamera != nullptr){
        mainCamera = _overrideCamera->cam;
        hasMainCamera = true;
    }

    spotlightShadowPassCount = 0;
    bool hasShadow = false;
    auto view = scene()->GetRegistry().view<LightComponent, TransformComponent>();
    for(auto entity: view){
        auto& light = view.get<LightComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        /*if(light.type == LightComponent::Type::Directional && light.renderShadow == true){
            directinallightShadowPass.Render(light, transform, *this);
            hasShadow = true;
        }*/

        /*if(light.type == LightComponent::Type::Directional && light.renderShadow == true){
            CascadeShadow::UpdateCascadeShadow2(cascadeShadows, mainCamera, transform);
            for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
                cascadeShadows[i].Render(light, transform, *this);
            }
        }*/

        if(light.type == LightComponent::Type::Directional && light.renderShadow == true){
            CascadeShadow::UpdateCascadeShadow2(cascadeShadows, mainCamera, transform);
            RenderCascadeShadow(light, transform, *this);
        }

        /*if(light.type == LightComponent::Type::Spot && light.renderShadow == true && spotlightShadowPassCount < MAX_SPOTLIGHT_SHADOWS){
            spotlightShadowPass[spotlightShadowPassCount].Render(light, transform, *this);
            spotlightShadowPassCount += 1;
        }*/
    }

    //if(hasShadow == false){
    //    directinallightShadowPass.Clean(*this);
        /*_shadowMap->Bind();
        ClearSceneShadow();
        _shadowMap->Unbind();*/
    //}
    
    // ---------- Render Cameras ---------- 

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
            RenderCamera renderCam = {
                c.camera(),
                CreateFrustumFromCamera(t, (float)width / (float)height, Mathf::Deg2Rad(c.fieldOfView), c.nearClipPlane, c.farClipPlane)
            };

            RenderScene(
                _overrideCamera != nullptr ? *_overrideCamera : renderCam, 
                true, 
                _overrideCamera != nullptr ? _overrideCameraTrans.localPosition() : t.position()
            );
            break;
        }
    }
    scene()->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    /*auto view3 = scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    int aabbGizmosCount = 0;
    for(auto entity: view3){
        auto& c = view3.get<MeshRendererComponent>(entity);
        auto& t = view3.get<TransformComponent>(entity);

        AABB aabb = c.getGlobalAABB(t);
        aabb.Expand(Vector3One*2.0f);
        Transform _t;
        _t.localPosition(aabb.center);
        _t.localScale(aabb.extents);

        Renderer::DrawWireCube(_t.GetLocalModelMatrix(), Vector3(0,1,0), 2);
        aabbGizmosCount += 1;
    }
    Renderer::drawCalls -= aabbGizmosCount;
    */

    /*for(auto entity: view2){
        auto& c = view2.get<CameraComponent>(entity);
        auto& t = view2.get<TransformComponent>(entity);
        DrawFrustum(
            CreateFrustumFromCamera(t, (float)width / (float)height, Mathf::Deg2Rad(c.fieldOfView), c.nearClipPlane, c.farClipPlane)
        );
    }*/

    //if(_overrideCamera != nullptr){ DrawFrustum(_overrideCamera->frustum); }

    //LogInfo("ReadPixel(1): %d", _finalColor->ReadPixel(1, 50, 50));

    {
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
}

/////////////////////////////////////////////////

void StandRendererSystem::SetStandUniforms(Vector3 viewPos, Shader& shader){
    int baseShadowMapIndex = 1;

    shader.Bind();

    shader.SetVector3("viewPos", viewPos);

    shader.SetVector3("ambientLight", environmentSettings.ambient);
    shader.SetFloat("shadowBias", environmentSettings.shadowBias);

    //shader.SetMatrix4("lightSpaceMatrix", directinallightShadowPass._lightSpaceMatrix);
    //shader.SetFramebuffer("shadowMap", *directinallightShadowPass._shadowMap, baseShadowMapIndex, -1); 
    //baseShadowMapIndex += 1;

    shader.SetFramebuffer("cascadeShadowMapsA", *_cascadeShadowMap, baseShadowMapIndex, -1);
    baseShadowMapIndex += 1;
    
    shader.SetInt("cascadeShadowCount", SHADOW_MAP_CASCADE_COUNT);
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        std::string setFlt = "cascadeShadowSplitDistances[" + std::to_string(i) + "]";
        shader.SetFloat(setFlt.c_str(), cascadeShadows[i].splitDistance);

        std::string setMat = "cascadeShadowMatrixs[" + std::to_string(i) + "]";
        shader.SetMatrix4(setMat.c_str(), cascadeShadows[i].projViewMatrix);

        std::string setFram = "cascadeShadowMaps[" + std::to_string(i) + "]";
        shader.SetFramebuffer(setFram.c_str(), *cascadeShadows[i]._shadowMap, baseShadowMapIndex, -1);
        baseShadowMapIndex += 1;
    }

    /*shader.SetInt("spotlightShadowCount", spotlightShadowPassCount);
    for(int i = 0; i < spotlightShadowPassCount; i++){
        baseShadowMapIndex += 1;

        std::string setMat = "spotlightSpaceMatrixs[" + std::to_string(i) + "]";
        shader.SetMatrix4(setMat.c_str(), spotlightShadowPass[i]._lightSpaceMatrix);

        std::string setFram = "spotlightShadowMaps[" + std::to_string(i) + "]";
        shader.SetFramebuffer(setFram.c_str(), *spotlightShadowPass[i]._shadowMap, baseShadowMapIndex, -1);
    }*/

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
        shader.SetVector3("directionalLightDir", Vector3Zero);
        shader.SetVector3("directionalLightColor", Vector3Zero);
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
            shader.SetFloat(buff, math::cos(Mathf::Deg2Rad(l.coneAngleOuter)));

            sprintf(buff, "lights[%d].coneAngleInner", i);
            shader.SetFloat(buff, math::cos(Mathf::Deg2Rad(l.coneAngleInner)));
        }
    }
}

void StandRendererSystem::RenderScene(RenderCamera& camera, bool isMain, Vector3 camPos){
    OD_PROFILE_SCOPE("StandRendererSystem::RenderScene");

    // --------- Render Targets --------- 
    std::unordered_map< Ref<Material>, std::vector<RenderTarget> > renderTargetsOpaques; 
    std::unordered_map< RenderTargetInstancing, std::vector<Matrix4>, InstancingKeyHasher > renderTargetOpaquesInstancing; 
    std::map<float, std::unordered_map<Ref<Material>,std::vector<RenderTarget>> >renderTargetsBlend; 
    std::set<Ref<Shader>> shaderTargets;
    //std::set<Ref<Material>> materialTargets;

    // --------- Set Camera --------- 
    if(isMain){
        Renderer::SetCamera(_overrideCamera != nullptr ? _overrideCamera->cam : camera.cam);
    } else {
        Renderer::SetCamera(camera.cam);
    }

    // --------- Clean --------- 
    Renderer::Clean(environmentSettings.cleanColor.x, environmentSettings.cleanColor.y, environmentSettings.cleanColor.z, 1);

    #pragma region RenderSkyPass
    {
    // --------- Render Sky ---------
    OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::RenderSky"); 
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
    }
    #pragma endregion 

    {
    // --------- Setup Render --------- 
    OD_PROFILE_SCOPE("StandRendererSystem::Setups");
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(CullFace::BACK);

    auto view1 = scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view1){
        auto& c = view1.get<MeshRendererComponent>(_entity);
        auto& t = view1.get<TransformComponent>(_entity);

        /*if(c.model() == nullptr) continue;
        if(c._boundingVolumeIsDirty){
            c._boundingVolume = generateAABB(*c.model());
            c._boundingVolumeSphere = generateSphereBV(*c.model());
            c._boundingVolumeIsDirty = false;
        }
        AABB boundingVolume = c._boundingVolume.Scaled(t.localScale() * 2.0f);
        if(boundingVolume.isOnFrustum(camera.frustum, t) == false) continue;*/
        //if(c._boundingVolumeSphere.isOnFrustum(camera.frustum, t) == false) continue;

        int index = 0;
        for(Ref<Material> i: c.model()->materials){
            Ref<Material> targetMaterial = i;
            if(c.materialsOverride()[index] != nullptr) targetMaterial = c.materialsOverride()[index];

            shaderTargets.insert(targetMaterial->shader());
            //materialTargets.insert(targetMaterial);

            if(targetMaterial->isBlend()){
                float distance = math::distance(camPos, t.position());
                renderTargetsBlend[distance][targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix() * c.model()->matrixs[index]});
            } else {
                if(targetMaterial->enableInstancing() && targetMaterial->supportInstancing()){
                    renderTargetOpaquesInstancing[{targetMaterial, c.model()->meshs[index]}].push_back(t.globalModelMatrix() * c.model()->matrixs[index]);
                } else {
                    renderTargetsOpaques[targetMaterial].push_back({c.model()->meshs[index], t.globalModelMatrix() * c.model()->matrixs[index]});
                }
            }
                
            index += 1;
        }
    }
    }

    for(auto i: shaderTargets){
        SetStandUniforms(camPos, *i);
    }

    //for(auto i: materialTargets){
    //    i->UpdateUniforms();
    //

    {
    // --------- Render Opaques Instancing --------- 
    OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::RenderOpaquesInstancing");
    for(auto& i: renderTargetOpaquesInstancing){
        //SetStandUniforms(camPos, *i.first.material->shader());
        i.first.material->UpdateUniforms();

        i.first.mesh->instancingModelMatrixs.clear();
        for(auto j: i.second){
            i.first.mesh->instancingModelMatrixs.push_back(j);
        }
        i.first.mesh->UpdateMeshInstancingModelMatrixs();

        Renderer::DrawMeshInstancing(*i.first.mesh, *i.first.material->shader(), i.second.size());
    }
    }

    {
    // --------- Render Opaques --------- 
    OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::RenderOpaques");

    for(auto i: renderTargetsOpaques){
        //SetStandUniforms(camPos, *i.first->shader());
        i.first->UpdateUniforms();
        for(auto j: i.second){
            //Renderer::DrawMeshMVP(*j.mesh, j.trans, *i.first->shader());
            Renderer::DrawMesh(*j.mesh, j.trans, *i.first->shader());
            //Renderer::DrawMeshRaw(*j.mesh);
        }
    }

    }
    
    {
    // --------- Render Blends --------- 
    OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::RenderBlends");
    Renderer::SetCullFace(CullFace::NONE);
    Renderer::SetBlend(true);
    Renderer::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);

    for(auto it = renderTargetsBlend.rbegin(); it != renderTargetsBlend.rend(); it++){
        for(auto i: it->second){
            //SetStandUniforms(camPos, *i.first->shader(), *i.first);
            i.first->UpdateUniforms();
            for(auto j: i.second){
                Renderer::DrawMesh(*j.mesh, j.trans, *i.first->shader());
            }
        }
    }

    Renderer::SetBlend(false);
    }

    {
    // --------- Render Sprites --------- 
    OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::RenderSprites");
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

    Matrix4 lightProjection = Matrix4Identity;
    Matrix4 lightView = Matrix4Identity;

    if(light.type == LightComponent::Type::Spot){
        lightProjection = math::perspective(Mathf::Deg2Rad(light.coneAngleOuter*2), 1.0f, 0.1f, light.radius);
        lightView = math::lookAt(transform.position(), transform.position() - (-transform.forward()), Vector3Up);
    }

    if(light.type == LightComponent::Type::Directional){
        float near_plane = 0.05f, far_plane = 1000;
        float lightBoxHalfExtend = 50;
        lightProjection = math::ortho(-lightBoxHalfExtend, lightBoxHalfExtend, -lightBoxHalfExtend, lightBoxHalfExtend, near_plane, far_plane);
        Vector3 lightCenter = transform.position();
        Vector3 lightEye = lightCenter + (-(transform.forward() * lightBoxHalfExtend));
        lightView = math::lookAt(lightEye, lightCenter, Vector3(0.0f, 1.0f,  0.0f));
    }

    _lightSpaceMatrix = lightProjection * lightView; 

    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(environmentSettings.shadowBackFaceRender == false ? CullFace::FRONT : CullFace::BACK);
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);

    root._shadowMapShader->Bind();
    root._shadowMapShader->SetMatrix4("lightSpaceMatrix", _lightSpaceMatrix);

    auto view = root.scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view){
        auto& c = view.get<MeshRendererComponent>(_entity);
        auto& t = view.get<TransformComponent>(_entity);

        for(Ref<Mesh> m: c.model()->meshs){
            //root._shadowMapShader->Bind();
            //root._shadowMapShader->SetMatrix4("lightSpaceMatrix", _lightSpaceMatrix);
            Renderer::DrawMesh(*m, t.globalModelMatrix(), *root._shadowMapShader);
        }
    }

    _shadowMap->Unbind();
}

void StandRendererSystem::CascadeShadow::Clean(StandRendererSystem& root){
    OD_PROFILE_SCOPE("StandRendererSystem::CascadeShadow::Clean");
    _shadowMap->Bind();
    Renderer::SetViewport(0, 0, _shadowMap->width(), _shadowMap->height());
    Renderer::Clean(0.5f, 0.1f, 0.8f, 1);
    _shadowMap->Unbind();
}

void StandRendererSystem::CascadeShadow::Render(LightComponent& light, TransformComponent& transform, StandRendererSystem& root){
    OD_PROFILE_SCOPE("StandRendererSystem::CascadeShadow::Render");

    _shadowMap->Bind();
    Renderer::SetViewport(0, 0, _shadowMap->width(), _shadowMap->height());
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(environmentSettings.shadowBackFaceRender == false ? CullFace::FRONT : CullFace::BACK);
    //glEnable(GL_DEPTH_CLAMP);
    Renderer::Clean(1, 1, 1, 1);

    root._shadowMapShader->Bind();
    root._shadowMapShader->SetMatrix4("lightSpaceMatrix", projViewMatrix);

    auto view = root.scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view){
        auto& c = view.get<MeshRendererComponent>(_entity);
        auto& t = view.get<TransformComponent>(_entity);

        int index = 0;
        for(Ref<Mesh> m: c.model()->meshs){
            //root._shadowMapShader->Bind();
            //root._shadowMapShader->SetMatrix4("lightSpaceMatrix", projViewMatrix);
            root._shadowMapShader->SetMatrix4("model", t.globalModelMatrix() * c.model()->matrixs[index]);
            Renderer::DrawMeshRaw(*m);
            //Renderer::DrawMesh(*m, t.globalModelMatrix(), *root._shadowMapShader);
            index += 1;
        }
    }

    _shadowMap->Unbind();
    //glDisable(GL_DEPTH_CLAMP);
}

void StandRendererSystem::CascadeShadow::UpdateCascadeShadow(CascadeShadow* cascadeShadows, Camera& cam, TransformComponent& light){
    Matrix4 view = cam.view;
    Matrix4 proj = cam.projection;
    Vector4 lightPos = Vector4(light.forward(), 0);

    //LogInfo("Cam Near: %f Far: %f", cam.nearClip, cam.farClip);

    float cascadeSplitLambda = 0.95f;

    float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

    float nearClip = cam.nearClip;
    float farClip = cam.farClip;
    float clipRange = farClip - nearClip;

    float minZ = nearClip;
    float maxZ = nearClip + clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        float p = (i + 1) / (float) (SHADOW_MAP_CASCADE_COUNT);
        float log = (float) (minZ * math::pow(ratio, p));
        float uniform = minZ + range * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearClip) / clipRange;
    }

    // Calculate orthographic projection matrix for each cascade
    float lastSplitDist = 0;
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        float splitDist = cascadeSplits[i];

        Vector3 frustumCorners[] = {
            Vector3(-1.0f, 1.0f, -1.0f),
            Vector3(1.0f, 1.0f, -1.0f),
            Vector3(1.0f, -1.0f, -1.0f),
            Vector3(-1.0f, -1.0f, -1.0f),
            Vector3(-1.0f, 1.0f, 1.0f),
            Vector3(1.0f, 1.0f, 1.0f),
            Vector3(1.0f, -1.0f, 1.0f),
            Vector3(-1.0f, -1.0f, 1.0f)
        };

        // Project frustum corners into world space
        Matrix4 invCam = math::inverse(proj * view);
        for(int j = 0; j < 8; j++){
            Vector4 invCorner = Vector4(frustumCorners[j], 1) * invCam;
            frustumCorners[j] = Vector3(invCorner.x / invCorner.w, invCorner.y / invCorner.w, invCorner.z / invCorner.w);
        }

        for(int j = 0; j < 4; j++){
            Vector3 dist = frustumCorners[j+4] - frustumCorners[j];
            frustumCorners[j+4] = (frustumCorners[j] + dist) * splitDist;
            frustumCorners[j] = (frustumCorners[j] + dist) * lastSplitDist;
        }
        Vector3 frustumCenter = Vector3Zero;
        for(int j = 0; j < 8; j++){
            frustumCenter = frustumCenter + frustumCorners[j];
        }
        frustumCenter = frustumCenter / 8.0f;

        float radius = 0;
        for(int j = 0; j < 8; j++){
            float distance = math::length(frustumCorners[j] - frustumCenter);
            radius = math::max(radius, distance);
        }
        radius = math::ceil(radius * 16.0f) / 16.0f;

        Vector3 maxExtents = Vector3(radius);
        Vector3 minExtents = maxExtents * -1.0f;

        Vector3 lightDir = math::normalize(lightPos * -1.0f);
        Vector3 eye = (frustumCenter - lightDir) * -minExtents.z;
        Vector3 up = Vector3Up;
        Matrix4 lightViewMatrix = math::lookAt(eye, frustumCenter, up);
        Matrix4 lightOrthMatrix = math::ortho(minExtents.x, maxExtents.x, minExtents.y, minExtents.y, 0.0f, maxExtents.z - minExtents.z);
    
        cascadeShadows[i].splitDistance = (nearClip + splitDist * clipRange) * -1.0f;
        cascadeShadows[i].projViewMatrix = lightOrthMatrix * lightViewMatrix; 
        
        lastSplitDist = cascadeSplits[i];
    }
}

std::vector<Vector4> getFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view){
    const auto inv = math::inverse(proj * view);
    
    std::vector<Vector4> frustumCorners;
    for(unsigned int x = 0; x < 2; ++x){
        for(unsigned int y = 0; y < 2; ++y){
            for(unsigned int z = 0; z < 2; ++z){
                const Vector4 pt = inv * Vector4(
                    2.0f * x - 1.0f, 
                    2.0f * y - 1.0f, 
                    2.0f * z - 1.0f, 
                    1.0f
                );
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    
    return frustumCorners;
}

glm::mat4 getLightSpaceMatrix(Camera& cam, Vector3 lightDir, const float nearPlane, const float farPlane){
    const auto proj = glm::perspective(cam.fov, (float)cam.width / (float)cam.height, nearPlane, farPlane);
    const auto corners = getFrustumCornersWorldSpace(proj, cam.view);

    glm::vec3 center = glm::vec3(0, 0, 0);
    for(const auto& v : corners){
        center += glm::vec3(v);
    }
    center /= corners.size();

    const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for(const auto& v : corners){
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    // Tune this parameter according to the scene
    constexpr float zMult = 10.0f;
    if(minZ < 0){
        minZ *= zMult;
    } else {
        minZ /= zMult;
    }
    if(maxZ < 0){
        maxZ /= zMult;
    } else {
        maxZ *= zMult;
    }

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

void StandRendererSystem::CascadeShadow::UpdateCascadeShadow2(CascadeShadow* cascadeShadows, Camera& cam, TransformComponent& light){
    float shadowDistance = cam.farClip;
    //shadowDistance = 500;

    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f, cam.farClip };
    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f };
    std::vector<float> shadowCascadeLevels{ shadowDistance*0.1f, shadowDistance*0.25f, shadowDistance*0.5f, shadowDistance*1.0f };

    Assert(shadowCascadeLevels.size() == SHADOW_MAP_CASCADE_COUNT);
    
    Vector3 lightDir = -light.forward();
    float cameraNearPlane = cam.nearClip;
    float cameraFarPlane = cam.farClip;

    std::vector<glm::mat4> lightMatrixs;
    for(size_t i = 0; i < shadowCascadeLevels.size(); ++i){
        if(i == 0){
            lightMatrixs.push_back(getLightSpaceMatrix(cam, lightDir, cameraNearPlane, shadowCascadeLevels[i]));
        }else if (i < shadowCascadeLevels.size()){
            lightMatrixs.push_back(getLightSpaceMatrix(cam, lightDir, shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
        }
    }

    Assert(lightMatrixs.size() == SHADOW_MAP_CASCADE_COUNT);

    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        cascadeShadows[i].projViewMatrix = lightMatrixs[i];
        cascadeShadows[i].splitDistance = shadowCascadeLevels[i];
    }
}

void StandRendererSystem::RenderCascadeShadow(LightComponent& light, TransformComponent& transform, StandRendererSystem& root){
    OD_PROFILE_SCOPE("StandRendererSystem::RenderCascadeShadow");

    _cascadeShadowMap->Bind();
    Renderer::SetViewport(0, 0, _cascadeShadowMap->width(), _cascadeShadowMap->height());
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(environmentSettings.shadowBackFaceRender == false ? CullFace::FRONT : CullFace::BACK);
    //glEnable(GL_DEPTH_CLAMP);
    Renderer::Clean(1, 1, 1, 1);

    _cascadeShadowMapShader->Bind();
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        std::string setMat = "lightSpaceMatrices[" + std::to_string(i) + "]";
        _cascadeShadowMapShader->SetMatrix4(setMat.c_str(), cascadeShadows[i].projViewMatrix);
    }

    auto view = root.scene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view){
        auto& c = view.get<MeshRendererComponent>(_entity);
        auto& t = view.get<TransformComponent>(_entity);

        int index = 0;
        for(Ref<Mesh> m: c.model()->meshs){
            _cascadeShadowMapShader->SetMatrix4("model", t.globalModelMatrix() * c.model()->matrixs[index]);
            Renderer::DrawMeshRaw(*m);
            index += 1;
        }
    }

    _cascadeShadowMap->Unbind();
    //glDisable(GL_DEPTH_CLAMP);
}

}