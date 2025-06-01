#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Mesh.h"

//#define UseBulletPhysics
#define UseJoltPhysics

#if defined(UseBulletPhysics)
class btRigidBody;
class btTriangleMesh;
class btBvhTriangleMeshShape;
class btHeightfieldTerrainShape;
class btCollisionObject;
#endif

namespace OD{

//struct PhysicObject;
//struct JointObject;
//struct PhysicsWorld;

//using MeshShapeData = btBvhTriangleMeshShape;
class MeshShapeData;

Ref<MeshShapeData> OD_API CreateMeshShapeData(const Ref<Model>& model);
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

    inline static CollisionShape BoxShape(Vector3 size, Vector3 center = Vector3Zero){
        CollisionShape shape;
        shape.type = Type::Box;
        shape.size = size;
        shape.center = center;
        return shape;
    }

    inline static CollisionShape SphereShape(float radius, Vector3 center = Vector3Zero){
        CollisionShape shape;
        shape.type = Type::Sphere;
        shape.radius = radius;
        shape.center = center;
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

    int mask = AllLayers;
    //LayerMask mask = {AllLayers};

    RigidbodyComponent() = default;
    /*RigidbodyComponent(const RigidbodyComponent& other);
    RigidbodyComponent& operator=(const RigidbodyComponent& other);
    RigidbodyComponent(RigidbodyComponent&& other);
    RigidbodyComponent& operator=(RigidbodyComponent&& other);*/

    enum class Type{Dynamic, Static, Kinematic, Trigger, TestDisable};

    static void OnGui(Entity& e, Scene& scene);

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

    void SetAngularFactor(Vector3 v);

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(shape));
        ArchiveDump(ar, CEREAL_NVP(mass));
        ArchiveDump(ar, CEREAL_NVP(neverSleep));
    }

    DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(RigidbodyComponent, {
        COPY_OR_MOVE(shape);
        COPY_OR_MOVE(type);
        COPY_OR_MOVE(angularFactor);
        COPY_OR_MOVE(mass);
        COPY_OR_MOVE(neverSleep);
    });

private:
    CollisionShape shape;
    Type type = Type::Dynamic;
    Vector3 angularFactor = {1, 1, 1};
    float mass = 1;
    bool neverSleep = false;

    class PhysicObject* data = nullptr;

    void UpdateSettings();

    /*inline void Copy(const RigidbodyComponent& other){
        shape = other.shape;
        type = other.type;
        angularFactor = other.angularFactor;
        mass = other.mask;
        neverSleep = other.neverSleep;
    }

    inline void Move(RigidbodyComponent&& other){
        shape = std::move(other.shape);
        type = std::move(other.type);
        angularFactor = std::move(other.angularFactor);
        mass = std::move(other.mask);
        neverSleep = std::move(other.neverSleep);
    }*/
};

struct OD_API RagdollComponent{
    friend struct PhysicsSystem;

    RagdollComponent() = default;

    struct Part{
        CollisionShape shape;
        int parent = -1;
        int skinnedSkeletonIndex = -1;

        Vector3 constraintPos = Vector3Zero;
        Vector3 twistAxis = Vector3Zero;
        float twistAngleMin = 0;
        float twistAngleMax = 0;
        float normalAngle = 0;
        float planeAngle = 0;

        Vector3 pos = Vector3Zero;
        Quaternion rot = QuaternionIdentity;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, shape);
            ArchiveDumpNVP(ar, parent);
            ArchiveDumpNVP(ar, skinnedSkeletonIndex);
            ArchiveDumpNVP(ar, pos);
            ArchiveDumpNVP(ar, rot);
            ArchiveDumpNVP(ar, constraintPos);
            ArchiveDumpNVP(ar, twistAxis);
            ArchiveDumpNVP(ar, twistAngleMin);
            ArchiveDumpNVP(ar, twistAngleMax);
            ArchiveDumpNVP(ar, normalAngle);
            ArchiveDumpNVP(ar, planeAngle);
        }
    };

    bool isDirty = true;
    std::vector<Part> parts;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, isDirty);
        ArchiveDumpNVP(ar, parts);
    }

    DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(RagdollComponent, {
        COPY_OR_MOVE(isDirty);
        COPY_OR_MOVE(parts);
    });

private:
    struct RagdollObject* data = nullptr;
};

struct OD_API CollisionBodyComponent{
    friend struct PhysicsSystem;

    int mask = AllLayers;

    static void OnGui(Entity& e, Scene& scene);

    inline CollisionShape GetShape(){ return shape; }
    void SetShape(CollisionShape shape);
    
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(shape));
        ArchiveDump(ar, CEREAL_NVP(neverSleep));
    }

    /*CollisionBodyComponent(const CollisionBodyComponent& other){
        shape = other.shape;
        neverSleep = other.neverSleep;
    }

    CollisionBodyComponent& operator=(const CollisionBodyComponent& other){
        if(this == &other) return *this;

        shape = other.shape;
        neverSleep = other.neverSleep;
        return *this;
    }*/

private:
    CollisionShape shape;
    bool neverSleep = false;

    #if defined(UseBulletPhysics)
    class PhysicObject* data = nullptr; 
    #endif

    void UpdateSettings();
};

struct OD_API JointComponent{
    friend struct PhysicsSystem;

    Vector3 pivot = Vector3Zero;
    Vector3 axis = Vector3Zero;
    Vector3 connectedPivot = Vector3Zero;
    Vector3 connectedAxis = Vector3Zero;
    Vector3 angularLowerLimit = Vector3Zero;
    Vector3 angularUpperLimit = Vector3Zero;
    Entity connectedBody = EntityNull;
    bool autoConfigConnectedPivot = true;
    bool disableSelfCollision = true;

    //static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, connectedBody);
        ArchiveDumpNVP(ar, pivot);
        ArchiveDumpNVP(ar, connectedPivot);
        ArchiveDumpNVP(ar, angularLowerLimit);
        ArchiveDumpNVP(ar, angularUpperLimit);
        ArchiveDumpNVP(ar, disableSelfCollision);
    }

    /*JointComponent(const JointComponent& other){
        pivot = other.pivot;
        axis = other.axis;
        connectedPivot = other.connectedPivot;
        connectedAxis = other.connectedAxis;
        angularLowerLimit = other.angularLowerLimit;
        angularUpperLimit = other.angularUpperLimit;
        connectedBody = other.connectedBody;
        autoConfigConnectedPivot = other.autoConfigConnectedPivot;
        disableSelfCollision = other.disableSelfCollision;
    }

    JointComponent& operator=(const JointComponent& other){
        if(this == &other) return *this;

        pivot = other.pivot;
        axis = other.axis;
        connectedPivot = other.connectedPivot;
        connectedAxis = other.connectedAxis;
        angularLowerLimit = other.angularLowerLimit;
        angularUpperLimit = other.angularUpperLimit;
        connectedBody = other.connectedBody;
        autoConfigConnectedPivot = other.autoConfigConnectedPivot;
        disableSelfCollision = other.disableSelfCollision;
        return *this;
    }*/

private:

    #if defined(UseBulletPhysics)
    class JointObject* data = nullptr;
    #endif
};

struct OD_API HeightmapColliderComponent{
    friend struct PhysicsSystem;

    std::vector<float> heights;
    float width;
    float length;
    float scale;
    float minHeight;
    float maxHeight;

    Vector3 offset;

    template <class Archive>
    void serialize(Archive & ar){}
    
private:

    #if defined(UseBulletPhysics)
    btHeightfieldTerrainShape* shape = nullptr;
    btCollisionObject* body = nullptr;
    #endif
};

struct OD_API RayResult{
    Entity entity;
    Vector3 hitPoint;
    Vector3 hitNormal;
};

using OnCollisionCallback = void(*)(Scene&, Entity, Entity);
//using OnCollisionCallback = std::function<void(Entity, Entity)>;

struct OD_API PhysicsSystem: public System{
    friend struct RigidbodyComponent;

    PhysicsSystem(Scene* scene);
    ~PhysicsSystem() override;

    /*System* Clone(Scene* inScene) const override{ 
        PhysicsSystem* system = new PhysicsSystem(inScene);
        system->onCollisionEnterCallbacks = onCollisionEnterCallbacks;
        system->onCollisionExitCallbacks = onCollisionExitCallbacks;
        system->onTriggerEnterCallbacks = onTriggerEnterCallbacks;
        system->onTriggerExitCallbacks = onTriggerExitCallbacks;
        return system; 
    }*/
    
    virtual int Type() override { return SystemType::Physics; }
    virtual void PhysicsUpdate() override;
    virtual void OnDrawGizmos(Camera& cam) override;

    void ShowDebugGizmos();

    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit);
    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask);
    bool IsSimulationEnable();
    void Simulate(float step);
    void SynchronizeMotionStates();

    void AddOnCollisionEnterCallback(OnCollisionCallback callback);
    void RemoveOnCollisionEnterCallback(OnCollisionCallback callback);
    
    void AddOnCollisionExitCallback(OnCollisionCallback callback);
    void RemoveOnCollisionExitCallback(OnCollisionCallback callback);

    void AddOnTriggerEnterCallback(OnCollisionCallback callback);
    void RemoveOnTriggerEnterCallback(OnCollisionCallback callback);
    
    void AddOnTriggerExitCallback(OnCollisionCallback callback);
    void RemoveOnTriggerExitCallback(OnCollisionCallback callback);

    void* GetInternlWorld(); // Temp/Experimental 

private:
    void CheckForCollisionEvents();

    static void OnRemoveRigidbody(entt::registry& r, entt::entity e);
    void AddRigidbody(Entity entity, RigidbodyComponent& c, TransformComponent& t, InfoComponent& info);
    void RemoveRigidbody(Entity entity, RigidbodyComponent& c);

    static void OnRemoveCollisionBody(entt::registry& r, entt::entity e);
    void AddCollisionBody(Entity entity, CollisionBodyComponent& c, TransformComponent& t, InfoComponent& info);
    void RemoveCollisionBody(Entity entity, CollisionBodyComponent& c);

    static void OnRemoveJoint(entt::registry& r, entt::entity e);
    void AddJoint(Scene* scene, Entity entity, JointComponent& c, TransformComponent& t, InfoComponent& info);
    void RemoveJoint(Entity entity, JointComponent& c);

    //#if defined(UseBulletPhysics)
    class PhysicsWorld* physicsWorld = nullptr;
    //#endif
    
    std::vector<OnCollisionCallback> onCollisionEnterCallbacks;
    std::vector<OnCollisionCallback> onCollisionExitCallbacks;
    std::vector<OnCollisionCallback> onTriggerEnterCallbacks;
    std::vector<OnCollisionCallback> onTriggerExitCallbacks;
};

void PhysicsModuleInit();

}