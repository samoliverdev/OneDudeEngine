#pragma once
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Model.h"

namespace OD{
    struct AnimatorComponent;
}

using namespace OD;

namespace Standard{

class CharacterMovement;

class CharacterAnimation{
public:
    Ref<Model> baseModel = nullptr;

    Clip* idleAnimation;
    Clip* runningAnimation;

    bool hasStarted = false;

    void OnStart();
    void OnUpdate(TransformComponent& transform, AnimatorComponent& anim, CharacterMovement& movement);

    template <class Archive>
    void serialize(Archive& ar){
        AssetRefSerialize<Model> baseModelRef(baseModel);
        ArchiveDumpNVP(ar, baseModelRef);
    }
};

}