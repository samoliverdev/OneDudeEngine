#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Animation/Clip.h"
#include "OD/Animation/CrossFadeController.h"
#include <vector>

namespace OD{

//TODO: Make Serializable
struct OD_API AnimatorComponent{
    friend struct AnimatorSystem;
    void Play(Clip* clip);

    template <class Archive>
    void serialize(Archive & ar){}

    static void OnGui(Entity& e);

//private:
    //std::vector<Matrix4> posePalette;
    CrossFadeController controller;
};

struct OD_API AnimatorSystem: public System{
    AnimatorSystem(Scene* scene);
    inline System* Clone(Scene* inScene) const override{ return new AnimatorSystem(inScene); }

    virtual SystemType Type() override;
    virtual void Update() override;
};

}