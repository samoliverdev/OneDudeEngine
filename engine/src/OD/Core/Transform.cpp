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

/*
Matrix4 TransformComponent::globalModelMatrix(){
    if(entity->GetParent() != nullptr){
        return entity->GetParent()->GetLocalModelMatrix() * GetLocalModelMatrix();
    }
    return GetLocalModelMatrix();
}
*/

}