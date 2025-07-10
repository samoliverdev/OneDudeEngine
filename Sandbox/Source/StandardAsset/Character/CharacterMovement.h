#pragma once
#include "OD/Core/Math.h"

namespace OD{
    class Scene;
    struct TransformComponent;
    struct RigidbodyComponent;
}

using namespace OD;

namespace Standard{

class CharacterMovement{
public:
    float moveSpeed = 6;
    float turnSpeed = 20;

    bool hasStarted = false;

    Vector3 moveDir = Vector3Zero;

    void OnStart(RigidbodyComponent& rb);
    void OnUpdate(Scene& scene, TransformComponent& transform, RigidbodyComponent& rb);

    float GetAxisHorizontal();
    float GetAxisVertical();

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, moveSpeed);
        ArchiveDumpNVP(ar, turnSpeed);
    }
};

}