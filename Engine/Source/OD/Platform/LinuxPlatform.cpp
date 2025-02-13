#ifdef __linux__
#include "Platform.h"
#include "OD/Base.h"
#include <dlfcn.h>

namespace OD{

void* Platform::LoadDynamicLibrary(const char* dll){
    void* result = dlopen(dll, RTLD_NOW);
    Assert(result && "Failed to load dll");
    return result;
}

void* Platform::LoadDynamicFunction(void* dll, const char* funcName){
    void* proc = dlsym(dll, funcName);
    Assert(proc && "Failed to load function from dll");
    return proc;
}

bool Platform::FreeDynimicLibrary(void* dll){
    int freeResult = dlclose(dll);
    Assert(freeResult == 0 && "Failed to FreeLibrary");
    return freeResult == 0;
}

}
#endif