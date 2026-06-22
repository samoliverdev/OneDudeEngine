#pragma once
#include "OD/Core/Math.h"

namespace OD{

inline Vector4 ToLinear(Vector4 srgb){
    return math::pow(srgb, Vector4(2.2f, 2.2f, 2.2f, 1));
};

inline Vector3 ToLinear(Vector3 srgb){
    return math::pow(srgb, Vector3(2.2f, 2.2f, 2.2f));
};

}