#include "Platform.h"
#include "OD/Defines.h"
#include <Windows.h>

namespace OD{

void* Platform::LoadDynamicLibrary(char* dll){
    HMODULE result = LoadLibraryA(dll);
    Assert(result && "Failed to load dll");
    return result;
}

void* Platform::LoadDynamicFunction(void* dll, char* funcName){
    FARPROC proc = GetProcAddress((HMODULE)dll, funcName);
    Assert(proc && "Failed to load function from dll");
    return (void*)proc;
}

bool Platform::FreeDynimicLibrary(void* dll){
    BOOL freeResult = FreeLibrary((HMODULE)dll);
    Assert(freeResult && "Failed to FreeLibrary");
    return (bool)freeResult;
}

}