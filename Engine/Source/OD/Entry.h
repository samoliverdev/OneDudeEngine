#ifdef _WIN32
    //#define _CRTDBG_MAP_ALLOC //#define _CRTDBG_MAP_ALLOC_NEW
    //#include <crtdbg.h>
#endif

#include "OD/Core/Module.h"
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <iostream>
#include "CoreModulesStartup.h"

/*void* operator new(size_t size){
    LogInfo("Alloc: %zd", size);
    return malloc(size);
}
void operator delete(void* data){
    LogInfo("Dealloc");
    free(data);
}*/

extern OD::ApplicationConfig GetStartAppConfig();
extern OD::Module* CreateMainModule();

int main(int argc, char *argv[]){
    int* a = new int();

    #ifdef _WIN32
        //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    OD::CoreModulesStartup();

    for(int i = 0; i < argc; i++){
        OD::Application::GetArgs().push_back(std::string(argv[i]));
    }

    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig(), argc > 1 ? argv[1] : RESOURCES_PATH "")){
        printf("Application failed to create!.\n");
        return 1;
    }
    
    OD::Application::Run();

    #ifdef _WIN32 
        //_CrtDumpMemoryLeaks();
    #endif

    return 0;
}
