#pragma once
#include <OD/Scene/Scene.h>
#include <OD/Core/Color.h>
#include <OD/Graphics/InstancingBuffer.h>
#include <OD/RenderPipeline/RenderContext.h>

namespace OD{
    class Material;
}

using namespace OD;

namespace Standard{

class ParticleSystem{
public:
    friend struct PaticleComponent;

    enum class State{Stop, Running};

    struct Particle{
        Vector3 pos;
        Vector3 vel;
        Vector3 startSize;
        Vector3 size;
        Color color;
        float startLife = 0;
        float life = 0;
        float cameradistance;

        inline bool IsDead(){ return life <= 0; }

        inline bool operator<(Particle& that){
            return this->cameradistance > that.cameradistance; // Sort in reverse order : far particles drawn first.
        }
    };

    struct Emiter{
        int singleBustEmiterCount = 0;
        float overTimeEmiterRate = 200;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, singleBustEmiterCount);
            ArchiveDumpNVP(ar, overTimeEmiterRate);
        }
    };

    enum class ShapeType{Cone, Sphere};

    struct Shape{
        Vector2 minMaxVelX = {-5.0f, 5.0f};
        Vector2 minMaxVelY = {5.0f, 15.0f};
        Vector2 minMaxVelZ = {-5.0f, 5.0f};

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, minMaxVelX);
            ArchiveDumpNVP(ar, minMaxVelY);
            ArchiveDumpNVP(ar, minMaxVelZ);
        }
    };

    struct SizeOverLifeTime{
        bool enable = false;
        float maxSize = 1;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, enable);
            ArchiveDumpNVP(ar, maxSize);
        }
    };

    void OnGui();

    void Reset();
    void Play();
    void Stop();
    State CurState();

    void SetMaxParticle(int maxParticle);

    int GetNewParticle();
    void FreeParticle(int index);

    void Update(Vector3 camPos);
    void Sort();
    void SubmitDrawData(InstancingBuffer& buffer, const Matrix4& root = Matrix4Identity);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, maxParticles);
        ArchiveDumpNVP(ar, delay);
        ArchiveDumpNVP(ar, startLifeTimeMinMax);
        ArchiveDumpNVP(ar, startSpeedTimeMinMax);
        ArchiveDumpNVP(ar, gravityModifier);

        ArchiveDumpNVP(ar, emiter);
        ArchiveDumpNVP(ar, shape);
    }
    
private:    
    int maxParticles = 100;
    float delay = 0;
    Vector2 startLifeTimeMinMax = {5, 5};
    Vector2 startSpeedTimeMinMax = {5, 5};
    Vector2 startSizeTimeMinMax = {0.2f, 0.2f};
    float gravityModifier = 0;

    Emiter emiter;
    Shape shape;
    SizeOverLifeTime sizeOverLifeTime;

    State state = State::Stop;
    float curDelayTime = 0;

    std::vector<Particle> particles;
    std::vector<Matrix4> drawData;
    std::vector<int> freeParticles;
    int currentParticleIndex = 0;
    int particlesCount = 0;
    bool hasStarted = false;

    void SpawnParticle(Particle &particle);
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