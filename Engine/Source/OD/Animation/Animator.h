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
    bool enable = true;
    
    enum class LayerBlendMode { Override };
    struct Layer{
        CrossFadeController controller;
        std::vector<unsigned char> mask;
        LayerBlendMode blendMode = LayerBlendMode::Override;
        bool blendIfClipIsNull = false;
    };

    friend struct AnimatorSystem;
    void Play(ClipT* clip, int layer = 0);
    void FadeTo(ClipT* target, float fadeTime, int layer = 0);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, enable);
        ArchiveDumpNVP(ar, toPlay);
    }

    static void OnGui(Entity& e, Scene& scene);

    void PushLayer();
    void PopLayer();
    Layer& GetLayer(int layer);
    int LayerCount();

private:
    //std::vector<Matrix4> posePalette;

    std::vector<Layer> layers = {{}};
    //CrossFadeController controller;

    int toPlay = -1;
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