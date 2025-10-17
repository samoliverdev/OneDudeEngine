#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Animation/Pose.h"

#define UseJoltPhysics


namespace OD{

//struct PhysicObject;
//struct JointObject;
//struct PhysicsWorld;

//using MeshShapeData = btBvhTriangleMeshShape;
class MeshShapeData;

Ref<MeshShapeData> OD_API CreateMeshShapeData(Model& model);
Ref<MeshShapeData> OD_API CreateMeshShapeData(const Mesh& mesh);
Ref<MeshShapeData> OD_API CreateMeshShapeData(const std::vector<Vector3>& vertices, const std::vector<unsigned int> indices);

enum class RigidbodyConstraints: uint8_t{
    None				= 0b000000,									///< No degrees of freedom are allowed. Note that this is not valid and will crash. Use a static body instead.
	All					= 0b111111,									///< All degrees of freedom are allowed
	TranslationX		= 0b000001,									///< Body can move in world space X axis
	TranslationY		= 0b000010,									///< Body can move in world space Y axis
	TranslationZ		= 0b000100,									///< Body can move in world space Z axis
	RotationX			= 0b001000,									///< Body can rotate around world space X axis
	RotationY			= 0b010000,									///< Body can rotate around world space Y axis
	RotationZ			= 0b100000,									///< Body can rotate around world space Z axis
	Plane2D				= TranslationX | TranslationY | RotationZ,	///< Body can only move in X and Y axis and rotate around Z axis
};
/// Bitwise OR operator for EAllowedDOFs
constexpr RigidbodyConstraints operator | (RigidbodyConstraints inLHS, RigidbodyConstraints inRHS){ return RigidbodyConstraints(uint8_t(inLHS) | uint8_t(inRHS)); }
constexpr RigidbodyConstraints operator & (RigidbodyConstraints inLHS, RigidbodyConstraints inRHS){ return RigidbodyConstraints(uint8_t(inLHS) & uint8_t(inRHS)); }
constexpr RigidbodyConstraints operator ^ (RigidbodyConstraints inLHS, RigidbodyConstraints inRHS){ return RigidbodyConstraints(uint8_t(inLHS) ^ uint8_t(inRHS)); }
constexpr RigidbodyConstraints operator ~ (RigidbodyConstraints inAllowedDOFs){ return RigidbodyConstraints(~uint8_t(inAllowedDOFs)); }
constexpr RigidbodyConstraints & operator |= (RigidbodyConstraints &ioLHS, RigidbodyConstraints inRHS){ ioLHS = ioLHS | inRHS; return ioLHS; }
constexpr RigidbodyConstraints & operator &= (RigidbodyConstraints &ioLHS, RigidbodyConstraints inRHS){ ioLHS = ioLHS & inRHS; return ioLHS; }
constexpr RigidbodyConstraints & operator ^= (RigidbodyConstraints &ioLHS, RigidbodyConstraints inRHS){ ioLHS = ioLHS ^ inRHS; return ioLHS; }

enum class PhysicMotionQuality{
    Discrete,
    LinearCast
};

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
        shape.mesh = CreateMeshShapeData(*mesh);
        return shape;
    }

    inline static CollisionShape MeshShape(Ref<MeshShapeData> mesh){
        CollisionShape shape;
        shape.type = Type::Mesh;
        shape.mesh = mesh;
        return shape;
    }

    inline static CollisionShape MeshShape(){
        CollisionShape shape;
        shape.type = Type::Mesh;
        shape.mesh = nullptr;
        return shape;
    }
};

struct OD_API RigidbodyComponent{
    friend struct PhysicsSystem;
    friend class SelectedBodyDrawFilter;

    //int mask = AllLayers;
    LayerMask mask = {AllLayers};
    bool interpolate = false;

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

    inline float Friction(){ return friction; }
    void Friction(float mass);

    inline bool NeverSleep(){ return neverSleep; }
    void NeverSleep(bool value);

    Vector3 CenterOfMass();

    Vector3 Position();
    Vector3 PositionInterpoled();
    void Position(Vector3 position);

    Quaternion Rotation();
    void Rotation(Quaternion rotation);

    void SetTransform(const Vector3& pos, const Quaternion& rot);

    Vector3 Velocity();
    void Velocity(Vector3 v);

    Vector3 AngularVelocity();
    void AngularVelocity(Vector3 v);

    void ApplyForce(Vector3 v);
    void ApplyTorque(Vector3 v);
    void ApplyImpulse(Vector3 v);

    void AddExplosionImpulse(float force, Vector3 explosionPosition, float radius, float upwardsModifier);

    void SetAngularFactor(Vector3 v);

    float LinearDamping();
    void LinearDamping(float v);
    float AngularDamping();
    void AngularDamping(float v);

    PhysicMotionQuality MotionQuality();
    void MotionQuality(PhysicMotionQuality v);

    RigidbodyConstraints Constraints();
    void Constraints(RigidbodyConstraints constraints);

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(interpolate));
        ArchiveDump(ar, CEREAL_NVP(shape));
        ArchiveDump(ar, CEREAL_NVP(mass));
        ArchiveDump(ar, CEREAL_NVP(friction));
        ArchiveDump(ar, CEREAL_NVP(linearDamping));
        ArchiveDump(ar, CEREAL_NVP(angularDamping));
        ArchiveDump(ar, CEREAL_NVP(motionQuality));
        ArchiveDump(ar, CEREAL_NVP(constraints));
        ArchiveDump(ar, CEREAL_NVP(neverSleep));
        ArchiveDump(ar, CEREAL_NVP(mask));
    }

    DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(RigidbodyComponent, {
        COPY_OR_MOVE(shape);
        COPY_OR_MOVE(type);
        COPY_OR_MOVE(interpolate);
        COPY_OR_MOVE(angularFactor);
        COPY_OR_MOVE(mass);
        COPY_OR_MOVE(friction);
        COPY_OR_MOVE(linearDamping);
        COPY_OR_MOVE(angularDamping);
        COPY_OR_MOVE(motionQuality);
        COPY_OR_MOVE(constraints);
        COPY_OR_MOVE(neverSleep);
        COPY_OR_MOVE(mask);
    });

    inline const class PhysicObject* InternalData(){ return data; }

private:
    CollisionShape shape;
    Type type = Type::Dynamic;
    Vector3 angularFactor = {1, 1, 1};
    RigidbodyConstraints constraints = RigidbodyConstraints::All;
    float mass = 1;
    float friction = 0.2f;
    float linearDamping = 0;
    float angularDamping = 0.05;
    PhysicMotionQuality motionQuality;
    bool neverSleep = false;

    Vector3 previousPosition = Vector3Zero;
    Quaternion previousRotation = QuaternionIdentity;

    class PhysicObject* data = nullptr;

    void UpdateSettings();
};

struct OD_API RagdollComponent{
    friend struct PhysicsSystem;
    friend class SelectedBodyDrawFilter;

    RagdollComponent() = default;

    struct Part{
        CollisionShape shape;
        int parent = -1;
        int skinnedSkeletonIndex = -1;

        enum class OverrideType{None, Dynamic, Kinematic, Static};
        OverrideType overrideType = OverrideType::None;

        Vector3 constraintPos = Vector3Zero;
        Vector3 twistAxis = Vector3Zero;
        float twistAngleMin = 0;
        float twistAngleMax = 0;
        float normalAngle = 0;
        float planeAngle = 0;

        bool disableSync = false;
        bool isHips = false;

        Vector3 previousPosition = Vector3Zero;
        Quaternion previousRotation = QuaternionIdentity;

        //TODO: Maybe add this again later, this is to be used with a no skinned bone
        //Vector3 pos = Vector3Zero;
        //Quaternion rot = QuaternionIdentity;

        Quaternion initedRot = QuaternionIdentity;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, shape);
            ArchiveDumpNVP(ar, parent);
            ArchiveDumpNVP(ar, skinnedSkeletonIndex);
            ArchiveDumpNVP(ar, overrideType);
            //ArchiveDumpNVP(ar, pos);
            //ArchiveDumpNVP(ar, rot);
            ArchiveDumpNVP(ar, disableSync);
            ArchiveDumpNVP(ar, isHips);
            ArchiveDumpNVP(ar, constraintPos);
            ArchiveDumpNVP(ar, twistAxis);
            ArchiveDumpNVP(ar, twistAngleMin);
            ArchiveDumpNVP(ar, twistAngleMax);
            ArchiveDumpNVP(ar, normalAngle);
            ArchiveDumpNVP(ar, planeAngle);
        }
    };

    float globalMass = 75;
    Layers layer = Layers::Layer0;
    LayerMask mask = {AllLayers};

    enum class Type{Dynamic, Kinematic, Static, Trigger};
    Type type;
    bool interpolate = false;
    bool isDirty = true;
    std::vector<Part> parts;
    
    bool syncWithFinalPose = false;
    bool syncFromTheHips = true;
    bool useTorqueControl = false;
    float gain = 10;
    float damping = 1;
    float stiffness = 10; 

    Vector3 overrideStartVelocity = Vector3Zero;

    inline int TryFindHipIndex(){
        for(int i = 0; i < parts.size(); i++){
            if(parts[i].isHips) return i;
        }
        return -1;
    }

    float Mass(int boneIndex);

    Vector3 CenterOfMass(int boneIndex);

    Vector3 Position(int boneIndex);
    Vector3 PositionInterpoled(int boneIndex);
    void Position(int boneIndex, Vector3 position);
    Quaternion Rotation(int boneIndex);
    void Rotation(int boneIndex, Quaternion rotation);
    Vector3 Velocity(int boneIndex);
    void Velocity(int boneIndex, Vector3 v);
    Vector3 AngularVelocity(int boneIndex);
    void AngularVelocity(int boneIndex, Vector3 v);
    void ApplyForce(int boneIndex, Vector3 v);
    void ApplyTorque(int boneIndex, Vector3 v);
    void ApplyImpulse(int boneIndex, Vector3 v);

    void AddExplosionImpulse(float force, Vector3 explosionPosition, float radius, float upwardsModifier);

    static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, globalMass);
        ArchiveDumpNVP(ar, layer);
        ArchiveDumpNVP(ar, mask);
        //ArchiveDumpNVP(ar, isDirty);
        ArchiveDumpNVP(ar, type);
        ArchiveDumpNVP(ar, interpolate);
        ArchiveDumpNVP(ar, parts);

        ArchiveDumpNVP(ar, syncWithFinalPose);
        ArchiveDumpNVP(ar, syncFromTheHips);
        ArchiveDumpNVP(ar, useTorqueControl);
        ArchiveDumpNVP(ar, gain);
        ArchiveDumpNVP(ar, damping);
        ArchiveDumpNVP(ar, stiffness);
    }

    DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(RagdollComponent, {
        COPY_OR_MOVE(layer);
        COPY_OR_MOVE(mask);
        //COPY_OR_MOVE(isDirty);
        COPY_OR_MOVE(type);
        COPY_OR_MOVE(interpolate);
        COPY_OR_MOVE(parts);

        COPY_OR_MOVE(syncWithFinalPose);
        COPY_OR_MOVE(syncFromTheHips);
        COPY_OR_MOVE(useTorqueControl);
        COPY_OR_MOVE(gain);
        COPY_OR_MOVE(damping);
        COPY_OR_MOVE(stiffness);
    });

    Pose startPose;

private:
    struct RagdollObject* data = nullptr;
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

    HeightmapColliderComponent() = default;

    //TODO: Add DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED here
    DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(HeightmapColliderComponent, {
        COPY_OR_MOVE(heights);
        COPY_OR_MOVE(width);
        COPY_OR_MOVE(length);
        COPY_OR_MOVE(scale);
        COPY_OR_MOVE(minHeight);
        COPY_OR_MOVE(maxHeight);
        COPY_OR_MOVE(offset);
    });
    
private:

    #if defined(UseBulletPhysics)
    btHeightfieldTerrainShape* shape = nullptr;
    btCollisionObject* body = nullptr;
    #endif

    #if defined(UseJoltPhysics)
    class PhysicObject* data = nullptr; 
    #endif
};

struct OD_API RayResult{
    Entity entity;
    int subBodyIndex = -1;
    Vector3 hitPoint;
    Vector3 hitNormal;
};

struct OD_API MotorTest{
    bool inited = false;

    template <class Archive>
    void serialize(Archive & ar){}
};

using OnCollisionCallback = void(*)(Scene&, Entity, Entity, Vector3);
//using OnCollisionCallback = std::function<void(Entity, Entity)>;

struct OD_API PhysicsSystem: public System{
    friend struct RigidbodyComponent;
    friend class MyContactListener;

    void OnInit(Scene& scene) override;
    void OnEnd(Scene& scene) override;

    /*System* Clone(Scene* inScene) const override{ 
        PhysicsSystem* system = new PhysicsSystem(inScene);
        system->onCollisionEnterCallbacks = onCollisionEnterCallbacks;
        system->onCollisionExitCallbacks = onCollisionExitCallbacks;
        system->onTriggerEnterCallbacks = onTriggerEnterCallbacks;
        system->onTriggerExitCallbacks = onTriggerExitCallbacks;
        return system; 
    }*/
    
    virtual int Type() override { return SystemType::Physics; }
    virtual void PhysicsUpdate(Scene& scene) override;
    virtual void OnDrawGizmos(Scene& scene, Camera& cam) override;

    void ShowDebugGizmos();

    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit);
    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask);

    std::vector<RayResult> OverlapSphere(Vector3 center, float radius);

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

    inline float PhysicsAccumulator(){ return physicsAccumulator; }

private:
    void CheckForCollisionEvents();

    static void OnRemoveRagdoll(entt::registry& r, entt::entity e);
    static void OnRemoveHeightmap(entt::registry& r, entt::entity e);

    static void OnRemoveRigidbody(entt::registry& r, entt::entity e);
    void AddRigidbody(Entity entity, RigidbodyComponent& c, TransformComponent& t, InfoComponent& info);
    void RemoveRigidbody(Entity entity, RigidbodyComponent& c);

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

    float physicsAccumulator = 0.0f;

    Scene* scene;
};

void PhysicsModuleInit();

}