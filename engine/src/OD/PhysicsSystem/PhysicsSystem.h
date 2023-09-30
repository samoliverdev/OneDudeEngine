#pragma once

#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scene.h"
#include "Phys.h"

namespace OD{

struct Rigidbody;

struct RigidbodyComponent{
    friend struct PhysicsSystem;

    enum class Type{Dynamic, Static, Kinematic, Trigger};

    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);

    inline RigidbodyComponent::Type type(){ return _type; }
    void type(RigidbodyComponent::Type t);
    
    inline Vector3 shape(){ return _boxShapeSize; }
    void shape(Vector3 boxShapeSize);
    
    inline float mass(){ return _mass; }
    void mass(float mass);

    inline bool neverSleep(){ return _neverSleep; }
    void neverSleep(bool value);

    Vector3 position();
    void position(Vector3 position);

    Vector3 velocity();
    void velocity(Vector3 v);

    void ApplyForce(Vector3 v);
    void ApplyTorque(Vector3 v);
    void ApplyImpulse(Vector3 v);

private:
    Type _type = Type::Dynamic;
    Vector3 _boxShapeSize = {1,1,1};
    bool _shapeIsDity = false;
    float _mass = 1;
    bool _massIsDirty = false;
    bool _neverSleep = false;

    Rigidbody* _data = nullptr; 
};

struct RayResult{
    Entity entity;
    Vector3 hitPoint;
    Vector3 hitNormal;
};

typedef std::pair<const btRigidBody*, const btRigidBody*> CollisionPair;
typedef std::set<CollisionPair> CollisionPairs;

struct PhysicsSystem: public System{
    friend struct RigidbodyComponent;
    
    virtual SystemType Type() override { return SystemType::Physics; }

    virtual void Init(Scene* scene) override;
    virtual void Update() override;

    void ShowDebugGizmos();

    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit);

    PhysicsSystem();
    ~PhysicsSystem();

    static inline PhysicsSystem* Get(){ return instance; }

private:
    static void OnRemoveRigidbody(entt::registry & r, entt::entity e);

    void CheckForCollisionEvents();

    void AddRigidbody(EntityId entityId, RigidbodyComponent& c, TransformComponent& t);
    
    void SetShape(RigidbodyComponent& rb);
    void SetMass(RigidbodyComponent& rb);

    btBroadphaseInterface* _broadphase;
    btCollisionConfiguration* _collisionConfiguration;
    btCollisionDispatcher* _dispatcher;
    btConstraintSolver* _solver;
    btDynamicsWorld* _world;

    CollisionPairs _pairsLastUpdate;

    static PhysicsSystem* instance;
};

struct PhysicsSystemStartup{
    PhysicsSystemStartup(){
        SceneManager::Get().RegisterCoreComponent<RigidbodyComponent>("RigidbodyComponent");
        LogInfo("PhysicsSystemStartup");
    }
};

extern PhysicsSystemStartup physicsSystemStartup;

}