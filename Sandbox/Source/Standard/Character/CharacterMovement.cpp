#include "CharacterMovement.h"
#include "OD/Core/Input.h"
#include "OD/Core/Application.h"
#include "OD/Scene/Scene.h"
#include "OD/Physics/PhysicsSystem.h"

namespace Standard{

void CharacterMovement::OnStart(RigidbodyComponent& rb){
    hasStarted = true;
    //rb.SetAngularFactor(Vector3(0, 0, 0));
    rb.Constraints(RigidbodyConstraints::TranslationX | RigidbodyConstraints::TranslationY | RigidbodyConstraints::TranslationZ | RigidbodyConstraints::RotationY);
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

void CharacterMovement::OnUpdate(Scene& scene, TransformComponent& transform, RigidbodyComponent& rb) {
    if(enable == false) return; 
    
    RayResult hit;
    if (scene.GetSystem<PhysicsSystem>()->Raycast(transform.Position() + Vector3Up * 0.1f, Vector3Down * 0.25f, hit)) {
        //LogInfo("Hitting: %s", scene.GetComponent<InfoComponent>(hit.entity).name.c_str());
        onGround = true;
        groundNormal = hit.hitNormal;
    } else {
        onGround = false;
        groundNormal = Vector3Up;
    }

    if(handleInputs){
        TransformComponent& cam = scene.GetComponent<TransformComponent>(scene.GetMainCamera());
        moveDir = cam.Right() * GetAxisHorizontal() + cam.Back() * GetAxisVertical();
        moveDir.y = 0.0f;
        moveDir = math::normalizeSafe(moveDir);

        if(moveType == MoveType::Free){
            lookDir = moveDir;
            lookDir.y = 0;
            lookDir = math::normalizeSafe(lookDir);
        }

        if(moveType == MoveType::Strafe){
            lookDir = cam.Back();
            lookDir.y = 0;
            lookDir = math::normalizeSafe(lookDir);
        }
    }

    Vector3 velocity;

    auto ProjectOnPlane = [](const glm::vec3& vector, const glm::vec3& planeNormal){
        // Ensure the plane normal is normalized
        glm::vec3 normalizedNormal = glm::normalize(planeNormal);
        
        // Calculate the projection of vector onto the plane normal
        float dot = glm::dot(vector, normalizedNormal);
        
        // Subtract the projection component from the original vector
        // This gives us the vector component that's perpendicular to the normal
        return vector - (dot * normalizedNormal);
    };

    bool stickToTheFloor = math::length(moveDir) > 0.01f || lastStickToTheFloor == true;
    lastStickToTheFloor = math::length(moveDir) > 0.01f;

    if(onGround && stickToTheFloor && math::length2(moveDir) > (0.1f*0.1f)){
        //Vector3 alignedMoveDir = math::normalizeSafe(moveDir - groundNormal * math::dot(moveDir, groundNormal));
        Vector3 alignedMoveDir = moveDir; //(moveDir, groundNormal);
        velocity = alignedMoveDir * moveSpeed;
        velocity.y = -2.0f;
        velocity = ProjectOnPlane(velocity, groundNormal);
    } else {
        velocity = rb.Velocity();
        velocity.x = moveDir.x * moveSpeed;
        velocity.z = moveDir.z * moveSpeed;
    }

    /*Vector3 deltaVel = velocity - rb.Velocity();
    Vector3 impulse = deltaVel * rb.Mass();
    rb.ApplyImpulse(impulse);*/

    float acceleration = 10;
    Vector3 deltaVel = velocity - rb.Velocity();
    Vector3 force = deltaVel * acceleration * rb.Mass();
    rb.ApplyForce(force);

    if(math::length2(moveDir) < (0.1f*0.1f)){
        rb.LinearDamping(999);
    } else {
        rb.LinearDamping(0);
    }

    /*float maxSpeed = 1;
    Vector3 horizontalVel(rb.Velocity().x, 0.0f, rb.Velocity().z);
    float speed = math::length(horizontalVel);
    if(speed > maxSpeed){
        Vector3 excessVel = horizontalVel - math::normalizeSafe(horizontalVel) * maxSpeed;
        rb.ApplyForce(-excessVel * 10.0f * rb.Mass());
    }*/

    if(moveType == MoveType::Free){
        if(math::length(moveDir) > 0.001f){
            rb.Rotation(
                math::slerp(rb.Rotation(), math::quatLookAt(-moveDir, Vector3Up), freeTurnSpeed * Application::DeltaTime())
            );
        }
    }

    if(moveType == MoveType::Strafe){
        if (math::length(lookDir) > 0.001f) {
            rb.Rotation(
                math::slerp(rb.Rotation(), math::quatLookAt(-lookDir, Vector3Up), strafeTurnSpeed * Application::DeltaTime())
            );
        }
    }
}


}