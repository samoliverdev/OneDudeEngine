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

    TransformComponent& cam = scene.GetComponent<TransformComponent>(scene.GetMainCamera());
    moveDir = cam.Right() * GetAxisHorizontal() + cam.Back() * GetAxisVertical();
    moveDir.y = 0.0f;
    moveDir = math::normalizeSafe(moveDir);

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

    if(onGround && stickToTheFloor){
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

    rb.Velocity(velocity);

    if (math::length(moveDir) > 0.001f) {
        rb.Rotation(
            math::slerp(rb.Rotation(), math::quatLookAt(-moveDir, Vector3Up), turnSpeed * Application::DeltaTime())
        );
    }
}


}