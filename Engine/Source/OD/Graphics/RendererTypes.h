#pragma once
#include "OD/Defines.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace OD {

enum class OD_API_IMPORT DepthTest{
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

enum class OD_API_IMPORT CullFace{
    NONE            = 0,
    BACK            = 1,
    FRONT           = 2,
    FRONT_AND_BACK  = 3
};

enum class OD_API_IMPORT BlendMode{
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

struct OD_API UniformBufferDef{
    enum class Type{Int, Float, Vec2, Vec3, Vec4, Mat4};

    struct Member{
        //std::string name;
        //Type type;
        size_t pos;
        size_t size;
    };

    std::string name;
    size_t size;
    std::unordered_map<std::string, Member> members;
    //std::vector<Member> members;
};

}