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
    
    bool onGround = true;
    Vector3 groundNormal = Vector3Zero; 
    Vector3 moveDir = Vector3Zero;
    bool lastStickToTheFloor = true;

    void OnStart(RigidbodyComponent& rb);
    void OnUpdate(Scene& scene, TransformComponent& transform, RigidbodyComponent& rb);

    float GetAxisHorizontal();
    float GetAxisVertical();

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, moveSpeed);
        ArchiveDumpNVP(ar, turnSpeed);

        ArchiveDumpNVP(ar, onGround);
    }
};

}