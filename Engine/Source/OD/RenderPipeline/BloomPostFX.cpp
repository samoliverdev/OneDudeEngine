#include "OD/pch.h"
#include "BloomPostFX.h"
#include "OD/Serialization/CerealImGui.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Framebuffer.h"
#include "RenderContext.h"

//Source: https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/

namespace OD{

void BloomFeature::OnGui(){
    cereal::ImGuiArchive colorGradring;
    colorGradring(*this);
}

BloomFeature::BloomFeature(){
    enable = false;
    event = RenderPassEvent::PostProcess;
    blitShader = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
    bloomMat = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/BloomPostFX.glsl"));

    bloomHorizontalPassShader = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/BloomHorizontalPostFX.glsl"));
    bloomVerticalPassShader = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/BloomVerticalPostFX.glsl"));
    bloomCombinePassShader = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/BloomCombinePostFX.glsl"));
    bloomPrefilterPassShader = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/BloomPrefilterPostFX.glsl"));
}

inline float LinearToGammaSpaceExact(float value){
    if(value <= 0.0F){
        return 0.0F;
    } else if(value <= 0.0031308F){
        return 12.92F * value;
    } else if(value < 1.0F){
        return 1.055F * pow(value, 0.4166667F) - 0.055F;
    }
    
    return pow(value, 0.45454545F);
}

inline float GammaToLinearSpace(float v){ 
    return math::pow(v, 2.2f); 
}

void BloomFeature::AddRenderPasses(IRenderer& renderer, RenderContext& context){
    renderer.AddPass(this);
}

void BloomFeature::Execute(RenderContext& context, RenderFrameData& data){
    Ref<Framebuffer> deferred = context.GetDeferredFramebuffer();
    auto spec = data.src->Specification();

    auto Blit = [](Ref<Framebuffer>& _src, Ref<Framebuffer>& _dst, Ref<Material> blitMat, int pass = 0){
        Graphics::BeginFramebuffer(*_dst, false);
        Graphics::SetViewport(0, 0, _dst->Specification().width, _dst->Specification().height);
        blitMat->SetPass(pass);
        blitMat->SetTexture("mainTex", _src, 0);
        Graphics::DrawFullScreenQuad(*blitMat, Matrix4Identity);
        Graphics::EndFramebuffer();
    };
    

    //bloomMat->SetFloat("threshold", threshold);
    //bloomMat->SetFloat("softThreshold", thresholdKnee);
    float knee = threshold * thresholdKnee;
    Vector4 filter;
    filter.x = threshold;
    filter.y = filter.x - knee;
    filter.z = 2.0f * knee;
    filter.w = 0.25f / (knee + 0.00001f);
    bloomMat->SetVector4("_filter", filter);

    bloomMat->SetFloat("intensity", GammaToLinearSpace(intensity));

    const int BoxDownPrefilterPass = 0;
    const int BoxDownPass = 1;
	const int BoxUpPass = 2;
    const int ApplyBloomPass = 3;
    const int DebugBloomPass = 4;

    std::array<Ref<Framebuffer>, 16> textures{};
    //std::vector<Framebuffer*> releaseTemporary;

    Ref<Framebuffer> currentDestination = textures[0] = ResourceManager::Get().Create<Framebuffer>(spec);
    Blit(data.src, currentDestination, bloomMat, BoxDownPrefilterPass);

    Ref<Framebuffer> currentSource = currentDestination;

    int i = 1;
    for(; i < maxIterations; i++){
        spec.width /= 2;
        spec.height /= 2;
        if(spec.width < 2 || spec.height < 2){
            break;
        }

        currentDestination = textures[i] = ResourceManager::Get().Create<Framebuffer>(spec);
        
        Blit(currentSource, currentDestination, bloomMat, BoxDownPass);
        //releaseTemporary.push_back(currentSource);
        
        currentSource = currentDestination;
    }

    for(i -= 2; i >= 0; i--){
        currentDestination = textures[i];
        textures[i] = nullptr;
        
        Blit(currentSource, currentDestination, bloomMat, BoxUpPass);
        //releaseTemporary.push_back(currentSource);
        
        currentSource = currentDestination;
    }

    //Blit(currentSource, dst, bloomMat, BoxUpPass);

    if(debug){
        Blit(currentSource, data.dst, bloomMat, DebugBloomPass);
    } else {
        bloomMat->SetTexture("sourceTex", data.src, 0);
	    Blit(currentSource, data.dst, bloomMat, ApplyBloomPass);
    }
    //releaseTemporary.push_back(currentSource);

    //for(Framebuffer* cur: releaseTemporary) delete cur;

    /*auto temp1 = new Framebuffer(halfSpec);
    auto temp2 = new Framebuffer(halfSpec);

    Graphics::BeginFramebuffer(*temp1);
    Graphics::SetViewport(0, 0, halfSpec.width, halfSpec.height);
    bloomHorizontalPassShader->SetTexture("mainTex", src, 0);
    Graphics::DrawFullScreenQuad(*bloomHorizontalPassShader, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*temp2);
    Graphics::SetViewport(0, 0, halfSpec.width, halfSpec.height);
    bloomVerticalPassShader->SetTexture("mainTex", temp1, 0);
    Graphics::DrawFullScreenQuad(*bloomVerticalPassShader, Matrix4Identity);
    Graphics::EndFramebuffer();


    Graphics::BeginFramebuffer(*dst);
    Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
    blitShader->SetTexture("mainTex", temp2, 0);
    Graphics::DrawFullScreenQuad(*blitShader, Matrix4Identity);
    Graphics::EndFramebuffer();
    
    delete temp1;
    delete temp2;*/

    /*maxIterations = math::clamp<int>(maxIterations, 0, 16);
    if(downscaleLimit < 1) downscaleLimit = 1; 

    auto spec = src->Specification();

    int width = spec.width / 2;
    int height = spec.height / 2;

    if(maxIterations == 0 || intensity <= 0 || height < downscaleLimit*2 || width < downscaleLimit*2){
        Graphics::DrawQuadPostProcessing(src, dst, *blitShader);
        return;
    }

    Framebuffer* fromId = src;
    Framebuffer* toId;

    std::vector<Framebuffer*> temps;

    toId = new Framebuffer(spec);
    temps.push_back(toId);
    Vector4 _threshold;
    _threshold.x = LinearToGammaSpaceExact(threshold);//Mathf.GammaToLinearSpace(threshold);
    _threshold.y = _threshold.x * thresholdKnee;
    _threshold.z = 2.0f * _threshold.y;
    _threshold.w = 0.25f / (_threshold.y + 0.00001f);
    _threshold.y -= _threshold.x;
    bloomPrefilterPassShader->SetVector4("_BloomThreshold", _threshold);
    Graphics::DrawQuadPostProcessing(fromId, toId, *bloomPrefilterPassShader);
    fromId = toId;
    width /= 2;
    height /= 2;

    int i;
    for(i = 0; i < maxIterations; i++){
        if(height < downscaleLimit || width < downscaleLimit) break;

        spec.width = width;
        spec.height = height;
        
        toId = new Framebuffer(spec);
        temps.push_back(toId);
        Graphics::DrawQuadPostProcessing(fromId, toId, *bloomHorizontalPassShader);
        fromId = toId;

        toId = new Framebuffer(spec);
        temps.push_back(toId);
        Graphics::DrawQuadPostProcessing(fromId, toId, *bloomVerticalPassShader);
        fromId = toId;
        
        width /= 2;
        height /= 2;
    }

    bloomCombinePassShader->SetTexture("mainTex2", fromId, 1);
    bloomCombinePassShader->SetInt("_BloomBicubicUpsampling", bicubicUpsampling ? 1 : 0);
    bloomCombinePassShader->SetFloat("_BloomIntensity", intensity);
    Graphics::DrawQuadPostProcessing(src, dst, *bloomCombinePassShader);

    for(auto i: temps) delete i; //Graphics::FramebufferDestroy(*i); //i->Destroy();
    temps.clear();
    */
}

}