#include "OD/pch.h"
#include "ToneMappingPostFX.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Material.h"
#include "OD/Serialization/CerealImGui.h"
#include "RenderContext.h"
#include <magic_enum/magic_enum.hpp>

namespace OD{

ToneMappingFeature::ToneMappingFeature(){
    enable = false;
    event = RenderPassEvent::PostProcess;
    copyPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
    toneMappingReinhardPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingReinhardPostFX.glsl"));
    toneMappingNeutralPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingNeutralPostFX.glsl"));
    toneMappingACESPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingACESPostFX.glsl"));
}

void ToneMappingFeature::AddRenderPasses(IRenderer& renderer, RenderContext& context){
    renderer.AddPass(this);
}

void ToneMappingFeature::Execute(RenderContext& context, RenderFrameData& data){
    Ref<Material> pass = copyPass;
    if(mode == Mode::Neutral) pass = toneMappingNeutralPass;
    if(mode == Mode::Reinhard) pass = toneMappingReinhardPass;
    if(mode == Mode::ACES) pass = toneMappingACESPass;
        
    if(mode == Mode::Reinhard){
        toneMappingReinhardPass->SetFloat("exposure", exposure);
    }

    //Graphics::DrawQuadPostProcessing(src, dst, *pass);
    Graphics::BeginFramebuffer(*data.dst);
    pass->SetTexture("mainTex", data.src, 0);
    Graphics::DrawFullScreenQuad(*pass, Matrix4Identity);
    Graphics::EndFramebuffer();
}

void ToneMappingFeature::OnGui(){
    cereal::ImGuiArchive ar;
    ar(*this);
}

}