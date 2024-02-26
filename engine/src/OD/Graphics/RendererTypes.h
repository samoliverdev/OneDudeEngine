#pragma once
#include "OD/Defines.h"

namespace OD {

enum class OD_API DepthTest{
    DISABLE         = 0,
    LESS            = 1,
    LESS_EQUAL      = 2,
    EQUAL           = 3,
    GREATER         = 4,
    GREATER_EQUAL   = 5,
    DIFFERENT       = 6,
    NEVER           = 7,
    ALWAYS          = 8
};

enum class OD_API CullFace{
    NONE            = 0,
    BACK            = 1,
    FRONT           = 2,
    FRONT_AND_BACK  = 3
};

enum class OD_API BlendMode{
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA	
};

}