#include "ThirdPersonCamera.h"
#include <OD/Platform/Platform.h>
#include <OD/Core/Input.h>
#include <OD/Core/Application.h>

namespace Standard{

void ThirdPersonCamera::OnStart(){
    hasStarted = true;
    if(autoHiddenCursor) Platform::SetCursorState(CursorState::Disabled);
}

void ThirdPersonCamera::OnUpdate(Scene& scene, OD::TransformComponent& transform, bool updateInput){
    /*if(lookAtTarget == EntityNull) return;

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
    if(handleInputs == false) rawDelta = Vector2Zero;

    float deltaTime = Application::DeltaTime();
    // Normalize raw delta (optional, see note below)
    const float normalizationScale = 15.0f; // tune based on mouse resolution
    float rawX = math::clamp(rawDelta.x / normalizationScale, -3.0f, 3.0f); // allows brief bursts
    float rawY = math::clamp(rawDelta.y / normalizationScale, -3.0f, 3.0f);
    // Apply exponential smoothing
    //smoothMouseX = rawX; //math::mix(smoothMouseX, rawX, deltaTime * smoothingFactor);
    //smoothMouseY = rawY; //math::mix(smoothMouseY, rawY, deltaTime * smoothingFactor);
    smoothMouseX = smoothingFactor > 0 ? math::mix(smoothMouseX, rawX, deltaTime * smoothingFactor) : rawX;
    smoothMouseY = smoothingFactor > 0 ? math::mix(smoothMouseY, rawY, deltaTime * smoothingFactor) : rawY;
    float mouseXAxis = -smoothMouseX;
    float mouseYAxis = smoothMouseY;

    lastMousePosX = mousePosX;
    lastMousePosY = mousePosY;

    if(updateInput == true){
    currentX += mouseXAxis * sensivity;
    currentY += mouseYAxis * sensivity;
    currentY = math::clamp(currentY, yMin, yMax);
    }

    Vector3 Direction = Vector3(0, 0, -distance);
    Quaternion rotation = Quaternion(math::radians(Vector3(currentY, currentX, 0)));

    transform.Position(lookAtPos + rotation * Direction);
    transform.Rotation(math::quatLookAt(math::normalize(lookAtPos - transform.Position()), Vector3Up));*/

    if (lookAtTarget == EntityNull) return;

    if (autoHiddenCursor) {
        if (Input::IsKeyDown(KeyCode::Escape))
            Platform::SetCursorState(CursorState::Normal);
        if (Input::IsMouseButtonDown(MouseButton::Left))
            Platform::SetCursorState(CursorState::Disabled);
    }

    TransformComponent& lookAt = scene.GetComponent<TransformComponent>(lookAtTarget);
    Vector3 lookAtPos = lookAt.Position() + Vector3Up * upOffset;

    double mousePosX;
    double mousePosY;
    Input::GetMousePosition(&mousePosX, &mousePosY);

    Vector2 rawDelta = Input::GetMouseDelta();
    if (!handleInputs) rawDelta = Vector2Zero;

    float deltaTime = Application::DeltaTime();
    const float normalizationScale = 15.0f;
    float rawX = math::clamp(rawDelta.x / normalizationScale, -3.0f, 3.0f);
    float rawY = math::clamp(rawDelta.y / normalizationScale, -3.0f, 3.0f);

    smoothMouseX = smoothingFactor > 0 ? math::mix(smoothMouseX, rawX, deltaTime * smoothingFactor) : rawX;
    smoothMouseY = smoothingFactor > 0 ? math::mix(smoothMouseY, rawY, deltaTime * smoothingFactor) : rawY;

    float mouseXAxis = -smoothMouseX;
    float mouseYAxis = smoothMouseY;

    lastMousePosX = mousePosX;
    lastMousePosY = mousePosY;

    if (updateInput) {
        currentX += mouseXAxis * sensivity;
        currentY += mouseYAxis * sensivity;
        currentY = math::clamp(currentY, yMin, yMax);
    }

    // Orbital rotation (used to compute the base camera offset)
    Quaternion orbitalRot = Quaternion(math::radians(Vector3(currentY, currentX, 0)));

    // Base backward offset (camera position without shoulder offset)
    Vector3 cameraBasePos = lookAtPos + orbitalRot * Vector3(0, 0, -distance);

    // Compute world-space right vector from the base camera position (so it won't affect look rotation)
    Vector3 forwardFromBase = lookAtPos - cameraBasePos; // vector pointing from camera base -> target
    Vector3 right = math::normalize(math::cross(Vector3Up, forwardFromBase)); // right relative to forward/up

    // Apply shoulder offset only to the camera position
    Vector3 finalPos = cameraBasePos + right * rightOffset;

    // Set final position
    transform.Position(finalPos);

    // IMPORTANT: compute rotation using cameraBasePos (without rightOffset)
    Vector3 lookDir = math::normalize(lookAtPos - cameraBasePos);
    transform.Rotation(math::quatLookAt(lookDir, Vector3Up));
}    

}