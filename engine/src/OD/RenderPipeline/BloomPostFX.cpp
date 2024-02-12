#include "BloomPostFX.h"

namespace OD{

BloomPostFX::BloomPostFX(){
    blitShader = Shader::CreateFromFile("res/Engine/Shaders/Blit.glsl");
    bloomHorizontalPassShader = Shader::CreateFromFile("res/Engine/Shaders/BloomHorizontalPostFX.glsl");
    bloomVerticalPassShader = Shader::CreateFromFile("res/Engine/Shaders/BloomVerticalPostFX.glsl");
    bloomCombinePassShader = Shader::CreateFromFile("res/Engine/Shaders/BloomCombinePostFX.glsl");
    bloomPrefilterPassShader = Shader::CreateFromFile("res/Engine/Shaders/BloomPrefilterPostFX.glsl");
}

void BloomPostFX::OnSetup(){

}

inline float LinearToGammaSpaceExact(float value){
    if(value <= 0.0F)
        return 0.0F;
    else if(value <= 0.0031308F)
        return 12.92F * value;
    else if(value < 1.0F)
        return 1.055F * pow(value, 0.4166667F) - 0.055F;
    
    return pow(value, 0.45454545F);
}

void BloomPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst){
    maxIterations = math::clamp<int>(maxIterations, 0, 16);
    if(downscaleLimit < 1) downscaleLimit = 1; 

    auto spec = src->Specification();

    int width = spec.width / 2;
    int height = spec.height / 2;

    if(maxIterations == 0 || intensity <= 0 || height < downscaleLimit*2 || width < downscaleLimit*2){
        Graphics::BlitQuadPostProcessing(src, dst, *blitShader);
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
    Shader::Bind(*bloomPrefilterPassShader);
    bloomPrefilterPassShader->SetVector4("_BloomThreshold", _threshold);
    Graphics::BlitQuadPostProcessing(fromId, toId, *bloomPrefilterPassShader);
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
        Graphics::BlitQuadPostProcessing(fromId, toId, *bloomHorizontalPassShader);
        fromId = toId;

        toId = new Framebuffer(spec);
        temps.push_back(toId);
        Graphics::BlitQuadPostProcessing(fromId, toId, *bloomVerticalPassShader);
        fromId = toId;
        
        width /= 2;
        height /= 2;
    }

    Shader::Bind(*bloomCombinePassShader);
    bloomCombinePassShader->SetFramebuffer("mainTex2", *fromId, 1, 0);
    bloomCombinePassShader->SetInt("_BloomBicubicUpsampling", bicubicUpsampling ? 1 : 0);
    bloomCombinePassShader->SetFloat("_BloomIntensity", intensity);
    Graphics::BlitQuadPostProcessing(src, dst, *bloomCombinePassShader);

    for(auto i: temps) i->Destroy();
    temps.clear();
}

}