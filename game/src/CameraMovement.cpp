#include "CameraMovement.h"

void CameraMovement::OnStart(){
    pitch = transform->LocalEulerAngles().x;
    yaw = transform->LocalEulerAngles().y;
}

void CameraMovement::OnUpdate(){
    Vector3 pos = transform->LocalPosition();

    if(Input::IsKey(KeyCode::W)) pos += transform->Back() * moveSpeed * Application::DeltaTime();
    if(Input::IsKey(KeyCode::S)) pos += transform->Forward() * moveSpeed * Application::DeltaTime();
    if(Input::IsKey(KeyCode::A)) pos += transform->Left() * moveSpeed * Application::DeltaTime();
    if(Input::IsKey(KeyCode::D)) pos += transform->Right() * moveSpeed * Application::DeltaTime();

    double xpos;
    double ypos;
    Input::GetMousePosition(&xpos, &ypos);

    const float sensitivity = 0.6f;
    float inputRotateAxisX = static_cast<float>(-(xpos - lastX)) * sensitivity;
    float inputRotateAxisY = static_cast<float>(-(lastY - ypos)) * sensitivity; // reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    if(Input::IsMouseButton(MouseButton::Right)){
        yaw += inputRotateAxisX;
        pitch -= inputRotateAxisY;
        if(pitch > 90) pitch = 90;
        if(pitch < -90) pitch = -90;

        float newRotationX = yaw;
        float newRotationY = pitch;

        //LogInfo("%f", newRotationX);
        transform->LocalEulerAngles(Vector3(newRotationY, newRotationX, 0));
    }
    
    transform->LocalPosition(pos);
}

void CameraMovementScript::OnStart(){
    TransformComponent& transform = GetEntity().GetComponent<TransformComponent>();

    pitch = transform.LocalEulerAngles().x;
    yaw = transform.LocalEulerAngles().y;
}

void CameraMovementScript::OnUpdate(){
    TransformComponent& transform = GetEntity().GetComponent<TransformComponent>();

    Vector3 pos = transform.LocalPosition();

    if(Input::IsMouseButton(MouseButton::Right)){
        if(Input::IsKey(KeyCode::W)) pos += transform.Back() * moveSpeed * Application::DeltaTime();
        if(Input::IsKey(KeyCode::S)) pos += transform.Forward() * moveSpeed * Application::DeltaTime();
        if(Input::IsKey(KeyCode::A)) pos += transform.Left() * moveSpeed * Application::DeltaTime();
        if(Input::IsKey(KeyCode::D)) pos += transform.Right() * moveSpeed * Application::DeltaTime();
    }

    double xpos;
    double ypos;
    Input::GetMousePosition(&xpos, &ypos);

    const float sensitivity = 0.6f;
    float inputRotateAxisX = static_cast<float>(-(xpos - lastX)) * sensitivity;
    float inputRotateAxisY = static_cast<float>(-(lastY - ypos)) * sensitivity; // reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    if(Input::IsMouseButton(MouseButton::Right)){
        yaw += inputRotateAxisX;
        pitch -= inputRotateAxisY;
        if(pitch > 90) pitch = 90;
        if(pitch < -90) pitch = -90;

        float newRotationX = yaw;
        float newRotationY = pitch;

        //LogInfo("%f", newRotationX);
        transform.LocalEulerAngles(Vector3(newRotationY, newRotationX, 0));
    }
    
    transform.LocalPosition(pos);
}