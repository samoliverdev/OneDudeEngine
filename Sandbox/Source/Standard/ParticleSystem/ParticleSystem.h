#pragma once
#include <OD/Scene/Scene.h>
#include <OD/Core/Color.h>
#include <OD/Core/Action.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/InstancingBuffer.h>
#include <OD/RenderPipeline/RenderContext.h>
#include <OD/Serialization/ImGuiArchive.h>
#include <OD/Physics/PhysicsSystem.h>
#include "Standard/Ultis/ImGradientHDR.h"
#include "Standard/Ultis/AnimationCurve.h"

namespace OD{
    class Material;
    class PhysicsSystem;
}

using namespace OD;

namespace Standard{

class ParticleEmiter;
struct ParticleData;
struct ParticleRunningData;

class IParticleSpawnModule{
public:
    virtual void OnGui(){}
    virtual void OnStartSpawnUpdate(ParticleEmiter& system){};
    virtual void OnSpawnUpdate(ParticleEmiter& system){};
};

class IInitParticleModule{
public:
    virtual void OnGui(){}
    virtual void OnInitParticle(ParticleData& particle){};
};

class IParticleUpdateModule{
public:
    virtual void OnGui(){}
    virtual void OnParticleUpdate(ParticleData& particle, ParticleRunningData& runningData){};
};

class SpawnModule: public IParticleSpawnModule{
public:
    int singleBustEmiterCount = 0;
    float overTimeEmiterRate = 25;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, singleBustEmiterCount);
        ArchiveDumpNVP(ar, overTimeEmiterRate);
    }

    void OnGui() override;
    void OnStartSpawnUpdate(ParticleEmiter& system) override;
    void OnSpawnUpdate(ParticleEmiter& system) override;
private:
    float emissionAccumulator = 0; 
};

class InitialLifeModule: public IInitParticleModule{
public:
    float minLife = 1;
    float maxLife = 2;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, minLife);
        ArchiveDumpNVP(ar, maxLife);
    }

    void OnGui() override;
    void OnInitParticle(ParticleData& particle) override;
};

class InitialVelocityModule: public IInitParticleModule{
public:
    Vector3 minVelocity = {-1, 1, -1};
    Vector3 maxVelocity = {1, 2, 1};

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, minVelocity);
        ArchiveDumpNVP(ar, maxVelocity);
    }

    void OnGui() override;
    void OnInitParticle(ParticleData& particle) override;
};

class InitialSizeModule: public IInitParticleModule{
public:
    bool uniforSize = true;
    Vector3 minSize = {1,1,1};
    Vector3 maxSize = {1,1,1};

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, minSize);
        ArchiveDumpNVP(ar, maxSize);
        ArchiveDumpNVP(ar, uniforSize);
    }

    void OnGui() override;
    void OnInitParticle(ParticleData& particle) override;
};

class InitialColorModule: public IInitParticleModule{
public:
    Color color = {1, 1, 1, 1};

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, color);
    }

    void OnGui() override;
    void OnInitParticle(ParticleData& particle) override;
};

class UpdaterModule: public IParticleUpdateModule{
public:
    float gravityModifier = 0;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, gravityModifier);
    }

    void OnGui() override;
    void OnParticleUpdate(ParticleData& particle, ParticleRunningData& runningData) override;
};

class SizeOverLifetimeModule: public IParticleUpdateModule{
public:
    bool enable = false;
    float minSizeScale = 0;
    float maxSizeScale = 1;

    AnimationCurve curve = {
        {
            Keyframe(0.0f, 0.0f, CurveType::Linear),
            Keyframe(1.0f, 1.0f, CurveType::Linear)
        }
    };
    bool showCurve = false;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enable);
        ArchiveDumpNVP(ar, minSizeScale);
        ArchiveDumpNVP(ar, maxSizeScale);

        ArchiveDumpNVP(ar, curve);
    }

    void OnGui() override;
    void OnParticleUpdate(ParticleData& particle, ParticleRunningData& runningData) override;
};

class ColorOverLifetimeModule: public IParticleUpdateModule{
public:
    bool enable = false;
    Color colorA = {0, 0, 0, 1};
    Color colorB = {1, 1, 1, 1};
    
    ImGradientHDRState gradient;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enable);
        ArchiveDumpNVP(ar, colorA);
        ArchiveDumpNVP(ar, colorB);

        ArchiveDumpNVP(ar, gradient);
    }

    void OnGui() override;
    void OnParticleUpdate(ParticleData& particle, ParticleRunningData& runningData) override;
};

class CollisionPhysicModule: public IParticleUpdateModule{
public:
    bool enable = false;
    float rayOffset = 0.1f;
    int maxCollisionsCount = -1;
    LayerMask mask; 

    Action<void(Entity source, RayResult& result)> onCollision;
    bool isGlobalSpace = false;
    Matrix4 worldModel;
    Transform globalTrans;
    Scene* scene;
    Entity source;
    int* curParticleCollisionCount;
    PhysicsSystem* physicsSystem;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enable);
        ArchiveDumpNVP(ar, rayOffset);
        ArchiveDumpNVP(ar, maxCollisionsCount);
        ArchiveDumpNVP(ar, mask);
    }

    void OnGui() override;
    void OnParticleUpdate(ParticleData& particle, ParticleRunningData& runningData) override;
};

struct RendererModule{
    Ref<Material> material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Standard/Shaders/UnlitParticleBlend.glsl"), true);
    Ref<Model> model = AssetManager::Get().LoadAsset<Model>("Engine/Models/Cube.obj"); //Model::CreateFromFile(*mesh, "Engine/Models/Cube.obj", {nullptr, 1, false});

    enum class Orientation{ World, Velocity, View, ViewPlusVelocity};

    Orientation orientation;

    template <class Archive>
    void serialize(Archive& ar){
        AssetRefSerialize<Material> matRef(material);
        ArchiveDumpNVP(ar, matRef);

        AssetRefSerialize<Model> modelRef(model);
        ArchiveDumpNVP(ar, modelRef);

        ArchiveDumpNVP(ar, orientation);
    }

    void OnGui();
};

struct ParticleData{
    Vector3 pos = Vector3Zero;
    Vector3 lastPos = Vector3Zero;
    Vector3 vel = Vector3Zero;
    Vector3 startSize = Vector3One;
    Vector3 size = Vector3One;
    Color color = Color(1, 1, 1, 1);
    float startLife = 0;
    float life = 0;
    float cameradistance;
    bool handleCollision = false;

    inline bool IsDead(){ return life <= 0; }

    inline bool operator<(ParticleData& that){
        return this->cameradistance > that.cameradistance; // Sort in reverse order : far particles drawn first.
    }
};

struct ParticleRunningData{
    float delta;
    float lifetime;
};

class ParticleEmiter{
public:
    friend struct PaticleComponent;
    friend class ParticleSystem;
    friend class ParticleRendererFeature;

    enum class State{Stop, Running};
    enum class SimulationSpace{ WorldSpace, Local};

    void OnGui();

    void Reset();
    void Play();
    void Stop();
    State CurState();

    void SetMaxParticle(int maxParticle);

    int GetNewParticle();
    void FreeParticle(int index);

    void Update(Scene& scene, Entity e, TransformComponent& trans, Vector3 camPos);
    void Sort();
    void SubmitDrawData(InstancingBuffer& buffer, const Matrix4& root = Matrix4Identity, const Camera* cam = nullptr);

    void SpawnNewParticle();

    inline CollisionPhysicModule& GetCollisionPhysicModule(){ return collisionPhysicModule; }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, isLooping);
        ArchiveDumpNVP(ar, duration);

        ArchiveDumpNVP(ar, maxParticles);
        ArchiveDumpNVP(ar, delay);
        ArchiveDumpNVP(ar, simulationSpace);

        ArchiveDumpNVP(ar, spawnModule);
        ArchiveDumpNVP(ar, initialLifeModule);
        ArchiveDumpNVP(ar, initialVelocityModule);
        ArchiveDumpNVP(ar, initialSizeModule);
        ArchiveDumpNVP(ar, initialColorModule);

        ArchiveDumpNVP(ar, updaterModule);
        ArchiveDumpNVP(ar, sizeOverLifetimeModule);
        ArchiveDumpNVP(ar, colorOverLifetimeModule);
        ArchiveDumpNVP(ar, collisionPhysicModule);

        ArchiveDumpNVP(ar, rendererModule);
    }
    
    ParticleEmiter() = default;
    
    //TODO: This can be bug, see DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED comments, Review this later
    DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(ParticleEmiter, {
        COPY_OR_MOVE(isLooping);
        COPY_OR_MOVE(duration);

        COPY_OR_MOVE(maxParticles);
        COPY_OR_MOVE(delay);
        COPY_OR_MOVE(simulationSpace);

        COPY_OR_MOVE(spawnModule);
        COPY_OR_MOVE(initialLifeModule);
        COPY_OR_MOVE(initialVelocityModule);
        COPY_OR_MOVE(initialSizeModule);
        COPY_OR_MOVE(initialColorModule);
        
        COPY_OR_MOVE(updaterModule);
        COPY_OR_MOVE(sizeOverLifetimeModule);
        COPY_OR_MOVE(colorOverLifetimeModule);
        COPY_OR_MOVE(collisionPhysicModule);

        COPY_OR_MOVE(rendererModule);
    });

private:    
    Ref<InstancingBuffer> dataBuffer = CreateRef<InstancingBuffer>();

    bool isLooping = true;
    float duration = 5;
    int maxParticles = 100;
    float delay = 0;
    SimulationSpace simulationSpace;

    State state = State::Stop;
    float curDelayTime = 0;
    float runningTime = 0;

    Transform currentGlobalTrans;

    std::vector<ParticleData> particles;
    std::vector<Matrix4> drawData;
    std::vector<int> freeParticles;
    int currentParticleIndex = 0;
    int particlesCount = 0;
    int curParticleCollisionCount = 0;
    bool hasStarted = false;

    SpawnModule spawnModule;
    InitialLifeModule initialLifeModule;
    InitialVelocityModule initialVelocityModule;
    InitialSizeModule initialSizeModule; 
    InitialColorModule initialColorModule;
    
    UpdaterModule updaterModule;
    SizeOverLifetimeModule sizeOverLifetimeModule;
    ColorOverLifetimeModule colorOverLifetimeModule;
    CollisionPhysicModule collisionPhysicModule;

    RendererModule rendererModule;

    std::vector<IParticleSpawnModule*> spawnModules;// = {&spawnModule};
    std::vector<IInitParticleModule*> initParticleModules;// = {&initialLifeModule, &initialVelocityModule};
    std::vector<IParticleUpdateModule*> updateModules;// = {&updaterModule, &sizeOverLifeTimeModule};

    //void SpawnParticle(Particle &particle);

    void BindModules();
};

class ParticleSystem{
public:
    friend struct PaticleComponent;
    friend class ParticleRendererFeature;

    void OnGui();
    bool IsPlaying();
    void Play();
    void Stop();

    void Update(Scene& scene, Entity e, TransformComponent& trans, Vector3 camPos);

    inline std::vector<ParticleEmiter>& Emiters(){ return emiters; }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, emiters);
    }

private:
    std::vector<ParticleEmiter> emiters = {ParticleEmiter()};
};

struct ParticleComponent{
    ParticleSystem particleSystem;
    
    static void OnGui(Entity e, Scene& scene);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, particleSystem);
    }
};

class ParticleRendererFeature: public RenderFeature{
public:
    ParticleRendererFeature();
    void OnCollectRenderData(const Camera& cam, std::vector<RenderData>& outRenderData) override;
private:
    Ref<Material> material = nullptr;
    Ref<Model> mesh = nullptr;
};

class ParticleManageSystem: public System{
public:
    ParticleManageSystem(){ name = "ParticleManageSystem"; }
    int Type() override;
    bool ExecuteAlways() override;
    void Update(Scene& scene) override;
};

}