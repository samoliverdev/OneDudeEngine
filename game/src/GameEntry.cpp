#include <OD/Entry.h>

#include "BaseMesh_1.h"
#include "LoadModel_2.h"
#include "Light_3.h"
#include "ECS_4.h"
//#include "Framebuffer_5.h"
#include "Physics_6.h"
#include "SkinnedMesh_7.h"

#include <string>

OD::ApplicationConfig GetStartAppConfig(){
    return OD::ApplicationConfig{
        0, 0,
        800, 600,
        "Game1"
    };
}

OD::Module* CreateMainModule(){
    int i = 5;
    if(OD::Application::args.size() > 1) i = atoi(OD::Application::args[1].c_str());

    if(i == 0) return new BaseMesh_1();
    if(i == 1) return new LoadModel_2();
    if(i == 2) return new Light_3();
    if(i == 3) return new ECS_4();
    //if(i == 4) return new Framebuffer_5();
    if(i == 5) return new Physics_6();
    if(i == 6) return new SkinnedMesh_7();

    return new BaseMesh_1();
}
