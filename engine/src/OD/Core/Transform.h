#pragma once

#include "Math.h"
#include <stdio.h>

namespace OD {
    
class Transform{
public:
    Transform(){}
    Transform(Vector3 pos, Quaternion rot, Vector3 scale):
        _localPosition(pos), _localRotation(rot), _localScale(scale){}
    Transform(const Matrix4& m);

    Matrix4 GetLocalModelMatrix();

    inline Vector3 forward(){ return _localRotation * Vector3Forward; }
    inline Vector3 back(){ return _localRotation * Vector3Back; }
    inline Vector3 left(){ return _localRotation * Vector3Left; }
    inline Vector3 right(){ return _localRotation * Vector3Right; }
    inline Vector3 up(){ return _localRotation * Vector3Up; }
    inline Vector3 down(){ return _localRotation * Vector3Down; }

    inline Vector3 localPosition(){ return _localPosition; }
    inline void localPosition(Vector3 pos){ _localPosition = pos; _isDirt = true; }

    inline Vector3 localEulerAngles(){ 
        return _localEulerAngles;
    }

    inline void localEulerAngles(Vector3 euler){ 
        _localEulerAngles = euler;
        _localRotation = Quaternion(Mathf::Deg2Rad(_localEulerAngles));
        _isDirt = true;
    }

    inline Quaternion localRotation(){ return _localRotation; }
    inline void localRotation(Quaternion rot){ 
        _localRotation = rot; 
        _isDirt = true; 
        _localEulerAngles = Mathf::Rad2Deg(math::eulerAngles(_localRotation));
    }

    inline Vector3 localScale(){ return _localScale; }
    inline void localScale(Vector3 scale){ _localScale = scale; _isDirt = true; }

    inline void SetModelMatrix(Matrix4 matrix){ _isDirt = false; _localModelMatrix = matrix; }

    inline bool operator==(const Transform& b){
        return 
            this->_localPosition == b._localPosition &&
            this->_localRotation == b._localRotation &&
            this->_localScale == b._localScale;
    }

    inline bool operator!=(const Transform& b){
        return !(*this == b);
    }

    inline static Transform Combine(const Transform& a, const Transform& b){
        Transform out;

        out._localScale = a._localScale * b._localScale;
        out._localRotation = b._localRotation * a._localRotation;
        
        out._localPosition = a._localRotation * (a._localScale * b._localPosition);
        out._localPosition = a._localPosition + out._localPosition;

        return out;
    }

    inline static Transform Mix(const Transform& a, const Transform& b, float t){
        Quaternion bRot = b._localRotation;
        if(math::dot(a._localRotation, bRot) < 0.0f){
            bRot = -bRot;
        }

        return Transform(
            math::mix(a._localPosition, b._localPosition, t),
            math::lerp(a._localRotation, bRot, t),
            math::mix(a._localScale, b._localScale, t)
        );
    }

protected:
    bool _isDirt = true;
    Vector3 _localPosition = Vector3Zero;
    Vector3 _localScale = Vector3One;
    Quaternion _localRotation = QuaternionIdentity;
    Vector3 _localEulerAngles = Vector3Zero;
    Matrix4 _localModelMatrix = Matrix4Identity;
};

}