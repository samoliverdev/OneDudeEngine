#pragma once
#include <OD/Scene/Scene.h>
#include <OD/Core/Color.h>
#include <OD/Graphics/InstancingBuffer.h>
#include <OD/RenderPipeline/RenderContext.h>
#include <OD/Serialization/ImGuiArchive.h>
#include "Standard/Ultis/ImGradientHDR.h"

namespace OD{
    class Material;
}

using namespace OD;

namespace Standard{

class ParticleSystem;
struct ParticleData;
struct ParticleRunningData;

class IParticleSpawnModule{
public:
    virtual void OnGui(){}
    virtual void OnStartSpawnUpdate(ParticleSystem& system){};
    virtual void OnSpawnUpdate(ParticleSystem& system){};
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
    float overTimeEmiterRate = 200;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, singleBustEmiterCount);
        ArchiveDumpNVP(ar, overTimeEmiterRate);
    }

    void OnGui() override;
    void OnStartSpawnUpdate(ParticleSystem& system) override;
    void OnSpawnUpdate(ParticleSystem& system) override;
};

class InitialLifeModule: public IInitParticleModule{
public:
    float minLife;
    float maxLife;

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
    Vector3 minVelocity;
    Vector3 maxVelocity;

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
    Vector3 minSize = {1,1,1};
    Vector3 maxSize = {1,1,1};

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, minSize);
        ArchiveDumpNVP(ar, maxSize);
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
    float maxSize = 1;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, enable);
        ArchiveDumpNVP(ar, maxSize);
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

struct ParticleData{
    Vector3 pos = Vector3Zero;
    Vector3 vel = Vector3Zero;
    Vector3 startSize = Vector3One;
    Vector3 size = Vector3One;
    Color color = Color(1, 1, 1, 1);
    float startLife = 0;
    float life = 0;
    float cameradistance;

    inline bool IsDead(){ return life <= 0; }

    inline bool operator<(ParticleData& that){
        return this->cameradistance > that.cameradistance; // Sort in reverse order : far particles drawn first.
    }
};

struct ParticleRunningData{
    float delta;
    float lifetime;
};

class ParticleSystem{
public:
    friend struct PaticleComponent;

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

    void Update(TransformComponent& trans, Vector3 camPos);
    void Sort();
    void SubmitDrawData(InstancingBuffer& buffer, const Matrix4& root = Matrix4Identity);

    void SpawnNewParticle();

    template <class Archive>
    void serialize(Archive& ar){
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
    }
    
    ParticleSystem() = default;
    DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(ParticleSystem, {
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
    });

private:    
    int maxParticles = 100;
    float delay = 0;
    SimulationSpace simulationSpace;

    State state = State::Stop;
    float curDelayTime = 0;

    Transform currentGlobalTrans;

    std::vector<ParticleData> particles;
    std::vector<Matrix4> drawData;
    std::vector<int> freeParticles;
    int currentParticleIndex = 0;
    int particlesCount = 0;
    bool hasStarted = false;

    SpawnModule spawnModule;
    InitialLifeModule initialLifeModule;
    InitialVelocityModule initialVelocityModule;
    InitialSizeModule initialSizeModule; 
    InitialColorModule initialColorModule;

    UpdaterModule updaterModule;
    SizeOverLifetimeModule sizeOverLifetimeModule;
    ColorOverLifetimeModule colorOverLifetimeModule;

    std::vector<IParticleSpawnModule*> spawnModules;// = {&spawnModule};
    std::vector<IInitParticleModule*> initParticleModules;// = {&initialLifeModule, &initialVelocityModule};
    std::vector<IParticleUpdateModule*> updateModules;// = {&updaterModule, &sizeOverLifeTimeModule};

    //void SpawnParticle(Particle &particle);

    void BindModules();
};

struct ParticleComponent{
    Ref<InstancingBuffer> drawData = nullptr;
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
    int Type() override;
    bool ExecuteAlways() override;
    void Update(Scene& scene) override;
};

}