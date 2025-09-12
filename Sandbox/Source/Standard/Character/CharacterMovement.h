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
    enum class MoveType{
        Free, Strafe
    };

    bool enable = true;
    bool handleInputs = false;

    MoveType moveType; 
    float moveSpeed = 6;
    float freeTurnSpeed = 20;
    float strafeTurnSpeed = 20;

    bool hasStarted = false;
    
    bool onGround = true;
    Vector3 groundNormal = Vector3Zero; 
    Vector3 moveDir = Vector3Zero;
    Vector3 lookDir = Vector3Zero;
    bool lastStickToTheFloor = true;

    void OnStart(RigidbodyComponent& rb);
    void OnUpdate(Scene& scene, TransformComponent& transform, RigidbodyComponent& rb);

    float GetAxisHorizontal();
    float GetAxisVertical();

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enable);
        ArchiveDumpNVP(ar, handleInputs);

        ArchiveDumpNVP(ar, moveType);
        ArchiveDumpNVP(ar, moveSpeed);
        ArchiveDumpNVP(ar, freeTurnSpeed);
        ArchiveDumpNVP(ar, strafeTurnSpeed);

        ArchiveDumpNVP(ar, onGround);
    }
};

}