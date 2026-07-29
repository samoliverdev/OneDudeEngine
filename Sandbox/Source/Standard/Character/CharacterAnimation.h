#pragma once
#include <OD/Scene/Scene.h>
#include <OD/Graphics/Model.h>

namespace OD{
    struct AnimatorComponent;
}

using namespace OD;

namespace Standard{

class CharacterMovement;

class CharacterAnimation{
public:
    bool enable = true;

    float fadeTime = 0.2f;

    Ref<Model> baseModel = nullptr;

    ClipT* idleAnimation;
    ClipT* runningAnimation;

    bool hasStarted = false;
    
    void OnStart();
    void OnUpdate(TransformComponent& transform, AnimatorComponent& anim, CharacterMovement& movement);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enable);

        ResourceRefSerialize<Model> baseModelRef(baseModel);
        ArchiveDumpNVP(ar, baseModelRef);
    }
};

}