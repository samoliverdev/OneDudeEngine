#pragma once

#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "Math.h"
#include <stdio.h>

namespace OD {
    
class OD_API Transform{
    friend class TransformComponent;
public:
    Transform(){}
    Transform(Vector3 pos, Quaternion rot = QuaternionIdentity, Vector3 scale = Vector3(1, 1, 1)):
        localPosition(pos), localRotation(rot), localScale(scale), isDirt(true){}
    Transform(const Matrix4& m);

    Matrix4 GetLocalModelMatrix();

    inline Vector3 Forward(){ return localRotation * Vector3Forward; }
    inline Vector3 Back(){ return localRotation * Vector3Back; }
    inline Vector3 Left(){ return localRotation * Vector3Left; }
    inline Vector3 Right(){ return localRotation * Vector3Right; }
    inline Vector3 Up(){ return localRotation * Vector3Up; }
    inline Vector3 Down(){ return localRotation * Vector3Down; }

    inline Vector3 LocalPosition(){ return localPosition; }
    inline void LocalPosition(Vector3 pos){ localPosition = pos; isDirt = true; }

    inline Vector3 LocalEulerAngles(){ 
        return localEulerAngles;
    }

    inline void LocalEulerAngles(Vector3 euler){ 
        localEulerAngles = euler;
        localRotation = Quaternion(Mathf::Deg2Rad(localEulerAngles));
        isDirt = true;
    }

    inline Quaternion LocalRotation(){ return localRotation; }
    inline void LocalRotation(Quaternion rot){ 
        localRotation = rot; 
        isDirt = true; 
        localEulerAngles = Mathf::Rad2Deg(math::eulerAngles(localRotation));
    }

    inline Vector3 LocalScale(){ return localScale; }
    inline void LocalScale(Vector3 scale){ localScale = scale; isDirt = true; }

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
            this->localPosition == b.localPosition &&
            this->localRotation == b.localRotation &&
            this->localScale == b.localScale;
    }

    inline bool operator!=(const Transform& b){
        return !(*this == b);
    }

    inline static Transform Mix(const Transform& a, const Transform& b, float t) {
        Quaternion bRot = b.localRotation;
        /*if(math::dot(a._localRotation, bRot) < 0.0f) {
            bRot = -bRot;
        }*/
        return Transform(
            math::mix(a.localPosition, b.localPosition, t),
            math::slerp(a.localRotation, bRot, t),
            //math::normalize( math::slerp(math::normalize(a.localRotation), math::normalize(bRot), t) ),
            //math::normalize( math::lerp(math::normalize(a.localRotation), math::normalize(bRot), t) ),
            math::mix(a.localScale, b.localScale, t)
        );
    }

    inline static Transform Inverse(Transform& t){
        return Transform(math::inverse(t.GetLocalModelMatrix()));
    }

    inline static Transform Combine(Transform& a, Transform& b){
        return Transform(a.GetLocalModelMatrix() * b.GetLocalModelMatrix());
    }

    static void OnGui(Transform& e);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(localPosition)); 
        ArchiveDump(ar, CEREAL_NVP(localRotation));
        ArchiveDump(ar, CEREAL_NVP(localEulerAngles)); 
        ArchiveDump(ar, CEREAL_NVP(localScale)); 
        ArchiveDump(ar, CEREAL_NVP(isDirt));
    }

protected:
    bool isDirt = true;
    Vector3 localPosition = Vector3Zero;
    Vector3 localScale = Vector3One;
    Quaternion localRotation = QuaternionIdentity;
    Vector3 localEulerAngles = Vector3Zero;
    Matrix4 localModelMatrix = Matrix4Identity;
};

}