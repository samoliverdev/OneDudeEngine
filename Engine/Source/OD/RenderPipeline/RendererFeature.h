#pragma once
#include "OD/Defines.h"
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

namespace OD{

class RenderContext;

enum class RenderPassEvent{
    None = 0,
    BeforeRendering,
    ShadowMap,
    Opaque,
    GBuffer,
    Transparent,
    PostProcess,
    UI,
    AfterRendering
};

class OD_API RenderPass{
public:
    std::string name;
    RenderPassEvent event = RenderPassEvent::None;

    virtual ~RenderPass(){}
    virtual void Setup(RenderContext& context){}
    virtual void Execute(RenderContext& context) = 0;
};

class OD_API IRenderer{
public:
    virtual ~IRenderer(){}
    virtual void AddPass(RenderPass* pass){}
};

class OD_API RendererPasses: public IRenderer{
public:
    std::vector<RenderPass*> passes;

    void AddPass(RenderPass* pass) override{
        passes.push_back(pass);
    }

    void Execute(RenderContext& context){
        std::sort(
            passes.begin(), passes.end(),
            [](RenderPass* a, RenderPass* b){
                return a->event < b->event;
            }
        );

        for(RenderPass* pass: passes){
            pass->Setup(context);
            pass->Execute(context);
        }

        passes.clear();
    }
};

class OD_API RendererFeature{
public:
    virtual ~RendererFeature(){}
    virtual void AddRenderPasses(IRenderer& renderer, RenderContext& context){}
    virtual void OnGui(){}
};

class OD_API RendererFeatureGlobal{
public:
    using Data = std::vector<std::function<RendererFeature*()>>;

    static RendererFeatureGlobal& Get();

    template<typename T> 
    void RegisterRendererFeature(){ newRendererFeatureFuncs.push_back([](){ return new T(); }); }
    inline Data& GetNewRendererFeatureFuncs(){ return newRendererFeatureFuncs; }

private:
    Data newRendererFeatureFuncs; //TODO: use std::unored_map<typeid, std::function<RendererFeature*()>> to avoid duplicate
    RendererFeatureGlobal(){}
};

}