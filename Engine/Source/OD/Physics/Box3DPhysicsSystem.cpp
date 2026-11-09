#include "PhysicsSystem.h"

#if defined(OD_PHYSICS_BACKEND_BOX3D)

#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Graphics/Mesh.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include <box3d/collision.h>

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace OD {
struct ShapeUserData {
    Entity entity = EntityNull;
    int subBodyIndex = -1;
    bool trigger = false;
};
struct MeshShapeData {
    std::vector<Vector3> vertices;
    std::vector<unsigned int> indices;
    b3MeshData* mesh = nullptr;
    ~MeshShapeData() { if (mesh) b3DestroyMesh(mesh); }
};
struct Box3DHeightmapData {
    b3BodyId body = b3_nullBodyId;
    b3HeightFieldData* field = nullptr;
    ShapeUserData user;
};
namespace {

b3Vec3 ToB3(Vector3 v) { return {v.x, v.y, v.z}; }
b3Quat ToB3(Quaternion q) { return {{q.x, q.y, q.z}, q.w}; }
Vector3 FromB3(b3Vec3 v) { return {v.x, v.y, v.z}; }
Quaternion FromB3(b3Quat q) { return {q.v.x, q.v.y, q.v.z, q.s}; }
b3Pos ToB3Pos(Vector3 v) { return {v.x, v.y, v.z}; }
Vector3 FromB3Pos(b3Pos v) { return {float(v.x), float(v.y), float(v.z)}; }

struct Box3DWorld {
    b3WorldId id = b3_nullWorldId;
};

struct RayCastContext {
    Entity ignoredEntity = EntityNull;
    bool ignoreSensor = false;
    bool hit = false;
    float fraction = 1.0f;
    b3ShapeId shape = b3_nullShapeId;
    b3Pos point{};
    b3Vec3 normal{};
};

static float CollectRayHit(b3ShapeId shape, b3Pos point, b3Vec3 normal, float fraction,
                           uint64_t, int, int, void* context) {
    auto& query = *static_cast<RayCastContext*>(context);
    auto* data = static_cast<ShapeUserData*>(b3Shape_GetUserData(shape));
    if ((query.ignoreSensor && b3Shape_IsSensor(shape)) || (data && data->entity == query.ignoredEntity)) return -1.0f;
    if (fraction < query.fraction) {
        query.hit = true;
        query.fraction = fraction;
        query.shape = shape;
        query.point = point;
        query.normal = normal;
    }
    return query.fraction;
}

static void MakeB3Shape(b3BodyId body, CollisionShape& shape, RigidbodyComponent& rb,
                        Entity entity, int subIndex, std::vector<ShapeUserData*>& owned, uint32_t layer,
                        uint32_t mask, Vector3 scale = Vector3One, bool forceSensor = false) {
    b3ShapeDef def = b3DefaultShapeDef();
    def.filter = b3DefaultFilter();
    def.filter.categoryBits = uint64_t(1) << layer;
    def.filter.maskBits = mask;
    def.baseMaterial.friction = rb.Friction();
    def.density = rb.Mass();
    def.isSensor = forceSensor || rb.GetType() == RigidbodyComponent::Type::Trigger;
    def.enableSensorEvents = true;
    def.enableContactEvents = true;

    auto* user = new ShapeUserData{entity, subIndex, def.isSensor};
    owned.push_back(user);
    def.userData = user;

    b3ShapeId id = b3_nullShapeId;
    if (shape.type == CollisionShape::Type::Box) {
        b3BoxHull hull = b3MakeTransformedBoxHull(shape.size.x * scale.x * 0.5f, shape.size.y * scale.y * 0.5f,
                                                   shape.size.z * scale.z * 0.5f,
                                                   {{shape.center.x * scale.x, shape.center.y * scale.y, shape.center.z * scale.z}, ToB3(shape.rot)});
        id = b3CreateHullShape(body, &def, &hull.base);
    } else if (shape.type == CollisionShape::Type::Sphere) {
        b3Sphere sphere{ToB3(shape.center * scale), shape.radius * std::max({scale.x, scale.y, scale.z})};
        id = b3CreateSphereShape(body, &def, &sphere);
    } else if (shape.type == CollisionShape::Type::Capsule) {
        const float radius = shape.radius * std::max(scale.x, scale.z);
        const float halfSegment = std::max(0.0f, shape.height * scale.y * 0.5f - radius);
        const Vector3 center = shape.center * scale;
        b3Capsule capsule{{center.x, center.y - halfSegment, center.z},
                          {center.x, center.y + halfSegment, center.z}, radius};
        id = b3CreateCapsuleShape(body, &def, &capsule);
    } else if ((shape.type == CollisionShape::Type::Mesh || shape.type == CollisionShape::Type::Model) && shape.meshData) {
        std::vector<b3Vec3> points;
        points.reserve(shape.meshData->vertices.size());
        for (auto v : shape.meshData->vertices) points.push_back(ToB3(v));
        if (rb.GetType() == RigidbodyComponent::Type::Static && shape.meshData->mesh) {
            id = b3CreateMeshShape(body, &def, shape.meshData->mesh, ToB3(scale));
        } else if (points.size() >= 4) {
            for (auto& point : points) { point.x *= scale.x; point.y *= scale.y; point.z *= scale.z; }
            auto* hull = b3CreateHull(points.data(), int(points.size()), B3_MAX_HULL_VERTICES);
            if (hull) {
                id = b3CreateHullShape(body, &def, hull);
                b3DestroyHull(hull);
            }
        }
    }
    if (B3_IS_NULL(id)) {
        owned.pop_back();
        delete user;
    }
}

} // namespace

struct PhysicObject {
    b3BodyId body = b3_nullBodyId;
    void* world = nullptr;
    std::vector<ShapeUserData*> shapeData;
};
struct JointObject { b3JointId joint = b3_nullJointId; };
struct RagdollObject { std::vector<PhysicObject*> parts; std::vector<b3JointId> joints; };
struct VehiclePhysicData { b3BodyId chassis = b3_nullBodyId; std::vector<b3JointId> wheelJoints; std::vector<b3BodyId> wheelBodies; };
void PhysicsModuleInit() {
    SceneManager::Get().RegisterCoreComponent<RigidbodyComponent>("RigidbodyComponent", "Physics");
    SceneManager::Get().RegisterCoreComponent<RagdollComponent>("RagdollComponent", "Physics");
    SceneManager::Get().RegisterCoreComponent<JointComponent>("JointComponent", "Physics");
    SceneManager::Get().RegisterCoreComponent<HeightmapColliderComponent>("HeightmapColliderComponent", "Physics");
    SceneManager::Get().RegisterCoreComponent<VehiclePhysic>("VehiclePhysic", "Physics");
    SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
}
void PhysicsModuleShutdown() {}

Ref<MeshShapeData> CreateMeshShapeData(Model& model) {
    std::vector<Vector3> vertices;
    std::vector<unsigned int> indices;
    for (const auto& mesh : model.meshs) if (mesh) {
        const auto base = static_cast<unsigned int>(vertices.size());
        vertices.insert(vertices.end(), mesh->vertices.begin(), mesh->vertices.end());
        for (auto index : mesh->indices) indices.push_back(base + index);
    }
    return CreateMeshShapeData(vertices, indices);
}
Ref<MeshShapeData> CreateMeshShapeData(const Mesh& mesh) { return CreateMeshShapeData(mesh.vertices, mesh.indices); }
Ref<MeshShapeData> CreateMeshShapeData(const std::vector<Vector3>& vertices, const std::vector<unsigned int>& indices) {
    auto data = CreateRef<MeshShapeData>();
    data->vertices = vertices;
    data->indices = indices;
    if (vertices.size() >= 3 && indices.size() >= 3 && indices.size() % 3 == 0) {
        std::vector<b3Vec3> b3Vertices;
        std::vector<int32_t> b3Indices;
        b3Vertices.reserve(vertices.size());
        b3Indices.reserve(indices.size());
        for (auto v : vertices) b3Vertices.push_back(ToB3(v));
        for (auto i : indices) {
            if (i >= vertices.size()) return data;
            b3Indices.push_back(static_cast<int32_t>(i));
        }
        b3MeshDef def{};
        def.vertices = b3Vertices.data();
        def.indices = b3Indices.data();
        def.stride = sizeof(b3Vec3);
        def.vertexCount = static_cast<int>(b3Vertices.size());
        def.triangleCount = static_cast<int>(b3Indices.size() / 3);
        def.weldTolerance = 0.001f;
        def.weldVertices = true;
        data->mesh = b3CreateMesh(&def, nullptr, 0);
    }
    return data;
}

PhysicsSystem::PhysicsSystem() { name = "PhysicsSystem"; }
PhysicsSystem::~PhysicsSystem() {
    if (physicsWorld) {
        auto* world = reinterpret_cast<Box3DWorld*>(physicsWorld);
        b3DestroyWorld(world->id);
        delete world;
        physicsWorld = nullptr;
    }
}

bool PhysicsSystem::IsSimulationEnable() { return true; }
Vector3 PhysicsSystem::GetGravity() {
    if (!physicsWorld) return Vector3Zero;
    auto* world = reinterpret_cast<Box3DWorld*>(physicsWorld);
    return FromB3(b3World_GetGravity(world->id));
}
void PhysicsSystem::SetGravity(Vector3 gravity) {
    if (physicsWorld) b3World_SetGravity(reinterpret_cast<Box3DWorld*>(physicsWorld)->id, ToB3(gravity));
}

void PhysicsSystem::OnInit(Scene& inScene) {
    scene = &inScene;
    auto* world = new Box3DWorld;
    b3WorldDef def = b3DefaultWorldDef();
    def.gravity = ToB3({0.0f, -9.81f, 0.0f});
    world->id = b3CreateWorld(&def);
    physicsWorld = reinterpret_cast<class PhysicsWorld*>(world);
    inScene.GetRegistry().on_destroy<RigidbodyComponent>().connect<&PhysicsSystem::OnRemoveRigidbody>();
    inScene.GetRegistry().on_destroy<JointComponent>().connect<&PhysicsSystem::OnRemoveJoint>();
    inScene.GetRegistry().on_destroy<RagdollComponent>().connect<&PhysicsSystem::OnRemoveRagdoll>();
    inScene.GetRegistry().on_destroy<VehiclePhysic>().connect<&PhysicsSystem::OnRemoveVehicle>();
    inScene.GetRegistry().on_destroy<HeightmapColliderComponent>().connect<&PhysicsSystem::OnRemoveHeightmap>();
    inScene.GetRegistry().ctx().emplace<PhysicsSystem*>(this);
}

void PhysicsSystem::OnEnd(Scene& inScene) {
    auto& registry = inScene.GetRegistry();
    registry.on_destroy<RigidbodyComponent>().disconnect<&PhysicsSystem::OnRemoveRigidbody>();
    registry.on_destroy<JointComponent>().disconnect<&PhysicsSystem::OnRemoveJoint>();
    registry.on_destroy<RagdollComponent>().disconnect<&PhysicsSystem::OnRemoveRagdoll>();
    registry.on_destroy<VehiclePhysic>().disconnect<&PhysicsSystem::OnRemoveVehicle>();
    registry.on_destroy<HeightmapColliderComponent>().disconnect<&PhysicsSystem::OnRemoveHeightmap>();
    auto vehicles = registry.view<VehiclePhysic>();
    for (auto [entity, value] : vehicles.each()) RemoveVehicle(entity, value);
    auto joints = registry.view<JointComponent>();
    for (auto [entity, value] : joints.each()) RemoveJoint(entity, value);
    auto ragdolls = registry.view<RagdollComponent>();
    for (auto [entity, value] : ragdolls.each()) RemoveRagdoll(entity, value);
    auto bodies = registry.view<RigidbodyComponent>();
    for (auto [entity, value] : bodies.each()) RemoveRigidbody(entity, value);
    auto heightmaps = registry.view<HeightmapColliderComponent>();
    for (auto [entity, value] : heightmaps.each()) OnRemoveHeightmap(registry, entity);
    auto* world = reinterpret_cast<Box3DWorld*>(physicsWorld);
    if (world) { b3DestroyWorld(world->id); delete world; physicsWorld = nullptr; }
    hasStarted = false;
}

void PhysicsSystem::OnStart(Scene& inScene) {
    auto view = inScene.GetRegistry().view<RigidbodyComponent>();
    for (auto [e, rb] : view.each()) {
        rb.data = nullptr;
        rb.isDirt = true;
    }
    auto joints = inScene.GetRegistry().view<JointComponent>();
    for (auto [e, joint] : joints.each()) { joint.data = nullptr; joint.isDirty = true; }
    auto ragdolls = inScene.GetRegistry().view<RagdollComponent>();
    for (auto [e, ragdoll] : ragdolls.each()) { ragdoll.data = nullptr; ragdoll.isDirty = true; }
    auto vehicles = inScene.GetRegistry().view<VehiclePhysic>();
    for (auto [e, vehicle] : vehicles.each()) vehicle.data = nullptr;
    auto heightmaps = inScene.GetRegistry().view<HeightmapColliderComponent>();
    for (auto [e, heightmap] : heightmaps.each()) heightmap.data = nullptr;
    hasStarted = true;
}

void PhysicsSystem::PrePhysicsUpdate(Scene& inScene) {
    auto& registry = inScene.GetRegistry();
    auto bodies = registry.view<RigidbodyComponent, TransformComponent, InfoComponent>();
    for (auto [entity, rb, transform, info] : bodies.each()) {
        if (!rb.isDirt) continue;
        RemoveRigidbody(entity, rb);
        AddRigidbody(entity, rb, transform, info);
    }
    auto ragdolls = registry.view<RagdollComponent, TransformComponent, InfoComponent, SkinnedModelRendererComponent>();
    for (auto [entity, ragdoll, transform, info, skinned] : ragdolls.each()) {
        if (!ragdoll.isDirty) continue;
        RemoveRagdoll(entity, ragdoll);
        AddRagdoll(entity, ragdoll, transform, info, skinned);
    }
    auto vehicles = registry.view<VehiclePhysic, RigidbodyComponent, TransformComponent, InfoComponent>();
    for (auto [entity, vehicle, rb, transform, info] : vehicles.each()) {
        if (!vehicle.data && rb.data) AddVehicle(entity, vehicle, rb, transform, info);
    }
    auto joints = registry.view<JointComponent, TransformComponent, InfoComponent>();
    for (auto [entity, joint, transform, info] : joints.each()) {
        if (!joint.isDirty) continue;
        RemoveJoint(entity, joint);
        AddJoint(&inScene, entity, joint, transform, info);
    }
    auto heightmaps = registry.view<HeightmapColliderComponent, TransformComponent, InfoComponent>();
    for (auto [entity, heightmap, transform, info] : heightmaps.each()) {
        if (heightmap.data || !physicsWorld || heightmap.width < 2 || heightmap.length < 2 || heightmap.heights.size() < size_t(heightmap.width * heightmap.length)) continue;
        b3HeightFieldDef fieldDef{};
        fieldDef.heights = heightmap.heights.data();
        fieldDef.countX = int(heightmap.width);
        fieldDef.countZ = int(heightmap.length);
        const Vector3 transformScale = transform.Scale();
        fieldDef.scale = {std::max(0.01f, transformScale.x * heightmap.scale),
                          std::max(0.01f, transformScale.y),
                          std::max(0.01f, transformScale.z * heightmap.scale)};
        fieldDef.globalMinimumHeight = heightmap.minHeight;
        fieldDef.globalMaximumHeight = heightmap.maxHeight;
        auto* data = new Box3DHeightmapData;
        data->field = b3CreateHeightField(&fieldDef);
        if (!data->field) { delete data; continue; }
        b3BodyDef bodyDef = b3DefaultBodyDef();
        bodyDef.type = b3_staticBody;
        bodyDef.position = ToB3Pos(transform.Position() + heightmap.offset);
        bodyDef.rotation = ToB3(transform.Rotation());
        bodyDef.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entity));
        data->body = b3CreateBody(reinterpret_cast<Box3DWorld*>(physicsWorld)->id, &bodyDef);
        b3ShapeDef shapeDef = b3DefaultShapeDef();
        shapeDef.filter = b3DefaultFilter();
        shapeDef.filter.categoryBits = uint64_t(1) << uint32_t(info.layer);
        shapeDef.filter.maskBits = AllLayersMask;
        shapeDef.baseMaterial.friction = 0.8f;
        shapeDef.userData = &data->user;
        data->user.entity = entity;
        b3CreateHeightFieldShape(data->body, &shapeDef, data->field);
        heightmap.data = data;
    }
}
void PhysicsSystem::FixedPhysicsUpdate(Scene& inScene) {
    if (skipUpdate || !physicsWorld) return;
    b3World_Step(reinterpret_cast<Box3DWorld*>(physicsWorld)->id, 1.0f / 60.0f, 4);
    auto events = b3World_GetBodyEvents(reinterpret_cast<Box3DWorld*>(physicsWorld)->id);
    for (int i = 0; i < events.moveCount; ++i) {
        auto& event = events.moveEvents[i];
        if (!event.userData) continue;
        Entity e = static_cast<Entity>(reinterpret_cast<uintptr_t>(event.userData));
        if (!inScene.IsValid(e)) continue;
        if (auto* transform = inScene.GetRegistry().try_get<TransformComponent>(e)) {
            transform->Position(FromB3Pos(event.transform.p));
            transform->Rotation(FromB3(event.transform.q));
        }
    }
    auto vehicleView = inScene.GetRegistry().view<VehiclePhysic>();
    for (auto [entity, vehicle] : vehicleView.each()) {
        auto* data = static_cast<VehiclePhysicData*>(vehicle.data);
        if (!data) continue;
        for (size_t i = 0; i < data->wheelJoints.size(); ++i) {
            const b3JointId joint = data->wheelJoints[i];
            if (B3_IS_NULL(joint)) continue;
            if (vehicle.wheels.empty()) break;
            const VehiclePhysic::Wheel& wheel = vehicle.wheels[std::min(i, vehicle.wheels.size() - 1)];
            b3WheelJoint_SetSpinMotorSpeed(joint, vehicle.forwardInput * 30.0f);
            b3WheelJoint_SetMaxSpinTorque(joint, vehicle.brakeInput > 0.0f ? vehicle.brakeInput * vehicle.maxEngineTorque : vehicle.maxEngineTorque);
            if (wheel.isFront) b3WheelJoint_SetTargetSteeringAngle(joint, vehicle.rightInput * vehicle.maxSteeringAngle * 0.01745329252f);
        }
    }
    CheckForCollisionEvents();
}
void PhysicsSystem::PostPhysicsUpdate(Scene& inScene) {
    auto ragdolls = inScene.GetRegistry().view<RagdollComponent, SkinnedModelRendererComponent>();
    for (auto [entity, ragdoll, skinned] : ragdolls.each()) {
        auto* data = static_cast<RagdollObject*>(ragdoll.data);
        if (!data || !skinned.GetModel()) continue;
        if (skinned.finalPose.Size() != skinned.GetModel()->skeleton.GetBindPose().Size())
            skinned.finalPose = skinned.GetModel()->skeleton.GetBindPose();
        Pose animatedPose = skinned.finalPose;
        auto* trans = inScene.GetRegistry().try_get<TransformComponent>(entity);
        if (!trans) continue;
        const float poseWeight = glm::clamp(ragdoll.poseWeight, 0.0f, 1.0f);
        for (size_t i = 0; i < ragdoll.parts.size() && i < data->parts.size(); ++i) {
            const int bone = ragdoll.parts[i].skinnedSkeletonIndex;
            if (bone < 0 || size_t(bone) >= skinned.finalPose.Size() || !data->parts[i]) continue;
            const Vector3 worldPos = FromB3Pos(b3Body_GetPosition(data->parts[i]->body));
            const Quaternion worldRot = FromB3(b3Body_GetRotation(data->parts[i]->body));
            const Vector3 physicalPosition = trans->InverseTransformPoint(worldPos);
            const Quaternion physicalRotation = glm::inverse(trans->Rotation()) * worldRot;
            const Transform animated = animatedPose.GetGlobalTransform(unsigned(bone));
            Transform poseTransform = animated;
            poseTransform.Position(glm::mix(animated.Position(), physicalPosition, poseWeight));
            poseTransform.Rotation(glm::normalize(glm::slerp(animated.Rotation(), physicalRotation, poseWeight)));
            skinned.finalPose.SetGlobalTransform(unsigned(bone), poseTransform);
            ragdoll.parts[i].previousPosition = worldPos;
            ragdoll.parts[i].previousRotation = worldRot;

            const int parent = ragdoll.parts[i].parent;
            if (parent >= 0 && size_t(parent) < ragdoll.parts.size() && size_t(i) < data->joints.size() &&
                !B3_IS_NULL(data->joints[i]) && ragdoll.motorType != RagdollComponent::MotorType::None) {
                const int parentBone = ragdoll.parts[size_t(parent)].skinnedSkeletonIndex;
                if (parentBone >= 0 && size_t(parentBone) < animatedPose.Size()) {
                    const Quaternion target = glm::inverse(animatedPose.GetGlobalTransform(unsigned(parentBone)).Rotation()) *
                                              animatedPose.GetGlobalTransform(unsigned(bone)).Rotation();
                    b3SphericalJoint_SetTargetRotation(data->joints[i], ToB3(target));
                    b3SphericalJoint_SetSpringHertz(data->joints[i], std::max(0.0f, ragdoll.stiffness));
                    b3SphericalJoint_SetSpringDampingRatio(data->joints[i], std::max(0.0f, ragdoll.damping));
                    if (ragdoll.useTorqueControl) b3SphericalJoint_SetMaxMotorTorque(data->joints[i], ragdoll.gain);
                }
            }
        }
        skinned.postUpdatePosePalette = true;
    }
}
void PhysicsSystem::OnDrawGizmos(Scene&, Camera&) {}
int PhysicsSystem::ExecutionSortPriority(SystemType) { return 0; }
float PhysicsSystem::InterpolationAlpha() { return 0.0f; }
void* PhysicsSystem::GetInternlWorld() { return physicsWorld; }
void PhysicsSystem::Simulate(float step) { if (physicsWorld) b3World_Step(reinterpret_cast<Box3DWorld*>(physicsWorld)->id, step, 4); }
void PhysicsSystem::SynchronizeMotionStates() {}
void PhysicsSystem::_PostPhysicsUpdate(bool, bool) {}
void PhysicsSystem::ShowDebugGizmos() {}
void PhysicsSystem::CheckForCollisionEvents() {
    if (!physicsWorld) return;
    auto* world = reinterpret_cast<Box3DWorld*>(physicsWorld);
    auto toCollision = [](b3ShapeId a, b3ShapeId b) {
        Collision c{};
        auto* da = static_cast<ShapeUserData*>(b3Shape_GetUserData(a));
        auto* db = static_cast<ShapeUserData*>(b3Shape_GetUserData(b));
        if (da) { c.e1 = da->entity; c.e1SubBodyIndex = da->subBodyIndex; c.body1IsSensor = da->trigger; }
        if (db) { c.e2 = db->entity; c.e2SubBodyIndex = db->subBodyIndex; c.body2IsSensor = db->trigger; }
        return c;
    };
    const auto contacts = b3World_GetContactEvents(world->id);
    for (int i = 0; i < contacts.beginCount; ++i) {
        Collision c = toCollision(contacts.beginEvents[i].shapeIdA, contacts.beginEvents[i].shapeIdB);
        if (c.e1 == EntityNull || c.e2 == EntityNull) continue;
        auto& callbacks = c.body1IsSensor || c.body2IsSensor ? onTriggerEnterCallbacks : onCollisionEnterCallbacks;
        for (auto callback : callbacks) callback(*scene, c);
    }
    for (int i = 0; i < contacts.endCount; ++i) {
        const auto& e = contacts.endEvents[i];
        if (!b3Shape_IsValid(e.shapeIdA) || !b3Shape_IsValid(e.shapeIdB)) continue;
        Collision c = toCollision(e.shapeIdA, e.shapeIdB);
        auto& callbacks = c.body1IsSensor || c.body2IsSensor ? onTriggerExitCallbacks : onCollisionExitCallbacks;
        for (auto callback : callbacks) callback(*scene, c);
    }
    const auto sensors = b3World_GetSensorEvents(world->id);
    for (int i = 0; i < sensors.beginCount; ++i) {
        Collision c = toCollision(sensors.beginEvents[i].sensorShapeId, sensors.beginEvents[i].visitorShapeId);
        c.body1IsSensor = true;
        for (auto callback : onTriggerEnterCallbacks) callback(*scene, c);
    }
    for (int i = 0; i < sensors.endCount; ++i) {
        const auto& e = sensors.endEvents[i];
        if (!b3Shape_IsValid(e.sensorShapeId) || !b3Shape_IsValid(e.visitorShapeId)) continue;
        Collision c = toCollision(e.sensorShapeId, e.visitorShapeId);
        c.body1IsSensor = true;
        for (auto callback : onTriggerExitCallbacks) callback(*scene, c);
    }
}

bool PhysicsSystem::Raycast(Vector3 pos, Vector3 dir, RayResult& hit) { return Raycast(pos, dir, hit, LayerMask{AllLayersMask}, false); }
bool PhysicsSystem::Raycast(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask, bool ignoreSensor) {
    if (!physicsWorld) return false;
    const auto id = reinterpret_cast<Box3DWorld*>(physicsWorld)->id;
    b3QueryFilter filter = b3DefaultQueryFilter();
    filter.maskBits = mask.mask;
    RayCastContext context{};
    context.ignoreSensor = ignoreSensor;
    b3World_CastRay(id, ToB3Pos(pos), ToB3(dir * 10000.0f), filter, CollectRayHit, &context);
    if (!context.hit) return false;
    auto* user = static_cast<ShapeUserData*>(b3Shape_GetUserData(context.shape));
    if (!user) return false;
    hit.entity = user->entity;
    hit.subBodyIndex = user->subBodyIndex;
    hit.hitPoint = FromB3Pos(context.point);
    hit.hitNormal = FromB3(context.normal);
    return true;
}
bool PhysicsSystem::RaycastIgnoreSensor(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask) { return Raycast(pos, dir, hit, mask, true); }
bool PhysicsSystem::RaycastIgnoreEntity(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask, Entity ignoreEntity, bool ignoreSensor) {
    if (!physicsWorld) return false;
    b3QueryFilter filter = b3DefaultQueryFilter();
    filter.maskBits = mask.mask;
    RayCastContext context{};
    context.ignoredEntity = ignoreEntity;
    context.ignoreSensor = ignoreSensor;
    b3World_CastRay(reinterpret_cast<Box3DWorld*>(physicsWorld)->id, ToB3Pos(pos), ToB3(dir * 10000.0f), filter, CollectRayHit, &context);
    if (!context.hit) return false;
    auto* user = static_cast<ShapeUserData*>(b3Shape_GetUserData(context.shape));
    if (!user) return false;
    hit.entity = user->entity;
    hit.subBodyIndex = user->subBodyIndex;
    hit.hitPoint = FromB3Pos(context.point);
    hit.hitNormal = FromB3(context.normal);
    return true;
}

namespace {
struct OverlapContext { std::vector<RayResult>* results; };
bool CollectOverlap(b3ShapeId shape, void* context) {
    auto* data = static_cast<ShapeUserData*>(b3Shape_GetUserData(shape));
    if (!data) return true;
    auto* output = static_cast<OverlapContext*>(context)->results;
    RayResult result{};
    result.entity = data->entity;
    result.subBodyIndex = data->subBodyIndex;
    output->push_back(result);
    return true;
}
}
std::vector<RayResult> PhysicsSystem::OverlapSphere(Vector3 center, float radius) { return OverlapSphere(center, radius, LayerMask{AllLayersMask}); }
std::vector<RayResult> PhysicsSystem::OverlapSphere(Vector3 center, float radius, LayerMask) {
    std::vector<RayResult> results;
    if (!physicsWorld) return results;
    b3AABB box{{center.x - radius, center.y - radius, center.z - radius}, {center.x + radius, center.y + radius, center.z + radius}};
    OverlapContext context{&results};
    b3World_OverlapAABB(reinterpret_cast<Box3DWorld*>(physicsWorld)->id, box, b3DefaultQueryFilter(), CollectOverlap, &context);
    return results;
}
std::vector<RayResult> PhysicsSystem::OverlapBox(Vector3 size, Vector3 center, Quaternion, float scale, LayerMask, bool) {
    std::vector<RayResult> results;
    if (!physicsWorld) return results;
    const Vector3 half = size * (0.5f * scale);
    b3AABB box{{center.x - half.x, center.y - half.y, center.z - half.z}, {center.x + half.x, center.y + half.y, center.z + half.z}};
    OverlapContext context{&results};
    b3World_OverlapAABB(reinterpret_cast<Box3DWorld*>(physicsWorld)->id, box, b3DefaultQueryFilter(), CollectOverlap, &context);
    return results;
}

void PhysicsSystem::AddOnCollisionEnterCallback(OnCollisionCallback callback) { onCollisionEnterCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnCollisionEnterCallback(OnCollisionCallback callback) { auto& v = onCollisionEnterCallbacks; v.erase(std::remove(v.begin(), v.end(), callback), v.end()); }
void PhysicsSystem::AddOnCollisionExitCallback(OnCollisionCallback callback) { onCollisionExitCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnCollisionExitCallback(OnCollisionCallback callback) { auto& v = onCollisionExitCallbacks; v.erase(std::remove(v.begin(), v.end(), callback), v.end()); }
void PhysicsSystem::AddOnTriggerEnterCallback(OnCollisionCallback callback) { onTriggerEnterCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnTriggerEnterCallback(OnCollisionCallback callback) { auto& v = onTriggerEnterCallbacks; v.erase(std::remove(v.begin(), v.end(), callback), v.end()); }
void PhysicsSystem::AddOnTriggerExitCallback(OnCollisionCallback callback) { onTriggerExitCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnTriggerExitCallback(OnCollisionCallback callback) { auto& v = onTriggerExitCallbacks; v.erase(std::remove(v.begin(), v.end(), callback), v.end()); }

void PhysicsSystem::AddRigidbody(Entity entity, RigidbodyComponent& rb, TransformComponent& transform, InfoComponent& info) {
    RemoveRigidbody(entity, rb);
    if (!physicsWorld || rb.type == RigidbodyComponent::Type::Disable) return;
    auto* world = reinterpret_cast<Box3DWorld*>(physicsWorld);
    b3BodyDef def = b3DefaultBodyDef();
    def.type = rb.type == RigidbodyComponent::Type::Static ? b3_staticBody :
               rb.type == RigidbodyComponent::Type::Kinematic || rb.type == RigidbodyComponent::Type::Trigger ? b3_kinematicBody : b3_dynamicBody;
    def.position = ToB3Pos(transform.Position());
    def.rotation = ToB3(transform.Rotation());
    def.linearDamping = rb.linearDamping;
    def.angularDamping = rb.angularDamping;
    def.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entity));
    def.enableSleep = !rb.neverSleep;
    def.motionLocks = {!(uint8_t(rb.constraints) & 0x01), !(uint8_t(rb.constraints) & 0x02), !(uint8_t(rb.constraints) & 0x04),
                       !(uint8_t(rb.constraints) & 0x08), !(uint8_t(rb.constraints) & 0x10), !(uint8_t(rb.constraints) & 0x20)};
    auto* data = new PhysicObject;
    data->world = world;
    data->body = b3CreateBody(world->id, &def);
    if (B3_IS_NULL(data->body)) { delete data; return; }
        const Vector3 shapeScale = rb.useTransformScale ? transform.Scale() : Vector3One;
        MakeB3Shape(data->body, rb.shape, rb, entity, -1, data->shapeData, uint32_t(info.layer), rb.mask.mask, shapeScale);
    for (int i = 0; i < int(rb.extraShapes.size()); ++i)
        MakeB3Shape(data->body, rb.extraShapes[i], rb, entity, i, data->shapeData, uint32_t(info.layer), rb.mask.mask, shapeScale);
    b3Body_ApplyMassFromShapes(data->body);
    auto massData = b3Body_GetMassData(data->body);
    if (massData.mass > 0.0f) {
        const float scale = rb.mass / massData.mass;
        massData.mass = rb.mass;
        massData.inertia.cx = {massData.inertia.cx.x * scale, massData.inertia.cx.y * scale, massData.inertia.cx.z * scale};
        massData.inertia.cy = {massData.inertia.cy.x * scale, massData.inertia.cy.y * scale, massData.inertia.cy.z * scale};
        massData.inertia.cz = {massData.inertia.cz.x * scale, massData.inertia.cz.y * scale, massData.inertia.cz.z * scale};
        b3Body_SetMassData(data->body, massData);
    }
    rb.data = data;
    rb.isDirt = false;
}

void PhysicsSystem::RemoveRigidbody(Entity, RigidbodyComponent& rb) {
    auto* data = static_cast<PhysicObject*>(rb.data);
    if (!data) return;
    if (b3Body_IsValid(data->body)) b3DestroyBody(data->body);
    for (auto* user : data->shapeData) delete user;
    delete data;
    rb.data = nullptr;
    rb.isDirt = true;
}

void RigidbodyComponent::OnGui(Entity&, Scene&) {}
void RigidbodyComponent::SetShape(CollisionShape value) { shape = std::move(value); SetDirt(); }
void RigidbodyComponent::PushExtraShape(CollisionShape value) { extraShapes.push_back(std::move(value)); SetDirt(); }
void RigidbodyComponent::PopExtraShape() { if (!extraShapes.empty()) extraShapes.pop_back(); SetDirt(); }
void RigidbodyComponent::Mass(float value) { mass = std::max(0.001f, value); SetDirt(); }
void RigidbodyComponent::Friction(float value) { friction = std::max(0.0f, value); SetDirt(); }
void RigidbodyComponent::SetType(Type value) { type = value; SetDirt(); }
void RigidbodyComponent::NeverSleep(bool value) { neverSleep = value; SetDirt(); }
Vector3 RigidbodyComponent::CenterOfMass() {
    return data && b3Body_IsValid(data->body) ? FromB3Pos(b3Body_GetWorldCenter(data->body)) : Vector3Zero;
}
Vector3 RigidbodyComponent::Position() { return data && b3Body_IsValid(data->body) ? FromB3Pos(b3Body_GetPosition(data->body)) : Vector3Zero; }
Vector3 RigidbodyComponent::PositionInterpoled() { return Position(); }
void RigidbodyComponent::Position(Vector3 value) { if (data && b3Body_IsValid(data->body)) b3Body_SetTransform(data->body, ToB3Pos(value), b3Body_GetRotation(data->body)); }
Quaternion RigidbodyComponent::Rotation() { return data && b3Body_IsValid(data->body) ? FromB3(b3Body_GetRotation(data->body)) : QuaternionIdentity; }
void RigidbodyComponent::Rotation(Quaternion value) { if (data && b3Body_IsValid(data->body)) b3Body_SetTransform(data->body, b3Body_GetPosition(data->body), ToB3(value)); }
void RigidbodyComponent::SetTransform(const Vector3& position, const Quaternion& rotation) { if (data && b3Body_IsValid(data->body)) b3Body_SetTransform(data->body, ToB3Pos(position), ToB3(rotation)); }
Vector3 RigidbodyComponent::Velocity() { return data && b3Body_IsValid(data->body) ? FromB3(b3Body_GetLinearVelocity(data->body)) : Vector3Zero; }
void RigidbodyComponent::Velocity(Vector3 value) { if (data && b3Body_IsValid(data->body)) b3Body_SetLinearVelocity(data->body, ToB3(value)); }
Vector3 RigidbodyComponent::AngularVelocity() { return data && b3Body_IsValid(data->body) ? FromB3(b3Body_GetAngularVelocity(data->body)) : Vector3Zero; }
void RigidbodyComponent::AngularVelocity(Vector3 value) { if (data && b3Body_IsValid(data->body)) b3Body_SetAngularVelocity(data->body, ToB3(value)); }
void RigidbodyComponent::ApplyForce(Vector3 value) { if (data && b3Body_IsValid(data->body)) b3Body_ApplyForceToCenter(data->body, ToB3(value), true); }
void RigidbodyComponent::ApplyTorque(Vector3 value) { if (data && b3Body_IsValid(data->body)) b3Body_ApplyTorque(data->body, ToB3(value), true); }
void RigidbodyComponent::ApplyImpulse(Vector3 value) { if (data && b3Body_IsValid(data->body)) b3Body_ApplyLinearImpulseToCenter(data->body, ToB3(value), true); }
void RigidbodyComponent::AddLinearVelocity(Vector3 value) { Velocity(Velocity() + value); }
void RigidbodyComponent::AddExplosionImpulse(float force, Vector3 center, float radius, float upwardsModifier) {
    if (!data || !b3Body_IsValid(data->body) || radius <= 0.0f) return;
    Vector3 delta = Position() - center;
    float distance = glm::length(delta);
    if (distance >= radius) return;
    Vector3 direction = distance > 0.0001f ? delta / distance : Vector3Up;
    direction.y += upwardsModifier;
    const float falloff = 1.0f - distance / radius;
    ApplyImpulse(glm::normalize(direction) * force * falloff);
}
void RigidbodyComponent::SetAngularFactor(Vector3 value) {
    if (!data || !b3Body_IsValid(data->body)) { angularFactor = value; return; }
    angularFactor = value;
    auto locks = b3Body_GetMotionLocks(data->body);
    locks.angularX = value.x == 0.0f; locks.angularY = value.y == 0.0f; locks.angularZ = value.z == 0.0f;
    b3Body_SetMotionLocks(data->body, locks);
}
float RigidbodyComponent::LinearDamping() { return data && b3Body_IsValid(data->body) ? b3Body_GetLinearDamping(data->body) : linearDamping; }
void RigidbodyComponent::LinearDamping(float value) { linearDamping = value; if (data && b3Body_IsValid(data->body)) b3Body_SetLinearDamping(data->body, value); }
float RigidbodyComponent::AngularDamping() { return data && b3Body_IsValid(data->body) ? b3Body_GetAngularDamping(data->body) : angularDamping; }
void RigidbodyComponent::AngularDamping(float value) { angularDamping = value; if (data && b3Body_IsValid(data->body)) b3Body_SetAngularDamping(data->body, value); }
PhysicMotionQuality RigidbodyComponent::MotionQuality() { return motionQuality; }
void RigidbodyComponent::MotionQuality(PhysicMotionQuality value) { motionQuality = value; if (data && b3Body_IsValid(data->body)) b3Body_SetBullet(data->body, value == PhysicMotionQuality::LinearCast); }
RigidbodyConstraints RigidbodyComponent::Constraints() { return constraints; }
void RigidbodyComponent::Constraints(RigidbodyConstraints value) {
    constraints = value;
    if (!data || !b3Body_IsValid(data->body)) return;
    const uint8_t bits = uint8_t(value);
    b3Body_SetMotionLocks(data->body, {!(bits & 1), !(bits & 2), !(bits & 4), !(bits & 8), !(bits & 16), !(bits & 32)});
}

void RagdollComponent::OnGui(Entity&, Scene&) {}
float RagdollComponent::Mass(int boneIndex) {
    if (!data || boneIndex < 0 || size_t(boneIndex) >= data->parts.size() || !data->parts[boneIndex]) return 0.0f;
    return b3Body_GetMass(data->parts[boneIndex]->body);
}
Vector3 RagdollComponent::CenterOfMass(int boneIndex) {
    if (!data || boneIndex < 0 || size_t(boneIndex) >= data->parts.size() || !data->parts[boneIndex]) return Vector3Zero;
    return FromB3Pos(b3Body_GetWorldCenter(data->parts[boneIndex]->body));
}
Vector3 RagdollComponent::Position(int boneIndex) {
    if (!data || boneIndex < 0 || size_t(boneIndex) >= data->parts.size() || !data->parts[boneIndex]) return Vector3Zero;
    return FromB3Pos(b3Body_GetPosition(data->parts[boneIndex]->body));
}
Vector3 RagdollComponent::PositionInterpoled(int boneIndex) { return Position(boneIndex); }
void RagdollComponent::Position(int boneIndex, Vector3 value) {
    if (data && boneIndex >= 0 && size_t(boneIndex) < data->parts.size() && data->parts[boneIndex])
        b3Body_SetTransform(data->parts[boneIndex]->body, ToB3Pos(value), b3Body_GetRotation(data->parts[boneIndex]->body));
}
Quaternion RagdollComponent::Rotation(int boneIndex) {
    if (!data || boneIndex < 0 || size_t(boneIndex) >= data->parts.size() || !data->parts[boneIndex]) return QuaternionIdentity;
    return FromB3(b3Body_GetRotation(data->parts[boneIndex]->body));
}
void RagdollComponent::Rotation(int boneIndex, Quaternion value) {
    if (data && boneIndex >= 0 && size_t(boneIndex) < data->parts.size() && data->parts[boneIndex])
        b3Body_SetTransform(data->parts[boneIndex]->body, b3Body_GetPosition(data->parts[boneIndex]->body), ToB3(value));
}
Vector3 RagdollComponent::Velocity(int boneIndex) {
    if (!data || boneIndex < 0 || size_t(boneIndex) >= data->parts.size() || !data->parts[boneIndex]) return Vector3Zero;
    return FromB3(b3Body_GetLinearVelocity(data->parts[boneIndex]->body));
}
void RagdollComponent::Velocity(int boneIndex, Vector3 value) {
    if (data && boneIndex >= 0 && size_t(boneIndex) < data->parts.size() && data->parts[boneIndex])
        b3Body_SetLinearVelocity(data->parts[boneIndex]->body, ToB3(value));
}
Vector3 RagdollComponent::AngularVelocity(int boneIndex) {
    if (!data || boneIndex < 0 || size_t(boneIndex) >= data->parts.size() || !data->parts[boneIndex]) return Vector3Zero;
    return FromB3(b3Body_GetAngularVelocity(data->parts[boneIndex]->body));
}
void RagdollComponent::AngularVelocity(int boneIndex, Vector3 value) {
    if (data && boneIndex >= 0 && size_t(boneIndex) < data->parts.size() && data->parts[boneIndex])
        b3Body_SetAngularVelocity(data->parts[boneIndex]->body, ToB3(value));
}
void RagdollComponent::ApplyForce(int boneIndex, Vector3 value) {
    if (data && boneIndex >= 0 && size_t(boneIndex) < data->parts.size() && data->parts[boneIndex])
        b3Body_ApplyForceToCenter(data->parts[boneIndex]->body, ToB3(value), true);
}
void RagdollComponent::ApplyTorque(int boneIndex, Vector3 value) {
    if (data && boneIndex >= 0 && size_t(boneIndex) < data->parts.size() && data->parts[boneIndex])
        b3Body_ApplyTorque(data->parts[boneIndex]->body, ToB3(value), true);
}
void RagdollComponent::ApplyImpulse(int boneIndex, Vector3 value) {
    if (data && boneIndex >= 0 && size_t(boneIndex) < data->parts.size() && data->parts[boneIndex])
        b3Body_ApplyLinearImpulseToCenter(data->parts[boneIndex]->body, ToB3(value), true);
}
RigidbodyConstraints RagdollComponent::Constraints(int boneIndex) {
    if (!data || boneIndex < 0 || size_t(boneIndex) >= data->parts.size() || !data->parts[boneIndex]) return RigidbodyConstraints::All;
    auto locks = b3Body_GetMotionLocks(data->parts[boneIndex]->body);
    uint8_t bits = uint8_t((!locks.linearX ? 1 : 0) | (!locks.linearY ? 2 : 0) | (!locks.linearZ ? 4 : 0) |
                           (!locks.angularX ? 8 : 0) | (!locks.angularY ? 16 : 0) | (!locks.angularZ ? 32 : 0));
    return static_cast<RigidbodyConstraints>(bits);
}
void RagdollComponent::Constraints(int boneIndex, RigidbodyConstraints value) {
    if (!data || boneIndex < 0 || size_t(boneIndex) >= data->parts.size() || !data->parts[boneIndex]) return;
    const uint8_t bits = uint8_t(value);
    b3Body_SetMotionLocks(data->parts[boneIndex]->body, {!(bits & 1), !(bits & 2), !(bits & 4), !(bits & 8), !(bits & 16), !(bits & 32)});
}
RagdollComponent::Type RagdollComponent::GetType() { return type; }
void RagdollComponent::SetType(Type value) { type = value; isDirty = true; }
void RagdollComponent::UpdateInternalData(TransformComponent&, InfoComponent&, SkinnedModelRendererComponent&) { isDirty = true; }
void RagdollComponent::AddExplosionImpulse(float force, Vector3 center, float radius, float upwardsModifier) {
    if (!data || radius <= 0.0f) return;
    for (size_t i = 0; i < data->parts.size(); ++i) {
        const Vector3 delta = Position(int(i)) - center;
        const float distance = glm::length(delta);
        if (distance >= radius) continue;
        Vector3 direction = distance > 0.0001f ? delta / distance : Vector3Up;
        direction.y += upwardsModifier;
        ApplyImpulse(int(i), glm::normalize(direction) * force * (1.0f - distance / radius));
    }
}

void JointComponent::OnGui(Entity&, Scene&) {}
void JointComponent::SetTargets(Entity a, int aIndex, Entity b, int bIndex) { bodyA = a; bodyASubIndex = aIndex; bodyB = b; bodyBSubIndex = bIndex; isDirty = true; }
JointSpace JointComponent::GetJointSpace() { return jointSpace; }
void JointComponent::SetJointSpace(JointSpace value) { jointSpace = value; isDirty = true; }
void JointComponent::CreateFixed(const FixedSettings& value) { fixedSettings = value; type = Type::Fixed; isDirty = true; }
void JointComponent::CreateDistance(const DistanceSettings& value) { distanceSettings = value; type = Type::Distance; isDirty = true; }
void JointComponent::CreateSixDOF(const SixDOFSettings& value) { sixDOFSettings = value; type = Type::SixDOF; isDirty = true; }
void JointComponent::SetDistance(float minimum, float maximum) { distanceSettings.minDistance = minimum; distanceSettings.maxDistance = maximum; isDirty = true; }
Vector3 JointComponent::GetWorldSpacePoint1Pos() {
    if (!scene || bodyA == EntityNull || !scene->GetRegistry().all_of<RigidbodyComponent>(bodyA)) return Vector3Zero;
    auto& rb = scene->GetRegistry().get<RigidbodyComponent>(bodyA);
    auto* body = const_cast<PhysicObject*>(static_cast<const PhysicObject*>(rb.InternalData()));
    if (!body) return Vector3Zero;
    Vector3 local = type == Type::Distance ? distanceSettings.point1 : type == Type::SixDOF ? sixDOFSettings.point1 : fixedSettings.point1;
    return FromB3Pos(b3Body_GetWorldPoint(body->body, ToB3(local)));
}
Vector3 JointComponent::GetWorldSpacePoint2Pos() {
    if (!scene || bodyB == EntityNull || !scene->GetRegistry().all_of<RigidbodyComponent>(bodyB)) return Vector3Zero;
    auto& rb = scene->GetRegistry().get<RigidbodyComponent>(bodyB);
    auto* body = const_cast<PhysicObject*>(static_cast<const PhysicObject*>(rb.InternalData()));
    if (!body) return Vector3Zero;
    Vector3 local = type == Type::Distance ? distanceSettings.point2 : type == Type::SixDOF ? sixDOFSettings.point2 : fixedSettings.point2;
    return FromB3Pos(b3Body_GetWorldPoint(body->body, ToB3(local)));
}

Transform VehiclePhysic::GetWheelWorldTransform(int wheelIndex, Vector3, Vector3) {
    Transform out;
    auto* data = static_cast<VehiclePhysicData*>(this->data);
    if (!data || wheelIndex < 0 || size_t(wheelIndex) >= data->wheelBodies.size()) return out;
    const b3WorldTransform transform = b3Body_GetTransform(data->wheelBodies[size_t(wheelIndex)]);
    out.Position(FromB3Pos(transform.p));
    out.Rotation(FromB3(transform.q));
    return out;
}
Transform VehiclePhysic::GetWheelLocalTransform(int wheelIndex, Vector3, Vector3) {
    Transform out;
    if (wheelIndex >= 0 && size_t(wheelIndex) < wheels.size()) out.Position(wheels[size_t(wheelIndex)].pos);
    return out;
}

void PhysicsSystem::UpdateRigidbodyInternalData(Entity) {}
void PhysicsSystem::SetJointsAsDirtyIfBodyIsDirty(Entity) {}
void PhysicsSystem::AddJoint(Scene* ownerScene, Entity entity, JointComponent& joint, TransformComponent&, InfoComponent&) {
    RemoveJoint(entity, joint);
    joint.scene = ownerScene;
    if (!physicsWorld || !ownerScene || joint.bodyA == EntityNull || joint.bodyB == EntityNull) return;
    auto& registry = ownerScene->GetRegistry();
    auto* rbA = registry.try_get<RigidbodyComponent>(joint.bodyA);
    auto* rbB = registry.try_get<RigidbodyComponent>(joint.bodyB);
    if (!rbA || !rbB || !rbA->data || !rbB->data) return;
    auto* bodyA = static_cast<PhysicObject*>(rbA->data);
    auto* bodyB = static_cast<PhysicObject*>(rbB->data);
    auto* world = reinterpret_cast<Box3DWorld*>(physicsWorld);

    b3Vec3 pointA = ToB3(joint.fixedSettings.point1);
    b3Vec3 pointB = ToB3(joint.fixedSettings.point2);
    if (joint.type == JointComponent::Type::Distance) {
        pointA = ToB3(joint.distanceSettings.point1);
        pointB = ToB3(joint.distanceSettings.point2);
    } else if (joint.type == JointComponent::Type::SixDOF) {
        pointA = ToB3(joint.sixDOFSettings.point1);
        pointB = ToB3(joint.sixDOFSettings.point2);
    }
    if (joint.jointSpace == JointSpace::WorldSpace) {
        pointA = b3Body_GetLocalPoint(bodyA->body, ToB3Pos({pointA.x, pointA.y, pointA.z}));
        pointB = b3Body_GetLocalPoint(bodyB->body, ToB3Pos({pointB.x, pointB.y, pointB.z}));
    }

    b3JointId id = b3_nullJointId;
    if (joint.type == JointComponent::Type::Distance) {
        auto def = b3DefaultDistanceJointDef();
        def.base.bodyIdA = bodyA->body; def.base.bodyIdB = bodyB->body;
        def.base.localFrameA.p = pointA; def.base.localFrameB.p = pointB;
        def.enableSpring = joint.distanceSettings.springFequency > 0.0f;
        def.hertz = joint.distanceSettings.springFequency;
        def.dampingRatio = joint.distanceSettings.springDamping;
        def.enableLimit = joint.distanceSettings.minDistance >= 0.0f || joint.distanceSettings.maxDistance >= 0.0f;
        def.minLength = std::max(0.0f, joint.distanceSettings.minDistance);
        def.maxLength = joint.distanceSettings.maxDistance >= 0.0f ? joint.distanceSettings.maxDistance : def.minLength;
        id = b3CreateDistanceJoint(world->id, &def);
    } else if (joint.type == JointComponent::Type::SixDOF) {
        const auto& s = joint.sixDOFSettings;
        const bool oneLinearAxis = (s.linearLower.x != s.linearUpper.x) + (s.linearLower.y != s.linearUpper.y) + (s.linearLower.z != s.linearUpper.z) == 1;
        const bool oneAngularAxis = (s.angularLower.x != s.angularUpper.x) + (s.angularLower.y != s.angularUpper.y) + (s.angularLower.z != s.angularUpper.z) == 1;
        if (oneLinearAxis) {
            int axis = s.linearLower.x != s.linearUpper.x ? 0 : s.linearLower.y != s.linearUpper.y ? 1 : 2;
            auto def = b3DefaultPrismaticJointDef();
            def.base.bodyIdA = bodyA->body; def.base.bodyIdB = bodyB->body;
            def.base.localFrameA.p = pointA; def.base.localFrameB.p = pointB;
            def.enableLimit = true;
            def.lowerTranslation = (&s.linearLower.x)[axis]; def.upperTranslation = (&s.linearUpper.x)[axis];
            id = b3CreatePrismaticJoint(world->id, &def);
        } else if (oneAngularAxis) {
            auto def = b3DefaultRevoluteJointDef();
            def.base.bodyIdA = bodyA->body; def.base.bodyIdB = bodyB->body;
            def.base.localFrameA.p = pointA; def.base.localFrameB.p = pointB;
            int axis = s.angularLower.x != s.angularUpper.x ? 0 : s.angularLower.y != s.angularUpper.y ? 1 : 2;
            auto radians = [](float v) { return v * 0.017453292519943295f; };
            def.enableLimit = true;
            def.lowerAngle = radians((&s.angularLower.x)[axis]); def.upperAngle = radians((&s.angularUpper.x)[axis]);
            id = b3CreateRevoluteJoint(world->id, &def);
        } else {
            auto def = b3DefaultWeldJointDef();
            def.base.bodyIdA = bodyA->body; def.base.bodyIdB = bodyB->body;
            def.base.localFrameA.p = pointA; def.base.localFrameB.p = pointB;
            id = b3CreateWeldJoint(world->id, &def);
        }
    } else {
        auto def = b3DefaultWeldJointDef();
        def.base.bodyIdA = bodyA->body; def.base.bodyIdB = bodyB->body;
        def.base.localFrameA.p = pointA; def.base.localFrameB.p = pointB;
        id = b3CreateWeldJoint(world->id, &def);
    }
    if (!B3_IS_NULL(id)) {
        auto* data = new JointObject;
        data->joint = id;
        joint.data = data;
        joint.isDirty = false;
    }
}
void PhysicsSystem::RemoveJoint(Entity, JointComponent& joint) { if (joint.data) { auto* d = static_cast<JointObject*>(joint.data); if (!B3_IS_NULL(d->joint)) b3DestroyJoint(d->joint, true); delete d; joint.data = nullptr; } }
void PhysicsSystem::AddRagdoll(Entity entity, RagdollComponent& ragdoll, TransformComponent& trans, InfoComponent&, SkinnedModelRendererComponent& skinned) {
    RemoveRagdoll(entity, ragdoll);
    ragdoll.isDirty = false;
    ragdoll.physicSystem = this;
    ragdoll.scene = scene;
    ragdoll.entity = entity;
    if (!physicsWorld || ragdoll.type == RagdollComponent::Type::Disable || ragdoll.parts.empty()) return;
    auto* world = reinterpret_cast<Box3DWorld*>(physicsWorld);
    auto* data = new RagdollObject;
    data->parts.resize(ragdoll.parts.size(), nullptr);
    data->joints.resize(ragdoll.parts.size(), b3_nullJointId);
    for (size_t i = 0; i < ragdoll.parts.size(); ++i) {
        auto& part = ragdoll.parts[i];
        Vector3 position = trans.Position() + part.constraintPos;
        Quaternion rotation = trans.Rotation();
        if (part.skinnedSkeletonIndex >= 0 && size_t(part.skinnedSkeletonIndex) < skinned.finalPose.Size()) {
            const Transform bone = skinned.finalPose.GetGlobalTransform(unsigned(part.skinnedSkeletonIndex));
            position = trans.TransformPoint(bone.Position());
            rotation = trans.Rotation() * bone.Rotation();
        }
        b3BodyDef def = b3DefaultBodyDef();
        def.type = part.overrideType == RagdollComponent::Part::OverrideType::Static || ragdoll.type == RagdollComponent::Type::Static ? b3_staticBody :
                   part.overrideType == RagdollComponent::Part::OverrideType::Kinematic || ragdoll.type == RagdollComponent::Type::Kinematic || ragdoll.type == RagdollComponent::Type::Trigger ? b3_kinematicBody : b3_dynamicBody;
        def.position = ToB3Pos(position);
        def.rotation = ToB3(rotation);
        def.linearDamping = part.overrideLinearDamping >= 0.0f ? part.overrideLinearDamping : ragdoll.linearDamping;
        def.userData = nullptr;
        auto* rbData = new PhysicObject;
        rbData->world = world;
        rbData->body = b3CreateBody(world->id, &def);
        if (B3_IS_NULL(rbData->body)) { delete rbData; continue; }
        RigidbodyComponent shapeSettings;
        MakeB3Shape(rbData->body, part.shape, shapeSettings, entity, int(i), rbData->shapeData,
                    uint32_t(ragdoll.layer), ragdoll.mask.mask,
                    trans.Scale(),
                    ragdoll.type == RagdollComponent::Type::Trigger);
        b3Body_ApplyMassFromShapes(rbData->body);
        const float currentMass = b3Body_GetMass(rbData->body);
        const float desiredMass = ragdoll.globalMass / float(ragdoll.parts.size());
        if (currentMass > 0.0f && desiredMass > 0.0f) {
            auto massData = b3Body_GetMassData(rbData->body);
            const float scale = desiredMass / currentMass;
            massData.mass = desiredMass;
            massData.inertia.cx = ToB3(FromB3(massData.inertia.cx) * scale);
            massData.inertia.cy = ToB3(FromB3(massData.inertia.cy) * scale);
            massData.inertia.cz = ToB3(FromB3(massData.inertia.cz) * scale);
            b3Body_SetMassData(rbData->body, massData);
        }
        if (ragdoll.overrideStartVelocity != Vector3Zero)
            b3Body_SetLinearVelocity(rbData->body, ToB3(ragdoll.overrideStartVelocity));
        data->parts[i] = rbData;
    }
    for (size_t i = 0; i < ragdoll.parts.size(); ++i) {
        const int parent = ragdoll.parts[i].parent;
        if (parent < 0 || size_t(parent) >= data->parts.size() || !data->parts[i] || !data->parts[parent]) continue;
        const b3BodyId bodyA = data->parts[parent]->body;
        const b3BodyId bodyB = data->parts[i]->body;
        if (!b3Body_IsValid(bodyA) || !b3Body_IsValid(bodyB)) continue;
        // Box3D rejects constraints between two static bodies. Static ragdoll
        // parts already cannot move, so no joint is needed between them.
        if (b3Body_GetType(bodyA) == b3_staticBody && b3Body_GetType(bodyB) == b3_staticBody) continue;
        auto def = b3DefaultSphericalJointDef();
        def.base.bodyIdA = bodyA;
        def.base.bodyIdB = bodyB;
        def.base.localFrameA.p = ToB3(ragdoll.parts[i].constraintPos);
        def.base.localFrameB.p = {0,0,0};
        def.enableSpring = ragdoll.motorType != RagdollComponent::MotorType::None;
        def.hertz = ragdoll.stiffness;
        def.dampingRatio = ragdoll.damping;
        def.enableMotor = ragdoll.motorType != RagdollComponent::MotorType::None;
        def.maxMotorTorque = ragdoll.gain;
        if (ragdoll.parts[i].twistAngleMin != ragdoll.parts[i].twistAngleMax) {
            def.enableTwistLimit = true;
            def.lowerTwistAngle = ragdoll.parts[i].twistAngleMin;
            def.upperTwistAngle = ragdoll.parts[i].twistAngleMax;
        }
        if (ragdoll.parts[i].normalAngle > 0.0f) {
            def.enableConeLimit = true;
            def.coneAngle = ragdoll.parts[i].normalAngle;
        }
        auto joint = b3CreateSphericalJoint(world->id, &def);
        if (!B3_IS_NULL(joint)) {
            data->joints[i] = joint;
            b3SphericalJoint_SetSpringHertz(joint, ragdoll.stiffness);
            b3SphericalJoint_SetSpringDampingRatio(joint, ragdoll.damping);
            b3SphericalJoint_SetMaxMotorTorque(joint, ragdoll.gain);
            b3SphericalJoint_EnableMotor(joint, ragdoll.motorType != RagdollComponent::MotorType::None);
        }
    }
    ragdoll.data = data;
}
void PhysicsSystem::RemoveRagdoll(Entity, RagdollComponent& ragdoll) {
    auto* data = static_cast<RagdollObject*>(ragdoll.data);
    if (!data) return;
    for (auto joint : data->joints) if (!B3_IS_NULL(joint)) b3DestroyJoint(joint, true);
    for (auto* part : data->parts) if (part) {
        if (b3Body_IsValid(part->body)) b3DestroyBody(part->body);
        for (auto* user : part->shapeData) delete user;
        delete part;
    }
    delete data;
    ragdoll.data = nullptr;
    ragdoll.isDirty = true;
}
void PhysicsSystem::AddVehicle(Entity entity, VehiclePhysic& vehicle, RigidbodyComponent& chassis, TransformComponent&, InfoComponent&) {
    RemoveVehicle(entity, vehicle);
    if (!physicsWorld || !chassis.data) return;
    auto* world = reinterpret_cast<Box3DWorld*>(physicsWorld);
    auto* chassisData = static_cast<PhysicObject*>(chassis.data);
    auto* data = new VehiclePhysicData;
    data->chassis = chassisData->body;
    for (const auto& wheel : vehicle.wheels) {
        b3BodyDef bodyDef = b3DefaultBodyDef();
        bodyDef.type = b3_dynamicBody;
        bodyDef.position = b3Body_GetWorldPoint(chassisData->body, ToB3(wheel.pos));
        bodyDef.rotation = b3Body_GetRotation(chassisData->body);
        b3BodyId wheelBody = b3CreateBody(world->id, &bodyDef);
        b3ShapeDef shapeDef = b3DefaultShapeDef();
        shapeDef.density = 1.0f;
        b3Sphere sphere{{0,0,0}, wheel.radius};
        b3CreateSphereShape(wheelBody, &shapeDef, &sphere);
        b3Body_ApplyMassFromShapes(wheelBody);

        auto jointDef = b3DefaultWheelJointDef();
        jointDef.base.bodyIdA = chassisData->body;
        jointDef.base.bodyIdB = wheelBody;
        jointDef.base.localFrameA.p = ToB3(wheel.pos);
        jointDef.base.localFrameB.p = {0,0,0};
        jointDef.enableSuspensionSpring = true;
        jointDef.suspensionHertz = wheel.suspensionFrequency;
        jointDef.suspensionDampingRatio = wheel.suspensionDamping;
        jointDef.enableSuspensionLimit = true;
        jointDef.lowerSuspensionLimit = -wheel.suspensionMaxLength;
        jointDef.upperSuspensionLimit = -wheel.suspensionMinLength;
        jointDef.enableSteering = wheel.isFront;
        jointDef.enableSteeringLimit = wheel.isFront;
        jointDef.lowerSteeringLimit = -vehicle.maxSteeringAngle * 0.01745329252f;
        jointDef.upperSteeringLimit = vehicle.maxSteeringAngle * 0.01745329252f;
        jointDef.maxSteeringTorque = 100.0f;
        jointDef.enableSpinMotor = true;
        jointDef.maxSpinTorque = vehicle.maxEngineTorque;
        auto joint = b3CreateWheelJoint(world->id, &jointDef);
        if (!B3_IS_NULL(joint)) { data->wheelJoints.push_back(joint); data->wheelBodies.push_back(wheelBody); }
        else b3DestroyBody(wheelBody);
    }
    vehicle.data = data;
}
void PhysicsSystem::RemoveVehicle(Entity, VehiclePhysic& vehicle) {
    auto* data = static_cast<VehiclePhysicData*>(vehicle.data);
    if (!data) return;
    for (auto joint : data->wheelJoints) if (!B3_IS_NULL(joint)) b3DestroyJoint(joint, true);
    for (auto body : data->wheelBodies) if (b3Body_IsValid(body)) b3DestroyBody(body);
    delete data;
    vehicle.data = nullptr;
}
void PhysicsSystem::OnRemoveRigidbody(entt::registry& r, entt::entity e) { if (auto* p = r.ctx().find<PhysicsSystem*>()) { auto& rb = r.get<RigidbodyComponent>(e); (*p)->RemoveRigidbody(e, rb); } }
void PhysicsSystem::OnRemoveJoint(entt::registry& r, entt::entity e) { if (auto* p = r.ctx().find<PhysicsSystem*>()) { auto& c = r.get<JointComponent>(e); (*p)->RemoveJoint(e, c); } }
void PhysicsSystem::OnRemoveRagdoll(entt::registry& r, entt::entity e) { if (auto* p = r.ctx().find<PhysicsSystem*>()) { auto& c = r.get<RagdollComponent>(e); (*p)->RemoveRagdoll(e, c); } }
void PhysicsSystem::OnRemoveVehicle(entt::registry& r, entt::entity e) { if (auto* p = r.ctx().find<PhysicsSystem*>()) { auto& c = r.get<VehiclePhysic>(e); (*p)->RemoveVehicle(e, c); } }
void PhysicsSystem::OnRemoveHeightmap(entt::registry& registry, entt::entity entity) {
    auto& heightmap = registry.get<HeightmapColliderComponent>(entity);
    auto* data = heightmap.data;
    if (!data) return;
    if (b3Body_IsValid(data->body)) b3DestroyBody(data->body);
    if (data->field) b3DestroyHeightField(data->field);
    delete data;
    heightmap.data = nullptr;
}

// Advanced component and query methods are implemented below as Box3D equivalents.

} // namespace OD

#endif