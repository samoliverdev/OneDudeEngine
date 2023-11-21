#include <OD/Entry.h>

#include "1_BaseMesh.h"
#include "2_LoadModel.h"
#include "3_Light.h"
#include "4_ECS.h"
//#include "Framebuffer_5.h"
#include "6_Physics.h"
#include "7_Animation.h"
#include "9_Serialization.h"
#include "10_SynthCity.h"

#include <string>

OD::ApplicationConfig GetStartAppConfig(){
    return OD::ApplicationConfig{
        0, 0,
        800, 600,
        "Game1"
    };
}

OD::Module* CreateMainModule(){
    int i = 6;
    if(OD::Application::args.size() > 1) i = atoi(OD::Application::args[1].c_str());

    if(i == 0) return new BaseMesh_1();
    if(i == 1) return new LoadModel_2();
    if(i == 2) return new Light_3();
    if(i == 3) return new ECS_4();
    //if(i == 4) return new Framebuffer_5();
    if(i == 5) return new Physics_6();
    if(i == 6) return new Animation_7();
    if(i == 8) return new Serialization_9();
    if(i == 9) return new SynthCity_10();

    return new BaseMesh_1();
}
