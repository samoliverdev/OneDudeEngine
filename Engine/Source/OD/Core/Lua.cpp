#include "OD/pch.h"
#include "Lua.h"

namespace OD{

LuaBindsDB luaBindsDB;

LuaBindsDB& LuaBindsDB::Get(){
    return luaBindsDB;
}

}