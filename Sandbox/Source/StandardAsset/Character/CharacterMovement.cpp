#include "CharacterMovement.h"
#include "OD/Core/Input.h"
#include "OD/Core/Application.h"
#include "OD/Scene/Scene.h"
#include "OD/Physics/PhysicsSystem.h"

namespace Standard{

void CharacterMovement::OnStart(RigidbodyComponent& rb){
    hasStarted = true;
    rb.SetAngularFactor(Vector3(0, 0, 0));
}

float CharacterMovement::GetAxisHorizontal(){
    float x = Input::IsKey(KeyCode::D) ? 1 : 0;
    float y = Input::IsKey(KeyCode::A) ? 1 : 0;
    return math::clamp(x - y, -1.0f, 1.0f);
}

float CharacterMovement::GetAxisVertical(){
    float x = Input::IsKey(KeyCode::W) ? 1 : 0;
    float y = Input::IsKey(KeyCode::S) ? 1 : 0;
    return math::clamp(x - y, -1.0f, 1.0f);
}

void CharacterMovement::OnUpdate(Scene& scene, TransformComponent& transform, RigidbodyComponent& rb){
    TransformComponent& cam = scene.GetComponent<TransformComponent>(scene.GetMainCamera());
    moveDir = cam.Right() * GetAxisHorizontal() + cam.Back() * GetAxisVertical();
    moveDir.y = 0.0f;
    if(math::length(moveDir) > 1) moveDir = math::normalize(moveDir);

    Assert(Mathf::IsNan(moveDir) == false);

    rb.Velocity(moveDir * moveSpeed);
    if(moveDir != Vector3Zero){
        rb.Rotation(
            math::slerp(rb.Rotation(), math::quatLookAt(-moveDir, Vector3Up), turnSpeed * Application::DeltaTime())
        );
    }
}    

}