#pragma once
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Graphics/Model.h"
#include "OD/Animation/Pose.h"
#include "concurrentqueue.h"

#define UseJoltPhysics

namespace JPH{
    class BodyInterface;
}
 
namespace OD{

//class Model;
class Mesh;

struct SkinnedModelRendererComponent;

//struct PhysicObject;
//struct JointObject;
//struct PhysicsWorld;

struct PhysicsSystem;

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
constexpr RigidbodyConstraints operator|(RigidbodyConstraints inLHS, RigidbodyConstraints inRHS){ return RigidbodyConstraints(uint8_t(inLHS) | uint8_t(inRHS)); }
constexpr RigidbodyConstraints operator&(RigidbodyConstraints inLHS, RigidbodyConstraints inRHS){ return RigidbodyConstraints(uint8_t(inLHS) & uint8_t(inRHS)); }
constexpr RigidbodyConstraints operator^(RigidbodyConstraints inLHS, RigidbodyConstraints inRHS){ return RigidbodyConstraints(uint8_t(inLHS) ^ uint8_t(inRHS)); }
constexpr RigidbodyConstraints operator~(RigidbodyConstraints inAllowedDOFs){ return RigidbodyConstraints(~uint8_t(inAllowedDOFs)); }
constexpr RigidbodyConstraints& operator|=(RigidbodyConstraints &ioLHS, RigidbodyConstraints inRHS){ ioLHS = ioLHS | inRHS; return ioLHS; }
constexpr RigidbodyConstraints& operator&=(RigidbodyConstraints &ioLHS, RigidbodyConstraints inRHS){ ioLHS = ioLHS & inRHS; return ioLHS; }
constexpr RigidbodyConstraints& operator^=(RigidbodyConstraints &ioLHS, RigidbodyConstraints inRHS){ ioLHS = ioLHS ^ inRHS; return ioLHS; }

enum class PhysicMotionQuality{
    Discrete,
    LinearCast
};

struct OD_API CollisionShape{
    enum class Type{Box, Sphere, Capsule, Mesh, Model};

    Type type;
    Vector3 center = {0, 0, 0};
    Vector3 rotation = {0, 0, 0};
    Vector3 size = {1,1,1};
    float radius = 1;
    float height = 1;
    Ref<MeshShapeData> meshData = nullptr;
    Ref<Model> modelSource = nullptr;
    int modelSourceMeshIndex = -1;

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(center));
        ArchiveDump(ar, CEREAL_NVP(rotation));
        ArchiveDump(ar, CEREAL_NVP(size));
        ArchiveDump(ar, CEREAL_NVP(radius));
        ArchiveDump(ar, CEREAL_NVP(height));

        AssetRefSerialize<Model> modelSourceRef(modelSource);
        ArchiveDumpNVP(ar, modelSourceRef);
        ArchiveDumpNVP(ar, modelSourceMeshIndex);
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
        shape.meshData = CreateMeshShapeData(*mesh);
        return shape;
    }

    inline static CollisionShape MeshShape(Ref<MeshShapeData> mesh){
        CollisionShape shape;
        shape.type = Type::Mesh;
        shape.meshData = mesh;
        return shape;
    }

    inline static CollisionShape MeshShape(){
        CollisionShape shape;
        shape.type = Type::Mesh;
        shape.meshData = nullptr;
        return shape;
    }

    inline static CollisionShape ModelShape(Ref<Model> model, int meshIndex = 0){
        CollisionShape shape;
        shape.type = Type::Model;
        shape.modelSource = model;
        shape.modelSourceMeshIndex = meshIndex;
        shape.meshData = CreateMeshShapeData(*model->meshs[meshIndex]);
        return shape;
    }
};

struct OD_API RigidbodyComponent{
    friend struct PhysicsSystem;
    friend class SelectedBodyDrawFilter;

    //int mask = AllLayers;
    LayerMask mask = {AllLayersMask};// {AllLayers};
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

        ArchiveDump(ar, CEREAL_NVP(overrideCenterOfMass));
        ArchiveDump(ar, CEREAL_NVP(centerOfMass));
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

        COPY_OR_MOVE(overrideCenterOfMass);
        COPY_OR_MOVE(centerOfMass);
    });

    inline const class PhysicObject* InternalData(){ return data; }

private:
    CollisionShape shape;
    Type type = Type::Dynamic;
    Vector3 angularFactor = {1, 1, 1};
    bool overrideCenterOfMass = false;
    Vector3 centerOfMass = {0, 0, 0};
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
    bool isDirt = true;
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
        float stiffnessMult = 1; 

        float overrideLinearDamping = -1;

        RigidbodyConstraints constraints = RigidbodyConstraints::All;

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
            ArchiveDumpNVP(ar, stiffnessMult);
            ArchiveDumpNVP(ar, overrideLinearDamping);
            ArchiveDumpNVP(ar, constraints);
            ArchiveDumpNVP(ar, constraintPos);
            ArchiveDumpNVP(ar, twistAxis);
            ArchiveDumpNVP(ar, twistAngleMin);
            ArchiveDumpNVP(ar, twistAngleMax);
            ArchiveDumpNVP(ar, normalAngle);
            ArchiveDumpNVP(ar, planeAngle);
        }
    };

    float globalMass = 75;
    float linearDamping = 0;
    Layers layer = Layers::Layer0;
    LayerMask mask = {AllLayersMask};

    enum class Type{Dynamic, Kinematic, Static, Trigger, Disable};
    enum class MotorType{None, Jolt, TargetRot, TargetRotLocal};

    Type type;
    MotorType motorType;

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

    RigidbodyConstraints Constraints(int boneIndex);
    void Constraints(int boneIndex, RigidbodyConstraints constraints);

    Type GetType();
    void SetType(Type type);

    void UpdateInternalData(TransformComponent& t, InfoComponent& info, SkinnedModelRendererComponent& skinned);

    void AddExplosionImpulse(float force, Vector3 explosionPosition, float radius, float upwardsModifier);

    static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, globalMass);
        ArchiveDumpNVP(ar, linearDamping);
        ArchiveDumpNVP(ar, layer);
        ArchiveDumpNVP(ar, mask);
        //ArchiveDumpNVP(ar, isDirty);
        ArchiveDumpNVP(ar, type);
        ArchiveDumpNVP(ar, motorType);
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
        COPY_OR_MOVE(globalMass);
        COPY_OR_MOVE(linearDamping);
        COPY_OR_MOVE(layer);
        COPY_OR_MOVE(mask);
        //COPY_OR_MOVE(isDirty);
        COPY_OR_MOVE(type);
        COPY_OR_MOVE(motorType);
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
    PhysicsSystem* physicSystem = nullptr;
    Scene* scene = nullptr;
    Entity entity;
};

enum class JointSpace{
	Local,
	WorldSpace
};

struct OD_API JointComponent{
    friend struct PhysicsSystem;

    enum class Type{
        Fixed,		// fixed in place completely
        //Point,		// fixed to a point but can rotate around it
        Distance,	// point constraint within specified distance
        //Hinge,		// rotation around a point on the UP axis of the contraint transform
        //Cone,		// constrain to a cone shape specified by the cone angle (cone axis: UP)
        //SixDOF,		// manual specification of axes movement and rotation limits
        //SwingTwist,	// cone (UP axis) + rotational limits
        //Slider,		// constrain on the RIGHT axis between limits
    };

    struct FixedSettings{
        Vector3 point1 = Vector3Zero;
        Vector3 point2 = Vector3Zero;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, point1);
            ArchiveDumpNVP(ar, point2);
        }
	};

    struct DistanceSettings{
        Vector3 point1 = Vector3Zero;
        Vector3 point2 = Vector3Zero;

        float minDistance = -1;
        float maxDistance = -1;

        float springFequency = 0.0f;
	    float springDamping = 0.0f;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, point1);
            ArchiveDumpNVP(ar, point2);

            ArchiveDumpNVP(ar, minDistance);
            ArchiveDumpNVP(ar, maxDistance);

            ArchiveDumpNVP(ar, springFequency);
            ArchiveDumpNVP(ar, springDamping);
        }
    };

    static void OnGui(Entity& e, Scene& scene);

    void SetTargets(Entity bodyA, int bodyASubIndex, Entity bodyB, int bodyBSubIndex);

    JointSpace GetJointSpace();
    void SetJointSpace(JointSpace injointSpace);

    void CreateFixed(const FixedSettings& settings);
    void CreateDistance(const DistanceSettings& settings);

    void SetDistance(float min, float max);

    Vector3 GetWorldSpacePoint1Pos();
    Vector3 GetWorldSpacePoint2Pos();

    //static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, bodyA);
        ArchiveDumpNVP(ar, bodyASubIndex);
        ArchiveDumpNVP(ar, bodyB);
        ArchiveDumpNVP(ar, bodyBSubIndex);

        ArchiveDumpNVP(ar, type);
        ArchiveDumpNVP(ar, jointSpace);

        //ArchiveDumpNVP(ar, disableSelfCollision);

        ArchiveDumpNVP(ar, fixedSettings);
        ArchiveDumpNVP(ar, distanceSettings);

        /*ArchiveDumpNVP(ar, pivot);
        ArchiveDumpNVP(ar, connectedPivot);
        ArchiveDumpNVP(ar, angularLowerLimit);
        ArchiveDumpNVP(ar, angularUpperLimit);
        ArchiveDumpNVP(ar, disableSelfCollision);*/
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
    Entity bodyA = EntityNull;
    int bodyASubIndex = -1;

    Entity bodyB = EntityNull;
    int bodyBSubIndex = -1;

    Type type = Type::Fixed;
    JointSpace jointSpace = JointSpace::Local;

    FixedSettings fixedSettings;
    DistanceSettings distanceSettings;
    
    //bool autoConfigConnectedPivot = true;
    //bool disableSelfCollision = true;
    bool isDirty = true;

    class JointObject* data = nullptr;
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

struct OD_API VehiclePhysic{
    friend struct PhysicsSystem;
    friend class SelectedBodyDrawFilter;

    struct Wheel{
        bool isFront;
        Vector3 pos;
        float radius;
        float width = 0.1f;

        float suspensionMinLength = 0.3f;
		float suspensionMaxLength = 0.5f;
		float suspensionFrequency = 1.5f;
		float suspensionDamping = 0.5f;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, isFront);
            ArchiveDumpNVP(ar, pos);
            ArchiveDumpNVP(ar, radius);
            ArchiveDumpNVP(ar, width);

            ArchiveDumpNVP(ar, suspensionMinLength);
            ArchiveDumpNVP(ar, suspensionMaxLength);
            ArchiveDumpNVP(ar, suspensionFrequency);
            ArchiveDumpNVP(ar, suspensionDamping);
        }
    };

    std::vector<Wheel> wheels = {
        {true, {0.9f, 0, 2}, 0.3f},
        {true, {-0.9f, 0, 2}, 0.3f},
        {false, {0.9f, 0, -2}, 0.3f},
        {false, {-0.9f, 0, -2}, 0.3f},
    };

    float maxSteeringAngle = 30;
    float maxRollAngle = 60;

    float maxEngineTorque = 500.0f;
    float clutchStrength = 10.0f;

    float antiRollBarsStiffness = 1000.0f;
    float longitudinalImpulseMultplier = 10;
    float lateralImpulseMultplier = 1;

    //Inputs
    float forwardInput = 0;
    float rightInput = 0;
    float brakeInput = 0;
    float handBrakeInput = 0;

    bool handleDebugInputs = false;
    float previousForward = 0;

    Transform GetWheelWorldTransform(int wheelIndex, Vector3 up, Vector3 right);
    Transform GetWheelLocalTransform(int wheelIndex, Vector3 up, Vector3 right);

    VehiclePhysic() = default;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive & ar){ 
        ArchiveDump(ar, CEREAL_NVP(wheels));
        ArchiveDump(ar, CEREAL_NVP(maxSteeringAngle));
        ArchiveDump(ar, CEREAL_NVP(maxRollAngle));
        ArchiveDump(ar, CEREAL_NVP(maxEngineTorque));
        ArchiveDump(ar, CEREAL_NVP(clutchStrength));

        ArchiveDump(ar, CEREAL_NVP(antiRollBarsStiffness));
        ArchiveDump(ar, CEREAL_NVP(longitudinalImpulseMultplier));
        ArchiveDump(ar, CEREAL_NVP(lateralImpulseMultplier));

        ArchiveDump(ar, CEREAL_NVP(handleDebugInputs));
    }

    DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(VehiclePhysic, {
        COPY_OR_MOVE(wheels);
        COPY_OR_MOVE(maxSteeringAngle);
        COPY_OR_MOVE(maxRollAngle);
        COPY_OR_MOVE(maxEngineTorque);
        COPY_OR_MOVE(clutchStrength);

        COPY_OR_MOVE(antiRollBarsStiffness);
        COPY_OR_MOVE(longitudinalImpulseMultplier);
        COPY_OR_MOVE(lateralImpulseMultplier);

        COPY_OR_MOVE(handleDebugInputs);
    });

private:
    class VehiclePhysicData* data = nullptr;
};

struct OD_API Collision{
    Vector3 relativeVelocity;
    Vector3 normal;
    Vector3 relativeContactPointOn1;
    Vector3 relativeContactPointOn2;
    Entity e1;
    Entity e2;
    float penetrationDepth;
    bool body1IsSensor;
    bool body2IsSensor;
};

using OnCollisionCallback = void(*)(Scene&, const Collision&);
//using OnCollisionCallback = std::function<void(Entity, Entity)>;

struct OD_API PhysicsSystem: public System{
    friend struct RigidbodyComponent;
    friend struct RagdollComponent;
    friend class MyContactListener;

    void OnInit(Scene& scene) override;
    void OnEnd(Scene& scene) override;

    PhysicsSystem(){ name = "PhysicsSystem"; }
    virtual ~PhysicsSystem() override;

    /*System* Clone(Scene* inScene) const override{ 
        PhysicsSystem* system = new PhysicsSystem(inScene);
        system->onCollisionEnterCallbacks = onCollisionEnterCallbacks;
        system->onCollisionExitCallbacks = onCollisionExitCallbacks;
        system->onTriggerEnterCallbacks = onTriggerEnterCallbacks;
        system->onTriggerExitCallbacks = onTriggerExitCallbacks;
        return system; 
    }*/
    
    virtual int Type() override { return SystemType::PrePhysics | SystemType::FixedPhysics | SystemType::PostPhysics; }
    virtual int ExecutionSortPriority(SystemType type) override;
    virtual void PrePhysicsUpdate(Scene& scene) override;
    virtual void FixedPhysicsUpdate(Scene& scene) override;
    virtual void PostPhysicsUpdate(Scene& scene) override;
    virtual void OnDrawGizmos(Scene& scene, Camera& cam) override;

    void _PostPhysicsUpdate(bool onlyPostSync, bool canInterpolate);

    void ShowDebugGizmos();

    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit);
    bool Raycast(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask);
    bool RaycastIgnoreSensor(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask);//Created this becose a strange bug with if(ignoreSensor), but still is bug TODO: Fix this later

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

    float InterpolationAlpha();

private:
    void CheckForCollisionEvents();

    static void OnRemoveRagdoll(entt::registry& r, entt::entity e);
    void AddRagdoll(Entity entity, RagdollComponent& c, TransformComponent& t, InfoComponent& info, SkinnedModelRendererComponent& skinned);
    void RemoveRagdoll(Entity entity, RagdollComponent& c); 

    static void OnRemoveHeightmap(entt::registry& r, entt::entity e);

    static void OnRemoveRigidbody(entt::registry& r, entt::entity e);
    void AddRigidbody(Entity entity, RigidbodyComponent& c, TransformComponent& t, InfoComponent& info);
    void RemoveRigidbody(Entity entity, RigidbodyComponent& c);

    static void OnRemoveVehicle(entt::registry& r, entt::entity e);
    void AddVehicle(Entity entity, VehiclePhysic& veh, RigidbodyComponent& c, TransformComponent& t, InfoComponent& info);
    void RemoveVehicle(Entity entity, VehiclePhysic& c);

    static void OnRemoveJoint(entt::registry& r, entt::entity e);
    void AddJoint(Scene* scene, Entity entity, JointComponent& c, TransformComponent& t, InfoComponent& info);
    void RemoveJoint(Entity entity, JointComponent& c);
    void SetJointsAsDirtyIfBodyIsDirty(Entity e);


    //#if defined(UseBulletPhysics)
    class PhysicsWorld* physicsWorld = nullptr;
    //#endif
    
    std::vector<OnCollisionCallback> onCollisionEnterCallbacks;
    std::vector<OnCollisionCallback> onCollisionExitCallbacks;
    std::vector<OnCollisionCallback> onTriggerEnterCallbacks;
    std::vector<OnCollisionCallback> onTriggerExitCallbacks;

    moodycamel::ConcurrentQueue<Collision> onContactAddedData;
    moodycamel::ConcurrentQueue<Collision> onContactRemovedData;

    //float physicsAccumulator = 0.0f;

    Scene* scene;

public:
    static void _SyncRagdollToPose(Scene& scene, SkinnedModelRendererComponent& skinned, RagdollComponent& ragdoll, TransformComponent& trans, InfoComponent& info, JPH::BodyInterface& bodyInterface, bool canInterpolate);

    //INFO: I think this can be a litter performace impruvment instead of using lambda
    //friend struct _SyncRagdollToPose2;
    struct _SyncRagdollToPose2{
        Scene& scene; 
        SkinnedModelRendererComponent& skinned; 
        RagdollComponent& ragdoll; 
        TransformComponent& trans; 
        InfoComponent& info; 
        JPH::BodyInterface& bodyInterface; 
        bool canInterpolate;
        void operator()();
    };
};

void PhysicsModuleInit();

}