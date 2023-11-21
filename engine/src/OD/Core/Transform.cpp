#include "Transform.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace OD{

Transform::Transform(const Matrix4& m){
    Vector3 s;
    Quaternion r;
    Vector3 t;
    Vector3 sk;
    Vector4 p;
    math::decompose(m, s, r, t, sk, p);

    _localPosition = t;
    _localRotation = r;
    _localScale = s;
}

Matrix4 Transform::GetLocalModelMatrix(){
    if(_isDirt == false) return _localModelMatrix;
    _localModelMatrix = Mathf::TRS(_localPosition, _localRotation, _localScale);
    _isDirt = false;
    return _localModelMatrix;

    //return Mathf::TRS(_localPosition, _localRotation, _localScale);
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