#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"

class btRigidBody;
class btBroadphaseInterface;
class btCollisionConfiguration;
class btCollisionDispatcher;
class btConstraintSolver;
class btDynamicsWorld;
class btGeneric6DofConstraint;

namespace OD{

struct Rigidbody;

struct OD_API JointComponent{
    //OD_REGISTER_CORE_COMPONENT_TYPE(JointComponent);

    friend struct PhysicsSystem;

    Vector3 pivot;
    float strength = 0.5f;
    EntityId rb;

    static void OnGui(Entity& e);

    template<class Archive>
    void serialize(Archive& ar){
        /*ar(
            CEREAL_NVP(pivot),
            CEREAL_NVP(strength),
            CEREAL_NVP(rb)
        );*/
        ArchiveDump(ar, CEREAL_NVP(pivot));
        ArchiveDump(ar, CEREAL_NVP(strength));
        ArchiveDump(ar, CEREAL_NVP(rb));
    }

private:
    btGeneric6DofConstraint* joint;
};

struct OD_API CollisionShape{
    enum class Type{Box, Sphere};

    Type type;
    Vector3 boxShapeSize = {1,1,1};
    float sphereRadius = 1;

    template <class Archive>
    void serialize(Archive & ar){
        /*ar(
            CEREAL_NVP(type),
            CEREAL_NVP(boxShapeSize),
            CEREAL_NVP(sphereRadius)
        );*/
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
        /*ar(
            CEREAL_NVP(type),
            CEREAL_NVP(shape),
            CEREAL_NVP(mass),
            CEREAL_NVP(neverSleep)
        );*/
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

typedef std::pair<const btRigidBody*, const btRigidBody*> CollisionPair;
typedef std::set<CollisionPair> CollisionPairs;

struct OD_API PhysicsSystem: public System{
    friend struct RigidbodyComponent;

    PhysicsSystem(Scene* scene);
    PhysicsSystem();
    ~PhysicsSystem();

    System* Clone(Scene* inScene) const override{ return new PhysicsSystem(inScene); }
    
    virtual SystemType Type() override { return SystemType::Physics; }
    virtual void Update() override;

    void ShowDebugGizmos();

    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit);

    static PhysicsSystem* Get();

private:
    static void OnRemoveRigidbody(entt::registry& r, entt::entity e);

    void CheckForCollisionEvents();
    void AddRigidbody(EntityId entityId, RigidbodyComponent& c, TransformComponent& t);
    
    btBroadphaseInterface* broadphase;
    btCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btConstraintSolver* solver;
    btDynamicsWorld* world;

    CollisionPairs pairsLastUpdate;

    //static PhysicsSystem* instance;
};

}