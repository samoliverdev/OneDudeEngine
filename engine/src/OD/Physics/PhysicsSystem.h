#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/ImGuiArchive.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"

namespace OD{

struct Rigidbody;
struct PhysicsWorld;

struct OD_API CollisionShape{
    enum class Type{Box, Sphere};

    Type type;
    Vector3 boxShapeSize = {1,1,1};
    float sphereRadius = 1;

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(boxShapeSize));
        ArchiveDump(ar, CEREAL_NVP(sphereRadius));
    }

    inline static CollisionShape BoxShape(Vector3 size){
        return CollisionShape{
            Type::Box,
            size,
            1
        };
    }

    inline static CollisionShape SphereShape(float radius){
        return CollisionShape{
            Type::Sphere,
            Vector3One,
            radius
        };
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

}