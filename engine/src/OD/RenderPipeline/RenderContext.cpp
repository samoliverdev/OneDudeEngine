#include "RenderContext.h"
#include "CameraComponent.h"
#include "MeshRendererComponent.h"
#include "ModelRendererComponent.h"
#include "OD/Animation/Animator.h"
#include "OD/Core/Application.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Defines.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/Navmesh/Navmesh.h"

namespace OD{

RenderContextSettings settings;

RenderContextSettings& RenderContext::GetSettings(){
    return settings;
}

RenderContext::RenderContext(Scene* inScene){
    scene = inScene;

    FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA16F}, {FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
    framebufferSpecification.sample = 2;
    outColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA16F}};
    framebufferSpecification.sample = 1;
    finalColor = new Framebuffer(framebufferSpecification);
    postFx1 = new Framebuffer(framebufferSpecification);
    postFx2 = new Framebuffer(framebufferSpecification);

    blitShader = AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Blit.glsl");

    skyboxMesh = Mesh::SkyboxCube();
}

RenderContext::~RenderContext(){
    delete outColor;
    delete finalColor;
}

void RenderContext::Begin(){

}

void RenderContext::End(){
    Material::CleanGlobalUniformsData();
}

void RenderContext::BeginDrawToScreen(){
    //glEnable(GL_FRAMEBUFFER_SRGB); 

    int width = Application::ScreenWidth();
    int height = Application::ScreenHeight();

    if(overrideFramebuffer != nullptr){
        width = overrideFramebuffer->Width();
        height = overrideFramebuffer->Height();
    }

    outColor->Resize(width, height);
    finalColor->Resize(width, height);
    postFx1->Resize(width, height);
    postFx2->Resize(width, height);

    Framebuffer::Bind(*outColor);
}

void RenderContext::EndDrawToScreen(){
    Graphics::BlitFramebuffer(outColor, finalColor);
    //Graphics::BlitQuadPostProcessing(outColor, finalColor, *blitShader);

    if(overrideFramebuffer != nullptr){
        Graphics::BlitQuadPostProcessing(finalColor, overrideFramebuffer, *blitShader);
    } else {
        Graphics::BlitQuadPostProcessing(finalColor, nullptr, *blitShader);
    }

    Framebuffer::Unbind();

    //glDisable(GL_FRAMEBUFFER_SRGB); 
}

void RenderContext::DrawPostFXs(std::vector<PostFX*>& postFXs){
    bool step = false;
    Framebuffer* finalFramebuffer = postFx1;
    Graphics::BlitFramebuffer(outColor, postFx1);
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

    Graphics::BlitQuadPostProcessing(finalFramebuffer, outColor, *blitShader);
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
}

void RenderContext::ScreenClean(){
    //Renderer::Clean(cam.cleanColor.x, cam.cleanColor.y, cam.cleanColor.z, 1);
    Graphics::Clean(0, 0, 1, 1);
}

void RenderContext::SetupLoop(std::function<void(RenderData&)> onReciveRenderData){
    //globalUniformBuffer->CleanData();

    auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshView){
        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RenderData data;
        data.transform = t.ToTransform();
        data.distance = math::distance(cam.viewPos, t.Position());
        data.targetMaterial = c.material;
        data.targetMesh = c.mesh;
        data.targetMatrix =  t.GlobalModelMatrix();
        data.posePalette = nullptr;
        data.aabb = c.boundingVolume;

        onReciveRenderData(data);
    }

    auto meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;
        //if(c.GetAABB().isOnFrustum(cam.frustum, t) == false) continue;

        for(auto i: c.GetModel()->renderTargets){
            RenderData data;
            data.distance = math::distance(cam.viewPos, t.Position());
            data.targetMaterial = c.GetModel()->materials[i.materialIndex];
            data.targetMesh = c.GetModel()->meshs[i.meshIndex];
            data.targetMatrix =  t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.transform = Transform(data.targetMatrix); //t.ToTransform();
            data.posePalette = nullptr;
            data.aabb = c.GetAABB();
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex];
            }

            onReciveRenderData(data);
        }
    }

    auto skinnedView = GetScene()->GetRegistry().view<SkinnedModelRendererComponent, TransformComponent>();
    for(auto e: skinnedView){
        SkinnedModelRendererComponent& c = skinnedView.get<SkinnedModelRendererComponent>(e);
        TransformComponent& t = skinnedView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;
        //if(c.GetAABB().isOnFrustum(cam.frustum, t) == false) continue;

        for(auto i: c.GetModel()->renderTargets){
            RenderData data;
            data.distance = math::distance(cam.viewPos, t.Position());
            data.targetMaterial = c.GetModel()->materials[i.materialIndex];
            data.targetMesh = c.GetModel()->meshs[i.meshIndex];
            data.targetMatrix =  t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            data.transform = Transform(data.targetMatrix); //t.ToTransform();
            data.posePalette = &c.posePalette;
            data.aabb = c.GetAABB();
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex];
            }

            onReciveRenderData(data);
        }
    }
}

void RenderContext::AddDrawRenderers(RenderData& data, DrawingSettings& settings, CommandBuffer& target){
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
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix,
            data.posePalette
        }, data.distance);
        return;
    }

    if(isInstancing){
        target.AddDrawInstancingCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix
        });

    } else {
        target.AddDrawCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix,
            data.distance
        }, data.distance);
    } 
}

void RenderContext::SetStandUniforms(Camera& cam, Shader& shader){
    //Shader::Bind(shader);
    shader.SetMatrix4("view", cam.view);
    shader.SetMatrix4("projection", cam.projection);
    shader.SetVector3("viewPos", cam.viewPos);
}

void RenderContext::RenderSkybox(){
    OD_PROFILE_SCOPE("RenderContext::RenderSkybox"); 
    
    if(skyMaterial != nullptr){
        Assert(skyMaterial->GetShader() != nullptr);

        //skyMaterial->UpdateDatas();
        Material::SubmitGraphicDatas(*skyMaterial);

        Graphics::SetCullFace(CullFace::BACK);
        Graphics::SetDepthMask(false);
        Graphics::SetBlend(false);
        Shader::Bind(*skyMaterial->GetShader());
        
        //environmentSettings.sky->shader()->SetCubemap("mainTex", *_skyboxCubemap, 0);
        skyMaterial->GetShader()->SetMatrix4("projection", cam.projection);
        Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
        skyMaterial->GetShader()->SetMatrix4("view", skyboxView);
        Graphics::DrawMeshRaw(*skyboxMesh);

        Graphics::SetDepthMask(true);
    }
}

void RenderContext::RenderSkybox(Ref<Cubemap>& skyTexture){
    OD_PROFILE_SCOPE("RenderContext::RenderSkybox"); 
    
    if(skyMaterial != nullptr){
        Assert(skyMaterial->GetShader() != nullptr);

        //skyMaterial->UpdateDatas();
        Material::SetGlobalCubemap("_SpecCube0", skyTexture);
        Material::SubmitGraphicDatas(*skyMaterial);

        Graphics::SetCullFace(CullFace::BACK);
        Graphics::SetDepthMask(false);
        Graphics::SetBlend(false);
        Shader::Bind(*skyMaterial->GetShader());
        
        //environmentSettings.sky->shader()->SetCubemap("mainTex", *_skyboxCubemap, 0);
        skyMaterial->GetShader()->SetMatrix4("projection", cam.projection);
        Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
        skyMaterial->GetShader()->SetMatrix4("view", skyboxView);
        Graphics::DrawMeshRaw(*skyboxMesh);

        Graphics::SetDepthMask(true);
    }
}

void RenderContext::DrawRenderersBuffer(CommandBuffer& commandBuffer){
    commandBuffer.Sort();
    commandBuffer.onUpdateMaterial = [&](Material& material){ 
        if(material.GetShader() == nullptr) return;
        SetStandUniforms(cam, *material.GetShader()); 
    };
    commandBuffer.Submit();
    commandBuffer.onUpdateMaterial = nullptr;
}

void _DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color);

void RenderContext::DrawGizmos(){
    if(SceneManager::Get().GetActiveScene() == nullptr) return;
    if(SceneManager::Get().GetActiveScene()->Running() && settings.enableGizmosRuntime == false) return;
    if(SceneManager::Get().GetActiveScene()->Running() == false && settings.enableGizmos == false) return;

    //Renderer::SetCamera(cam);
    Graphics::SetDepthTest(DepthTest::LESS);
    Graphics::SetCullFace(CullFace::BACK);
    Graphics::SetBlend(false);

    scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    for(System* s: scene->GetStandSystems()){
        s->OnDrawGizmos();
    }

    Camera cm = cam;
    _DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,0,0));

    auto cameraView = scene->GetRegistry().view<CameraComponent, TransformComponent>();
    for(auto e: cameraView){
        auto& c = cameraView.get<CameraComponent>(e);
        auto& t = cameraView.get<TransformComponent>(e);

        c.UpdateCameraData(t, finalColor->Width(), finalColor->Height());
        cm = c.GetCamera(); //Camera cm = c.GetCamera();
        _DrawFrustum(cm.frustum, Matrix4Identity, Vector3(1,1,1));
    }

    auto meshRenderView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
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

    auto navmeshView = scene->GetRegistry().view<NavmeshComponent>();
    for(auto e: navmeshView){
        auto& n = navmeshView.get<NavmeshComponent>(e);
        n.navmesh->DrawDebug();
    }
}

void RenderContext::BeginDrawShadow(Framebuffer* shadowMap, int layer){
    Assert(shadowMap != nullptr);

    Framebuffer::Bind(*shadowMap, layer);

    Graphics::SetViewport(0, 0, shadowMap->Width(), shadowMap->Height());
    Graphics::Clean(1, 1, 1, 1);
}

void RenderContext::EndDrawShadow(){
    Framebuffer::Unbind();
}

void RenderContext::AddDrawShadow(RenderData& data, ShadowDrawingSettings& settings, CommandBuffer& commandBuffer){
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
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix,
            data.posePalette
        }, data.distance);
        return;
    }

    if(isInstancing){
        commandBuffer.AddDrawInstancingCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix
        });

    } else {
        commandBuffer.AddDrawCommand({
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix,
            data.distance
        }, data.distance);
    } 
}

void RenderContext::DrawShadows(CommandBuffer& commandBuffer, ShadowSplitData& splitData, Ref<Material>& shadowPass){
    commandBuffer.Sort();

    commandBuffer.SetOverrideMaterial(shadowPass);

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

}