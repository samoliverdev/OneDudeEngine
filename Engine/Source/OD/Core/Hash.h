#pragma once
#include "OD/Defines.h"
#include <string_view>
#include <cstdint>

namespace OD{

struct OD_API Hash{
    static constexpr uint32_t StringToHash(std::string_view str){
        uint32_t hash = 2166136261u; // offset basis

        for(char c : str){
            hash ^= static_cast<uint32_t>(c);
            hash *= 16777619u; // FNV prime
        }

        return hash;
    }
};

}