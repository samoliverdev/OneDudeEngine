#include "RenderContext.h"
#include "MeshRendererComponent.h"
#include "OD/Core/Application.h"
#include "OD/Core/AssetManager.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Defines.h"
#include "OD/Physics/PhysicsSystem.h"

namespace OD{

RenderContext::RenderContext(Scene* inScene){
    scene = inScene;

    FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA16F}, {FramebufferTextureFormat::RED_INTEGER}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
    framebufferSpecification.sample = 2;
    outColor = new Framebuffer(framebufferSpecification);

    framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA8}};
    framebufferSpecification.sample = 1;
    finalColor = new Framebuffer(framebufferSpecification);

    blitShader = AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Blit.glsl");

    skyboxMesh = Mesh::SkyboxCube();
}

RenderContext::~RenderContext(){
    delete outColor;
    delete finalColor;
}

void RenderContext::Begin(){
    int width = Application::ScreenWidth();
    int height = Application::ScreenHeight();

    if(overrideFramebuffer != nullptr){
        width = overrideFramebuffer->Width();
        height = overrideFramebuffer->Height();
    }

    outColor->Resize(width, height);
    finalColor->Resize(width, height);

    Framebuffer::Bind(*outColor);
}

void RenderContext::End(){
    Renderer::BlitFramebuffer(outColor, finalColor);
    //Renderer::BlitQuadPostProcessing(finalColor, nullptr, *blitShader);

    if(overrideFramebuffer != nullptr){
        Renderer::BlitQuadPostProcessing(finalColor, overrideFramebuffer, *blitShader);
    } else {
        Renderer::BlitQuadPostProcessing(finalColor, nullptr, *blitShader);
    }

    finalColor->Unbind();
    outColor->Unbind();
    if(overrideFramebuffer != nullptr) overrideFramebuffer->Unbind();
}

void RenderContext::SetupCameraProperties(Camera inCam){
    cam = inCam;

    Renderer::SetCamera(cam);
    Renderer::SetViewport(
        0, 
        0, 
        cam.width, 
        cam.height
    );
}

void RenderContext::SetupRenderers(DrawingTarget* targets, int count){
    for(int j = 0; j < count; j++){
        DrawingTarget* target = &targets[j];
        target->commandBuffer.Clean();
    }

    auto meshRenderView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<MeshRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;
        //if(c.GetGlobalAABB(t).isOnFrustum(cam.frustum, t) == false) continue;

        for(auto i: c.GetModel()->renderTargets){
            CommandBaseData data;
            data.distance = math::distance(cam.viewPos, t.Position());
            data.targetMaterial = c.GetModel()->materials[i.materialIndex];
            data.targetMesh = c.GetModel()->meshs[i.meshIndex];
            data.targetMatrix =  t.GlobalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
            if(i.materialIndex < c.GetMaterialsOverride().size() && c.GetMaterialsOverride()[i.materialIndex] != nullptr){
                data.targetMaterial = c.GetMaterialsOverride()[i.materialIndex];
            }

            globalShaders.insert(data.targetMaterial->GetShader());

            for(int j = 0; j < count; j++){
                SetupDrawTarget(data, targets[j]);
            }
        }
    }
}

void RenderContext::SetupDrawTarget(CommandBaseData& data, DrawingTarget& target){
    MaterialBind2 bind = {
        data.distance,
        data.targetMaterial->MaterialId()
    };

    bool isBlend = data.targetMaterial->IsBlend();
    bool isInstancing = data.targetMaterial->EnableInstancingValid();
    if(target.settings.enableIntancing == false){
        isInstancing = false;
    }

    if(target.settings.sortType == SortType::None){
        target.commandBuffer.drawCommands.sortFunction = nullptr;
    }
    if(target.settings.sortType == SortType::CommonOpaque){
        target.commandBuffer.drawCommands.sortFunction = [](auto& a, auto& b){
            if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            return a.first.distance < b.first.distance;
        };
    }
    if(target.settings.sortType == SortType::CommonTransparent){
        target.commandBuffer.drawCommands.sortFunction = [](auto& a, auto& b){
            if(a.first.materialId != b.first.materialId) return a.first.materialId < b.first.materialId;
            return a.first.distance > b.first.distance;
        };
    }

    if(target.settings.renderQueueRange == RenderQueueRange::Transparent){
        if(isBlend == false) return;
        isInstancing = false;
    }
    if(target.settings.renderQueueRange == RenderQueueRange::Opaue){
        if(isBlend == true) return;
    }

    if(isInstancing){
        DrawInstancingCommand& cm = target.commandBuffer.drawIntancingCommands.Get(data.targetMaterial, data.targetMesh);
        cm.material = data.targetMaterial;
        cm.meshs = data.targetMesh;
        cm.trans.push_back(data.targetMatrix);
    } else {
        target.commandBuffer.drawCommands.Add(bind, DrawCommand{
            data.targetMaterial,
            data.targetMesh,
            data.targetMatrix
        });
    } 
}

void RenderContext::SetStandUniforms(Camera& cam, Shader& shader){
    //Shader::Bind(shader);
    shader.SetMatrix4("view", cam.view);
    shader.SetMatrix4("projection", cam.projection);
    shader.SetVector3("viewPos", cam.viewPos);
}

void RenderContext::DrawRenderers(DrawingTarget* targets, int count){
    for(int j = 0; j < count; j++){
        DrawingTarget* target = &targets[j];
        target->commandBuffer.Sort();
    }

    Ref<Material> lastMat = nullptr;

    for(int j = 0; j < count; j++){
        DrawingTarget* target = &targets[j];

        if(target->settings.renderQueueRange == RenderQueueRange::Opaue){
            /*Renderer::SetDepthTest(DepthTest::LESS);
            Renderer::SetCullFace(CullFace::BACK);
            Renderer::SetBlend(false);*/
        }

        if(target->settings.renderQueueRange == RenderQueueRange::Transparent){
            //Renderer::SetDepthTest(DepthTest::DISABLE);
            /*Renderer::SetCullFace(CullFace::NONE);
            Renderer::SetBlend(true);
            Renderer::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);*/
        }

        target->commandBuffer.drawCommands.Each([&](auto& cm){
            if(cm.material != lastMat){
                cm.material->UpdateDatas();
                cm.material->GetShader()->SetFloat("useInstancing", 0.0f); 
                SetStandUniforms(cam, *cm.material->GetShader());
            }
        
            lastMat = cm.material;
            cm.material->GetShader()->SetMatrix4("model", cm.trans);
            Renderer::DrawMesh(*cm.meshs);
        });

        target->commandBuffer.drawIntancingCommands.Each([&](auto& cm){
            cm.material->UpdateDatas();
            cm.material->GetShader()->SetFloat("useInstancing", 1); 
            SetStandUniforms(cam, *cm.material->GetShader());

            cm.meshs->instancingModelMatrixs.clear();

            for(auto j: cm.trans){
                cm.meshs->instancingModelMatrixs.push_back(j);
            }
            cm.meshs->UpdateMeshInstancingModelMatrixs();
            Renderer::DrawMeshInstancing(*cm.meshs, cm.trans.size());
        });
    }
}

void RenderContext::DrawGizmos(){
    Renderer::SetDepthTest(DepthTest::LESS);
    Renderer::SetCullFace(CullFace::BACK);
    Renderer::SetBlend(false);

    scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();

    auto meshRenderView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<MeshRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;

        AABB aabb = c.GetGlobalAABB(t);
        Renderer::DrawWireCube(Mathf::TRS(t.Position(), QuaternionIdentity, aabb.extents), Vector3(0,0,1), 1);
        //Renderer::DrawWireCube(t.GlobalModelMatrix(), Vector3(0,1,0), 1);
    }
}

void RenderContext::Clean(){
    //Renderer::Clean(cam.cleanColor.x, cam.cleanColor.y, cam.cleanColor.z, 1);
    Renderer::Clean(0, 0, 1, 1);
}

void RenderContext::RenderSkybox(){
    OD_PROFILE_SCOPE("RenderContext::RenderSkybox"); 
    
    if(skyMaterial != nullptr){
        Assert(skyMaterial->GetShader() != nullptr);

        skyMaterial->UpdateDatas();

        Renderer::SetCullFace(CullFace::BACK);
        Renderer::SetDepthMask(false);
        Renderer::SetBlend(false);
        Shader::Bind(*skyMaterial->GetShader());
        
        //environmentSettings.sky->shader()->SetCubemap("mainTex", *_skyboxCubemap, 0);
        skyMaterial->GetShader()->SetMatrix4("projection", cam.projection);
        Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
        skyMaterial->GetShader()->SetMatrix4("view", skyboxView);
        Renderer::DrawMesh(skyboxMesh);

        Renderer::SetDepthMask(true);
    }
}

void RenderContext::Submit(){
    
}


}