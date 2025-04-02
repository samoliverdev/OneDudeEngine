#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Animation/Clip.h"
#include "OD/Animation/CrossFadeController.h"

namespace OD{

//TODO: Make Serializable
struct OD_API AnimatorComponent{
    friend struct AnimatorSystem;
    void Play(Clip* clip);
    void FadeTo(Clip* target, float fadeTime);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, enable);
    }

    static void OnGui(Entity& e, Scene& scene);

//private:
    //std::vector<Matrix4> posePalette;
    CrossFadeController controller;

    bool enable = true;
};

struct OD_API AnimatorSystem: public System{
    AnimatorSystem(Scene* scene);
    //inline System* Clone(Scene* inScene) const override{ return new AnimatorSystem(inScene); }

    virtual SystemType Type() override;
    virtual void Update() override;
};

void AnimatorModuleInit();

}