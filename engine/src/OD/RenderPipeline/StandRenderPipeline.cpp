#include "OD/Graphics/Graphics.h"
#include "StandRenderPipeline.h"
#include "SpriteComponent.h"
#include "MeshRendererComponent.h"
#include "EnvironmentComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "OD/Physics/PhysicsSystem.h"
#include <unordered_map>
#include <map>
#include "OD/Core/Application.h"
#include "OD/Core/JobSystem.h"
#include "OD/Utils/FastMap.h"
#include "RenderPipelineUtils.h"
#include "CommandBuffer.h"
#include <mutex>

namespace OD{

union MaterialBind{
    uint64_t big;
    struct{
        float distance;
        uint32_t materialId;
        //float distance;
    };
};

class TestPP1: public PostProcessingPass{
public:
    TestPP1(int option):_option(option){
        _ppShader = Shader::CreateFromFile("res/Engine/Shaders/BasicPostProcessing.glsl");
        Assert(_ppShader != nullptr);
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        Shader::Bind(*_ppShader);
        _ppShader->SetFloat("option", _option);
        Graphics::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    int _option;
    Ref<Shader> _ppShader;
};

class GameCorrectionPP: public PostProcessingPass{
public:
    GameCorrectionPP(){
        _ppShader = Shader::CreateFromFile("res/Engine/Shaders/GamaCorrectionPP.glsl");
        Assert(_ppShader != nullptr);
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        Shader::Bind(*_ppShader);
        Graphics::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    Ref<Shader> _ppShader;
};

class ToneMappingPP: public PostProcessingPass{
public:
    ToneMappingPP(){
        _ppShader = Shader::CreateFromFile("res/Engine/Shaders/ToneMappingPP.glsl");
        Assert(_ppShader != nullptr);

        enable = false;
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        Shader::Bind(*_ppShader);
        Graphics::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

private:
    Ref<Shader> _ppShader;
};

EnvironmentSettings environmentSettings;

StandRenderPipeline::StandRenderPipeline(Scene* inScene):BaseRenderPipeline(inScene){
    //glEnable(GL_FRAMEBUFFER_SRGB); 

    spriteMesh = Mesh::CenterQuad(true);
    spriteShader = AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Sprite.glsl");

    FrameBufferSpecification specification;
    specification.width = 1024 * 4;
    specification.height = 1024 * 4;
    specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};

    directinallightShadowPass.shadowMap = new Framebuffer(specification);
    for(int i = 0; i < MAX_SPOTLIGHT_SHADOWS; i++){
        spotlightShadowPass[i].shadowMap = new Framebuffer(specification);
    }
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        cascadeShadows[i].shadowMap = new Framebuffer(specification);
    }

    FrameBufferSpecification specificationC;
    specificationC.width = 1024 * 2;
    specificationC.height = 1024 * 2;
    specificationC.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    specificationC.sample = SHADOW_MAP_CASCADE_COUNT;
    specificationC.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
    cascadeShadowMap = new Framebuffer(specificationC);

    shadowMapShader = AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/ShadowMap.glsl");
    cascadeShadowMapShader = AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/ShadowMapCascade.glsl");

    postProcessingShader = AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/BasicPostProcessing.glsl");
    blitShader = AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Blit.glsl");

    FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA8}, {FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
    framebufferSpecification.sample = 2;
    finalColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::None};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.sample = 1;
    objectsId = new Framebuffer(framebufferSpecification);

    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA8}};
    framebufferSpecification.sample = 1;
    finalColor2 = new Framebuffer(framebufferSpecification);
    pp1 = new Framebuffer(framebufferSpecification);
    pp2 = new Framebuffer(framebufferSpecification);

    skyboxMesh = Mesh::SkyboxCube();

    //_ppPass.push_back(new GameCorrectionPP());
    ppPass.push_back(new ToneMappingPP());
}

StandRenderPipeline::~StandRenderPipeline(){
    for(auto i: ppPass){
        delete i;
    }

    delete directinallightShadowPass.shadowMap;
    delete cascadeShadowMap;

    for(int i = 0; i < MAX_SPOTLIGHT_SHADOWS; i++){
        delete spotlightShadowPass[i].shadowMap;
    }

    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        delete cascadeShadows[i].shadowMap;
    }

    delete finalColor;
    delete pp1;
    delete pp2;
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
        Graphics::DrawLine(model, nearCorners[i], nearCorners[( i + 1 ) % 4], Vector3(1,1,1), 1); //near corners on the created projection matrix
        Graphics::DrawLine(model, farCorners[i], farCorners[( i + 1 ) % 4], Vector3(1,1,1), 1); //far corners on the created projection matrix
        Graphics::DrawLine(model, nearCorners[i], farCorners[i], Vector3(1,1,1), 1); //sides of the created projection matrix
    }
}

void StandRenderPipeline::Update(){
    OD_PROFILE_SCOPE("StandRendererSystem::Update");

    int width = Application::ScreenWidth();
    int height = Application::ScreenHeight();

    if(outFramebuffer != nullptr){
        width = outFramebuffer->Width();
        height = outFramebuffer->Height();
    }

    // ---------- Setups ---------- 
    environmentSettings = EnvironmentSettings();
    auto _view = GetScene()->GetRegistry().view<EnvironmentComponent>();
    for(auto entity: _view){
        EnvironmentComponent& environmentComponent = _view.get<EnvironmentComponent>(entity);
        if(environmentComponent.inited == false) environmentComponent.Init();

        /*if(environmentComponent.settings.msaaQuality != environmentSettings.msaaQuality && environmentComponent.settings.antiAliasing != environmentSettings.antiAliasing){
            FrameBufferSpecification fs1 = _finalColor->specification();
            fs1.sample = 1;
            if(environmentComponent.settings.antiAliasing == AntiAliasing::MSAA){
                fs1.sample = MSAAQualityLookup[(int)environmentComponent.settings.msaaQuality];
            }
            _finalColor->Reload(fs1);
        }*/

        /*
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
        */

        if(environmentComponent.settings.colorCorrection != environmentSettings.colorCorrection){
            if(environmentComponent.settings.colorCorrection == ColorCorrection::ColorCorrection){
                ppPass[ppPass.size()-1]->enable = true;
            } else {
                ppPass[ppPass.size()-1]->enable = false;
            }
        }

        environmentSettings = environmentComponent.settings;
        break;
    }

    // ---------- Render Shadows ---------- 

    bool hasMainCamera = false;
    Camera mainCamera;
    
    auto cameraView = GetScene()->GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto e: cameraView){
        CameraComponent& c = cameraView.get<CameraComponent>(e);
        TransformComponent& t = cameraView.get<TransformComponent>(e);
        if(c.isMain){
            c.UpdateCameraData(t, width, height);
            mainCamera = c.GetCamera();
            hasMainCamera = true;
            break;
        }
    }
    if(overrideCamera != nullptr){
        mainCamera = *overrideCamera;
        hasMainCamera = true;
    }

    // ---------- Render Cameras ---------- 

    finalColor->Resize(width, height);
    pp1->Resize(width, height);
    pp2->Resize(width, height);
    
    //_finalColor->Bind();

    auto view2 = GetScene()->GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto entity: view2){
        auto& c = view2.get<CameraComponent>(entity);
        auto& t = view2.get<TransformComponent>(entity);

        c.UpdateCameraData(t, width, height);

        if(c.isMain){
            Camera renderCam = c.GetCamera();
            renderCam.frustum = CreateFrustumFromCamera(t, (float)width / (float)height, Mathf::Deg2Rad(c.fieldOfView), c.nearClipPlane, c.farClipPlane);

            Vector3 camPos = overrideCamera != nullptr ? overrideCameraTrans.LocalPosition() : t.Position();
            {
                OD_PROFILE_SCOPE("StandRendererSystem::UpdateAllCommands");
                UpdateAllCommands(camPos);
            }

            auto lightView = GetScene()->GetRegistry().view<LightComponent, TransformComponent>();
            for(auto entity: lightView){
                auto& light = lightView.get<LightComponent>(entity);
                auto& transform = lightView.get<TransformComponent>(entity);

                if(light.type == LightComponent::Type::Directional && light.renderShadow == true){
                    CascadeShadow::UpdateCascadeShadow2(cascadeShadows, mainCamera, transform);
                    RenderCascadeShadow(light, transform, *this);
                    break;
                }
            }

            Framebuffer::Bind(*finalColor);
            Graphics::SetViewport(0, 0, width, height);
            RenderScene(
                overrideCamera != nullptr ? *overrideCamera : renderCam, 
                true, 
                overrideCamera != nullptr ? overrideCameraTrans.LocalPosition() : t.Position()
            );

            break;
        }
    }
    GetScene()->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    {
    Graphics::BlitFramebuffer(finalColor, pp1);
    Graphics::BlitFramebuffer(finalColor, objectsId, 1);
    //Renderer::BlitQuadPostProcessing(_finalColor, _pp1, *_blitShader);

    bool step = false;
    Framebuffer* finalFramebuffer = pp1;

    for(auto i: ppPass){
        finalFramebuffer = step == false ? pp2 : pp1;

        if(i->enable){
            i->OnRenderImage(
                step == false ? pp1 : pp2, 
                step == false ? pp2 : pp1
            );
        } else {
            Graphics::BlitFramebuffer(
                step == false ? pp1 : pp2,
                step == false ? pp2 : pp1
            );
        }

        step = !step;
    }

    if(outFramebuffer != nullptr){
        Graphics::BlitQuadPostProcessing(finalFramebuffer, outFramebuffer, *blitShader);
    } else {
        Graphics::BlitQuadPostProcessing(finalFramebuffer, nullptr, *blitShader);
    }

    finalColor->Unbind();
    pp1->Unbind();
    pp2->Unbind();
    if(outFramebuffer != nullptr) outFramebuffer->Unbind();
    }
}

/////////////////////////////////////////////////

CommandBucket1<uint64_t, DrawCommand> opaquesDrawCommands;
//CommandBucket3<Ref<Material>, DrawCommand> opaquesDrawCommands;

CommandBucket4<Ref<Material>, Ref<Mesh>, DrawInstancingCommand> opaquesDrawIntancingCommands;
CommandBucket1<uint64_t, DrawCommand> blendDrawCommands;
CommandBucket2<uint64_t, DrawCommand> cascadeShadowDrawCommands;
CommandBucket4<Ref<Material>, Ref<Mesh>, DrawInstancingCommand> cascadeShadowDrawIntancingCommands;
std::set<Ref<Shader>> shaderTargets;

void StandRenderPipeline::UpdateAllCommands(Vector3 viewPos){
    opaquesDrawCommands.Clear();
    opaquesDrawIntancingCommands.Clear();
    blendDrawCommands.Clear();
    cascadeShadowDrawCommands.Clear();
    cascadeShadowDrawIntancingCommands.Clear();

    opaquesDrawCommands.sortFunction = [](auto& a, auto& b){
        return (a.first > b.first);
    };
    blendDrawCommands.sortFunction = [](auto& a, auto& b){
        return (a.first < b.first);
    };

    auto view1 = GetScene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();

    /*
    auto& dataSet = *view1.handle();
    std::mutex mtx;
    JobSystem::Dispatch(dataSet.size(), dataSet.size()/4, [&](JobDispatchArgs args){
        //LogInfo("JobIndex: %d, Group Index: %d", args.jobIndex, args.groupIndex);
        auto _entity = dataSet[args.jobIndex];

        auto& c = view1.get<MeshRendererComponent>(_entity);
        auto& t = view1.get<TransformComponent>(_entity);

        for(auto i: c.model()->renderTargets){
            CommandBaseData data;

            data.targetMaterial = c.model()->materials[i.materialIndex];
            data.targetMesh = c.model()->meshs[i.meshIndex];
            data.targetMatrix =  t.globalModelMatrix() * c.model()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            if(i.materialIndex < c.materialsOverride().size() && c.materialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.materialsOverride()[i.materialIndex];
            }

            data.distance = math::distance(viewPos, t.position());

            shaderTargets.insert(data.targetMaterial->shader());

            mtx.lock();

            UpdateOpaquesCommands(data);
            UpdateOpaquesIntancingCommands(data);
            UpdateBlendCommands(data);
            UpdateCascadeShadowCommands(data);
            UpdateCascadeShadowIntancingCommands(data);

            mtx.unlock();
        }

    });
    JobSystem::Wait();
    */

    ///*
    for(auto _entity: view1){
        auto& c = view1.get<MeshRendererComponent>(_entity);
        auto& t = view1.get<TransformComponent>(_entity);
        
        if(c.GetModel() == nullptr) continue;

        for(auto i: c.GetModel()->renderTargets){
            CommandBaseData data;

            data.targetMaterial = c.GetModel()->materials[i.materialIndex];
            data.targetMesh = c.GetModel()->meshs[i.meshIndex];
            data.targetMatrix =  t.GlobalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex];
            }

            data.distance = math::distance(viewPos, t.Position());

            shaderTargets.insert(data.targetMaterial->GetShader());

            UpdateOpaquesCommands(data);
            UpdateOpaquesIntancingCommands(data);
            UpdateBlendCommands(data);
            UpdateCascadeShadowCommands(data);
            UpdateCascadeShadowIntancingCommands(data);
        }
    }
    //*/

    /*opaquesDrawCommands.Sort();
    opaquesDrawIntancingCommands.Sort();
    blendDrawCommands.Sort();
    cascadeShadowDrawCommands.Sort();
    cascadeShadowDrawIntancingCommands.Sort();*/

    JobSystem::Execute([&](){ opaquesDrawCommands.Sort(); });
    JobSystem::Execute([&](){ opaquesDrawIntancingCommands.Sort(); });
    JobSystem::Execute([&](){ blendDrawCommands.Sort(); });
    JobSystem::Execute([&](){ cascadeShadowDrawCommands.Sort(); });
    JobSystem::Execute([&](){ cascadeShadowDrawIntancingCommands.Sort(); });
    JobSystem::Wait();
}

void StandRenderPipeline::UpdateOpaquesCommands(CommandBaseData& data){
    if(data.targetMaterial->IsBlend()) return;
    if(data.targetMaterial->EnableInstancingValid() == true) return;

    MaterialBind bind = {0};
    bind.distance = data.distance;
    bind.materialId = data.targetMaterial->MaterialId();
    
    //opaquesDrawCommands.Add(data.targetMaterial, DrawCommand{
    opaquesDrawCommands.Add(bind.big, DrawCommand{
    //opaquesDrawCommands.Add(data.targetMaterial->MaterialId(), DrawCommand{
        data.targetMaterial,
        data.targetMesh,
        data.targetMatrix
    });
}

void StandRenderPipeline::UpdateOpaquesIntancingCommands(CommandBaseData& data){
    if(data.targetMaterial->IsBlend()) return;
    if(data.targetMaterial->EnableInstancingValid() == false) return;

    DrawInstancingCommand& cm = opaquesDrawIntancingCommands.Get(data.targetMaterial, data.targetMesh);
    cm.material = data.targetMaterial;
    cm.meshs = data.targetMesh;
    cm.trans.push_back(data.targetMatrix);
}

void StandRenderPipeline::UpdateBlendCommands(CommandBaseData& data){
    if(data.targetMaterial->IsBlend() == false) return;

    MaterialBind bind = {0};
    bind.distance = data.distance;
    bind.materialId = data.targetMaterial->MaterialId();
    
    blendDrawCommands.Add(bind.big, DrawCommand{
        data.targetMaterial,
        data.targetMesh,
        data.targetMatrix
    });
}

void StandRenderPipeline::UpdateCascadeShadowCommands(CommandBaseData& data){
    //if(data.targetMaterial->enableInstancingValid() == true) return;

    MaterialBind bind = {0};
    bind.distance = data.distance;
    bind.materialId = data.targetMaterial->MaterialId();
    
    cascadeShadowDrawCommands.Add(bind.big, DrawCommand{
        data.targetMaterial,
        data.targetMesh,
        data.targetMatrix
    });
}

void StandRenderPipeline::UpdateCascadeShadowIntancingCommands(CommandBaseData& data){
    return;

    if(data.targetMaterial->IsBlend()) return;
    if(data.targetMaterial->EnableInstancingValid() == false) return;

    DrawInstancingCommand& cm = cascadeShadowDrawIntancingCommands.Get(data.targetMaterial, data.targetMesh);
    cm.material = data.targetMaterial;
    cm.meshs = data.targetMesh;
    cm.trans.push_back(data.targetMatrix);
}

//////////////////////////////////////////////////

void StandRenderPipeline::SetStandUniforms(Vector3 viewPos, Shader& shader){
    int baseShadowMapIndex = 1;

    Shader::Bind(shader);

    shader.SetVector3("viewPos", viewPos);

    shader.SetVector3("ambientLight", environmentSettings.ambient);
    shader.SetFloat("shadowBias", environmentSettings.shadowBias);

    //shader.SetMatrix4("lightSpaceMatrix", directinallightShadowPass._lightSpaceMatrix);
    //shader.SetFramebuffer("shadowMap", *directinallightShadowPass._shadowMap, baseShadowMapIndex, -1); 
    //baseShadowMapIndex += 1;

    shader.SetFramebuffer("cascadeShadowMapsA", *cascadeShadowMap, baseShadowMapIndex, -1);
    baseShadowMapIndex += 1;
    
    shader.SetInt("cascadeShadowCount", SHADOW_MAP_CASCADE_COUNT);
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        std::string setFlt = "cascadeShadowSplitDistances[" + std::to_string(i) + "]";
        shader.SetFloat(setFlt.c_str(), cascadeShadows[i].splitDistance);

        std::string setMat = "cascadeShadowMatrixs[" + std::to_string(i) + "]";
        shader.SetMatrix4(setMat.c_str(), cascadeShadows[i].projViewMatrix);

        std::string setFram = "cascadeShadowMaps[" + std::to_string(i) + "]";
        shader.SetFramebuffer(setFram.c_str(), *cascadeShadows[i].shadowMap, baseShadowMapIndex, -1);
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

    auto view = GetScene()->GetRegistry().view<LightComponent, TransformComponent>();
    for(auto e: view){
        auto& light = view.get<LightComponent>(e);
        auto& transform = view.get<TransformComponent>(e);

        if(light.type == LightComponent::Type::Directional){
            shader.SetVector3("directionalLightColor", light.color * light.intensity);
            shader.SetVector3("directionalLightDir", -transform.Forward());
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
        LightComponent& l = GetScene()->GetRegistry().get<LightComponent>(lights[i]);
        TransformComponent& t = GetScene()->GetRegistry().get<TransformComponent>(lights[i]);

        sprintf(buff, "lights[%d].type", i);
        shader.SetFloat(buff, (int)l.type);

        sprintf(buff, "lights[%d].pos", i);
        shader.SetVector3(buff, t.Position());

        sprintf(buff, "lights[%d].dir", i);
        shader.SetVector3(buff, t.Forward());

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

void StandRenderPipeline::RenderScene(Camera& camera, bool isMain, Vector3 camPos){
    OD_PROFILE_SCOPE("StandRendererSystem::RenderScene");

    // --------- Set Camera --------- 
    if(isMain){
        Graphics::SetCamera(overrideCamera != nullptr ? *overrideCamera : camera);
    } else {
        Graphics::SetCamera(camera);
    }

    // --------- Clean --------- 
    Graphics::Clean(environmentSettings.cleanColor.x, environmentSettings.cleanColor.y, environmentSettings.cleanColor.z, 1);

    {
        // --------- Render Sky ---------
        OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::RenderSky"); 
        if(environmentSettings.sky != nullptr){
            Assert(environmentSettings.sky->GetShader() != nullptr);

            environmentSettings.sky->UpdateDatas();

            Graphics::SetCullFace(CullFace::BACK);
            Graphics::SetDepthMask(false);
            Shader::Bind(*environmentSettings.sky->GetShader());
            //environmentSettings.sky->shader()->SetCubemap("mainTex", *_skyboxCubemap, 0);
            environmentSettings.sky->GetShader()->SetMatrix4("projection", Graphics::GetCamera().projection);
            Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(Graphics::GetCamera().view)));
            environmentSettings.sky->GetShader()->SetMatrix4("view", skyboxView);
            Graphics::DrawMesh(skyboxMesh);
            Graphics::SetDepthMask(true);
        }
    }

    // --------- Setup Render --------- 
    Graphics::SetDepthTest(DepthTest::LESS);
    Graphics::SetCullFace(CullFace::BACK);
    Ref<Material> lastMat = nullptr;

    for(auto i: shaderTargets){
        SetStandUniforms(camPos, *i);
        i->SetMatrix4("view", camera.view);
        i->SetMatrix4("projection", camera.projection);
    }

    {
        OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::OpaquesDrawCommands");
        opaquesDrawCommands.Each([&](auto& cm){
            if(cm.material != lastMat){
                cm.material->UpdateDatas();
                cm.material->GetShader()->SetFloat("useInstancing", 0.0f); 
                //SetStandUniforms(camPos, *cm.material->shader());
            }
        
            lastMat = cm.material;
            //cm.material->shader()->Bind();
            cm.material->GetShader()->SetMatrix4("model", cm.trans);
            Graphics::DrawMesh(*cm.meshs);
        });
    }

    {
        OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::OpaquesDrawIntancingCommands");
        opaquesDrawIntancingCommands.Each([&](auto& cm){
            cm.material->UpdateDatas();
            cm.material->GetShader()->SetFloat("useInstancing", 1); 
            //SetStandUniforms(camPos, *cm.material->shader());

            cm.meshs->instancingModelMatrixs.clear();

            for(auto j: cm.trans){
                cm.meshs->instancingModelMatrixs.push_back(j);
            }
            cm.meshs->UpdateMeshInstancingModelMatrixs();
            Graphics::DrawMeshInstancing(*cm.meshs, cm.trans.size());
        });
    }

    lastMat = nullptr;

    auto skinnedView = GetScene()->GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent>();
    for(auto e: skinnedView){
        SkinnedMeshRendererComponent& c = skinnedView.get<SkinnedMeshRendererComponent>(e);
        TransformComponent& t = skinnedView.get<TransformComponent>(e);

        if(c.GetModel() == nullptr) continue;

        for(auto i: c.GetModel()->renderTargets){
            CommandBaseData data;

            data.targetMaterial = c.GetModel()->materials[i.materialIndex];
            data.targetMesh = c.GetModel()->meshs[i.meshIndex];
            data.targetMatrix = t.GlobalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex];
            }

            if(data.targetMaterial != lastMat){
                data.targetMaterial->UpdateDatas();
                data.targetMaterial->GetShader()->SetMatrix4("view", camera.view);
                data.targetMaterial->GetShader()->SetMatrix4("projection", camera.projection);
                data.targetMaterial->GetShader()->SetFloat("useInstancing", 0.0f); 
                SetStandUniforms(camPos, *data.targetMaterial->GetShader());
            }
            lastMat = data.targetMaterial;

            Assert(c.posePalette.size() == c.GetModel()->skeleton.GetRestPose().Size());

            //Shader::Bind(*data.targetMaterial->GetShader());
            data.targetMaterial->GetShader()->SetMatrix4("animated", c.posePalette);
            data.targetMaterial->GetShader()->SetMatrix4("model", data.targetMatrix);
            Graphics::DrawMesh(*data.targetMesh);            
        } 
    }
    
    {
        // --------- Render Blends --------- 
        OD_PROFILE_SCOPE("StandRendererSystem::RenderScene::BlendDrawCommands");
        Graphics::SetCullFace(CullFace::NONE);
        Graphics::SetBlend(true);
        Graphics::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);

        lastMat = nullptr;
        for(auto it = blendDrawCommands.commands.rbegin(); it != blendDrawCommands.commands.rend(); it++){
            auto& cm = it->second;

            if(cm.material != lastMat){
                cm.material->UpdateDatas();
                cm.material->GetShader()->SetFloat("useInstancing", 0.0f); 
                SetStandUniforms(camPos, *cm.material->GetShader());
                
            }
        
            lastMat = cm.material;
            Shader::Bind(*cm.material->GetShader());
            cm.material->GetShader()->SetMatrix4("model", cm.trans);
            Graphics::DrawMesh(*cm.meshs);
        }

        /*lastMat = nullptr;
        blendDrawCommands.Each([&](auto& cm){
            if(cm.material != lastMat){
                cm.material->UpdateUniforms();
                cm.material->shader()->SetFloat("useInstancing", 0.0f); 
                SetStandUniforms(camPos, *cm.material->shader());
            }
        
            lastMat = cm.material;
            cm.material->shader()->Bind();
            cm.material->shader()->SetMatrix4("model", cm.trans);
            Renderer::DrawMesh(*cm.meshs);
        });*/

        Graphics::SetBlend(false);
    }
}

void StandRenderPipeline::ShadowRenderPass::Clean(StandRenderPipeline& root){
    OD_PROFILE_SCOPE("StandRendererSystem::ShadowRenderPass::Clean");
    Framebuffer::Bind(*shadowMap);
    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());
    Graphics::Clean(0.5f, 0.1f, 0.8f, 1);
    Framebuffer::Unbind();
}

void StandRenderPipeline::ShadowRenderPass::Render(LightComponent& light, TransformComponent& transform, StandRenderPipeline& root){
    OD_PROFILE_SCOPE("StandRendererSystem::ShadowRenderPass::Render");

    Framebuffer::Bind(*shadowMap);
    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());

    Matrix4 lightProjection = Matrix4Identity;
    Matrix4 lightView = Matrix4Identity;

    if(light.type == LightComponent::Type::Spot){
        lightProjection = math::perspective(Mathf::Deg2Rad(light.coneAngleOuter*2), 1.0f, 0.1f, light.radius);
        lightView = math::lookAt(transform.Position(), transform.Position() - (-transform.Forward()), Vector3Up);
    }

    if(light.type == LightComponent::Type::Directional){
        float near_plane = 0.05f, far_plane = 1000;
        float lightBoxHalfExtend = 50;
        lightProjection = math::ortho(-lightBoxHalfExtend, lightBoxHalfExtend, -lightBoxHalfExtend, lightBoxHalfExtend, near_plane, far_plane);
        Vector3 lightCenter = transform.Position();
        Vector3 lightEye = lightCenter + (-(transform.Forward() * lightBoxHalfExtend));
        lightView = math::lookAt(lightEye, lightCenter, Vector3(0.0f, 1.0f,  0.0f));
    }

    lightSpaceMatrix = lightProjection * lightView; 

    Graphics::SetDepthTest(DepthTest::LESS);
    Graphics::SetCullFace(environmentSettings.shadowBackFaceRender == false ? CullFace::FRONT : CullFace::BACK);
    Graphics::Clean(0.5f, 0.1f, 0.8f, 1);

    Shader::Bind(*root.shadowMapShader);
    root.shadowMapShader->SetMatrix4("lightSpaceMatrix", lightSpaceMatrix);

    auto view = root.GetScene()->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto _entity: view){
        auto& c = view.get<MeshRendererComponent>(_entity);
        auto& t = view.get<TransformComponent>(_entity);

        if(c.GetModel() == nullptr) continue;

        /*for(Ref<Mesh> m: c.model()->meshs){
            //root._shadowMapShader->Bind();
            //root._shadowMapShader->SetMatrix4("lightSpaceMatrix", _lightSpaceMatrix);
            Renderer::DrawMesh(*m, t.globalModelMatrix(), *root._shadowMapShader);
        }*/

        for(auto i: c.GetModel()->renderTargets){
            Matrix4 m = 
                t.GlobalModelMatrix() * 
                c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);

            Shader::Bind(*root.shadowMapShader);
            root.shadowMapShader->SetMatrix4("model", m);
            Graphics::DrawMesh(*c.GetModel()->meshs[i.meshIndex]);
        }
    }

    shadowMap->Unbind();
}

void StandRenderPipeline::CascadeShadow::Clean(StandRenderPipeline& root){
    OD_PROFILE_SCOPE("StandRendererSystem::CascadeShadow::Clean");
    Framebuffer::Bind(*shadowMap);
    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());
    Graphics::Clean(0.5f, 0.1f, 0.8f, 1);
    Framebuffer::Unbind();
}

void StandRenderPipeline::CascadeShadow::Render(LightComponent& light, TransformComponent& transform, StandRenderPipeline& root){
    OD_PROFILE_SCOPE("StandRendererSystem::CascadeShadow::Render");

    Framebuffer::Bind(*shadowMap);
    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());
    Graphics::SetDepthTest(DepthTest::LESS);
    Graphics::SetCullFace(environmentSettings.shadowBackFaceRender == false ? CullFace::FRONT : CullFace::BACK);
    //glEnable(GL_DEPTH_CLAMP);
    Graphics::Clean(1, 1, 1, 1);

    Shader::Bind(*root.shadowMapShader);
    root.shadowMapShader->SetMatrix4("lightSpaceMatrix", projViewMatrix);

    cascadeShadowDrawCommands.Each([&](auto& cm){
        root.shadowMapShader->SetMatrix4("model", cm.trans);
        Graphics::DrawMesh(*cm.meshs);
    });

    shadowMap->Unbind();
    //glDisable(GL_DEPTH_CLAMP);
}

void StandRenderPipeline::CascadeShadow::UpdateCascadeShadow(CascadeShadow* cascadeShadows, Camera& cam, TransformComponent& light){
    Matrix4 view = cam.view;
    Matrix4 proj = cam.projection;
    Vector4 lightPos = Vector4(light.Forward(), 0);

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

void StandRenderPipeline::CascadeShadow::UpdateCascadeShadow2(CascadeShadow* cascadeShadows, Camera& cam, TransformComponent& light){
    float shadowDistance = cam.farClip;
    shadowDistance = 500;

    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f, cam.farClip };
    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f };
    std::vector<float> shadowCascadeLevels{ shadowDistance*0.1f, shadowDistance*0.25f, shadowDistance*0.5f, shadowDistance*1.0f };

    Assert(shadowCascadeLevels.size() == SHADOW_MAP_CASCADE_COUNT);
    
    Vector3 lightDir = -light.Forward();
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

void StandRenderPipeline::RenderCascadeShadow(LightComponent& light, TransformComponent& transform, StandRenderPipeline& root){
    OD_PROFILE_SCOPE("StandRendererSystem::RenderCascadeShadow");

    Framebuffer::Bind(*cascadeShadowMap);
    Graphics::SetViewport(0, 0, cascadeShadowMap->Width(), cascadeShadowMap->Height());
    Graphics::SetDepthTest(DepthTest::LESS);
    Graphics::SetCullFace(environmentSettings.shadowBackFaceRender == false ? CullFace::FRONT : CullFace::BACK);
    //glEnable(GL_DEPTH_CLAMP);
    Graphics::Clean(1, 1, 1, 1);

    Shader::Bind(*cascadeShadowMapShader);
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++){
        std::string setMat = "lightSpaceMatrices[" + std::to_string(i) + "]";
        cascadeShadowMapShader->SetMatrix4(setMat.c_str(), cascadeShadows[i].projViewMatrix);
    }

    cascadeShadowDrawCommands.Each([&](auto& cm){
        cascadeShadowMapShader->SetMatrix4("model", cm.trans);
        Graphics::DrawMesh(*cm.meshs);
    });

    cascadeShadowMapShader->Unbind();
    cascadeShadowMap->Unbind();
    //glDisable(GL_DEPTH_CLAMP);
}

}