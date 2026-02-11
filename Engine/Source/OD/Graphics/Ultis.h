#pragma once
#include "OD/Core/Math.h"

namespace OD{
   
inline int CalculateMipCount(int width, int height){
    int levels = 1;
    while(width > 1 || height > 1){
        width  = math::max(1, width  / 2);
        height = math::max(1, height / 2);
        levels++;
    }
    return levels;
};

}