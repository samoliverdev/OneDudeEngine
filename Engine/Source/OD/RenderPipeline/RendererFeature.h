#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/SerializationFull.h"
#include "RenderData.h"
#include "ChunkedVector.h"
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

namespace OD{

class RenderContext;
class Framebuffer;

enum class RenderPassEvent{
    None = 0,
    BeforeRendering,
    ShadowMap,
    Opaque,
    GBuffer,
    Transparent,
    PostProcess,
    PostProcessBeforeForward,
    UI,
    AfterRendering,
    Count
};

struct OD_API RenderFrameData{
    Ref<Framebuffer> src = nullptr; 
    Ref<Framebuffer> dst = nullptr;
};

enum PassPriority{
    PassPriorityPre = -100,
    PassPriorityDefault = 0,
    PassPriorityPost = 100
};

class OD_API RenderPass{
public:
    std::string passName;
    RenderPassEvent event = RenderPassEvent::None;
    int priority = PassPriorityDefault; 

    virtual ~RenderPass(){}
    virtual void Setup(RenderContext& context){}
    virtual void Execute(RenderContext& context, RenderFrameData& data) = 0;
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

    void Execute(RenderContext& context, RenderFrameData& data){
        std::sort(
            passes.begin(), passes.end(),
            [](RenderPass* a, RenderPass* b){
                return a->event < b->event;
            }
        );

        for(RenderPass* pass: passes){
            pass->Setup(context);
            pass->Execute(context, data);
        }

        passes.clear();
    }
};

class OD_API RendererFeature{
public:
    bool enable = true;

    virtual ~RendererFeature(){}
    virtual void AddRenderPasses(IRenderer& renderer, RenderContext& context){}
    virtual void OnCollectRenderData(RenderContext& context, std::vector<RenderData>& outRenderData){}
    virtual void OnCollectRenderData(RenderContext& context, ChunkedVector<RenderData>& data){}
    virtual void OnGui(){}

    virtual Type GetTypeId() const = 0;
};

template<typename T>
class RendererFeatureBase: public RendererFeature{
public:
    Type GetTypeId() const override { return GetType<T>(); }
};

class RendererFeatureGlobal;

class OD_API RendererFeatures{
public:
    template<typename T>
    bool HasFeature(){
        return instances.count(GetType<T>());
    }

    template<typename T>
    Ref<T> AddFeature(){
        Assert(HasFeature<T>() == false);
        static_assert(std::is_base_of<OD::RendererFeature, T>::value);

        Ref<T> c = CreateRef<T>();

        DataHolder holder = {
            c,
            [](DataHolder& s) -> Ref<RendererFeature>{ 
                Ref<T> r = CreateRef<T>();
                if(s.instance != nullptr) *r = *std::static_pointer_cast<T>(s.instance);
                return r; 
            }
        };
        instances[GetType<T>()] = holder;
        
        return c;
    }

    template <typename T>
    Ref<T> GetFeature(){
        static_assert(std::is_base_of<OD::RendererFeature, T>::value);
        return std::static_pointer_cast<T>(instances[GetType<T>()].instance);
    }

    template<typename T>
    Ref<T> AddOrFeature(){
        if(HasFeature<T>() == false) return AddFeature<T>();
        return GetFeature<T>();
    }

    template<typename T>
    void RemoveFeature(){
        if(HasFeature<T>() == false) return;
        instances.erase(GetType<T>());
    }

    RendererFeatures() = default;

    RendererFeatures(const RendererFeatures& s){
        for(auto i: s.instances){
            DataHolder holder = {i.second.Instantiate(i.second), i.second.Instantiate};
            instances[i.first] = holder;
        }
    }

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar){
        if constexpr(std::is_same_v<Archive, ODOutputArchive>){
            std::vector<std::string> typeIds;
            std::vector<Ref<RendererFeature>> _instances;

            for(auto& i: instances){
                for(auto& j: RendererFeatureGlobal::Get().GetNewRendererFeatureFuncs()){
                    if(j.second.getType() == i.second.instance->GetTypeId()){
                        typeIds.push_back(j.first);
                        _instances.push_back(i.second.instance);
                        break;
                    }
                }
            }

            ArchiveDumpNVP(ar, typeIds);
            for(int i = 0; i < typeIds.size(); i++){
                RendererFeatureGlobal::Get().GetNewRendererFeatureFuncs()[typeIds[i]].save(ar, _instances[i]);
                //SceneManager::Get().scriptsSerializer[typeIds[i]].scriptSave(ar, _instances[i]);
            }
        }

        if constexpr(std::is_same_v<Archive, ODInputArchive>){
            std::vector<std::string> typeIds;
            ArchiveDumpNVP(ar, typeIds);

            for(int i = 0; i < typeIds.size(); i++){
                auto instance = RendererFeatureGlobal::Get().GetNewRendererFeatureFuncs()[typeIds[i]].add(*this);
                RendererFeatureGlobal::Get().GetNewRendererFeatureFuncs()[typeIds[i]].load(ar, instance);
                //Script* instance = SceneManager::Get().scriptsSerializer[typeIds[i]].addScript(*this);
                //SceneManager::Get().scriptsSerializer[typeIds[i]].scriptLoad(ar, instance);
            }
        }
    }

    template<typename Func>
    void ForEachFeature(Func&& func) {
        for (auto& [type, holder] : instances) {
            if (holder.instance != nullptr) {
                func(holder.instance);
            }
        }
    }

private:
    struct DataHolder{
        Ref<RendererFeature> instance = nullptr;
        Ref<RendererFeature> (*Instantiate)(DataHolder&);
    };

    std::unordered_map<Type, DataHolder> instances;
};

class OD_API RendererFeatureGlobal{
public:
    struct SerializeFuncs{
        std::function<bool(RendererFeatures& container)> has;
        std::function<void(RendererFeatures& container)> onGui;
        std::function<Ref<RendererFeature>(RendererFeatures& container)> add;
        std::function<void(RendererFeatures& container)> remove;
        std::function<void(ODOutputArchive& ar, Ref<RendererFeature> instance)> save;
        std::function<void(ODInputArchive& ar, Ref<RendererFeature> instance)> load;
        std::function<Type()> getType;
    };

    using Data = std::unordered_map<std::string, SerializeFuncs>;

    static RendererFeatureGlobal& Get();

    inline Data& GetNewRendererFeatureFuncs(){ return newRendererFeatureFuncs; }

    template<typename T> 
    void RegisterRendererFeature(const std::string& id){ 
        SerializeFuncs funcs;

        funcs.has = [](RendererFeatures& container){
            return container.HasFeature<T>();
        };

        funcs.onGui = [](RendererFeatures& container){
            container.GetFeature<T>()->OnGui();
        };
        
        funcs.add = [](RendererFeatures& container){
            return container.AddFeature<T>();
        };

        funcs.remove = [](RendererFeatures& container){
            container.RemoveFeature<T>();
        };

        funcs.save = [](ODOutputArchive& ar, Ref<RendererFeature> instance){
            T* c = dynamic_cast<T*>(instance.get());
            Assert(c != nullptr);
            ar(*c);
        };

        funcs.load = [](ODInputArchive& ar, Ref<RendererFeature> instance){
            T* c = dynamic_cast<T*>(instance.get());
            Assert(c != nullptr);
            ar(*c);
        };

        funcs.getType = [](){
            return GetType<T>();
        };

        newRendererFeatureFuncs[id] = funcs;
    }
private:
    Data newRendererFeatureFuncs; //TODO: use std::unored_map<typeid, std::function<RendererFeature*()>> to avoid duplicate
    RendererFeatureGlobal(){}
};

}