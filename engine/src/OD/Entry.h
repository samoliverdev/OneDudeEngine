#include "OD/Core/Module.h"
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include <stdio.h>

extern OD::ApplicationConfig GetStartAppConfig();
extern OD::Module* CreateMainModule();

int main(int argc, char *argv[]) {
    for(int i = 0; i < argc; i++){
        OD::Application::args.push_back(std::string(argv[i]));
    }

    OD_PROFILE_BEGIN_SESSION("Startup", "OD_Profile_Startup.json");
    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig())) {
        printf("Application failed to create!.\n");
        return 1;
    }
    OD_PROFILE_END_SESSION();
    
    OD_PROFILE_BEGIN_SESSION("Runtime", "OD_Profile_Runtime.json");
    OD::Application::Run();
    OD_PROFILE_END_SESSION();

    return 0;
}