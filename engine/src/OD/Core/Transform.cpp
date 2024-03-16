#include "Transform.h"
#include "OD/Core/ImGui.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace OD{

Transform::Transform(const Matrix4& m){
    Vector3 s;
    Quaternion r;
    Vector3 t;
    Vector3 sk;
    Vector4 p;
    math::decompose(m, s, r, t, sk, p);

    localPosition = t;
    localRotation = r;
    localScale = s;
}

Matrix4 Transform::GetLocalModelMatrix(){
    if(isDirt == false) return localModelMatrix;
    localModelMatrix = Mathf::TRS(localPosition, localRotation, localScale);
    isDirt = false;
    return localModelMatrix;

    //return Mathf::TRS(_localPosition, _localRotation, _localScale);
}

Vector3 Transform::InverseTransformDirection(Vector3 dir){
    Matrix4 matrix4 = GetLocalModelMatrix();
    return math::inverse(matrix4) * Vector4(dir.x, dir.y, dir.z, 0);
}

Vector3 Transform::TransformDirection(Vector3 dir){
    Matrix4 matrix4 = GetLocalModelMatrix();
    return matrix4 * Vector4(dir.x, dir.y, dir.z, 0);
}

Vector3 Transform::InverseTransformPoint(Vector3 point){
    Matrix4 matrix4 = GetLocalModelMatrix();
    return math::inverse(matrix4) * Vector4(point.x, point.y, point.z, 1);
}

Vector3 Transform::TransformPoint(Vector3 point){
    Matrix4 matrix4 = GetLocalModelMatrix();
    return matrix4 * Vector4(point.x, point.y, point.z, 1);
}

void Transform::OnGui(Transform& transform){
    //ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f);

    float p[] = {transform.LocalPosition().x, transform.LocalPosition().y, transform.LocalPosition().z};
    if(ImGui::DragFloat3("Position", p, 0.5f)){
        transform.LocalPosition(Vector3(p[0], p[1], p[2]));
    }

    float r[] = {transform.LocalEulerAngles().x, transform.LocalEulerAngles().y, transform.LocalEulerAngles().z};
    if(ImGui::DragFloat3("Rotation", r, 0.5f)){
        transform.LocalEulerAngles(Vector3(r[0], r[1], r[2]));
    }  

    float s[] = {transform.LocalScale().x, transform.LocalScale().y, transform.LocalScale().z};
    if(ImGui::DragFloat3("Scale", s, 0.5f)){
        transform.LocalScale(Vector3(s[0], s[1], s[2]));
    } 

    //ImGui::PopItemWidth();
}

/*
Matrix4 TransformComponent::globalModelMatrix(){
    if(entity->GetParent() != nullptr){
        return entity->GetParent()->GetLocalModelMatrix() * GetLocalModelMatrix();
    }
    return GetLocalModelMatrix();
}
*/

}