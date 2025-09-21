#include "RenderContext.h"
#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "ModelRendererComponent.h"
#include "SpriteRendererComponent.h"
#include "StaticRendererClusterComponent.h"
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
        {FramebufferTextureFormat::RED_INTEGER}
    };
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 1;
    entityIdOutColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.colorAttachments = {
        {FramebufferTextureFormat::RGBA32F}, 
        //{FramebufferTextureFormat::RGBA8}//, 
        //{FramebufferTextureFormat::RED_INTEGER}
    };
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 1;
    forwardOutColor = new Framebuffer(framebufferSpecification);
    //forwardOutColor = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());

    framebufferSpecification.colorAttachments = {
        {FramebufferTextureFormat::RGB32F}, // Pos
        {FramebufferTextureFormat::RGB32F}, // Normal
        {FramebufferTextureFormat::RGBA16F}, // Albedo
        {FramebufferTextureFormat::RGB16F}, // Emission
        {FramebufferTextureFormat::RGB16F}//, // Spec, Metalic, AO
        //{FramebufferTextureFormat::RED_INTEGER} // Object ID
    };
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 1;
    deferredOutColor = new Framebuffer(framebufferSpecification);
    deferredOutColor->ColorAttachmentId(3);
    //deferredOutColor = new Framebuffer(FramebufferType::Deffered, Application::ScreenWidth(), Application::ScreenHeight());

    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.colorAttachments = {
        {FramebufferTextureFormat::RGBA32F}
        //{FramebufferTextureFormat::RGBA8}
    };
    framebufferSpecification.sample = 1;
    finalColor = new Framebuffer(framebufferSpecification);
    postFx1 = new Framebuffer(framebufferSpecification);
    postFx2 = new Framebuffer(framebufferSpecification);
    //finalColor = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());
    //postFx1 = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());
    //postFx2 = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());

    entityIdShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/EntityId.shader"));

    blitShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Blit.glsl"));
    //deferredGBufferShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredGBuffer.glsl"));
    //deferredLightPassShader = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredLightPassLit.glsl"));
    deferredLightPass = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredLightPassLit.glsl"));

    deferredLightDirSinglePass = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredLightPassSingleLit.glsl"));
    deferredLightDirSingleOtherPass = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/DeferredLightPassSingleOtherLit.glsl"));

    skyboxMesh = Mesh::SkyboxCube();
    spriteMesh = Mesh::CenterQuad(false);
    fullScreenQuad = Mesh::FullScreenQuad();

    sphereMesh = CreateRef<Model>();
    Model::CreateFromFile(*sphereMesh, "Engine/Models/Sphere.obj", {nullptr, 1, false});

    coneMesh = CreateRef<Model>();
    Model::CreateFromFile(*coneMesh, "Engine/Models/Cone.obj", {nullptr, 1, false});

    pipelineDataBuffer = UniformBuffer::Create();

    //meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    //meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>();
}

RenderContext::~RenderContext(){
    delete entityIdOutColor;
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

    entityIdOutColor->Resize(width, height);
    deferredOutColor->Resize(width, height);
    forwardOutColor->Resize(width, height);
    finalColor->Resize(width, height);
    postFx1->Resize(width, height);
    postFx2->Resize(width, height);
}

void RenderContext::BeginDrawEntityIds(){
    Graphics::BeginFramebuffer(*entityIdOutColor);
    ScreenClean();
}

void RenderContext::EndDrawEntityIds(){
    Graphics::EndFramebuffer();
}

void RenderContext::DrawEntityIds(RendererList& commandBuffer){
    OD_PROFILE_SCOPE("RenderContext::DrawRenderersBuffer");

    //if(sort) commandBuffer.Sort();
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        //if(material.GetShader() == nullptr) return;
        //SetStandUniforms(cam, *material.GetShader()); 

        //Graphics::SetDepthTest(DepthTest::EQUAL);

        /*if(deferred){
            material.EnableKeyword("Deferred");
        } else {
            material.EnableKeyword("Forward");
        }*/
    };
    commandBuffer.overrideMaterial = entityIdShader;
    commandBuffer.Submit(false);
    //commandBuffer.onUpdateMaterial = nullptr;
}

int RenderContext::ReadPixeIntFromEntityIdsFramebuffer(int x, int y){
    return entityIdOutColor->ReadPixel(0, x, y);
}

void RenderContext::BeginForwardPass(){
    Graphics::BeginFramebuffer(*forwardOutColor);
    ScreenClean();
}

void RenderContext::EndForwardPass(){
    Graphics::EndFramebuffer();
}

void RenderContext::BeginDeferredPass(){
    //Assert(false);
    //Framebuffer::Bind(*deferredOutColor);
    Graphics::BeginFramebuffer(*deferredOutColor);
    ScreenClean();
}

void RenderContext::EndDeferredPass(){
    Graphics::EndFramebuffer();
}

void RenderContext::DeferredCopyToForwardPass(){
    Graphics::BlitFramebuffer(deferredOutColor, forwardOutColor, -1);
}

void RenderContext::DrawDeferredLight(int index){
    if(index < 0){
        /*deferredLightPass->SetTexture("gPosition", deferredOutColor, 0);
        deferredLightPass->SetTexture("gNormal", deferredOutColor, 1);
        deferredLightPass->SetTexture("gAlbedoSpec", deferredOutColor, 2);
        deferredLightPass->SetTexture("gEmission", deferredOutColor, 3);
        deferredLightPass->SetTexture("gOther", deferredOutColor, 4);
        Graphics::BindMaterial(*deferredLightPass);
        Graphics::DrawMesh(*fullScreenQuad, *deferredLightPass, Matrix4Identity);*/

        deferredLightDirSinglePass->EnableKeyword("INDIRECT");
        deferredLightDirSinglePass->SetTexture("gPosition", deferredOutColor, 0);
        deferredLightDirSinglePass->SetTexture("gNormal", deferredOutColor, 1);
        deferredLightDirSinglePass->SetTexture("gAlbedoSpec", deferredOutColor, 2);
        deferredLightDirSinglePass->SetTexture("gEmission", deferredOutColor, 3);
        deferredLightDirSinglePass->SetTexture("gOther", deferredOutColor, 4);
        Graphics::DrawMesh(*fullScreenQuad, *deferredLightDirSinglePass, Matrix4Identity);
    } else {
        deferredLightDirSinglePass->EnableKeyword("DIRECTIONAL");
        deferredLightDirSinglePass->SetTexture("gPosition", deferredOutColor, 0);
        deferredLightDirSinglePass->SetTexture("gNormal", deferredOutColor, 1);
        deferredLightDirSinglePass->SetTexture("gAlbedoSpec", deferredOutColor, 2);
        deferredLightDirSinglePass->SetTexture("gEmission", deferredOutColor, 3);
        deferredLightDirSinglePass->SetTexture("gOther", deferredOutColor, 4);
        deferredLightDirSinglePass->SetInt("lightIndex", index);
        Graphics::DrawMesh(*fullScreenQuad, *deferredLightDirSinglePass, Matrix4Identity);
    }
}

void RenderContext::DrawDeferredLightOther(int index, Vector3 pos, Vector3 dir, float size, bool isCone){
    //deferredLightDirSingleOtherPass->EnableKeyword("OTHER");
    deferredLightDirSingleOtherPass->SetTexture("gPosition", deferredOutColor, 0);
    deferredLightDirSingleOtherPass->SetTexture("gNormal", deferredOutColor, 1);
    deferredLightDirSingleOtherPass->SetTexture("gAlbedoSpec", deferredOutColor, 2);
    deferredLightDirSingleOtherPass->SetTexture("gEmission", deferredOutColor, 3);
    deferredLightDirSingleOtherPass->SetTexture("gOther", deferredOutColor, 4);
    deferredLightDirSingleOtherPass->SetInt("lightIndex", index);
    deferredLightDirSingleOtherPass->SetFloat("screenWidth", cam.width);
    deferredLightDirSingleOtherPass->SetFloat("screenHeight", cam.height);

    glm::mat4 rotation = glm::mat4(1.0f); // identidade como fallback
    if(glm::length(dir) > 1e-4f){
        rotation = glm::toMat4(glm::quatLookAt(dir, Vector3Up));
    }
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(size));
    Matrix4 worldMatrix = translation * rotation * scale;

    Graphics::DrawMesh(isCone ? *coneMesh->meshs[0] : *sphereMesh->meshs[0], *deferredLightDirSingleOtherPass, worldMatrix);
}

void RenderContext::EndDeferredPassAndCopyToForwardPass(){
    EndDeferredPass();

    //Framebuffer::Bind(*forwardOutColor);
    Graphics::BeginFramebuffer(*forwardOutColor);
    Graphics::Clean(0, 1, 0, 1);

    deferredLightPass->SetTexture("gPosition", deferredOutColor, 0);
    deferredLightPass->SetTexture("gNormal", deferredOutColor, 1);
    deferredLightPass->SetTexture("gAlbedoSpec", deferredOutColor, 2);
    deferredLightPass->SetTexture("gEmission", deferredOutColor, 3);
    deferredLightPass->SetTexture("gOther", deferredOutColor, 4);

    Graphics::DrawMesh(*fullScreenQuad, *deferredLightPass, Matrix4Identity);
    
    Graphics::BlitFramebuffer(deferredOutColor, forwardOutColor, -1);
    //Framebuffer::Bind(*forwardOutColor);
    
    //Graphics::BeginFramebuffer(*forwardOutColor);

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
    Graphics::BeginFramebuffer(*finalColor);
    blitShader->SetTexture("mainTex", forwardOutColor, 0);
    Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();

    if(overrideFramebuffer != nullptr){
        Graphics::BeginFramebuffer(*overrideFramebuffer);
        blitShader->SetTexture("mainTex", finalColor, 0);
        Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
        Graphics::EndFramebuffer();
        Graphics::BeginRenderToScreen();
        Graphics::EndRenderToScreen();
    } else {
        Graphics::BeginRenderToScreen();
        blitShader->SetTexture("mainTex", finalColor, 0);
        Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
        Graphics::EndRenderToScreen();
    }
    return;

    /*Graphics::EndFramebuffer();
    Graphics::BeginRenderToScreen();
    blitShader->SetTexture("mainTex", forwardOutColor, 0);
    Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
    Graphics::EndRenderToScreen();
    return;*/

    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*overrideFramebuffer);
    blitShader->SetTexture("mainTex", forwardOutColor, 0);
    Graphics::DrawMesh(*fullScreenQuad, *blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginRenderToScreen();
    Graphics::EndRenderToScreen();
    return;

    //Graphics::DrawQuadPostProcessing(forwardOutColor, nullptr, *blitShader);
    //Graphics::EndFramebuffer();
    //return;
    
    //Framebuffer::Unbind();
    //Graphics::EndFramebuffer();
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


Framebuffer* finalFramebuffer;

void RenderContext::DrawPostFXs(std::vector<PostFX*>& postFXs){
    //Graphics::SetDepthMask(false);

    bool step = false;
    /*Framebuffer**/ finalFramebuffer = postFx1;
    Graphics::BlitFramebuffer(forwardOutColor, postFx1);
    //Graphics::BlitQuadPostProcessing(outColor, postFx1, *blitShader);

    for(auto i: postFXs){
        finalFramebuffer = step == false ? postFx2 : postFx1;

        if(i->enable){
            i->OnRenderImage(
                step == false ? postFx1 : postFx2, 
                step == false ? postFx2 : postFx1,
                this
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

    //Graphics::DrawQuadPostProcessing(finalFramebuffer, forwardOutColor, *blitShader);
    /*Graphics::BeginFramebuffer(*forwardOutColor);
    blitShader->SetTexture("mainTex", finalFramebuffer, 0);
    Graphics::DrawFullScreenQuad(*blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();*/
}

void RenderContext::BeginUIPass(){
    Graphics::BeginFramebuffer(*forwardOutColor);
    blitShader->SetTexture("mainTex", finalFramebuffer, 0);
    Graphics::DrawFullScreenQuad(*blitShader, Matrix4Identity);

    /*auto uiCamera = Camera{
        OD::Matrix4Identity, 
        OD::math::ortho(0.0f, (float)cam.width, 0.0f, (float)cam.height, -10.0f, 10.0f)
        //OD::math::ortho(0.0f, (float)cam.width, (float)cam.height, 0.0f, -10.0f, 10.0f)
    };
    Graphics::SetCamera(uiCamera);*/
    //Graphics::SetCamera(cam);
    //Graphics::CleanDepthOnly();
}

void RenderContext::EndUIPass(){
    Graphics::EndFramebuffer();
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

void RenderContext::RunComputeRenderListShadow(ComputeRenderListSettings settings, ShadowDrawingSettings drawSettings, RendererList& renderList, Material* shadowPass){
    RenderDataLoop2([&](auto& data){
        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) return;
        if(data.customShadowPass == nullptr) data.customShadowPass = shadowPass;
        AddDrawShadow(data, drawSettings, renderList);
    });
}

void RenderContext::RunComputeRenderList(ComputeRenderListSettings settings, DrawingSettings drawSettings, RendererList& renderList){
    RenderDataLoop2([&](auto& data){
        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) return;
        AddDrawRenderers(data, drawSettings, renderList);
    });

    /*auto staticMeshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: staticMeshView){
        auto& info = staticMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = staticMeshView.get<MeshRendererComponent>(e);
        auto& t = staticMeshView.get<TransformComponent>(e);
        auto& s = staticMeshView.get<StaticRendererComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        if(s.staticDatas.size() != 1) s.staticDatas.resize(1);
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
        
        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
        AddDrawRenderers(data, drawSettings, renderList);
    }

    auto meshStaticRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: meshStaticRenderView){
        auto& info = meshStaticRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshStaticRenderView.get<ModelRendererComponent>(e);
        auto& t = meshStaticRenderView.get<TransformComponent>(e);
        auto& s = meshStaticRenderView.get<StaticRendererComponent>(e);
    
        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        if(s.staticDatas.size() != model->renderTargets.size()){
            s.staticDatas.resize(model->renderTargets.size());
            for(auto& i: s.staticDatas) i.isDirt = true;
        }

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            if(s.staticDatas[_i].isDirt){
                s.staticDatas[_i].isDirt = false;
                s.staticDatas[_i].m = t.GlobalModelMatrix();
                s.staticDatas[_i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), s.staticDatas[_i].m);
            }

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  s.staticDatas[_i].m;
            data.aabb = s.staticDatas[_i].aabb;
            data.posePalette = nullptr;
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            _i += 1;

            if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
            AddDrawRenderers(data, drawSettings, renderList);
        }
    }

    auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable>
    );
    for(auto e: meshView){
        auto& info = meshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix = t.GlobalModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
        AddDrawRenderers(data, drawSettings, renderList);
    }

    auto meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable>
    );
    for(auto e: meshRenderView){
        auto& info = meshRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //if(c.renderData.size() != model->renderTargets.size()) continue;

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix = t.GlobalModelMatrix()  * c.localTransform.GetLocalModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.posePalette = nullptr;
            //data.aabb = c.GetGlobalAABB(t);
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix); //Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());
            //data.aabb.Expand2(Vector3(5.5f));
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            _i += 1;

            if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
            AddDrawRenderers(data, drawSettings, renderList);
        }
    }

    auto skinnedMeshView = GetScene()->GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: skinnedMeshView){
        auto& info = skinnedMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        SkinnedMeshRendererComponent& c = skinnedMeshView.get<SkinnedMeshRendererComponent>(e);
        TransformComponent& t = skinnedMeshView.get<TransformComponent>(e);

        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  t.GlobalModelMatrix();;
        //data.transform = Transform(data.targetMatrix); //t.ToTransform();
        
        //INFO: Try optimize
        if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
            c.finalPose.GetMatrixPalette(c.posePalette, c.skeleton.GetInvBindPose());
        } 
        data.posePalette = &c.posePalette;
        
        //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = (int)e;

        if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
        AddDrawRenderers(data, drawSettings, renderList);
    }

    auto skinnedView = GetScene()->GetRegistry().view<SkinnedModelRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto [e, c, t, info]: skinnedView.each()){
        //auto& info = skinnedView.get<InfoComponent>(e);
        if(info.enable == false) continue;
        //SkinnedModelRendererComponent& c = skinnedView.get<SkinnedModelRendererComponent>(e);
        //TransformComponent& t = skinnedView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //TODO: Revisar isto, fix temporariamente o model nao esta send renderizando sem chama UpdatePosePalette
        if(c.posePalette.size() == 0) c.UpdatePosePalette();

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;
            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            //data.transform = Transform(data.targetMatrix); //t.ToTransform();
            
            //INFO: Try optimize
            if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
                c.finalPose.GetMatrixPalette(c.posePalette, model->skeleton.GetInvBindPose()); 
            }
            data.posePalette = &c.posePalette;
            
            //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);//Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());

            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = (int)e;
            
            _i += 1;

            if(settings.checkOnFrustum && data.aabb.isOnFrustum(settings.frustum) == false) continue;
            AddDrawRenderers(data, drawSettings, renderList);
        }
    }*/
}

void RenderContext::RenderDataLoop2(std::function<void(RenderData&)> onReciveRenderData){
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop");

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::0");
    /*auto staticMeshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: staticMeshView){
        auto& info = staticMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = staticMeshView.get<MeshRendererComponent>(e);
        auto& t = staticMeshView.get<TransformComponent>(e);
        auto& s = staticMeshView.get<StaticRendererComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        if(s.staticDatas.size() != 1) s.staticDatas.resize(1);
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
        
        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }*/
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::1");
    /*auto meshStaticRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto e: meshStaticRenderView){
        auto& info = meshStaticRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshStaticRenderView.get<ModelRendererComponent>(e);
        auto& t = meshStaticRenderView.get<TransformComponent>(e);
        auto& s = meshStaticRenderView.get<StaticRendererComponent>(e);
    
        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        if(s.staticDatas.size() != model->renderTargets.size()){
            s.staticDatas.resize(model->renderTargets.size());
            for(auto& i: s.staticDatas) i.isDirt = true;
        }

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            if(s.staticDatas[_i].isDirt){
                s.staticDatas[_i].isDirt = false;
                s.staticDatas[_i].m = t.GlobalModelMatrix();
                s.staticDatas[_i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), s.staticDatas[_i].m);
            }

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  s.staticDatas[_i].m;
            data.aabb = s.staticDatas[_i].aabb;
            data.posePalette = nullptr;
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            onReciveRenderData(data);
            _i += 1;
        }
    }*/
    //}

    //////////////////////////////////////////////////////////

    auto staticRendererClusterView = scene->GetRegistry().view<StaticRendererClusterComponent, TransformComponent, InfoComponent>(
        entt::exclude<HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto [entity, c, t, i]: staticRendererClusterView.each()){
        for(auto& chunk: c.chunks){
            if(chunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

            for(auto& subchunk: chunk.subchunks){
                if(subchunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

                for(auto& renderTarget: subchunk.targets){
                    RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = renderTarget.targetMaterial.get();
                    data.customShadowPass = nullptr;
                    data.targetMesh = renderTarget.targetMesh.get();
                    data.targetMatrix = renderTarget.targetMatrix;
                    data.posePalette = nullptr;
                    data.aabb = renderTarget.aabb;

                    data.perDrawData.int_0.resize(1);
                    data.perDrawData.int_0[0] = 0;

                    #if EnableExperimentalPerDrawCustomData
                    data.useCustomData = c.useCustomData;
                    data.customData = c.customData;
                    #endif

                    onReciveRenderData(data);
                }

                subchunk.drawIntancingCommands.Each([&](DrawInstancingCommand2& cmd){
                    RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = cmd.material;
                    data.customShadowPass = nullptr;
                    data.targetMesh = cmd.meshs;
                    data.targetMatrix = Matrix4Identity;
                    data.posePalette = nullptr;
                    data.aabb = subchunk.renderBounds;
                    data.perDrawData.int_0.clear();
                    data.instancingBuffer = cmd.buffer.get();
                    
                    onReciveRenderData(data);
                });
            }
        }
    }

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::2");
    auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto e: meshView){
        const auto& info = meshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        const auto& c = meshView.get<MeshRendererComponent>(e);
        const auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix = t.GlobalModelMatrixReadSafe();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::3");
    auto meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto e: meshRenderView){
        const auto& info = meshRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        const auto& t = meshRenderView.get<TransformComponent>(e);

        if(c.draw == false) continue;

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //if(c.renderData.size() != model->renderTargets.size()) continue;

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix = t.GlobalModelMatrixReadSafe() /** c.localTransform.GetLocalModelMatrix()*/ * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.posePalette = nullptr;
            //data.aabb = c.GetGlobalAABB(t);
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix); //Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrixReadSafe());
            //data.aabb.Expand2(Vector3(5.5f));
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            onReciveRenderData(data);
            _i += 1;
        }
    }
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::4");
    auto skinnedMeshView = GetScene()->GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: skinnedMeshView){
        const auto& info = skinnedMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        SkinnedMeshRendererComponent& c = skinnedMeshView.get<SkinnedMeshRendererComponent>(e);
        const TransformComponent& t = skinnedMeshView.get<TransformComponent>(e);

        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
        data.targetMaterial = c.material.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  t.GlobalModelMatrixReadSafe();
        //data.transform = Transform(data.targetMatrix); //t.ToTransform();
        
        //INFO: Try optimize
        /*if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
            c.finalPose.GetMatrixPalette(c.posePalette, c.skeleton.GetInvBindPose());
        }*/ 
        data.posePalette = &c.posePalette;
        
        //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = (int)e;

        onReciveRenderData(data);
    }
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::5");
    auto skinnedView = GetScene()->GetRegistry().view<SkinnedModelRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: skinnedMeshView){
        const auto& info = skinnedView.get<InfoComponent>(e);
        if(info.enable == false) continue;
        SkinnedModelRendererComponent& c = skinnedView.get<SkinnedModelRendererComponent>(e);
        const TransformComponent& t = skinnedView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //TODO: Revisar isto, fix temporariamente o model nao esta send renderizando sem chama UpdatePosePalette
        //if(c.posePalette.size() == 0) c.UpdatePosePalette();

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;
            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  t.GlobalModelMatrixReadSafe() /** c.localTransform.GetLocalModelMatrix()*/ * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            //data.transform = Transform(data.targetMatrix); //t.ToTransform();
            
            //INFO: Try optimize
            /*if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
                c.finalPose.GetMatrixPalette(c.posePalette, model->skeleton.GetInvBindPose()); 
            }*/
            data.posePalette = &c.posePalette;
            
            //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);//Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);

            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = (int)e;
            if(c.updateWhenOffscreen) data.awalsDraw = true;
            
            onReciveRenderData(data);
            _i += 1;
        }
    }
    //}

    //{
    //OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::6");
    /*auto spriteView = GetScene()->GetRegistry().view<TransformComponent, SpriteRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable>);
    for(auto entity: spriteView){
        auto& info = spriteView.get<InfoComponent>(entity);
        if(info.enable == false) continue;

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
    }*/
    //}
}

void RenderContext::RenderDataLoop(std::function<void(RenderData&)> onReciveRenderData){
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop");

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::-1");
    auto staticRendererClusterView = scene->GetRegistry().view<StaticRendererClusterComponent, TransformComponent, InfoComponent>(
        entt::exclude<HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto [entity, c, t, i]: staticRendererClusterView.each()){
        for(auto& chunk: c.chunks){
            //if(chunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

            /*
            for(auto* r : receivers){
                uint32_t wants = r->Wants(...);
                if(wants){ 
                    interested.push_back(r);
                }
            }
            */

            for(auto& subchunk: chunk.subchunks){
                //if(subchunk.renderBounds.isOnFrustum(cam.frustum) == false) continue;

                /*
                for(auto* r : interested){
                    uint32_t wants = r->Wants(...);
                    if(wants == false){ 
                        interested.remove(r);
                    }
                }
                */

                for(auto& renderTarget: subchunk.targets){
                    RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = renderTarget.targetMaterial.get();
                    data.customShadowPass = nullptr;
                    data.targetMesh = renderTarget.targetMesh.get();
                    data.targetMatrix = renderTarget.targetMatrix;
                    data.posePalette = nullptr;
                    data.aabb = renderTarget.aabb;

                    /*
                    for(auto* r : interested){
                        uint32_t wants = r->Wants(...);
                        if(wants == false){ 
                            interested.remove(r);
                        }
                    }
                    */

                    data.perDrawData.int_0.resize(1);//TODO: Optimaze this, this can be make heap allocation
                    data.perDrawData.int_0[0] = 0;

                    #if EnableExperimentalPerDrawCustomData
                    data.useCustomData = c.useCustomData;
                    data.customData = c.customData;
                    #endif

                    onReciveRenderData(data);
                }

                subchunk.drawIntancingCommands.Each([&](DrawInstancingCommand2& cmd){
                    RenderData data;
                    data.distance = math::distance2(cam.viewPos, t.PositionReadSafe());
                    data.targetMaterial = cmd.material;
                    data.customShadowPass = nullptr;
                    data.targetMesh = cmd.meshs;
                    data.targetMatrix = Matrix4Identity;
                    data.posePalette = nullptr;
                    data.aabb = subchunk.renderBounds;
                    data.perDrawData.int_0.clear();
                    data.instancingBuffer = cmd.buffer.get();
                    
                    onReciveRenderData(data);
                });
            }
        }
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::0");
    auto staticMeshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: staticMeshView){
        auto& info = staticMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = staticMeshView.get<MeshRendererComponent>(e);
        auto& t = staticMeshView.get<TransformComponent>(e);
        auto& s = staticMeshView.get<StaticRendererComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        if(s.staticDatas.size() != 1) s.staticDatas.resize(1);
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
        
        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        if(c.useCustomData){
            data.perDrawData.vector4_0.resize(1);
            data.perDrawData.vector4_0[0] = c.customData;
        }

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::1");
    auto meshStaticRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, StaticRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: meshStaticRenderView){
        auto& info = meshStaticRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshStaticRenderView.get<ModelRendererComponent>(e);
        if(c.draw == false) continue;

        auto& t = meshStaticRenderView.get<TransformComponent>(e);
        auto& s = meshStaticRenderView.get<StaticRendererComponent>(e);
    
        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        if(s.staticDatas.size() != model->renderTargets.size()){
            s.staticDatas.resize(model->renderTargets.size());
            for(auto& i: s.staticDatas) i.isDirt = true;
        }

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            if(s.staticDatas[_i].isDirt){
                s.staticDatas[_i].isDirt = false;
                s.staticDatas[_i].m = t.GlobalModelMatrix();
                s.staticDatas[_i].aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), s.staticDatas[_i].m);
            }

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  s.staticDatas[_i].m;
            data.aabb = s.staticDatas[_i].aabb;
            data.posePalette = nullptr;
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            if(c.useCustomData){
                data.perDrawData.vector4_0.resize(1);
                data.perDrawData.vector4_0[0] = c.customData;
            }

            #if EnableExperimentalPerDrawCustomData
            data.useCustomData = c.useCustomData;
            data.customData = c.customData;
            #endif

            onReciveRenderData(data);
            _i += 1;
        }
    }
    }

    //////////////////////////////////////////////////////////

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::2");
    auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto e: meshView){
        auto& info = meshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.customShadowPass = c.customShadowPass == nullptr ? nullptr : c.customShadowPass.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix = t.GlobalModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = ((int)e) + 1;

        if(c.useCustomData){
            data.perDrawData.vector4_0.resize(1);
            data.perDrawData.vector4_0[0] = c.customData;
        }

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::3");
    auto meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(
        entt::exclude<StaticRendererComponent, HideInEditor, SelfDisable, SkipDraw>
    );
    for(auto e: meshRenderView){
        auto& info = meshRenderView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        if(c.draw == false) continue;

        auto& t = meshRenderView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //if(c.renderData.size() != model->renderTargets.size()) continue;

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;

            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix = t.GlobalModelMatrix()  * c.localTransform.GetModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.posePalette = nullptr;
            //data.aabb = c.GetGlobalAABB(t);
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix); //Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());
            //data.aabb.Expand2(Vector3(5.5f));
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = ((int)e) + 1;

            if(c.useCustomData){
                data.perDrawData.vector4_0.resize(1);
                data.perDrawData.vector4_0[0] = c.customData;
            }

            #if EnableExperimentalPerDrawCustomData
            data.useCustomData = c.useCustomData;
            data.customData = c.customData;
            #endif

            onReciveRenderData(data);
            _i += 1;
        }
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::4");
    auto skinnedMeshView = GetScene()->GetRegistry().view<SkinnedMeshRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto e: skinnedMeshView){
        auto& info = skinnedMeshView.get<InfoComponent>(e);
        if(info.enable == false) continue;

        SkinnedMeshRendererComponent& c = skinnedMeshView.get<SkinnedMeshRendererComponent>(e);
        TransformComponent& t = skinnedMeshView.get<TransformComponent>(e);

        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.targetMesh = c.mesh.get();
        data.targetMatrix =  t.GlobalModelMatrix();;
        //data.transform = Transform(data.targetMatrix); //t.ToTransform();
        
        //INFO: Try optimize
        if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
            c.finalPose.GetMatrixPalette(c.posePalette, c.skeleton.GetInvBindPose());
        } 
        data.posePalette = &c.posePalette;
        
        //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
        data.aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, data.targetMatrix);

        data.perDrawData.int_0.resize(1);
        data.perDrawData.int_0[0] = (int)e;

        if(c.useCustomData){
            data.perDrawData.vector4_0.resize(1);
            data.perDrawData.vector4_0[0] = c.customData;
        }

        #if EnableExperimentalPerDrawCustomData
        data.useCustomData = c.useCustomData;
        data.customData = c.customData;
        #endif

        onReciveRenderData(data);
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::5");
    auto skinnedView = GetScene()->GetRegistry().view<SkinnedModelRendererComponent, TransformComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto [e, c, t, info]: skinnedView.each()){
        //auto& info = skinnedView.get<InfoComponent>(e);
        if(info.enable == false) continue;
        if(c.draw == false) continue;
        //SkinnedModelRendererComponent& c = skinnedView.get<SkinnedModelRendererComponent>(e);
        //TransformComponent& t = skinnedView.get<TransformComponent>(e);

        Ref<Model> model = c.GetModel();
        if(model == nullptr) continue;

        //TODO: Revisar isto, fix temporariamente o model nao esta send renderizando sem chama UpdatePosePalette
        if(c.posePalette.size() == 0) c.UpdatePosePalette();

        int _i = 0;
        for(auto i: model->renderTargets){
            if(_i < c.GetRenderTargetVisibility().size() && c.GetRenderTargetVisibility()[_i] == false) continue;
            RenderData data;
            data.distance = math::distance2(cam.viewPos, t.Position());
            data.targetMaterial = model->materials[i.materialIndex].get();
            data.targetMesh = model->meshs[i.meshIndex].get();
            data.targetMatrix =  t.GlobalModelMatrix() * c.localTransform.GetModelMatrix() * model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            //data.transform = Transform(data.targetMatrix); //t.ToTransform();
            
            //INFO: Try optimize
            if(c.finalPose.Size() > 0 && c.postUpdatePosePalette){
                c.finalPose.GetMatrixPalette(c.posePalette, model->skeleton.GetInvBindPose()); 
            }
            data.posePalette = &c.posePalette;
            
            //data.aabb = c.GetGlobalAABB(t);// c.GetAABB();
            //data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), data.targetMatrix);//Isto pode esta errado pq o aabb é do model interior, nao por mesh
            data.aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), t.GlobalModelMatrix());

            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex].get();
            }

            data.perDrawData.int_0.resize(1);
            data.perDrawData.int_0[0] = (int)e;

            data.customShadowPass = data.targetMaterial->DepthPass() != -1 ? data.targetMaterial : nullptr; 

            if(c.useCustomData){
                data.perDrawData.vector4_0.resize(1);
                data.perDrawData.vector4_0[0] = c.customData;
            }

            #if EnableExperimentalPerDrawCustomData
            data.useCustomData = c.useCustomData;
            data.customData = c.customData;
            #endif

            if(c.updateWhenOffscreen) data.awalsDraw = true;
            
            onReciveRenderData(data);
            _i += 1;
        }
    }
    }

    {
    OD_PROFILE_SCOPE("RenderContext::RenderDataLoop::6");
    auto spriteView = GetScene()->GetRegistry().view<TransformComponent, SpriteRendererComponent, InfoComponent>(entt::exclude<HideInEditor, SelfDisable, SkipDraw>);
    for(auto entity: spriteView){
        auto& info = spriteView.get<InfoComponent>(entity);
        if(info.enable == false) continue;

        TransformComponent& t = spriteView.get<TransformComponent>(entity);
        SpriteRendererComponent& c = spriteView.get<SpriteRendererComponent>(entity);
        //if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        AABB aabb(Vector3(0), c.sprite->Width() / c.pixelUnitSize, c.sprite->Height() / c.pixelUnitSize, 1);
        Transform scale;
        scale.Scale(Vector3(c.sprite->Width() / c.pixelUnitSize, c.sprite->Height() / c.pixelUnitSize, 1));

        c.material->SetVector4("color", c.color);
        c.material->SetTexture("mainTex", c.sprite);

        RenderData data;
        data.distance = math::distance2(cam.viewPos, t.Position());
        data.targetMaterial = c.material.get();
        data.targetMesh = spriteMesh.get();
        data.targetMatrix =  t.GlobalModelMatrix() * scale.GetModelMatrix();
        data.posePalette = nullptr;
        //data.aabb = c.GetGlobalAABB(t);
        data.aabb = transform_aabb_optimized_abs_center_extents(aabb, data.targetMatrix);

        onReciveRenderData(data);
    }
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
            data.posePalette,
            data.perDrawData
        }, data.distance);
        return;
    }

    if(isInstancing){
        if(data.instancingBuffer != nullptr){
            target.AddDrawInstancingCommand({
                data.instancingBuffer, data.targetMaterial, data.targetMesh
            });
        } else{
            target.AddDrawInstancingCommand({
                data.targetMatrix,
                data.targetMaterial,
                data.targetMesh,
            });
        }
    } else {
        target.AddDrawCommand({
            data.targetMatrix,
            data.targetMaterial,
            data.targetMesh,
            data.distance,
            data.perDrawData
            /*#if EnableExperimentalPerDrawCustomData
            data.useCustomData,
            data.customData,
            #endif*/
        }, data.distance);
    } 
}

void RenderContext::RenderSkyboxLater(){
    OD_PROFILE_SCOPE("RenderContext::RenderSkybox"); 
    if(skyMaterial == nullptr) return;
    
    Assert(skyMaterial->GetShader() != nullptr);

    //skyMaterial->UpdateDatas();
    //Material::SubmitGraphicDatas(*skyMaterial);

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

        if(material.MainPass() != -1) material.SetPass(material.MainPass());

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
    commandBuffer.Submit();
}

void _DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color);

void RenderContext::DrawGizmos(){
    OD_PROFILE_SCOPE("RenderContext::DrawGizmos"); 

    if(SceneManager::Get().GetActiveScene() == nullptr) return;
    if(SceneManager::Get().GetActiveScene()->Running() && settings.enableGizmosRuntime == false) return;
    if(SceneManager::Get().GetActiveScene()->Running() == false && settings.enableGizmos == false) return;

    //Renderer::SetCamera(cam);
    
    Camera cm = cam;
    
    //scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    //FIXME: This could be call OnDrawGizmos Twice
    /*for(System* s: scene->GetPhysicsSystems()) s->OnDrawGizmos(cm);
    for(System* s: scene->GetStandSystems()) s->OnDrawGizmos(cm);
    for(System* s: scene->GetLateSystems()) s->OnDrawGizmos(cm);
    for(System* s: scene->GetRendererSystems()) s->OnDrawGizmos(cm);*/
    for(auto& s: scene->GetSystems()) s.second->OnDrawGizmos(cm);

    Editor* editor = Application::GetModuleByType<Editor>();
    if(editor != nullptr){
        //if(scene->IsValid(editor->GetSelectionEntity())){
            /*for(System* s: scene->GetPhysicsSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
            for(System* s: scene->GetStandSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
            for(System* s: scene->GetLateSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
            for(System* s: scene->GetRendererSystems()) s->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());*/
        //    for(auto& s: scene->GetSystems()) s.second->OnDrawGizmosSelected(cm, editor->GetSelectionEntity());
        //}

        for(auto& i: editor->GetSelectedEntities()){
            if(scene->IsValid(i) == false) continue;
            for(auto& s: scene->GetSystems()) s.second->OnDrawGizmosSelected(cm, i);
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
    Graphics::BeginFramebuffer(*shadowMap, Vector4(0, 0, 0, 1), layer);
    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());
    Graphics::Clean(1, 1, 1, 1);
    //Framebuffer::Unbind();
    Graphics::EndFramebuffer();
}

void RenderContext::BeginDrawShadow(Framebuffer* shadowMap, int layer){
    Assert(shadowMap != nullptr);

    //Framebuffer::Bind(*shadowMap, layer);
    Graphics::BeginFramebuffer(*shadowMap, Vector4(0, 0, 0, 1), layer);
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
            data.posePalette,
            data.perDrawData
        }, data.distance);
        return;
    }

    if(isInstancing){
        if(data.instancingBuffer != nullptr){
            commandBuffer.AddDrawInstancingCommand({
                data.instancingBuffer, 
                data.customShadowPass, //data.targetMaterial, 
                data.targetMesh
            });
        } else{
            commandBuffer.AddDrawInstancingCommand({
                data.targetMatrix,
                data.customShadowPass, 
                //data.targetMaterial,
                data.targetMesh
            });
        }

    } else {
        /*commandBuffer.AddDrawCommand({
            data.targetMatrix,
            data.customShadowPass, 
            //data.targetMaterial,
            data.targetMesh,
            data.distance
        }, data.distance);*/

        commandBuffer.AddDrawCommand({
            data.targetMatrix,
            data.customShadowPass,
            data.targetMesh,
            data.distance,
            data.perDrawData
            /*#if EnableExperimentalPerDrawCustomData
            data.useCustomData,
            data.customData,
            #endif*/
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
        if(material.DepthPass() != -1){
            material.SetPass(material.DepthPass());
            //LogInfo("Set Shadow Pass of: %s", material.GetShader()->Path().c_str());
        }
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
    constexpr float zMult = 10; //10.0f;
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
        //splitData[i].frustum = frustums[i];
        splitData[i].frustum = CreateFrustumFromMatrix2(math::transpose(lightMatrixs[i]));
    }
}

void ShadowSplitData::ComputeSpotShadowData(ShadowSplitData* splitData, LightComponent& light, Transform& transform){
    auto lightProjection = glm::perspective(Mathf::Deg2Rad(light.coneAngleOuter*2), 1.0f, 0.1f, light.radius);
    auto lightView = glm::lookAt(transform.Position(), transform.Position() - (-transform.Forward()), Vector3Up);

    splitData->projViewMatrix = lightProjection * lightView;
    splitData->frustum = CreateFrustumFromMatrix2(math::transpose(splitData->projViewMatrix));
}

void ShadowSplitData::ComputePointShadowData(ShadowSplitData* splitData, LightComponent& light, Transform& transform){
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light.radius); 
    Vector3 lightPos = transform.Position();

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