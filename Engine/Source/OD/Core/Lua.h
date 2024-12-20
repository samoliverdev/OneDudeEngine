#pragma once
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "OD/Defines.h"

namespace OD{

class OD_API LuaBindsDB{
public:
    template<typename T>
    void RegisterLuaBind(){
        bindFuncs.push_back(&T::CreateLuaBind);
    }

    inline void RegisterLuaBind(std::function<void(sol::state&)> func){
        bindFuncs.push_back(func);
    }

    static LuaBindsDB& Get();

    std::vector<std::function<void(sol::state&)>> bindFuncs;
};

}