#include "ThirdPersonCamera.h"
#include "OD/Platform/Platform.h"
#include "OD/Core/Input.h"
#include "OD/Core/Application.h"

namespace Standard{

void ThirdPersonCamera::OnStart(){
    hasStarted = true;
    if(autoHiddenCursor) Platform::SetCursorState(CursorState::Disabled);
}

void ThirdPersonCamera::OnUpdate(Scene& scene, OD::TransformComponent& transform){
    if(lookAtTarget == EntityNull) return;

    if(autoHiddenCursor){
        if(Input::IsKeyDown(KeyCode::Escape)) Platform::SetCursorState(CursorState::Normal);
        if(Input::IsMouseButtonDown(MouseButton::Left)) Platform::SetCursorState(CursorState::Disabled);
    }

    TransformComponent& lookAt = scene.GetComponent<TransformComponent>(lookAtTarget); //target.GetComponent<TransformComponent>();
    Vector3 lookAtPos = lookAt.Position() + Vector3Up * upOffset;

    double mousePosX;
    double mousePosY;
    Input::GetMousePosition(&mousePosX, &mousePosY);

    //Test1
    //float mouseXAxis = -(mousePosX - lastMousePosX);
    //float mouseYAxis = -(lastMousePosY - mousePosY);

    //Test2
    Vector2 rawDelta = Input::GetMouseDelta();
    float deltaTime = Application::DeltaTime();
    // Normalize raw delta (optional, see note below)
    const float normalizationScale = 15.0f; // tune based on mouse resolution
    float rawX = math::clamp(rawDelta.x / normalizationScale, -3.0f, 3.0f); // allows brief bursts
    float rawY = math::clamp(rawDelta.y / normalizationScale, -3.0f, 3.0f);
    // Apply exponential smoothing
    smoothMouseX = rawX; //math::mix(smoothMouseX, rawX, deltaTime * smoothingFactor);
    smoothMouseY = rawY; //math::mix(smoothMouseY, rawY, deltaTime * smoothingFactor);
    float mouseXAxis = -smoothMouseX;
    float mouseYAxis = smoothMouseY;

    //Test3
    /*Vector2 mouseDelta = Input::GetMouseDelta();
    float mouseXAxis = -mouseDelta.x;
    float mouseYAxis = mouseDelta.y;*/

    lastMousePosX = mousePosX;
    lastMousePosY = mousePosY;

    currentX += mouseXAxis * sensivity;
    currentY += mouseYAxis * sensivity;

    currentY = math::clamp(currentY, yMin, yMax);

    Vector3 Direction = Vector3(0, 0, -distance);
    Quaternion rotation = Quaternion(math::radians(Vector3(currentY, currentX, 0)));

    transform.Position(lookAtPos + rotation * Direction);
    //transform.Position(math::mix(transform.Position(), lookAtPos + rotation * Direction, deltaTime * 25));

    //transform.LookAt(lookAt.Position());
    transform.Rotation(math::quatLookAt(math::normalize(lookAtPos - transform.Position()), Vector3Up));
}    

}