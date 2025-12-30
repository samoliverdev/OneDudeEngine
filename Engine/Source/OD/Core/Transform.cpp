#include "OD/pch.h"
#include "Transform.h"
#include "OD/Core/ImGui.h"

namespace OD{

Transform::Transform(const Matrix4& m){
    Vector3 s;
    Quaternion r;
    Vector3 t;
    Vector3 sk;
    Vector4 p;
    math::decompose(m, s, r, t, sk, p);

    position = t;
    rotation = r;
    scale = s;

    #ifndef TransformLessDataOptimzation
    isDirt = true;
    #endif
}

Transform Transform::DecomposeTransform(const glm::mat4& m){
    Transform result;

    // Extract translation
    result.position = glm::vec3(m[3]);

    // Remove perspective if any (ignore last row)
    glm::mat4 localMatrix = m;
    localMatrix[3] = glm::vec4(0, 0, 0, 1);

    // Extract scale factors from the basis vectors
    glm::vec3 col0 = glm::vec3(localMatrix[0]);
    glm::vec3 col1 = glm::vec3(localMatrix[1]);
    glm::vec3 col2 = glm::vec3(localMatrix[2]);

    result.scale.x = glm::length(col0);
    result.scale.y = glm::length(col1);
    result.scale.z = glm::length(col2);

    // Prevent division by zero
    if(result.scale.x != 0) col0 /= result.scale.x;
    if(result.scale.y != 0) col1 /= result.scale.y;
    if(result.scale.z != 0) col2 /= result.scale.z;

    // Reconstruct pure rotation matrix
    glm::mat3 rotationMatrix;
    rotationMatrix[0] = col0;
    rotationMatrix[1] = col1;
    rotationMatrix[2] = col2;

    result.rotation = glm::quat_cast(rotationMatrix);

    return result;
}

Transform Transform::DecomposePosRot(const glm::mat4& m){
    Transform t;
    math::extractPosRot(m, t.position, t.rotation);
    return t;
}

/*
Matrix4 Transform::GetModelMatrix(){
    #ifdef TransformLessDataOptimzation
    return Mathf::TRS(position, rotation, scale);
    #else
    if(isDirt == false) return modelMatrix;
    modelMatrix = Mathf::TRS(position, rotation, scale);
    isDirt = false;
    return modelMatrix;
    #endif

    //return Mathf::TRS(localPosition, localRotation, localScale);
}
*/

Vector3 Transform::InverseTransformDirection(Vector3 dir){
    Matrix4 matrix4 = GetModelMatrix();
    
    return math::inverse(matrix4) * Vector4(dir.x, dir.y, dir.z, 0);
    //return math::simdMul(math::inverse(matrix4), Vector4(dir.x, dir.y, dir.z, 0));
}

Vector3 Transform::TransformDirection(Vector3 dir){
    /*auto rotation = math::mat3(GetLocalModelMatrix()); // upper-left 3x3
    return rotation * dir;*/

    Matrix4 matrix4 = GetModelMatrix();
    
    return matrix4 * Vector4(dir.x, dir.y, dir.z, 0);
    //return math::simdMul(matrix4, Vector4(dir.x, dir.y, dir.z, 0));
}

Vector3 Transform::InverseTransformPoint(Vector3 point){
    Matrix4 matrix4 = GetModelMatrix();
    
    return math::inverse(matrix4) * Vector4(point.x, point.y, point.z, 1);
    //return math::simdMul(math::inverse(matrix4), Vector4(point.x, point.y, point.z, 1));
}

Vector3 Transform::TransformPoint(Vector3 point){
    Matrix4 matrix4 = GetModelMatrix();
    
    return matrix4 * Vector4(point.x, point.y, point.z, 1);
    //return math::simdMul(matrix4, Vector4(point.x, point.y, point.z, 1));
}

void Transform::OnGui(Transform& transform){
    //ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f);

    float p[] = {transform.Position().x, transform.Position().y, transform.Position().z};
    if(ImGui::DragFloat3("Position", p, 0.5f)){
        transform.Position(Vector3(p[0], p[1], p[2]));
    }

    float r[] = {transform.EulerAngles().x, transform.EulerAngles().y, transform.EulerAngles().z};
    if(ImGui::DragFloat3("Rotation", r, 0.5f)){
        transform.EulerAngles(Vector3(r[0], r[1], r[2]));
    }  

    float s[] = {transform.Scale().x, transform.Scale().y, transform.Scale().z};
    if(ImGui::DragFloat3("Scale", s, 0.5f)){
        transform.Scale(Vector3(s[0], s[1], s[2]));
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