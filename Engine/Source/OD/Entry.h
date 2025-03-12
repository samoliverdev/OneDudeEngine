#include "OD/Core/Module.h"
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include <stdio.h>
#include <filesystem>
#include "CoreModulesStartup.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

extern OD::ApplicationConfig GetStartAppConfig();
extern OD::Module* CreateMainModule();

int CustomReportHandler(int reportType, char* message, int* returnValue){
    // Break into the debugger
    __debugbreak();
    // Optionally, handle the message (e.g., log it) or pass it to the default handler
    // ...
    // Return 1 to invoke the default report handler after breaking
    return 1;
}

int main(int argc, char *argv[]){
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetBreakAlloc(-1);

    //_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    //_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CustomReportHandler);
    //Assert(false && "Test");

    OD::CoreModulesStartup();

    /*for(int i = 0; i < argc; i++){
        OD::Application::GetArgs().push_back(std::string(argv[i]));
    }*/

    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig(), argc > 1 ? argv[1] : RESOURCES_PATH "")){
        printf("Application failed to create!.\n");
        return 1;
    }
    
    OD::Application::Run();

    return 0;
}