#include "OD/Core/Module.h"
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include <stdio.h>
#include <filesystem>

extern OD::ApplicationConfig GetStartAppConfig();
extern OD::Module* CreateMainModule();

int main(int argc, char *argv[]){
    //std::filesystem::current_path("../../");
    std::filesystem::current_path(RESOURCES_PATH "/");

    LogInfo("Setting Current Path: %s", RESOURCES_PATH "/");
    
    for(int i = 0; i < argc; i++){
        OD::Application::GetArgs().push_back(std::string(argv[i]));
    }

    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig())){
        printf("Application failed to create!.\n");
        return 1;
    }
    
    OD::Application::Run();

    return 0;
}