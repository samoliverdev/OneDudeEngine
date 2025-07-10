#include "ThirdPersonCamera.h"
#include "OD/Platform/Platform.h"
#include "OD/Core/Input.h"

namespace Standard{

void ThirdPersonCamera::OnStart(){
    hasStarted = true;
    if(autoHiddenCursor) Platform::SetCursorState(CursorState::Disabled);
}

void ThirdPersonCamera::OnUpdate(Scene& scene, OD::TransformComponent& transform){
    if(autoHiddenCursor){
        if(Input::IsKeyDown(KeyCode::Escape)) Platform::SetCursorState(CursorState::Normal);
        if(Input::IsMouseButtonDown(MouseButton::Left)) Platform::SetCursorState(CursorState::Disabled);
    }

    //Entity target(lookAtTarget, GetEntity().GetScene());
    TransformComponent& lookAt = scene.GetComponent<TransformComponent>(lookAtTarget); //target.GetComponent<TransformComponent>();
    Vector3 lookAtPos = lookAt.Position() + Vector3Up * upOffset;

    double mousePosX;
    double mousePosY;
    Input::GetMousePosition(&mousePosX, &mousePosY);

    float mouseXAxis = -(mousePosX - lastMousePosX);
    float mouseYAxis = -(lastMousePosY - mousePosY);
    mouseXAxis = math::clamp(mouseXAxis, -1.0f, 1.0f);
    mouseYAxis = math::clamp(mouseYAxis, -1.0f, 1.0f);

    lastMousePosX = mousePosX;
    lastMousePosY = mousePosY;

    currentX += mouseXAxis * sensivity;
    currentY += mouseYAxis * sensivity;

    currentY = math::clamp(currentY, yMin, yMax);

    Vector3 Direction = Vector3(0, 0, -distance);
    Quaternion rotation = Quaternion(math::radians(Vector3(currentY, currentX, 0)));
    transform.Position(lookAtPos + rotation * Direction);

    //transform.LookAt(lookAt.Position());
    transform.Rotation(math::quatLookAt(math::normalize(lookAtPos - transform.Position()), Vector3Up));
}    

}