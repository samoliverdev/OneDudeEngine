#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/ImGuiArchive.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Graphics/Mesh.h"

class btTriangleMesh;
class btBvhTriangleMeshShape;

namespace OD{

struct Rigidbody;
struct PhysicsWorld;

using MeshShapeData = btBvhTriangleMeshShape;
Ref<MeshShapeData> OD_API CreateMeshShapeData(const Ref<Mesh>& mesh);
Ref<MeshShapeData> OD_API CreateMeshShapeData(const std::vector<Vector3>& vertices, const std::vector<unsigned int> indices);

struct OD_API CollisionShape{
    enum class Type{Box, Sphere, Capsule, Mesh};

    Type type;
    Vector3 center = {0, 0, 0};
    Vector3 size = {1,1,1};
    float radius = 1;
    float height = 1;
    Ref<MeshShapeData> mesh;

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(center));
        ArchiveDump(ar, CEREAL_NVP(size));
        ArchiveDump(ar, CEREAL_NVP(radius));
        ArchiveDump(ar, CEREAL_NVP(height));
    }

    inline static CollisionShape BoxShape(Vector3 size){
        CollisionShape shape;
        shape.type = Type::Box;
        shape.size = size;
        return shape;
    }

    inline static CollisionShape SphereShape(float radius){
        CollisionShape shape;
        shape.type = Type::Sphere;
        shape.radius = radius;
        return shape;
    }

    inline static CollisionShape CapsuleShape(float radius, float height, Vector3 center){
        CollisionShape shape;
        shape.type = Type::Capsule;
        shape.radius = radius;
        shape.height = height;
        shape.center = center;
        return shape;
    }

    inline static CollisionShape MeshShape(Ref<Mesh> mesh){
        CollisionShape shape;
        shape.type = Type::Mesh;
        shape.mesh = CreateMeshShapeData(mesh);
        return shape;
    }

    inline static CollisionShape MeshShape(Ref<MeshShapeData> mesh){
        CollisionShape shape;
        shape.type = Type::Mesh;
        shape.mesh = mesh;
        return shape;
    }
};

struct OD_API RigidbodyComponent{
    friend struct PhysicsSystem;

    enum class Type{Dynamic, Static, Kinematic, Trigger};

    static void OnGui(Entity& e);

    inline RigidbodyComponent::Type GetType(){ return type; }
    void SetType(RigidbodyComponent::Type t);
    
    inline CollisionShape GetShape(){ return shape; }
    void SetShape(CollisionShape shape);
    
    inline float Mass(){ return mass; }
    void Mass(float mass);

    inline bool NeverSleep(){ return neverSleep; }
    void NeverSleep(bool value);

    Vector3 Position();
    void Position(Vector3 position);

    Quaternion Rotation();
    void Rotation(Quaternion rotation);

    Vector3 Velocity();
    void Velocity(Vector3 v);

    void ApplyForce(Vector3 v);
    void ApplyTorque(Vector3 v);
    void ApplyImpulse(Vector3 v);

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(shape));
        ArchiveDump(ar, CEREAL_NVP(mass));
        ArchiveDump(ar, CEREAL_NVP(neverSleep));
    }

private:
    Type type = Type::Dynamic;
    
    CollisionShape shape;

    float mass = 1;
    bool neverSleep = false;
    Rigidbody* data = nullptr; 

    void UpdateSettings();
};

struct OD_API RayResult{
    Entity entity;
    Vector3 hitPoint;
    Vector3 hitNormal;
};

using OnCollisionCallback = void(*)(Entity, Entity);
//using OnCollisionCallback = std::function<void(Entity, Entity)>;

struct OD_API PhysicsSystem: public System{
    friend struct RigidbodyComponent;

    PhysicsSystem(Scene* scene);
    ~PhysicsSystem() override;

    System* Clone(Scene* inScene) const override{ 
        PhysicsSystem* system = new PhysicsSystem(inScene);
        system->onCollisionEnterCallbacks = onCollisionEnterCallbacks;
        system->onCollisionExitCallbacks = onCollisionExitCallbacks;
        system->onTriggerEnterCallbacks = onTriggerEnterCallbacks;
        system->onTriggerExitCallbacks = onTriggerExitCallbacks;
        return system; 
    }
    
    virtual SystemType Type() override { return SystemType::Physics; }
    virtual void Update() override;
    virtual void OnDrawGizmos() override;

    void ShowDebugGizmos();
    
    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit);
    bool IsSimulationEnable();

    void AddOnCollisionEnterCallback(OnCollisionCallback callback);
    void RemoveOnCollisionEnterCallback(OnCollisionCallback callback);
    
    void AddOnCollisionExitCallback(OnCollisionCallback callback);
    void RemoveOnCollisionExitCallback(OnCollisionCallback callback);

    void AddOnTriggerEnterCallback(OnCollisionCallback callback);
    void RemoveOnTriggerEnterCallback(OnCollisionCallback callback);
    
    void AddOnTriggerExitCallback(OnCollisionCallback callback);
    void RemoveOnTriggerExitCallback(OnCollisionCallback callback);

private:
    static void OnRemoveRigidbody(entt::registry& r, entt::entity e);

    void CheckForCollisionEvents();
    void AddRigidbody(EntityId entityId, RigidbodyComponent& c, TransformComponent& t);
    void RemoveRigidbody(EntityId entityId, RigidbodyComponent& c);

    PhysicsWorld* physicsWorld;
    
    std::vector<OnCollisionCallback> onCollisionEnterCallbacks;
    std::vector<OnCollisionCallback> onCollisionExitCallbacks;
    std::vector<OnCollisionCallback> onTriggerEnterCallbacks;
    std::vector<OnCollisionCallback> onTriggerExitCallbacks;
};

void PhysicsModuleInit();

}