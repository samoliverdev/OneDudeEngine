#include "OD/Core/Module.h"
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include <stdio.h>
#include <filesystem>

extern OD::ApplicationConfig GetStartAppConfig();
extern OD::Module* CreateMainModule();

int main(int argc, char *argv[]){
    for(int i = 0; i < argc; i++){
        OD::Application::GetArgs().push_back(std::string(argv[i]));
    }

    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig(), argc > 1 ? argv[1] : RESOURCES_PATH "/")){
        printf("Application failed to create!.\n");
        return 1;
    }
    
    OD::Application::Run();

    return 0;
}