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

void Transform::OnGui(Transform& transform){
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