#pragma once

#include "Math.h"
#include <stdio.h>

namespace OD {
    
class Transform{
public:
    Matrix4 GetLocalModelMatrix();

    inline Vector3 forward(){ return _localRotation * Vector3::forward; }
    inline Vector3 back(){ return _localRotation * Vector3::back; }
    inline Vector3 left(){ return _localRotation * Vector3::left; }
    inline Vector3 right(){ return _localRotation * Vector3::right; }

    inline Vector3 localPosition(){ return _localPosition; }
    inline void localPosition(Vector3 pos){ _localPosition = pos; _isDirt = true; }

    inline Vector3 localEulerAngles(){ 
        //Vector3 result = _localRotation.eulerAngles();
        //return Vector3(Mathf::Rad2Deg(result.x), Mathf::Rad2Deg(result.y), Mathf::Rad2Deg(result.z));
        return _localEulerAngles;
    }

    inline void localEulerAngles(Vector3 euler){ 
        //_localRotation.eulerAngles(Vector3(Mathf::Deg2Rad(euler.x), Mathf::Deg2Rad(euler.y), Mathf::Deg2Rad(euler.z)));
        //_isDirt = true;
        _localEulerAngles = euler;
        _localRotation.eulerAngles(Vector3(Mathf::Deg2Rad(_localEulerAngles.x), Mathf::Deg2Rad(_localEulerAngles.y), Mathf::Deg2Rad(_localEulerAngles.z)));
        _isDirt = true;
    }

    inline Quaternion localRotation(){ return _localRotation; }
    inline void localRotation(Quaternion rot){ 
        _localRotation = rot; 
        _isDirt = true; 
        _localEulerAngles = _localRotation.eulerAngles();
    }

    inline Vector3 localScale(){ return _localScale; }
    inline void localScale(Vector3 scale){ _localScale = scale; _isDirt = true; }

    inline void SetModelMatrix(Matrix4 matrix){ _isDirt = false; _localModelMatrix = matrix; }

protected:
    bool _isDirt = true;
    Vector3 _localPosition = Vector3::zero;
    Vector3 _localScale = Vector3::one;
    Quaternion _localRotation = Quaternion::identity;
    Vector3 _localEulerAngles = Vector3::zero;
    Matrix4 _localModelMatrix = Matrix4::identity;

    //Entity* entity;
};

}