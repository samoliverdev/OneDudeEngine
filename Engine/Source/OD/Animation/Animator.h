#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Animation/Clip.h"
#include "OD/Animation/CrossFadeController.h"

#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"

namespace OD{

//TODO: Make Serializable
struct OD_API AnimatorComponent{
    friend struct AnimatorSystem;
    enum class LayerBlendMode { Override };
    struct Layer{
        CrossFadeController controller;
        std::vector<unsigned char> mask;
        LayerBlendMode blendMode = LayerBlendMode::Override;
        bool blendIfClipIsNull = false;
    };

    
    void Play(ClipT* clip, int layer = 0);
    void FadeTo(ClipT* target, float fadeTime, int layer = 0);
    void PushLayer();
    void PopLayer();
    Layer& GetLayer(int layer);
    int LayerCount();

    inline bool Enable(){ return enable; }
    inline void Enable(bool v){ enable = v; }

    static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, enable);
        ArchiveDumpNVP(ar, toPlay);
    }

private:
    std::vector<Layer> layers = {{}};
    int toPlay = -1;
    bool enable = true;
};

struct OD_API AnimatorSystem: public System{
    AnimatorSystem();
    //inline System* Clone(Scene* inScene) const override{ return new AnimatorSystem(inScene); }

    virtual int Type() override;
    virtual void Update(Scene& scene) override;
    virtual void AnimationUpdate(Scene& scene) override;
};

void AnimatorModuleInit();

}