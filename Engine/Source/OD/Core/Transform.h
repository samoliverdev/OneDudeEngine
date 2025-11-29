#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "Math.h"
#include <stdio.h>

namespace OD {
    
class Scene;

//TODO: This is bug my Bone Aim
//#define ExperimentalTransformOptimzation

#define TransformLessDataOptimzation

class OD_API alignas(16) Transform{
    friend class TransformComponent;
    //friend class Scene;
public:
    Transform(){}
    Transform(const Matrix4& m);
    
    #ifdef TransformLessDataOptimzation
    Transform(Vector3 inpos, Quaternion inrot = QuaternionIdentity, Vector3 inscale = Vector3(1, 1, 1)):
        position(inpos), rotation(inrot), scale(inscale){}
    #else
    Transform(Vector3 pos, Quaternion rot = QuaternionIdentity, Vector3 inscale = Vector3(1, 1, 1)):
        position(pos), rotation(rot), scale(inscale), isDirt(true){}
    #endif
    
    //Matrix4 GetModelMatrix();

    inline Matrix4 GetModelMatrix() const {
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

    inline Vector3 Forward() const { return rotation * Vector3Forward; }
    inline Vector3 Back() const { return rotation * Vector3Back; }
    inline Vector3 Left() const { return rotation * Vector3Left; }
    inline Vector3 Right() const { return rotation * Vector3Right; }
    inline Vector3 Up() const { return rotation * Vector3Up; }
    inline Vector3 Down() const { return rotation * Vector3Down; }

    inline Vector3 Position() const { return position; }
    
    inline void Position(Vector3 pos){ 
        position = pos; 
        #ifndef TransformLessDataOptimzation
        isDirt = true; 
        #endif
    }

    inline Vector3 EulerAngles() const { 
        #ifdef TransformLessDataOptimzation
        return Mathf::Rad2Deg(math::eulerAngles(rotation));
        #else
        return eulerAngles; 
        #endif
    }

    inline void EulerAngles(Vector3 euler){ 
        #ifdef TransformLessDataOptimzation
        rotation = Quaternion(Mathf::Deg2Rad(euler));
        #else
        eulerAngles = euler;
        rotation = Quaternion(Mathf::Deg2Rad(eulerAngles));
        isDirt = true;
        #endif
    }

    inline Quaternion Rotation() const { return rotation; }
    inline void Rotation(Quaternion rot){ 
        rotation = rot; 
        #ifndef TransformLessDataOptimzation
        isDirt = true; 
        eulerAngles = Mathf::Rad2Deg(math::eulerAngles(rotation));
        #endif
    }

    inline Vector3 Scale() const { return scale; }
    
    inline void Scale(Vector3 inscale){ 
        scale = inscale; 
        #ifndef TransformLessDataOptimzation
        isDirt = true; 
        #endif
    }

    inline void Rotate(Vector3 ineulerAngles, bool relativeToLocal = true) {
        Quaternion _rotation = Quaternion(Mathf::Deg2Rad(ineulerAngles));
        if (relativeToLocal) {
            rotation = rotation * _rotation;
        } else {
            rotation = _rotation * rotation;
        }
        #ifndef TransformLessDataOptimzation
        eulerAngles = Mathf::Rad2Deg(math::eulerAngles(rotation));
        isDirt = true;
        #endif
    }

    inline void LookAt(Vector3 target, Vector3 worldUp = Vector3Up) {
        Vector3 direction = math::normalize(target - position);// Calculate the forward direction
    
        if(math::abs(math::dot(direction, worldUp)) > 0.9999f){// Avoid degenerate case when direction is parallel to up vector
            worldUp = Vector3Right;// If direction is almost exactly up or down, use a different up vector
        }

        Quaternion newRotation = math::quatLookAt(direction, worldUp);
        Rotation(newRotation);// Apply the rotation
    }

    inline void LookAtDirection(Vector3 direction, Vector3 worldUp = Vector3Up){
        direction = math::normalize(direction);
        
        if(math::abs(math::dot(direction, worldUp)) > 0.9999f){// Avoid degenerate case when direction is parallel to up vector
            worldUp = Vector3Right;// If direction is almost exactly up or down, use a different up vector
        }

        Quaternion newRotation = math::quatLookAt(direction, worldUp);
        Rotation(newRotation);
    }

    //Transforms a direction from world space to local space. The opposite of Transform.TransformDirection.
    Vector3 InverseTransformDirection(Vector3 dir); 
    
    //Transforms direction from local space to world space.
    Vector3 TransformDirection(Vector3 dir); 

    //Transforms position from world space to local space.
    Vector3 InverseTransformPoint(Vector3 point); 

    //Transforms position from local space to world space.
    Vector3 TransformPoint(Vector3 point); 

    inline bool operator==(const Transform& b){
        return 
            this->position == b.position &&
            this->rotation == b.rotation &&
            this->scale == b.scale;
    }

    inline bool operator!=(const Transform& b){
        return !(*this == b);
    }

    static Transform DecomposeTransform(const Matrix4& m);
    static Transform DecomposePosRot(const Matrix4& m);

    inline static Transform Mix(const Transform& a, const Transform& b, float t) {
        Quaternion bRot = b.rotation;
        /*if(math::dot(a._localRotation, bRot) < 0.0f) {
            bRot = -bRot;
        }*/
        return Transform(
            math::mix(a.position, b.position, t),
            math::slerp(a.rotation, bRot, t),
            //math::normalize( math::slerp(math::normalize(a.localRotation), math::normalize(bRot), t) ),
            //math::normalize( math::lerp(math::normalize(a.localRotation), math::normalize(bRot), t) ),
            math::mix(a.scale, b.scale, t)
        );
    }

    inline static Transform Inverse(Transform& t){
        //#ifdef ExperimentalTransformOptimzation
        #define VEC3_EPSILON 0.000001f
        
        /*Transform inv;
        inv.Rotation(math::inverse(t.Rotation()));
        inv.scale.x = fabs(t.scale.x) < VEC3_EPSILON ? 0.0f : 1.0f / t.scale.x;
        inv.scale.y = fabs(t.scale.y) < VEC3_EPSILON ? 0.0f : 1.0f / t.scale.y;
        inv.scale.z = fabs(t.scale.z) < VEC3_EPSILON ? 0.0f : 1.0f / t.scale.z;
        Vector3 invTranslation = t.Position() * -1.0f;
        inv.Position(inv.Rotation() * (inv.Scale() * invTranslation) );
        return inv;*/

        Transform inv;
        inv.Rotation(glm::inverse(t.Rotation()));// Invert rotation and scale
        inv.Scale({
            fabs(t.Scale().x) < VEC3_EPSILON ? 0.0f : 1.0f / t.Scale().x,
            fabs(t.Scale().y) < VEC3_EPSILON ? 0.0f : 1.0f / t.Scale().y,
            fabs(t.Scale().z) < VEC3_EPSILON ? 0.0f : 1.0f / t.Scale().z
        });
        glm::vec3 invTranslation = -t.Position();// Compute inverse position: -R⁻¹ * S⁻¹ * t.Position()
        inv.Position(inv.Rotation() * (inv.Scale() * invTranslation));
        return inv;

        /*#else
        return Transform(math::inverse(t.GetModelMatrix()));
        #endif*/
    }

    inline static Transform Combine(const Transform& a, const Transform& b){
        //#ifdef ExperimentalTransformOptimzation

        /*Transform out;
        out.Scale(a.Scale() * b.Scale());
        out.Rotation(b.Rotation() * a.Rotation());
        out.Position(a.Rotation() * (a.Scale() * b.Position()));
        out.Position(a.Position() + out.Position());
        return out;*/

        Transform out;
        out.Scale(a.Scale() * b.Scale());
        out.Rotation(a.Rotation() * b.Rotation()); // ✅ Correct order
        out.Position(a.Position() + (a.Rotation() * (a.Scale() * b.Position())));
        return out;

        /*#else
        return Transform(math::simdMul(a.GetModelMatrix(), b.GetModelMatrix()));
        #endif*/
    }

    static void OnGui(Transform& e);

    template <class Archive>
    void serialize(Archive& ar){
        #ifdef TransformLessDataOptimzation
        ArchiveDump(ar, CEREAL_NVP(position)); 
        ArchiveDump(ar, CEREAL_NVP(rotation));
        ArchiveDump(ar, CEREAL_NVP(scale)); 
        #else
        ArchiveDump(ar, CEREAL_NVP(position)); 
        ArchiveDump(ar, CEREAL_NVP(rotation));
        ArchiveDump(ar, CEREAL_NVP(eulerAngles)); 
        ArchiveDump(ar, CEREAL_NVP(scale)); 
        //ArchiveDump(ar, CEREAL_NVP(isDirt));
        #endif
    }

protected:
    #ifdef TransformLessDataOptimzation
    Quaternion rotation = QuaternionIdentity;
    Vector3 position = Vector3Zero;
    Vector3 scale = Vector3One;
    #else
    Matrix4 modelMatrix = Matrix4Identity;
    Quaternion rotation = QuaternionIdentity;
    Vector3 position = Vector3Zero;
    Vector3 scale = Vector3One;
    Vector3 eulerAngles = Vector3Zero;
    bool isDirt = true;
    #endif
};

}