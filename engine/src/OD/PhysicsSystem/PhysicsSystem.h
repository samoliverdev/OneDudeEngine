#pragma once

#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scene.h"
#include "Phys.h"

namespace OD{

struct Rigidbody;

struct RigidbodyComponent{
    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);
    
    friend struct PhysicsSystem;

    inline Vector3 shape(){ return _boxShapeSize; }
    
    inline void shape(Vector3 boxShapeSize){
        _boxShapeSize = boxShapeSize;
        _shapeIsDity = true;
    }

    inline float mass(){ return _mass; }

    inline void mass(float mass){
        _mass = mass;
        _massIsDirty = true;
    }

private:
    Vector3 _boxShapeSize = {1,1,1};
    bool _shapeIsDity = false;
    
    float _mass = 1;
    bool _massIsDirty = false;

    Rigidbody* _data = nullptr;
};

struct PhysicsSystem: public System{
    friend struct RigidbodyComponent;
    
    virtual SystemType Type() override { return SystemType::Physics; }

    virtual void Init(Scene* scene) override;
    virtual void Update() override;

    PhysicsSystem();
    ~PhysicsSystem();

private:
    static void OnRemoveRigidbody(entt::registry & r, entt::entity e);

    void AddRigidbody(RigidbodyComponent& c, TransformComponent& t);
    void SetShape(RigidbodyComponent& rb);
    void SetMass(RigidbodyComponent& rb);

    btBroadphaseInterface* _broadphase;
    btCollisionConfiguration* _collisionConfiguration;
    btCollisionDispatcher* _dispatcher;
    btConstraintSolver* _solver;
    btDynamicsWorld* _world;
};

}