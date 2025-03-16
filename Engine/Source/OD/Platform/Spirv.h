#if defined(WEBGPU_SUPPORT)
#pragma once
#include "OD/Defines.h"
#include <unordered_map>
#include <string>

namespace OD{

struct MaterialMainSetDef{
    struct Member{
        size_t pos;
        size_t size;
    };

    std::string bufferName;
    size_t bufferSize;
    std::unordered_map<std::string, Member> bufferMembers;
    
    std::unordered_map<std::string, int> textureBindings;
};

bool SpirvReflectMainSet(void* data, size_t size, MaterialMainSetDef& out);

}
#endif