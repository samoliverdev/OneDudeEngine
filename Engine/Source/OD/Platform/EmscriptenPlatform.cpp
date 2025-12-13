#ifdef __EMSCRIPTEN__
#include "OD/pch.h"
#include "Platform.h"
#include "OD/Base.h"

namespace OD{

void* Platform::LoadDynamicLibrary(const char* dll){
    return nullptr;
}

void* Platform::LoadDynamicFunction(void* dll, const char* funcName){
    return nullptr;
}

bool Platform::FreeDynimicLibrary(void* dll){
    return false;
}

}
#endif