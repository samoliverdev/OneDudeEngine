#include <OD/Entry.h>

#include "Samples/BaseMesh.h"
#include "Samples/LoadModel.h"
#include "Samples/Light.h"
#include "Samples/ECS.h"
//#include "Framebuffer_5.h"
#include "Samples/Physics.h"
#include "Samples/Animation.h"
#include "Samples/Serialization.h"
#include "Samples/SynthCity.h"
#include "Samples/Animator.h"
#include "Samples/UniformBuffer.h"
#include "Samples/RenderPipeline.h"
#include "Samples/UberShader.h"
#include "Samples/DynamicModule.h"
#include "Samples/CharacterController.h"
#include "Samples/Navmesh.h"
#include "Samples/Boids.h"
#include "Samples/Sponza.h"
#include "Minicraft/Minicraft.h"
#include "ProceduralTerrain/ProceduralTerrain.h"
#include "TerrainRenderer/TerrainRenderer.h"
#include "TerrainRenderer/TerrainRenderer2.h"

#include <string>

OD::ApplicationConfig GetStartAppConfig(){
    return OD::ApplicationConfig{
        0, 0,
        800, 600,
        "Game1"
    };
}

OD::Module* CreateMainModule(){
    int i = 12;
    if(OD::Application::GetArgs().size() > 1) i = atoi(OD::Application::GetArgs()[1].c_str());

    if(i == 0) return new BaseMeshSample();
    if(i == 1) return new LoadModelSample();
    if(i == 2) return new LightSample();
    if(i == 3) return new ECSSample();
    //if(i == 4) return new Framebuffer_5();
    if(i == 5) return new PhysicsSample();
    if(i == 6) return new AnimationSample();
    if(i == 8) return new SerializationSample();
    if(i == 9) return new SynthCitySample();
    if(i == 10) return new AnimatorSample();
    if(i == 11) return new UniformBufferSample();
    if(i == 12) return new RenderPipelineSample();
    if(i == 14) return new UberShaderSample();
    if(i == 15) return new Minicraft();
    if(i == 16) return new DynamicModuleSample();
    if(i == 17) return new ProceduralTerrain();
    if(i == 18) return new TerrainRenderer();
    if(i == 19) return new TerrainRenderer2();
    if(i == 20) return new CharacterControllerSample();
    if(i == 21) return new NavmeshSample();
    if(i == 22) return new BoidsSample();
    if(i == 23) return new SponzaSample();

    return new BaseMeshSample();
}
