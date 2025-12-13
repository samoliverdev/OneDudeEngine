#include "OD/pch.h"
#include "EditorCamera.h"
#include "OD/Core/Input.h"
#include "OD/Core/Application.h"

namespace OD{

void EditorCamera::OnStart(){
    transform.Position(Vector3(0, 15, 15));
    transform.EulerAngles(Vector3(-25, 0, 0));

    pitch = transform.EulerAngles().x;
    yaw = transform.EulerAngles().y;
}

void EditorCamera::OnUpdate(){
    Vector3 pos = transform.Position();

    if(Input::IsMouseButton(MouseButton::Right)){
        float targetSpeed = moveSpeed;
        if(Input::IsKey(KeyCode::Shift)){
            targetSpeed = fastMoveSpeed;
        }

        if(Input::IsKey(KeyCode::W)) pos += transform.Back() * targetSpeed * Application::DeltaTime();
        if(Input::IsKey(KeyCode::S)) pos += transform.Forward() * targetSpeed * Application::DeltaTime();
        if(Input::IsKey(KeyCode::A)) pos += transform.Left() * targetSpeed * Application::DeltaTime();
        if(Input::IsKey(KeyCode::D)) pos += transform.Right() * targetSpeed * Application::DeltaTime();
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
        transform.EulerAngles(Vector3(newRotationY, newRotationX, 0));
    }
    
    transform.Position(pos);
}

void AssetPreviewCamera::OnStart() {
    /*// Extrai yaw e pitch do transform
    Vector3 rot = transform.LocalEulerAngles();
    pitch = rot.x;
    yaw = rot.y;
    // Calcula distância com base na posição e no target
    distance = math::length(transform.LocalPosition() - target);*/

    lastX = 400; 
    lastY = 300;

    Vector3 pos = transform.Position();
    Vector3 dir = glm::normalize(pos - target);

    distance = glm::length(pos - target);
    pitch = glm::degrees(std::asin(dir.y));
    yaw = glm::degrees(std::atan2(dir.x, dir.z));
}

void AssetPreviewCamera::OnUpdate() {
    double xpos, ypos;
    Input::GetMousePosition(&xpos, &ypos);
    xpos = -xpos;

    float deltaX = static_cast<float>(xpos - lastX);
    float deltaY = static_cast<float>(ypos - lastY);
    lastX = xpos;
    lastY = ypos;

    if (Input::IsMouseButton(MouseButton::Right)) {
        yaw += deltaX * orbitSpeed;
        pitch += deltaY * orbitSpeed;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    float scroll = 0;//Input::GetMouseScroll(); // Espera-se que seja float
    distance -= scroll * zoomSpeed;
    if (distance < 1.0f) distance = 1.0f;

    float radYaw = Mathf::Deg2Rad(yaw);
    float radPitch = Mathf::Deg2Rad(pitch);

    Vector3 offset;
    offset.x = distance * std::cos(radPitch) * std::sin(radYaw);
    offset.y = distance * std::sin(radPitch);
    offset.z = distance * std::cos(radPitch) * std::cos(radYaw);

    Vector3 cameraPos = target + offset;

    transform.Position(cameraPos);
    transform.LookAt(target);
}

}