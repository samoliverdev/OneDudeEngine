#include "OD/pch.h"
#include "Boids.h"
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include <OD/Scene/Scene.h>
#include <OD/Scene/SceneManager.h>
#include <OD/Graphics/Graphics.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/Core/Application.h>
#include <OD/Core/Instrumentor.h>
#include <OD/Editor/Editor.h>

#undef max
#undef min

static constexpr float boundsSize = 100; //200.0f;

struct BoidComponent{
    Vector3 velocity = Vector3Zero;

    template <class Archive>
    void serialize(Archive& ar) {
        ArchiveDumpNVP(ar, velocity);
    }
};

struct BoidSystem : public OD::System {
    float perceptionRadius = 6.0f;
    float cellSize = 6.0f;

    float boidSpeed = 25.0f;
    float maxSteerForce = 10.0f;

    float separationWeight = 25.0f;
    float cohesionWeight = 4.0f;
    float alignmentWeight = 8.0f;

    float avoidBoundsTurnDist = 20.0f;
    float avoidBoundsWeight = 25.0f;

    int maxNeighbors = 32;
    int rotationUpdateRate = 1; // 1 = every frame, 2 = each 2 frames, etc.

    struct CellCoord {
        int x, y, z;
    };

    struct BoidData {
        entt::entity entity;
        TransformComponent* transform;
        BoidComponent* boid;

        Vector3 position;
        Vector3 velocity;
        CellCoord cell;

        Vector3 newPosition;
        Vector3 newVelocity;
        Quaternion newRotation;
    };

    struct GridEntry {
        uint64_t hash;
        int boidIndex;
    };

    struct CellRange {
        uint64_t hash;
        int begin;
        int end;
    };

    std::vector<BoidData> boids;
    std::vector<GridEntry> gridEntries;
    std::vector<CellRange> cellRanges;

    tf::Executor executor;
    tf::Taskflow taskflow;

    int frameIndex = 0;

    BoidSystem() {
        name = "BoidSystem";

        separationWeight = 8.0f*2;   // lower, so they stay closer
        cohesionWeight   = 12.0f;  // higher, pulls group together
        alignmentWeight  = 18.0f;  // higher, fly same direction

        perceptionRadius = 10.0f;
        maxNeighbors = 24;

        boidSpeed = 18.0f;
        maxSteerForce = 4.0f;
    }

    CellCoord WorldToCell(const Vector3& p) const {
        return CellCoord {
            (int)std::floor(p.x / cellSize),
            (int)std::floor(p.y / cellSize),
            (int)std::floor(p.z / cellSize)
        };
    }

    uint64_t HashCell(const CellCoord& c) const {
        uint64_t x = (uint32_t)c.x * 73856093u;
        uint64_t y = (uint32_t)c.y * 19349663u;
        uint64_t z = (uint32_t)c.z * 83492791u;
        return x ^ y ^ z;
    }

    Vector3 SafeNormalize(Vector3 v, Vector3 fallback = Vector3Forward) const {
        float lenSq = math::dot(v, v);

        if(lenSq <= 0.000001f)
            return fallback;

        return v * (1.0f / std::sqrt(lenSq));
    }

    Vector3 LimitLength(Vector3 v, float maxLen) const {
        float lenSq = math::dot(v, v);
        float maxLenSq = maxLen * maxLen;

        if(lenSq <= maxLenSq || lenSq <= 0.000001f)
            return v;

        return v * (maxLen / std::sqrt(lenSq));
    }

    Vector3 SteerTowards(Vector3 desired, Vector3 currentVelocity) const {
        desired = SafeNormalize(desired, currentVelocity) * boidSpeed;

        Vector3 steer = desired - currentVelocity * boidSpeed;
        return LimitLength(steer, maxSteerForce);
    }

    Vector3 BoundsForce(const Vector3& pos) const {
        Vector3 force = Vector3Zero;

        float distX = boundsSize - math::abs(pos.x);
        float distY = boundsSize - math::abs(pos.y);
        float distZ = boundsSize - math::abs(pos.z);

        if(distX < avoidBoundsTurnDist)
            force.x += pos.x > 0.0f ? -1.0f : 1.0f;

        if(distY < avoidBoundsTurnDist)
            force.y += pos.y > 0.0f ? -1.0f : 1.0f;

        if(distZ < avoidBoundsTurnDist)
            force.z += pos.z > 0.0f ? -1.0f : 1.0f;

        float lenSq = math::dot(force, force);

        if(lenSq <= 0.000001f)
            return Vector3Zero;

        return force * (avoidBoundsWeight / std::sqrt(lenSq));
    }

    const CellRange* FindCellRange(uint64_t hash) const {
        int left = 0;
        int right = (int)cellRanges.size() - 1;

        while(left <= right) {
            int mid = (left + right) >> 1;

            if(cellRanges[mid].hash == hash)
                return &cellRanges[mid];

            if(cellRanges[mid].hash < hash)
                left = mid + 1;
            else
                right = mid - 1;
        }

        return nullptr;
    }

    template<typename Fn>
    void ParallelFor(size_t count, size_t batchSize, Fn&& fn) {
        if(count == 0)
            return;

        taskflow.clear();

        for(size_t begin = 0; begin < count; begin += batchSize) {
            size_t end = std::min(begin + batchSize, count);

            taskflow.emplace([begin, end, &fn]() {
                fn(begin, end);
            });
        }

        executor.run(taskflow).wait();
    }

    void SnapshotBoids(Scene& scene) {
        auto view = scene.GetRegistry().view<TransformComponent, BoidComponent>();

        boids.clear();
        boids.reserve(view.size_hint());

        for(auto e : view) {
            TransformComponent& trans = view.get<TransformComponent>(e);
            BoidComponent& boid = view.get<BoidComponent>(e);

            Vector3 pos = trans.LocalPosition();
            Vector3 vel = SafeNormalize(boid.velocity, trans.Forward());

            BoidData data;
            data.entity = e;
            data.transform = &trans;
            data.boid = &boid;
            data.position = pos;
            data.velocity = vel;
            data.cell = WorldToCell(pos);
            data.newPosition = pos;
            data.newVelocity = vel;
            data.newRotation = trans.LocalRotation();

            boids.push_back(data);
        }
    }

    void BuildGrid() {
        gridEntries.resize(boids.size());

        ParallelFor(boids.size(), 256, [&](size_t begin, size_t end) {
            for(size_t i = begin; i < end; i++) {
                boids[i].cell = WorldToCell(boids[i].position);

                gridEntries[i].hash = HashCell(boids[i].cell);
                gridEntries[i].boidIndex = (int)i;
            }
        });

        std::sort(
            gridEntries.begin(),
            gridEntries.end(),
            [](const GridEntry& a, const GridEntry& b) {
                return a.hash < b.hash;
            }
        );

        cellRanges.clear();
        cellRanges.reserve(gridEntries.size());

        int begin = 0;
        int count = (int)gridEntries.size();

        while(begin < count) {
            uint64_t hash = gridEntries[begin].hash;

            int end = begin + 1;
            while(end < count && gridEntries[end].hash == hash)
                end++;

            cellRanges.push_back({ hash, begin, end });
            begin = end;
        }
    }

    void ComputeBoidRange(size_t begin, size_t end, float dt) {
        float radiusSq = perceptionRadius * perceptionRadius;

        for(size_t i = begin; i < end; i++) {
            BoidData& self = boids[i];

            Vector3 separationSum = Vector3Zero;
            Vector3 cohesionSum = Vector3Zero;
            Vector3 alignmentSum = Vector3Zero;

            int neighborCount = 0;
            bool reachedMaxNeighbors = false;

            for(int x = -1; x <= 1 && !reachedMaxNeighbors; x++) {
                for(int y = -1; y <= 1 && !reachedMaxNeighbors; y++) {
                    for(int z = -1; z <= 1 && !reachedMaxNeighbors; z++) {
                        CellCoord neighborCell {
                            self.cell.x + x,
                            self.cell.y + y,
                            self.cell.z + z
                        };

                        const CellRange* range = FindCellRange(HashCell(neighborCell));

                        if(range == nullptr)
                            continue;

                        for(int entryIndex = range->begin; entryIndex < range->end; entryIndex++) {
                            int otherIndex = gridEntries[entryIndex].boidIndex;

                            if(otherIndex == (int)i)
                                continue;

                            const BoidData& other = boids[otherIndex];

                            Vector3 offset = self.position - other.position;
                            float distSq = math::dot(offset, offset);

                            if(distSq <= 0.000001f || distSq > radiusSq)
                                continue;

                            float invDist = 1.0f / std::sqrt(distSq);

                            separationSum += offset * invDist;
                            cohesionSum += other.position;
                            alignmentSum += other.velocity;

                            neighborCount++;

                            if(neighborCount >= maxNeighbors) {
                                reachedMaxNeighbors = true;
                                break;
                            }
                        }
                    }
                }
            }

            Vector3 acceleration = Vector3Zero;

            if(neighborCount > 0) {
                float invCount = 1.0f / (float)neighborCount;

                Vector3 separation = separationSum * invCount;
                Vector3 cohesion = cohesionSum * invCount - self.position;
                Vector3 alignment = alignmentSum * invCount;

                acceleration += SteerTowards(separation, self.velocity) * separationWeight;
                acceleration += SteerTowards(cohesion, self.velocity) * cohesionWeight;
                acceleration += SteerTowards(alignment, self.velocity) * alignmentWeight;
            }

            acceleration += BoundsForce(self.position);

            /*Vector3 newVelocity = self.velocity + acceleration * dt;
            newVelocity = SafeNormalize(newVelocity, self.velocity);

            self.newVelocity = newVelocity;
            self.newPosition = self.position + newVelocity * boidSpeed * dt;*/

            //----------New-----------
            Vector3 desiredVelocity = self.velocity * boidSpeed + acceleration * dt;

            float minSpeed = boidSpeed * 0.75f;
            float maxSpeed = boidSpeed * 1.25f;

            float speed = std::sqrt(math::dot(desiredVelocity, desiredVelocity));

            if(speed < minSpeed)
                desiredVelocity = SafeNormalize(desiredVelocity, self.velocity) * minSpeed;
            else if(speed > maxSpeed)
                desiredVelocity = desiredVelocity * (maxSpeed / speed);

            Vector3 newVelocity = SafeNormalize(desiredVelocity, self.velocity);

            self.newVelocity = newVelocity;
            self.newPosition = self.position + desiredVelocity * dt;
            //---------------------

            if(rotationUpdateRate <= 1 || frameIndex % rotationUpdateRate == 0)
                self.newRotation = math::quatLookAt(-newVelocity, Vector3Up);
            else
                self.newRotation = self.transform->LocalRotation();
        }
    }

    void ApplyBoidRange(size_t begin, size_t end) {
        for(size_t i = begin; i < end; i++) {
            BoidData& data = boids[i];

            data.boid->velocity = data.newVelocity;
            data.transform->LocalPosition(data.newPosition);
            data.transform->LocalRotation(data.newRotation);
        }
    }

    void Update(Scene& scene) override {
        OD_PROFILE_SCOPE("BoidSystem::Update");

        float dt = Application::DeltaTime();

        {
            OD_PROFILE_SCOPE("BoidSystem::Snapshot");
            SnapshotBoids(scene);
        }

        if(boids.empty())
            return;

        cellSize = perceptionRadius;

        {
            OD_PROFILE_SCOPE("BoidSystem::BuildGrid");
            BuildGrid();
        }

        constexpr size_t batchSize = 256;

        {
            OD_PROFILE_SCOPE("BoidSystem::Compute");
            ParallelFor(boids.size(), batchSize, [&](size_t begin, size_t end) {
                ComputeBoidRange(begin, end, dt);
            });
        }

        {
            OD_PROFILE_SCOPE("BoidSystem::Apply");
            ParallelFor(boids.size(), batchSize, [&](size_t begin, size_t end) {
                ApplyBoidRange(begin, end);
            });
        }

        frameIndex++;
    }

    void OnDrawGizmos(Scene& scene, Camera& cam) override {
        Transform trans;
        trans.Scale(Vector3(boundsSize));
        Graphics::DrawWireCube(trans.GetModelMatrix(), Vector3(0, 1, 0), 1);
    }
};

void BoidsSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    auto random = [](int min, int max) -> int{
        static bool first = true;
        if(first) {  
            srand(time(NULL));
            first = false;
        }
        return min + rand() % (( max + 1 ) - min);
    };

    auto& SceneManager = SceneManager::Get();
    SceneManager.RegisterComponent<BoidComponent>("BoidComponent");
    SceneManager.RegisterSystem<BoidSystem>("BoidSystem");
    OD::Ref<OD::Scene> scene = SceneManager.NewScene();

    Ref<Material> floorMaterial = CreateRef<Material>();
    floorMaterial->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit.glsl"));
    floorMaterial->SetVector4("color", Vector4(0.8f, 0.8f, 0.8f, 1));

    Ref<Material> boidMaterial = CreateRef<Material>();
    boidMaterial->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit.glsl"));
    boidMaterial->SetVector4("color", Vector4(1, 0, 0, 1));
    boidMaterial->SetEnableInstancing(true);

    Ref<Model> floorModel = ResourceManager::Get().LoadByPath<Model>("Sandbox/Models/plane.glb");
    floorModel->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit.glsl"));

    Ref<Model> boidModel = ResourceManager::Get().LoadByPath<Model>("Sandbox/Models/Boid.glb");
    boidModel->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit.glsl"));

    Entity env = scene->AddEntity("Env");
    EnvironmentComponent& _env = scene->AddComponent<EnvironmentComponent>(env);
    _env.settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};
    _env.settings.shadowDistance = 1000;

    Entity e = scene->AddEntity("Floor");
    scene->GetComponent<TransformComponent>(e).Position(Vector3(0, -(boundsSize*2), 0));
    scene->GetComponent<TransformComponent>(e).LocalScale(Vector3(15, 1, 15));
    ModelRendererComponent& _meshRenderer = scene->AddComponent<ModelRendererComponent>(e);
    _meshRenderer.SetModel(floorModel);
    _meshRenderer.GetMaterialsOverride()[0] = floorMaterial;

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 150, 150));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    //scene->AddComponent<Standard::FreeCamera>(camera).moveSpeed = 100;
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    //scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    //camMove.transform = &camera->GetComponent<TransformComponent>()();
    //camMove.moveSpeed = 60;
    cam.farClipPlane = 1000;

    Entity light = scene->AddEntity("Directional Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    lightComponent.intensity = 1.5f;
    lightComponent.renderShadow = true;
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));

    float posRange = 200;
    Entity boids = scene->AddEntity("Boids");
    
    for(int i = 0; i < 1000*20; i++){
        Entity boid = scene->AddEntity("Boid" + std::to_string(random(0, 200)));
        ModelRendererComponent& mr = scene->AddComponent<ModelRendererComponent>(boid);
        mr.SetModel(boidModel);
        mr.GetMaterialsOverride()[0] = boidMaterial;

        TransformComponent& trans = scene->GetComponent<TransformComponent>(boid);
        trans.LocalPosition(Vector3(random(-posRange, posRange), random(-posRange, posRange), random(-posRange, posRange)));
        trans.LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));

        //if(i % 2 == 0) continue;
        //if(i % 2 == 0) 
            scene->AddComponent<SkinnedModelRendererComponent>(boid);

        BoidComponent& boidComponet = scene->AddComponent<BoidComponent>(boid);
        boidComponet.velocity = trans.Forward();
    
        //scene->SetParent(boids, boid);
    }

    Application::AddModule<Editor>();
    scene->Start();
}

void BoidsSample::OnUpdate(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Update();
}   

void BoidsSample::OnRender(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Draw();
}

void BoidsSample::OnGUI(){}
void BoidsSample::OnResize(int width, int height){}
void BoidsSample::OnExit(){}