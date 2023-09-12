#include "Transform.h"
//#include "OD/ECS/ECS.h"

namespace OD{

Matrix4 Transform::GetLocalModelMatrix(){
    if(_isDirt == false) return _localModelMatrix;
    _localModelMatrix = Matrix4::TRS(_localPosition, _localRotation, _localScale);
    _isDirt = false;
    return _localModelMatrix;
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