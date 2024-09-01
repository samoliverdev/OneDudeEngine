#include "CoreModulesStartup.h"
#include "OD/Core/Application.h"
#include "OD/Core/Asset.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Scene/Scripts.h"
#include "OD/Animation/Animator.h"
#include "OD/Physics/PhysicsSystem.h"  
#include "OD/RenderPipeline/StandRenderPipeline.h"
#include "OD/Audio/AudioClip.h"
#include "OD/Audio/AudioSystem.h"
#include "OD/Navmesh/Navmesh.h"
#include <filesystem>

namespace OD{

void CoreModulesStartup(){
    LogInfo("CoreModulesStartup");
    GraphicsModuleInit();
    StandRenderPipelineModuleInit();
    PhysicsModuleInit();
    ScriptModuleInit();
    AnimatorModuleInit();
    AudioModuleInit();
    NavmeshModuleInit();
    SceneManagerModuleInit();
}

}