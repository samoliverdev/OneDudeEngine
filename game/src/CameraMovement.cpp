#include "CameraMovement.h"

void CameraMovement::OnStart(){
    pitch = transform->localEulerAngles().x;
    yaw = transform->localEulerAngles().y;
}

void CameraMovement::OnUpdate(){
    Vector3 pos = transform->localPosition();

    if(Input::IsKey(KeyCode::W)) pos += transform->back() * moveSpeed * Application::deltaTime();
    if(Input::IsKey(KeyCode::S)) pos += transform->forward() * moveSpeed * Application::deltaTime();
    if(Input::IsKey(KeyCode::A)) pos += transform->left() * moveSpeed * Application::deltaTime();
    if(Input::IsKey(KeyCode::D)) pos += transform->right() * moveSpeed * Application::deltaTime();

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
        transform->localEulerAngles(Vector3(newRotationY, newRotationX, 0));
    }
    
    transform->localPosition(pos);
}

void CameraMovementScript::OnStart(){
    TransformComponent& transform = entity().GetComponent<TransformComponent>();

    pitch = transform.localEulerAngles().x;
    yaw = transform.localEulerAngles().y;
}

void CameraMovementScript::OnUpdate(){
    TransformComponent& transform = entity().GetComponent<TransformComponent>();

    Vector3 pos = transform.localPosition();

    if(Input::IsKey(KeyCode::W)) pos += transform.back() * moveSpeed * Application::deltaTime();
    if(Input::IsKey(KeyCode::S)) pos += transform.forward() * moveSpeed * Application::deltaTime();
    if(Input::IsKey(KeyCode::A)) pos += transform.left() * moveSpeed * Application::deltaTime();
    if(Input::IsKey(KeyCode::D)) pos += transform.right() * moveSpeed * Application::deltaTime();

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
        transform.localEulerAngles(Vector3(newRotationY, newRotationX, 0));
    }
    
    transform.localPosition(pos);
}