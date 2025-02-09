#include "RenderContext.h"
#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "ModelRendererComponent.h"
#include "SpriteRendererComponent.h"
#include "OD/Animation/Animator.h"
#include "OD/Core/Application.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Defines.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/Navmesh/Navmesh.h"
#include "OD/Graphics/Geometry.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Editor/Editor.h"
#include <taskflow/taskflow.hpp> 

namespace OD{

RenderContextSettings settings;

RenderContextSettings& RenderContext::GetSettings(){
    return settings;
}

RenderContext::RenderContext(Scene* inScene){
    scene = inScene;

    FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
    framebufferSpecification.colorAttachments = {
        //{FramebufferTextureFormat::RGBA16F}, 
        {FramebufferTextureFormat::RGBA8}//, 
        //{FramebufferTextureFormat::RED_INTEGER}
    };
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 1;
    forwardOutColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.colorAttachments = {
        {FramebufferTextureFormat::RGB32F}, // Pos
        {FramebufferTextureFormat::RGB32F}, // Normal
        {FramebufferTextureFormat::RGBA16F}, // Albedo
        {FramebufferTextureFormat::RGB16F}, // Emission
        {FramebufferTextureFormat::RGB16F}, // Spec, Metalic, AO
        {FramebufferTextureFormat::RED_INTEGER} // Object ID
    };
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 1;
    deferredOutColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.colorAttachments = {
        //{FramebufferTextureFormat::RGBA16F}
        {FramebufferTextureFormat::RGBA8}
    };
    framebufferSpecification.sample = 1;
    finalColor = new Framebuffer(framebufferSpecification);
    postFx1 = new Framebuffer(framebufferSpecification);
    postFx2 = new Framebuffer(framebufferSpecification);

    blitShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Blit.glsl"));
    deferredGBufferShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredGBuffer.glsl"));
    deferredLightPassShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredLightPassLit.glsl"));
    deferredLightPass = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredLightPassLit.glsl"));

    skyboxMesh = Mesh::SkyboxCube();
    spriteMesh = Mesh::CenterQuad(false);

    //meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    //meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>();
}

RenderContext::~RenderContext(){
    delete deferredOutColor;
    delete forwardOutColor;
    delete finalColor;
    delete postFx1;
    delete postFx2;
}

void RenderContext::Begin(){

}

void RenderContext::End(){
    Material::CleanGlobalUniformsData();
}

void RenderContext::BeginDrawToScreen(){
    int width = Application::ScreenWidth();
    int height = Application::ScreenHeight();

    if(overrideFramebuffer != nullptr){
        width = overrideFramebuffer->Width();
        height = overrideFramebuffer->Height();
    }

    deferredOutColor->Resize(width, height);
    forwardOutColor->Resize(width, height);
    finalColor->Resize(width, height);
    postFx1->Resize(width, height);
    postFx2->Resize(width, height);
}

void RenderContext::BeginForwardPass(){
    //Framebuffer::Unbind(); 
    //ScreenClean();
    //return;

    //Framebuffer::Bind(*forwardOutColor);
    Graphics::BeginFramebuffer(*forwardOutColor, 0);
    
    //Framebuffer::Unbind(); 
    ScreenClean();
}

void RenderContext::BeginDeferredPass(){
    //Assert(false);
    //Framebuffer::Bind(*deferredOutColor);
    Graphics::BeginFramebuffer(*deferredOutColor, 0);
    ScreenClean();
}

void RenderContext::EndDeferredPassAndCopyToForwardPass(){
    //Framebuffer::Bind(*forwardOutColor);
    Graphics::BeginFramebuffer(*forwardOutColor, 0);
    Graphics::Clean(0, 1, 0, 1);

    deferredLightPass->SetTexture("gPosition", deferredOutColor, 0);
    deferredLightPass->SetTexture("gNormal", deferredOutColor, 1);
    deferredLightPass->SetTexture("gAlbedoSpec", deferredOutColor, 2);
    deferredLightPass->SetTexture("gEmission", deferredOutColor, 3);
    deferredLightPass->SetTexture("gOther", deferredOutColor, 4);
    Material::SubmitGraphicDatas(*deferredLightPass);
    //Graphics::BlitQuadPostProcessingRaw(forwardOutColor);
    Graphics::DrawQuadPostProcessing(forwardOutColor, *deferredLightPass);
    
    Graphics::BlitFramebuffer(deferredOutColor, forwardOutColor, -1);
    //Framebuffer::Bind(*forwardOutColor);
    Graphics::BeginFramebuffer(*forwardOutColor, 0);

    /*Framebuffer::Bind(*forwardOutColor);
    Graphics::Clean(0, 1, 0, 1);

    Shader::Bind(*deferredLightPassShader);
    deferredLightPassShader->SetInt("lightsCount", 0);
    deferredLightPassShader->SetVector3("viewPos", cam.viewPos);
    deferredLightPassShader->SetFramebuffer("gPosition", *deferredOutColor, 0, 0);
    deferredLightPassShader->SetFramebuffer("gNormal", *deferredOutColor, 1, 1);
    deferredLightPassShader->SetFramebuffer("gAlbedoSpec", *deferredOutColor, 2, 2);
    Material temp;
    Material::SubmitGraphicDatasCustomShader(temp, *deferredLightPassShader);
    Graphics::BlitQuadPostProcessingRaw(forwardOutColor);
    Graphics::SetDepthTest(DepthTest::LESS); 
    
    Graphics::BlitFramebuffer(deferredOutColor, forwardOutColor, -1);
    Framebuffer::Bind(*forwardOutColor);*/
}

void RenderContext::EndDrawToScreen(){
    //Framebuffer::Unbind(); 
    //return;

    Graphics::BlitFramebuffer(forwardOutColor, finalColor);
    Graphics::DrawQuadPostProcessing(forwardOutColor, finalColor, *blitShader);

    if(overrideFramebuffer != nullptr){
        Graphics::DrawQuadPostProcessing(finalColor, overrideFramebuffer, *blitShader);
    } else {
        Graphics::DrawQuadPostProcessing(finalColor, nullptr, *blitShader);
    }

    //Framebuffer::Unbind(); 
    Graphics::EndFramebuffer();
}

void RenderContext::DrawPostFXs(std::vector<PostFX*>& postFXs){
    //Graphics::SetDepthMask(false);

    bool step = false;
    Framebuffer* finalFramebuffer = postFx1;
    Graphics::BlitFramebuffer(forwardOutColor, postFx1);
    //Graphics::BlitQuadPostProcessing(outColor, postFx1, *blitShader);

    for(auto i: postFXs){
        finalFramebuffer = step == false ? postFx2 : postFx1;

        if(i->enable){
            i->OnRenderImage(
                step == false ? postFx1 : postFx2, 
                step == false ? postFx2 : postFx1
            );
        } else {
            Graphics::BlitFramebuffer(
                step == false ? postFx1 : postFx2,
                step == false ? postFx2 : postFx1
            );
            /*Graphics::BlitQuadPostProcessing(
                step == false ? postFx1 : postFx2, 
                step == false ? postFx2 : postFx1,
                *blitShader
            );*/
        }

        step = !step;
    }

    Graphics::DrawQuadPostProcessing(finalFramebuffer, forwardOutColor, *blitShader);
}

void RenderContext::SetupCameraProperties(Camera inCam){
    cam = inCam;

    Graphics::SetCamera(cam);
    Graphics::SetViewport(
        0, 
        0, 
        cam.width, 
        cam.height
    );

    Material::SetGlobalMatrix4("view", cam.view);
    Material::SetGlobalMatrix4("projection", cam.projection);
    Material::SetGlobalVector3("viewPos", cam.viewPos);
}

void RenderContext::ScreenClean(){
    //Renderer::Clean(cam.cleanColor.x, cam.cleanColor.y, cam.cleanColor.z, 1);
    Graphics::Clean(0, 0, 1, 1);
}

void RenderContext::RenderDataLoop(std::function<void(RenderData&)> onReciveRenderData){
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop");

    auto staticMeshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, StaticRendererComponent>();
    for(auto e: staticMeshView){
        auto& c = staticMeshView.get<MeshRendererComponent>(e);
        auto& t = staticMeshView.get<TransformComponent>(e);
        auto& s = staticMeshView.get<StaticRendererComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        if(s.staticDatas.size() != 0) s.staticDatas.resize(1);
        if(s.staticDatas[0].isDirt){
            s.staticDatas[0].isDirt = false;
            s.staticDatas[0].m = t.GlobalModelMatrix();
            s.staticDatas[0].aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, s.staticDatas[0].m);
        }

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  s.staticDatas[0].m;
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = s.staticDatas[0].aabb;

        onReciveRenderData(data);
    }

    auto meshStaticRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, StaticRendererComponent>();
    for(auto e: meshStaticRenderView){
        auto& c = meshStaticRenderView.get<ModelRendererComponent>(e);
        auto& t = meshStaticRenderView.get<TransformComponent>(e);
        auto& s = meshStaticRenderView.get<StaticRendererComponent>(e);
        if(c.GetModel() == nullptr) continue;

        if(s.staticDatas.size() != c.GetModel()->renderTargets.size()){
            s.staticDatas.resize(c.GetModel()->renderTargets.size());
            for(auto& i: s.staticDatas) i.isDirt = true;
        }

        int _i = 0;
        for(auto i: c.GetModel()->renderTargets){
            if(s.staticDatas[_i].isDirt){
                s.staticDatas[_i].isDirt = false;
                s.staticDatas[_i].m = t.GlobalModelMatrix();
                s.staticDatas[_i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), s.staticDatas[_i].m);
            }

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = c.GetModel()->materials[i.materialIndex].get();
            data.targetMesh = c.GetModel()->meshs[i.meshIndex].get();
            data.targetMatrix =  s.staticDatas[_i].m;
            data.aabb = s.staticDatas[_i].aabb;
            data.posePalette = nullptr;
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            onReciveRenderData(data);
            _i += 1;
        }
    }

    //////////////////////////////////////////////////////////

    auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>(
        entt::exclude<StaticRendererComponent>
    );
    for(auto e: meshView){
        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  t.GlobalModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        onReciveRenderData(data);
    }

    auto meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>(
        entt::exclude<StaticRendererComponent>
    );
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        for(auto i: c.GetModel()->renderTargets){
            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = c.GetModel()->materials[i.materialIndex].get();
            data.targetMesh = c.GetModel()->meshs[i.meshIndex].get();
            data.targetMatrix =  t.GlobalModelMatrix();// * c.localTransform.GetLocalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.posePalette = nullptr;
            //data.aabb = c.GetGlobalAABB(t);
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            onReciveRenderData(data);
        }
    }

    auto skinnedView = GetScene()->GetRegistry().view<SkinnedModelRendererComponent, TransformComponent>();
    for(auto e: skinnedView){
        SkinnedModelRendererComponent& c = skinnedView.get<SkinnedModelRendererComponent>(e);
        TransformComponent& t = skinnedView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        for(auto i: c.GetModel()->renderTargets){
            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = c.GetModel()->materials[i.materialIndex].get();
            data.targetMesh = c.GetModel()->meshs[i.meshIndex].get();
            data.targetMatrix =  t.GlobalModelMatrix();// * c.localTransform.GetLocalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            //data.transform = Transform(data.targetMatrix); //t.ToTransform();
            data.posePalette = &c.posePalette;
            //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            onReciveRenderData(data);
        }
    }

    auto spriteView = GetScene()->GetRegistry().view<TransformComponent, SpriteRendererComponent>();
    for(auto entity: spriteView){
        TransformComponent& t = spriteView.get<TransformComponent>(entity);
        SpriteRendererComponent& c = spriteView.get<SpriteRendererComponent>(entity);
        //if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        AABB aabb(Vector3(0), c.sprite->Width() / c.pixelUnitSize, c.sprite->Height() / c.pixelUnitSize, 1);
        Transform scale;
        scale.LocalScale(Vector3(c.sprite->Width() / c.pixelUnitSize, c.sprite->Height() / c.pixelUnitSize, 1));

        c.material->SetVector4("color", c.color);
        c.material->SetTexture("mainTex", c.sprite);

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.targetMesh = spriteMesh.get();
        data.targetMatrix =  t.GlobalModelMatrix() * scale.GetLocalModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(aabb, data.targetMatrix);

        onReciveRenderData(data);
    }
}

void RenderContext::AddDrawRenderers(RenderData& data, DrawingSettings& settings, RendererList& target){
    bool isBlend = data.targetMaterial->IsBlend();
    bool isInstancing = data.targetMaterial->EnableInstancingValid();
    if(settings.enableIntancing == false){
        isInstancing = false;
    }

    if(settings.renderQueueRange == RenderQueueRange::Transparent){
        if(isBlend == false) return;
        isInstancing = false;
    }
    if(settings.renderQueueRange == RenderQueueRange::Opaue){
        if(isBlend == true) return;
    }

    if(data.posePalette != nullptr){
        target.AddSkinnedDrawCommand({
            data.targetMatrix,
            data.targetMaterial,
            data.targetMesh,
            data.posePalette
        }, data.distance);
        return;
    }

    if(isInstancing){
        target.AddDrawInstancingCommand({
            data.targetMatrix,
            data.targetMaterial,
            data.targetMesh,
        });

    } else {
        target.AddDrawCommand({
            data.targetMatrix,
            data.targetMaterial,
            data.targetMesh,
            data.distance
        }, data.distance);
    } 
}

/*void RenderContext::SetStandUniforms(Camera& cam, SubShader& shader){
    //Shader::Bind(shader);
    shader.SetMatrix4("view", cam.view);
    shader.SetMatrix4("projection", cam.projection);
    shader.SetVector3("viewPos", cam.viewPos);
}*/

void RenderContext::RenderSkyboxLater(){
    OD_PROFILE_SCOPE("RenderContext::RenderSkybox"); 
    if(skyMaterial == nullptr) return;
    
    Assert(skyMaterial->GetShader() != nullptr);

    //skyMaterial->UpdateDatas();
    Material::SubmitGraphicDatas(*skyMaterial);

    /*Graphics::SetCullFace(CullFace::BACK);
    //Graphics::SetDepthMask(false);
    Graphics::SetDepthTest(DepthTest::LESS_EQUAL);
    Graphics::SetBlend(false);
    Shader::Bind(*skyMaterial->GetShader());*/
    
    //environmentSettings.sky->shader()->SetCubemap("mainTex", *_skyboxCubemap, 0);
    /*skyMaterial->GetShader()->SetMatrix4("projection", cam.projection);
    Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
    skyMaterial->GetShader()->SetMatrix4("view", skyboxView);
    Graphics::DrawMeshRaw(*skyboxMesh);*/

    //skyMaterial->SetMatrix4("projection", cam.projection);
    Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
    skyMaterial->SetMatrix4("skyboxView", skyboxView);
    Graphics::DrawMesh(*skyboxMesh, *skyMaterial, Matrix4Identity);

    //Graphics::SetDepthTest(DepthTest::LESS);
    //Graphics::SetDepthMask(true);
}

void RenderContext::DrawRenderersBuffer(RendererList& commandBuffer, bool sort, bool deferred){
    OD_PROFILE_SCOPE("RenderContext::DrawRenderersBuffer");

    if(sort) commandBuffer.Sort();
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        //if(material.GetShader() == nullptr) return;
        //SetStandUniforms(cam, *material.GetShader()); 

        //Graphics::SetDepthTest(DepthTest::EQUAL);

        if(deferred){
            material.EnableKeyword("Deferred");
        } else {
            material.EnableKeyword("Forward");
        }
    };
    commandBuffer.Submit();
    //commandBuffer.onUpdateMaterial = nullptr;
}

void RenderContext::DrawZPreePassRenderersBuffer(RendererList& commandBuffer, bool sort, bool post){
    OD_PROFILE_SCOPE("RenderContext::DrawZPreePassRenderersBuffer");

    if(sort) commandBuffer.Sort();
    commandBuffer.postUpdateMaterial = [&](Material& material){ 
        /*if(post){
            Graphics::SetColorMask(1, 1, 1, 1);
            Graphics::SetDepthTest(DepthTest::EQUAL);
        } else {
            Graphics::SetColorMask(0, 0, 0, 0);
            Graphics::SetDepthTest(DepthTest::LESS);
        }*/
    };
    commandBuffer.Submit();
    commandBuffer.postUpdateMaterial = nullptr;
}

void _DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color);

void RenderContext::DrawGizmos(){
    OD_PROFILE_SCOPE("RenderContext::DrawGizmos"); 

    if(SceneManager::Get().GetActiveScene() == nullptr) return;
    if(SceneManager::Get().GetActiveScene()->Running() && settings.enableGizmosRuntime == false) return;
    if(SceneManager::Get().GetActiveScene()->Running() == false && settings.enableGizmos == false) return;

    //Renderer::SetCamera(cam);
    
    /*Graphics::SetDepthMask(false);
    Graphics::SetDepthTest(DepthTest::LESS);
    Graphics::SetCullFace(CullFace::BACK);
    Graphics::SetBlend(false);*/

    //Graphics::SetBlendFunc(BlendMode::ONE, BlendMode::ONE_MINUS_SRC_ALPHA);
    //Graphics::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);
     
    Camera cm = cam;
    
    //scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    for(System* s: scene->GetPhysicsSystems()) s->OnDrawGizmos(cm);
    for(System* s: scene->GetStandSystems()) s->OnDrawGizmos(cm);
    for(System* s: scene->GetRendererSystems()) s->OnDrawGizmos(cm);

    Editor* editor = Application::GetModuleByType<Editor>();
    if(editor != nullptr){
        if(scene->IsValid(editor->GetSelectionEntity())){
            for(System* s: scene->GetPhysicsSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
            for(System* s: scene->GetStandSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
            for(System* s: scene->GetRendererSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
        }
    }

    //_DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,0,0));

    auto cameraView = scene->GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto e: cameraView){
        auto& c = cameraView.get<CameraComponent>(e);
        auto& t = cameraView.get<TransformComponent>(e);

        c.UpdateCameraData(t, finalColor->Width(), finalColor->Height());
        cm = c.GetCamera(); //Camera cm = c.GetCamera();
        _DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,1,1));
    }

    /*auto meshRenderView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<MeshRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;

        AABB aabb = c.boundingVolume;
        AABB globalAABB = c.GetGlobalAABB(t);

        Vector3 color = Vector3(0,0,1);
        Transform _t = t.ToTransform();
        if(aabb.isOnFrustum(cm.frustum, _t)) color = Vector3(1, 0, 0);

        Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
    }

    auto modelRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>();
    for(auto e: modelRenderView){
        auto& c = modelRenderView.get<ModelRendererComponent>(e);
        auto& t = modelRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        Transform globalTransform = Transform(t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix());

        AABB aabb = c.GetAABB();
        AABB globalAABB = c.GetGlobalAABB(globalTransform);

        Vector3 color = Vector3(0,0,1);
        if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(1, 0, 0);

        Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
    }

    auto skinnedModelRenderView = scene->GetRegistry().view<SkinnedModelRendererComponent, TransformComponent>();
    for(auto e: skinnedModelRenderView){
        auto& c = skinnedModelRenderView.get<SkinnedModelRendererComponent>(e);
        auto& t = skinnedModelRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        Transform globalTransform = Transform(t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix());

        AABB aabb = c.GetAABB();
        AABB globalAABB = c.GetGlobalAABB(globalTransform);

        Vector3 color = Vector3(0,0,1);
        if(aabb.isOnFrustum(cm.frustum, globalTransform)) color = Vector3(1, 0, 0);

        Graphics::DrawWireCube(Mathf::TRS(globalAABB.center, QuaternionIdentity, globalAABB.extents*2.0f), color, 1);
    }

    auto animView = scene->GetRegistry().view<AnimatorComponent, SkinnedModelRendererComponent, TransformComponent>();
    for(auto e: animView){
        auto& s = animView.get<SkinnedModelRendererComponent>(e);
        auto& c = animView.get<AnimatorComponent>(e);
        auto& t = animView.get<TransformComponent>(e);
        if(s.GetModel() == nullptr) continue;

        Transform globalTransform = Transform(
            t.GlobalModelMatrix() * s.localTransform.GetLocalModelMatrix() * s.skeletonTransform.GetLocalModelMatrix() * s.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(0)
        );

        Pose pose;
        if(scene->Running()){ 
            pose = c.controller.GetCurrentPose();
        } else {
            pose = s.GetModel()->skeleton.GetBindPose(); 
        }

        for(int i = 0; i < pose.Size(); i++){
            if(pose.GetParent(i) < 0) continue;
            Vector3 p0 = globalTransform.TransformPoint( pose.GetGlobalTransform(i).LocalPosition() );
            Vector3 p1 = globalTransform.TransformPoint( pose.GetGlobalTransform(pose.GetParent(i)).LocalPosition() );
            Graphics::DrawLine(p0, p1, Vector3(0, 0, 1), 1);

            Graphics::DrawWireCube(Transform(p0, QuaternionIdentity, Vector3(0.05f)).GetLocalModelMatrix(), Vector3(0, 0, 1), 1);
            Graphics::DrawWireCube(Transform(p1, QuaternionIdentity, Vector3(0.025f)).GetLocalModelMatrix(), Vector3(1, 0, 0), 1);
        }
    }

    auto drawGizmosView = scene->GetRegistry().view<GizmosDrawComponent, TransformComponent>();
    for(auto e: drawGizmosView){
        auto& g = drawGizmosView.get<GizmosDrawComponent>(e);
        auto& t = drawGizmosView.get<TransformComponent>(e);
        Graphics::DrawWireCube(Transform(t.Position(), t.Rotation(), g.globalScale).GetLocalModelMatrix(), Vector3(0, 1, 0), 1);
    }
    */

    /*auto navmeshView = scene->GetRegistry().view<NavmeshComponent>();
    for(auto e: navmeshView){
        auto& n = navmeshView.get<NavmeshComponent>(e);
        if(n.navmesh == nullptr) continue;
        n.navmesh->DrawDebug();
    }*/
}

void RenderContext::CleanShadow(Framebuffer* shadowMap, int layer){
    Assert(shadowMap != nullptr);
    //Framebuffer::Bind(*shadowMap, layer);
    Graphics::BeginFramebuffer(*shadowMap, layer);
    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());
    Graphics::Clean(1, 1, 1, 1);
    //Framebuffer::Unbind();
    Graphics::EndFramebuffer();
}

void RenderContext::BeginDrawShadow(Framebuffer* shadowMap, int layer){
    Assert(shadowMap != nullptr);

    //Framebuffer::Bind(*shadowMap, layer);
    Graphics::BeginFramebuffer(*shadowMap, layer);

    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());
    Graphics::Clean(1, 1, 1, 1);
}

void RenderContext::EndDrawShadow(){
    //Framebuffer::Unbind();
    Graphics::EndFramebuffer();
}

void RenderContext::AddDrawShadow(RenderData& data, ShadowDrawingSettings& settings, RendererList& commandBuffer){
    bool isBlend = data.targetMaterial->IsBlend();
    bool isInstancing = data.targetMaterial->EnableInstancingValid();
    if(settings.enableIntancing == false){
        isInstancing = false;
    }

    if(settings.renderQueueRange == RenderQueueRange::Transparent){
        if(isBlend == false) return;
        isInstancing = false;
    }
    if(settings.renderQueueRange == RenderQueueRange::Opaue){
        if(isBlend == true) return;
    }

    if(data.posePalette != nullptr){
        commandBuffer.AddSkinnedDrawCommand({
            data.targetMatrix,
            data.customShadowPass, 
            //data.targetMaterial,
            data.targetMesh,
            data.posePalette
        }, data.distance);
        return;
    }

    if(isInstancing){
        commandBuffer.AddDrawInstancingCommand({
            data.targetMatrix,
            data.customShadowPass, 
            //data.targetMaterial,
            data.targetMesh
        });

    } else {
        commandBuffer.AddDrawCommand({
            data.targetMatrix,
            data.customShadowPass, 
            //data.targetMaterial,
            data.targetMesh,
            data.distance
        }, data.distance);
    } 
}

void RenderContext::DrawShadows(RendererList& commandBuffer, ShadowSplitData& splitData, Ref<Material>& shadowPass){
    OD_PROFILE_SCOPE("RenderContext::DrawShadows");
    //commandBuffer.Sort();
    //commandBuffer.SetOverrideMaterial(shadowPass);

    Material::SetGlobalMatrix4("lightSpaceMatrix", splitData.projViewMatrix);
 
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        //Shader::SetMatrix4("lightSpaceMatrix", splitData.projViewMatrix);
    };
    commandBuffer.Submit();
    commandBuffer.onUpdateMaterial = nullptr;
    commandBuffer.SetOverrideMaterial(nullptr);
}

Vector3 _Plane3Intersect(Plane p1, Plane p2, Plane p3){ //get the intersection point of 3 planes
    return ( ( -p1.distance * math::cross( p2.normal, p3.normal ) ) +
            ( -p2.distance * math::cross( p3.normal, p1.normal ) ) +
            ( -p3.distance * math::cross( p1.normal, p2.normal ) ) ) /
        ( math::dot( p1.normal, math::cross( p2.normal, p3.normal ) ) );
}

// Source: https://forum.unity.com/threads/drawfrustum-is-drawing-incorrectly.208081/
void _DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color = Vector3(1,1,1)){
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
        nearCorners[i] = _Plane3Intersect(camPlanes[4], camPlanes[i], camPlanes[(i + 1) % 4]); //near corners on the created projection matrix
        farCorners[i] = _Plane3Intersect(camPlanes[5], camPlanes[i], camPlanes[(i + 1) % 4]); //far corners on the created projection matrix
    }

    for(int i = 0; i < 4; i++){
        Graphics::DrawLine(model, nearCorners[i], nearCorners[( i + 1 ) % 4], color, 1); //near corners on the created projection matrix
        Graphics::DrawLine(model, farCorners[i], farCorners[( i + 1 ) % 4], color, 1); //far corners on the created projection matrix
        Graphics::DrawLine(model, nearCorners[i], farCorners[i], color, 1); //sides of the created projection matrix
    }
}

std::vector<Vector4> getFrustumCornersWorldSpace2(const Matrix4& proj, const Matrix4& view){
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

glm::mat4 getLightSpaceMatrix2(Camera& cam, Vector3 lightDir, const float nearPlane, const float farPlane, Frustum* outFrustom = nullptr){
    const auto proj = glm::perspective(cam.fov, (float)cam.width / (float)cam.height, nearPlane, farPlane);
    const auto corners = getFrustumCornersWorldSpace2(proj, cam.view);

    glm::vec3 center = glm::vec3(0, 0, 0);
    for(const auto& v : corners){
        center += glm::vec3(v);
    }
    center /= corners.size();

    const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

    if(outFrustom != nullptr){
        //*outFrustom = CreateFrustumFromMatrix(lightView,proj);
        Matrix4 viewProj = lightView * proj;
        *outFrustom = CreateFrustumFromMatrix2(math::transpose( proj * lightView ));
    }

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

void ShadowSplitData::SetupCascade(ShadowSplitData* splitData, int count, Camera& cam, Transform& light, std::vector<float>& shadowCascadeLevels){
    //float shadowDistance = cam.farClip;
    //shadowDistance = 500;

    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f, cam.farClip };
    //std::vector<float> shadowCascadeLevels{ cam.farClip/50.0f, cam.farClip/25.0f, cam.farClip/10.0f };
    //std::vector<float> shadowCascadeLevels{ shadowDistance*0.1f, shadowDistance*0.25f, shadowDistance*0.5f, shadowDistance*1.0f };

    Assert(shadowCascadeLevels.size() == count);
    
    Vector3 lightDir = -light.Forward();
    float cameraNearPlane = cam.nearClip;
    float cameraFarPlane = cam.farClip;

    std::vector<glm::mat4> lightMatrixs;
    std::vector<Frustum> frustums;

    for(size_t i = 0; i < shadowCascadeLevels.size(); ++i){
        Frustum frustum;

        if(i == 0){
            lightMatrixs.push_back(getLightSpaceMatrix2(cam, lightDir, cameraNearPlane, shadowCascadeLevels[i], &frustum));
            frustums.push_back(frustum);
        }else if (i < shadowCascadeLevels.size()){
            lightMatrixs.push_back(getLightSpaceMatrix2(cam, lightDir, shadowCascadeLevels[i - 1], shadowCascadeLevels[i], &frustum));
            frustums.push_back(frustum);
        }
    }

    Assert(lightMatrixs.size() == count);

    for(int i = 0; i < count; i++){
        splitData[i].projViewMatrix = lightMatrixs[i];
        //splitData[i].splitDistance = shadowCascadeLevels[i];
        splitData[i].frustum = frustums[i];
        splitData[i].frustum = CreateFrustumFromMatrix2(math::transpose( lightMatrixs[i] ));
    }
}

void ShadowSplitData::ComputeSpotShadowData(ShadowSplitData* splitData, LightComponent& light, Transform& transform){
    auto lightProjection = glm::perspective(Mathf::Deg2Rad(light.coneAngleOuter*2), 1.0f, 0.1f, light.radius);
    auto lightView = glm::lookAt(transform.LocalPosition(), transform.LocalPosition() - (-transform.Forward()), Vector3Up);

    splitData->projViewMatrix = lightProjection * lightView;
    splitData->frustum = CreateFrustumFromMatrix2(math::transpose(splitData->projViewMatrix));
}

void ShadowSplitData::ComputePointShadowData(ShadowSplitData* splitData, LightComponent& light, Transform& transform){
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light.radius); 
    Vector3 lightPos = transform.LocalPosition();

    std::vector<glm::mat4> shadowMats;
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
    shadowMats.push_back(
        shadowProj * 
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

    for(int i = 0; i < 6; i++){
        splitData[i].projViewMatrix = shadowMats[i];
        splitData[i].frustum = CreateFrustumFromMatrix2(math::transpose(shadowMats[i]));
    }
}

}