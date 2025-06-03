#include "PhysicsSystem.h"

#if defined(UseJoltPhysics)
#include "OD/Core/Application.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Serialization/ImGuiArchive.h"
#include "OD/Graphics/Graphics.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include <set>
#include <algorithm>

//#define JPH_DEBUG_RENDERER

#include <Jolt/Jolt.h>
// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/TransformedShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollector.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include <iostream>
#include <cstdarg>
#include <thread>

namespace OD{

void PhysicsModuleInit(){
    SceneManager::Get().RegisterCoreComponent<RigidbodyComponent>("RigidbodyComponent");
	SceneManager::Get().RegisterCoreComponent<RagdollComponent>("RagdollComponent");
    SceneManager::Get().RegisterCoreComponent<CollisionBodyComponent>("CollisionBodyComponent");
    SceneManager::Get().RegisterCoreComponent<JointComponent>("JointComponent");
    SceneManager::Get().RegisterCoreComponent<HeightmapColliderComponent>("HeightmapColliderComponent");
    SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
}

#pragma region Core
// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// We're also using STL classes in this example
using namespace std;

inline Vector3 FromJolt(JPH::Vec3 v){ return Vector3(v.GetX(), v.GetY(), v.GetZ()); }
inline Quaternion FromJolt(JPH::Quat q){ return Quaternion(q.GetX(), q.GetY(), q.GetZ(), q.GetW()); }

inline JPH::Vec3 ToJolt(Vector3 v){ return JPH::Vec3(v.x, v.y, v.z); }
inline JPH::Quat ToJolt(Quaternion q){ return JPH::Quat(q.x, q.y, q.z, q.w); }

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char *inFMT, ...){
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	cout << buffer << endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine){
	// Print to the TTY
	cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << endl;

	// Breakpoint
	return true;
};

#endif // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace PhysicsLayers{
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING = 1;
	static constexpr ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter{
public:
	virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override{
		//return true;

		switch (inObject1)
		{
		case PhysicsLayers::NON_MOVING:
			return inObject2 == PhysicsLayers::MOVING; // Non moving only collides with moving
		case PhysicsLayers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers{
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface{
public:
    BPLayerInterfaceImpl(){
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[PhysicsLayers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[PhysicsLayers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint GetNumBroadPhaseLayers() const override{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer	GetBroadPhaseLayer(ObjectLayer inLayer) const override{
		JPH_ASSERT(inLayer < PhysicsLayers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer mObjectToBroadPhase[PhysicsLayers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter{
public:
	virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override{
		switch (inLayer1)
		{
		case PhysicsLayers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case PhysicsLayers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// An example contact listener
class MyContactListener : public ContactListener{
public:
	// See: ContactListener
	virtual ValidateResult	OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override{
		cout << "Contact validate callback" << endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override{
		cout << "A contact was added" << endl;
	}

	virtual void OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override{
		cout << "A contact was persisted" << endl;
	}

	virtual void OnContactRemoved(const SubShapeIDPair &inSubShapePair) override{
		cout << "A contact was removed" << endl;
	}
};

// An example activation listener
class MyBodyActivationListener : public BodyActivationListener{
public:
	virtual void OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override{
		cout << "A body got activated" << endl;
	}

	virtual void OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override{
		cout << "A body went to sleep" << endl;
	}
};

class MyGroupFilter : public JPH::GroupFilter {
public:
    virtual bool CanCollide(const JPH::CollisionGroup &a, const JPH::CollisionGroup &b) const override {
        /*int aMask = (int)a.GetSubGroupID();
        int bMask = (int)b.GetSubGroupID();
        int aLayer = (int)a.GetGroupID();
        int bLayer = (int)b.GetGroupID();
        return ((aMask & bLayer) != 0) && ((bMask & aLayer) != 0);*/

		/*Layers aLayer = (Layers)a.GetGroupID();
		Layers bLayer = (Layers)b.GetGroupID();
		int aMask = (int)a.GetSubGroupID();
		int bMask = (int)b.GetSubGroupID();

		bool r = (a.GetSubGroupID() & b.GetGroupID()) != 0 && (b.GetSubGroupID() & a.GetGroupID()) != 0;
		return r;*/

		int aLayer = a.GetGroupID();
        int bLayer = b.GetGroupID();
        int aMask = a.GetSubGroupID();
        int bMask = b.GetSubGroupID();
        bool canCollide = (aMask & bLayer) != 0 && (bMask & aLayer) != 0;
        std::cout << "CanCollide: aLayer=" << aLayer << ", aMask=" << aMask
                  << ", bLayer=" << bLayer << ", bMask=" << bMask
                  << ", Result=" << canCollide << std::endl;
        return canCollide;
    }
};

class MyDebugRenderer: public DebugRendererSimple {
public:
    virtual void DrawLine(JPH::RVec3 from, JPH::RVec3 to, JPH::Color color) override {
		Graphics::AddDrawLineCommand(
            Vector3(FromJolt(from)), 
            Vector3(FromJolt(to))
        );
        return;

        // Aqui você converte os vetores para seu tipo de vetor e desenha uma linha
        Graphics::DrawLine(
            FromJolt(from), 
            FromJolt(to), 
            Vector3(color.r, color.g, color.b),
            2
        );
    }

	virtual void DrawText3D(JPH::RVec3Arg inPosition, const string_view &inString, JPH::ColorArg inColor, float inHeight) override{}
};

class MeshShapeData{
public:
    MeshShapeData() = default;
	JPH::Ref<Shape> meshShape; // Jolt usa RefConst

	JPH::Array<JPH::Vec3> convexPoints;
	JPH::Array<JPH::Float3> joltVertices;
	JPH::Array<JPH::IndexedTriangle> joltTriangles;
};

Ref<MeshShapeData> OD_API CreateMeshShapeData(const Ref<Model>& model){
	std::vector<Vector3> vertices;
	std::vector<unsigned int> indices;

	auto AppedFrom = [&](Mesh& mesh, Matrix4 model){
        unsigned int vertexOffset = static_cast<unsigned int>(vertices.size());
		for(auto& vertex : mesh.vertices){
			vertices.push_back(model * Vector4(vertex, 1));
		}
        for(unsigned int index : mesh.indices){
            indices.push_back(index + vertexOffset);
        }
    };

	for(auto i: model->renderTargets){
		auto targetMesh = model->meshs[i.meshIndex].get();
		auto targetMatrix = model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
		AppedFrom(*targetMesh, targetMatrix);
	}

	return CreateMeshShapeData(vertices, indices);
}

Ref<MeshShapeData> CreateMeshShapeData(const Ref<Mesh>& mesh){
    return CreateMeshShapeData(mesh->vertices, mesh->indices); // Usa a função abaixo
}

Ref<MeshShapeData> OD_API CreateMeshShapeData(const std::vector<Vector3>& vertices, const std::vector<unsigned int> indices){
    Ref<MeshShapeData> out = CreateRef<MeshShapeData>();

	// Check for empty input
    if (vertices.empty() || indices.empty()) {
        LogError("CreateMeshShapeData: Empty vertices or indices");
        return nullptr;
    }

    // Check index count
    if (indices.size() % 3 != 0) {
        LogError("CreateMeshShapeData: Index count is not a multiple of 3!");
        return nullptr;
    }

    // Preenche os vértices convertidos
    //JPH::Array<JPH::Float3> joltVertices;
    out->joltVertices.reserve(vertices.size());
    for(const auto& v : vertices)
        out->joltVertices.push_back(JPH::Float3(v.x, v.y, v.z)); // Float3, não Vec3

    // Preenche os índices (cada 3 índices formam um triângulo)
    //JPH::Array<JPH::IndexedTriangle> joltTriangles;
    out->joltTriangles.reserve(indices.size() / 3);
    for(size_t i = 0; i < indices.size(); i += 3) {
        out->joltTriangles.push_back(JPH::IndexedTriangle(
            indices[i],
            indices[i + 1],
            indices[i + 2]
        ));
    }

	out->convexPoints.reserve(out->joltVertices.size());
	for(const auto& v : out->joltVertices)
		out->convexPoints.push_back(JPH::Vec3(v.x, v.y, v.z));

	if(indices.size() % 3 != 0) {
		LogError("CreateMeshShapeData: Index count is not a multiple of 3!");
		return nullptr;
	}

	if (out->joltVertices.empty() || out->joltTriangles.empty()) {
		LogError("Empty mesh data for entity ");
		return nullptr;
	}
	// Additional validations
    // 1. Check for valid vertex indices
    for (const auto& tri : out->joltTriangles) {
        if (tri.mIdx[0] >= out->joltVertices.size() ||
            tri.mIdx[1] >= out->joltVertices.size() ||
            tri.mIdx[2] >= out->joltVertices.size()) {
            LogError("CreateMeshShapeData: Invalid vertex index in triangle");
            return nullptr;
        }
    }

    // 2. Check for degenerate triangles
    bool hasDegenerate = false;
    for (const auto& tri : out->joltTriangles) {
        JPH::Vec3 v0 = out->convexPoints[tri.mIdx[0]];
        JPH::Vec3 v1 = out->convexPoints[tri.mIdx[1]];
        JPH::Vec3 v2 = out->convexPoints[tri.mIdx[2]];
        JPH::Vec3 edge1 = v1 - v0;
        JPH::Vec3 edge2 = v2 - v0;
        if (edge1.Cross(edge2).Length() < 1e-6f) {
            LogError("CreateMeshShapeData: Degenerate triangle detected");
            hasDegenerate = true;
        }
    }
    if (hasDegenerate) {
        return nullptr; // Stop if any degenerate triangles are found
    }

    // 3. Check for reasonable bounding box size
    JPH::Vec3 minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    JPH::Vec3 maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (const auto& v : out->convexPoints) {
        minBounds = JPH::Vec3::sMin(minBounds, v);
        maxBounds = JPH::Vec3::sMax(maxBounds, v);
    }
    JPH::Vec3 extent = maxBounds - minBounds;
    float maxExtent = extent.GetX();
    maxExtent = std::max(maxExtent, extent.GetY());
    maxExtent = std::max(maxExtent, extent.GetZ());
    if (maxExtent > 1000.0f) { // Adjust threshold based on your game’s scale
        LogError("CreateMeshShapeData: Mesh bounding box too large (extent: %s)", std::to_string(maxExtent).c_str());
        return nullptr;
    }

    // 4. Check for non-manifold or duplicate vertices (optional, advanced)
    std::set<uint32_t> uniqueVertices;
    for (const auto& tri : out->joltTriangles) {
        uniqueVertices.insert(tri.mIdx[0]);
        uniqueVertices.insert(tri.mIdx[1]);
        uniqueVertices.insert(tri.mIdx[2]);
    }
    if (uniqueVertices.size() < out->joltVertices.size()) {
        LogWarning("CreateMeshShapeData: Mesh contains unused vertices");
    }

    return out;
}

struct PhysicsWorld{
    BPLayerInterfaceImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectVsObjectLayerFilter;
    JPH::PhysicsSystem physicsSystem;
	//MyGroupFilter groupFilter;
	JPH::Ref<MyGroupFilter> groupFilter;

    TempAllocatorImpl* tempAllocator;
    JobSystemThreadPool jobSystem;

	MyDebugRenderer* renderer = nullptr;

    ~PhysicsWorld(){
		delete renderer;
        delete tempAllocator;
    }
};

class PhysicObject{
public:
    BodyID bodyID;
	PhysicsWorld* world = nullptr;
	bool isDirt = false;
};

struct RagdollObject{
	JPH::Ref<Ragdoll> ragdoll;
};

#pragma endregion

void CollisionBodyComponent::OnGui(Entity& e, Scene& scene){
    
}

void CollisionBodyComponent::SetShape(CollisionShape inShape){
    
}

void CollisionBodyComponent::UpdateSettings(){
    
}

#pragma region RigidbodyComponent

/*RigidbodyComponent::RigidbodyComponent(const RigidbodyComponent& other){
	shape = other.shape;
    type = other.type;
    angularFactor = other.angularFactor;
    mass = other.mask;
    neverSleep = other.neverSleep;
}

RigidbodyComponent& RigidbodyComponent::operator=(const RigidbodyComponent& other){
	if(this == &other) return *this;
	shape = other.shape;
    type = other.type;
    angularFactor = other.angularFactor;
    mass = other.mask;
    neverSleep = other.neverSleep;
	return *this;
}

RigidbodyComponent::RigidbodyComponent(RigidbodyComponent&& other){
	shape = std::move(other.shape);
    type = std::move(other.type);
    angularFactor = std::move(other.angularFactor);
    mass = std::move(other.mask);
    neverSleep = std::move(other.neverSleep);
}

RigidbodyComponent& RigidbodyComponent::operator=(RigidbodyComponent&& other){
	if(this == &other) return *this;
	shape = std::move(other.shape);
    type = std::move(other.type);
    angularFactor = std::move(other.angularFactor);
    mass = std::move(other.mask);
    neverSleep = std::move(other.neverSleep);
	return *this;
}
*/

void RigidbodyComponent::OnGui(Entity& e, Scene& scene){
	RigidbodyComponent& rb = scene.GetComponent<RigidbodyComponent>(e);

    const char* optionsString[] = {"Dynamic", "Static", "Kinematic", "Trigger"};
    const char* curOptionString = optionsString[(int)rb.GetType()];
    if(ImGui::BeginCombo("Type", curOptionString)){
        for(int i = 0; i < 4; i++){
            bool isSelected = curOptionString == optionsString[i];
            if(ImGui::Selectable(optionsString[i], isSelected)){
                curOptionString = optionsString[i];
                rb.SetType((RigidbodyComponent::Type)i);
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    float mass = rb.Mass();
    if(ImGui::DragFloat("mass", &mass)){
        rb.Mass(mass);
    }

    bool neverSleep = rb.NeverSleep();
    if(ImGui::Checkbox("neverSleep", &neverSleep)){
        rb.NeverSleep(neverSleep);
    }

	ImGui::DrawLayerMask("mask", rb.mask);

    CollisionShape shape = rb.GetShape();

    ImGui::Spacing();
    ImGui::SeparatorText("CollisionShape");

    CollisionShape::Type _shape = rb.GetShape().type;
    if(ImGui::DrawEnumCombo<CollisionShape::Type>("CollisionShape", &_shape)){
        shape.type = _shape;
        rb.SetShape(shape);
    }

    /*const char* shapeTypeString[] = {"Box", "Sphere", "Capsule"};
    const char* curShapeTypeString = shapeTypeString[(int)rb.GetShape().type];
    if(ImGui::BeginCombo("CollisionShape", curShapeTypeString)){
        for(int i = 0; i < 2; i++){
            bool isSelected = curShapeTypeString == shapeTypeString[i];
            if(ImGui::Selectable(shapeTypeString[i], isSelected)){
                curShapeTypeString = shapeTypeString[i];
                shape.type = (CollisionShape::Type)i;
                rb.SetShape(shape);
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }*/

    shape = rb.GetShape();

    if(rb.shape.type == CollisionShape::Type::Box){
        bool update = false;
        
        float _center[] = {shape.center.x, shape.center.y, shape.center.z};
        if(ImGui::DragFloat3("center", _center)){
            shape.center = Vector3(_center[0], _center[1], _center[2]);
            update = true;
        }
        float _shape[] = {shape.size.x, shape.size.y, shape.size.z};
        if(ImGui::DragFloat3("size", _shape)){
            shape.size = Vector3(_shape[0], _shape[1], _shape[2]);
            update = true;
        }

        if(update) rb.SetShape(shape);
    }

    if(rb.shape.type == CollisionShape::Type::Sphere){
        bool update = false;
        
        float _center[] = {shape.center.x, shape.center.y, shape.center.z};
        if(ImGui::DragFloat3("center", _center)){
            shape.center = Vector3(_center[0], _center[1], _center[2]);
            update = true;
        }
        float _radius = shape.radius;
        if(ImGui::DragFloat("radius", &_radius)){
            shape.radius = _radius;
            update = true;
        }

        if(update) rb.SetShape(shape);
    }

    if(rb.shape.type == CollisionShape::Type::Capsule){
        bool update = false;
        
        float _center[] = {shape.center.x, shape.center.y, shape.center.z};
        if(ImGui::DragFloat3("center", _center)){
            shape.center = Vector3(_center[0], _center[1], _center[2]);
            update = true;
        }
        float _radius = shape.radius;
        if(ImGui::DragFloat("radius", &_radius)){
            shape.radius = _radius;
            update = true;
        }
        float _height = shape.height;
        if(ImGui::DragFloat("height", &_height)){
            shape.height = _height;
            update = true;
        }

        if(update) rb.SetShape(shape);
    }
}

void RigidbodyComponent::SetShape(CollisionShape inShape){
    shape = inShape;
	UpdateSettings();
}

void RigidbodyComponent::UpdateSettings(){
	if(data == nullptr) return;
	data->isDirt = true;
}

void RigidbodyComponent::Mass(float m){
	mass = m;
	UpdateSettings();
}

void RigidbodyComponent::SetType(RigidbodyComponent::Type value){
    type = value;
	UpdateSettings();
}

void RigidbodyComponent::NeverSleep(bool value){

}

Vector3 RigidbodyComponent::Position(){
	if(data == nullptr) return Vector3Zero;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetPosition(data->bodyID));
}

void RigidbodyComponent::Position(Vector3 position){
	if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.SetPosition(data->bodyID, ToJolt(position), EActivation::Activate);
}

Quaternion RigidbodyComponent::Rotation(){
	if(data == nullptr) return QuaternionIdentity;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetRotation(data->bodyID));
}

void RigidbodyComponent::Rotation(Quaternion rotation){
	if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.SetRotation(data->bodyID, ToJolt(rotation), EActivation::Activate);
}

Vector3 RigidbodyComponent::Velocity(){
    if(data == nullptr) return Vector3Zero;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetLinearVelocity(data->bodyID));
}

void RigidbodyComponent::Velocity(Vector3 v){
    if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.SetLinearVelocity(data->bodyID, ToJolt(v));
	//bodyInterface.ActivateBody(data->bodyID);
	
	//bodyInterface.SetFriction(data->bodyID, 0.0f);
	//bodyInterface.SetLinearDamping(data->bodyID, 0.0f);
}

void RigidbodyComponent::ApplyForce(Vector3 v){
    if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.AddForce(data->bodyID, ToJolt(v));
}

void RigidbodyComponent::ApplyTorque(Vector3 v){
    if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.AddTorque(data->bodyID, ToJolt(v));
}

void RigidbodyComponent::ApplyImpulse(Vector3 v){
    if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.AddImpulse(data->bodyID, ToJolt(v));
}

void RigidbodyComponent::SetAngularFactor(Vector3 v){
    angularFactor = v;
    if(data == nullptr) return;

    BodyInterface& bodyInterface = data->world->physicsSystem.GetBodyInterface();
    BodyLockWrite lock(data->world->physicsSystem.GetBodyLockInterface(), data->bodyID);
    if(!lock.Succeeded()) return;

    Body& body = lock.GetBody();
    MotionProperties* motionProps = body.GetMotionProperties();
    if (motionProps) {
        Vec3 inertia = motionProps->GetInverseInertiaDiagonal();
        inertia.SetX(v.x != 0.0f ? inertia.GetX() : 0.0f);
        inertia.SetY(v.y != 0.0f ? inertia.GetY() : 0.0f);
        inertia.SetZ(v.z != 0.0f ? inertia.GetZ() : 0.0f);
		motionProps->SetInverseInertia(inertia, Quat::sIdentity());
    }

	/*angularFactor = v;
    if(data == nullptr) return;

    BodyInterface& bodyInterface = data->world->physicsSystem.GetBodyInterface();
    BodyLockWrite lock(data->world->physicsSystem.GetBodyLockInterface(), data->bodyID);
    if(!lock.Succeeded()) return;

    Body& body = lock.GetBody();
    MotionProperties* motionProps = body.GetMotionProperties();
    if(motionProps){
        Vec3 inertia = motionProps->GetInverseInertiaDiagonal();
        Quat inertiaRotation = motionProps->GetInertiaRotation();

        // Lock rotation if v.x/y/z is 0, otherwise keep original inertia
        inertia.SetX(v.x != 0.0f ? inertia.GetX() : 0.0f);
        inertia.SetY(v.y != 0.0f ? inertia.GetY() : 0.0f);
        inertia.SetZ(v.z != 0.0f ? inertia.GetZ() : 0.0f);

        motionProps->SetInverseInertia(inertia, inertiaRotation);
        bodyInterface.ActivateBody(data->bodyID);
    }*/
}
#pragma endregion

#pragma region PhysicsSystem

int ACCURACY = 10;

bool PhysicsSystem::IsSimulationEnable(){ return GetScene()->Running(); }

PhysicsSystem::PhysicsSystem(Scene* inScene):System(inScene){
   // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
	// This needs to be done before any other Jolt function is called.
	RegisterDefaultAllocator();

	// Install trace and assert callbacks
	Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

	// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
	// It is not directly used in this example but still required.
	Factory::sInstance = new Factory();

	// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
	// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
	// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
	RegisterTypes();

	// We need a temp allocator for temporary allocations during the physics update. We're
	// pre-allocating 10 MB to avoid having to do allocations during the physics update.
	// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	// malloc / free.
	//TempAllocatorImpl temp_allocator(10 * 1024 * 1024);

	// We need a job system that will execute physics jobs on multiple threads. Typically
	// you would implement the JobSystem interface yourself and let Jolt Physics run on top
	// of your own job scheduler. JobSystemThreadPool is an example implementation.
	//JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodies = 8192; //65536; //8192;// 1024;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodyPairs = 65536; //262144; //65536;// 1024;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const uint cMaxContactConstraints = 10240; //65536; //10240; //1024;

	// Create mapping table from object layer to broadphase layer
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
	//BPLayerInterfaceImpl broad_phase_layer_interface;

	// Create class that filters object vs broadphase layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
	//ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

	// Create class that filters object vs object layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
	//ObjectLayerPairFilterImpl object_vs_object_layer_filter;

	// Now we can create the actual physics system.
	//JPH::PhysicsSystem physics_system;
	//physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

    physicsWorld = new PhysicsWorld();
    physicsWorld->physicsSystem.Init(
        cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, 
        physicsWorld->broadPhaseLayerInterface, physicsWorld->objectVsBroadPhaseLayerFilter, physicsWorld->objectVsObjectLayerFilter
    );

    physicsWorld->tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
    physicsWorld->jobSystem.Init(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
	physicsWorld->renderer = new MyDebugRenderer();

	JPH::Ref<MyGroupFilter> groupFilter = new MyGroupFilter();
	physicsWorld->groupFilter = groupFilter;

	// Check that doesn't collide with self
	CollisionGroup g1(physicsWorld->groupFilter, Layer1, Layer0);
	Assert(g1.CanCollide(g1) == false);

	// Check that collides with other group
	CollisionGroup g2(physicsWorld->groupFilter, Layer1, AllLayers);
	Assert(g1.CanCollide(g2) == false);
	Assert(g2.CanCollide(g1) == false);

	JPH::DebugRenderer::sInstance = physicsWorld->renderer;

    this->scene->GetRegistry().on_destroy<RigidbodyComponent>().connect<&OnRemoveRigidbody>();
    this->scene->GetRegistry().ctx().emplace<PhysicsSystem*>(this);
}

void* PhysicsSystem::GetInternlWorld(){
    return nullptr;
}

PhysicsSystem::~PhysicsSystem(){
    this->scene->GetRegistry().on_destroy<RigidbodyComponent>().disconnect<&OnRemoveRigidbody>();

    UnregisterTypes();

	JPH::DebugRenderer::sInstance = nullptr;

    delete Factory::sInstance;
	Factory::sInstance = nullptr;

    delete physicsWorld;
}

RagdollSettings* CreateRagdollSettings(InfoComponent& info, TransformComponent& trans, RagdollComponent& ragdoll, Skeleton& skinnedSkeleton){
	auto GetShape = [](CollisionShape shape) -> Shape* {
		if(shape.type == CollisionShape::Type::Box) return new BoxShape(ToJolt(shape.size * 0.5f));
		if(shape.type == CollisionShape::Type::Sphere) return new SphereShape(shape.radius);
		if(shape.type == CollisionShape::Type::Capsule) return new CapsuleShape(shape.height * 0.5f, shape.radius);
		Assert(false);
		return nullptr;
	};

	JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;

	for(int i = 0; i < ragdoll.parts.size(); i++){
		Assert(ragdoll.parts[i].skinnedSkeletonIndex >= 0);
		//Assert(ragdoll.parts[i].parent > 0);

		if(ragdoll.parts[i].parent >= 0){
			skeleton->AddJoint(skinnedSkeleton.GetJointName(ragdoll.parts[i].skinnedSkeletonIndex), ragdoll.parts[i].parent);
		} else {
			skeleton->AddJoint(skinnedSkeleton.GetJointName(ragdoll.parts[i].skinnedSkeletonIndex));
		}
	}

	// Create ragdoll settings
	RagdollSettings *settings = new RagdollSettings;
	settings->mSkeleton = skeleton;
	settings->mParts.resize(skeleton->GetJointCount());
	for(int p = 0; p < skeleton->GetJointCount(); ++p){
		auto shapes = GetShape(ragdoll.parts[p].shape);
		Transform boneTrans = skinnedSkeleton.GetRestPose().GetGlobalTransform(ragdoll.parts[p].skinnedSkeletonIndex);
		auto positions = ToJolt(trans.TransformPoint(boneTrans.LocalPosition()/* + ragdoll.parts[p].shape.center*/));
		auto rotations = ToJolt(math::quat_cast(trans.GetLocalModelMatrix()) * boneTrans.LocalRotation()); //ToJolt(trans.Rotation() * boneTrans.LocalRotation());
		auto constraint_positions = ToJolt(trans.TransformPoint(boneTrans.TransformPoint(ragdoll.parts[p].constraintPos)));
		auto twist_axis = ToJolt(trans.TransformDirection(ragdoll.parts[p].twistAxis));
		//auto twist_angle = ragdoll.parts[p].twistAngle;
		auto normal_angle = ragdoll.parts[p].normalAngle;
		auto plane_angle = ragdoll.parts[p].planeAngle;

		RotatedTranslatedShapeSettings offsetShapeSettings(ToJolt(ragdoll.parts[p].shape.center), Quat::sIdentity(), shapes);
		RefConst<Shape> finalShape = offsetShapeSettings.Create().Get();

		RagdollSettings::Part &part = settings->mParts[p];
		part.SetShape(finalShape /*shapes*/);
		part.mPosition = positions;
		part.mRotation = rotations;
		part.mMotionType = EMotionType::Dynamic;
		part.mObjectLayer = PhysicsLayers::MOVING;
		part.mUserData = static_cast<uint64_t>(ragdoll.parts[p].skinnedSkeletonIndex); //static_cast<uint64>(ragdoll.parts[p].skinnedSkeletonIndex);

		// First part is the root, doesn't have a parent and doesn't have a constraint
		if(p > 0){
			SwingTwistConstraintSettings *constraint = new SwingTwistConstraintSettings;
			constraint->mDrawConstraintSize = 0.1f;
			constraint->mPosition1 = constraint->mPosition2 = constraint_positions;
			constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis;
			constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = Vec3::sAxisZ();
			constraint->mTwistMinAngle = DegreesToRadians(ragdoll.parts[p].twistAngleMin); //-DegreesToRadians(twist_angle);
			constraint->mTwistMaxAngle = DegreesToRadians(ragdoll.parts[p].twistAngleMax); //DegreesToRadians(twist_angle);
			constraint->mNormalHalfConeAngle = DegreesToRadians(normal_angle);
			constraint->mPlaneHalfConeAngle = DegreesToRadians(plane_angle);
			part.mToParent = constraint;
		}
	}

	settings->Stabilize();// Optional: Stabilize the inertia of the limbs
	settings->DisableParentChildCollisions();// Disable parent child collisions so that we don't get collisions between constrained bodies
	settings->CalculateBodyIndexToConstraintIndex();// Calculate the map needed for GetBodyIndexToConstraintIndex()

	return settings;
}

void PhysicsSystem::PhysicsUpdate(){
    if(GetScene()->Running() == false) return;

	//JPH::DebugRenderer::sInstance = physicsWorld->renderer;

	if(GetScene()->Running() == true){
		const int cCollisionSteps = 1;
		physicsWorld->physicsSystem.Update(
			Application::DeltaTime(), cCollisionSteps, physicsWorld->tempAllocator, &physicsWorld->jobSystem
		);
	}

    BodyInterface &bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();

	auto viewMesh = GetScene()->GetRegistry().view<RigidbodyComponent, ModelRendererComponent, TransformComponent>();
    for(auto e: viewMesh){
		RigidbodyComponent& rb = viewMesh.get<RigidbodyComponent>(e);
        TransformComponent& transform = viewMesh.get<TransformComponent>(e);
        ModelRendererComponent& mesh = viewMesh.get<ModelRendererComponent>(e);

		if(rb.shape.type == CollisionShape::Type::Mesh && rb.shape.mesh == nullptr){
			rb.shape.mesh = CreateMeshShapeData(mesh.GetModel());
		}
	}

    auto view = GetScene()->GetRegistry().view<RigidbodyComponent, TransformComponent, InfoComponent>();
    for(auto e: view){
        RigidbodyComponent& rb = view.get<RigidbodyComponent>(e);
        TransformComponent& transform = view.get<TransformComponent>(e);
        InfoComponent& info = view.get<InfoComponent>(e);

        if(rb.data == nullptr){
			rb.data = new PhysicObject();
			rb.data->world = physicsWorld;
			AddRigidbody(e, rb, transform, info);
		}
		if(rb.data->isDirt){
			rb.data->isDirt = false;
			RemoveRigidbody(e, rb);
			AddRigidbody(e, rb, transform, info);
		}
        Assert(rb.data != nullptr);

        if(rb.GetType() == RigidbodyComponent::Type::Dynamic/* || rb.GetType() == RigidbodyComponent::Type::Static*/){
            RVec3 pos;
            Quat rot;
            bodyInterface.GetPositionAndRotation(rb.data->bodyID, pos, rot);
            transform.Position(FromJolt(pos));
            transform.Rotation(FromJolt(rot));
        } else if(rb.GetType() == RigidbodyComponent::Type::Kinematic){
            bodyInterface.SetPosition(rb.data->bodyID, ToJolt(transform.Position()), EActivation::Activate);
            bodyInterface.SetRotation(rb.data->bodyID, ToJolt(transform.Rotation()), EActivation::Activate);
        }
    }

	auto view2 = GetScene()->GetRegistry().view<SkinnedModelRendererComponent, RagdollComponent, TransformComponent, InfoComponent>();
	for(auto [entity, skinned, ragdoll, trans, info]: view2.each()){
		/*if(ragdoll.data == nullptr){
			ragdoll.data = new RagdollObject();
			JPH::Ref<RagdollSettings> settings = CreateRagdollSettings(info, trans, ragdoll, skinned.GetModel()->skeleton);
			ragdoll.data->ragdoll = settings->CreateRagdoll(0, 0, &physicsWorld->physicsSystem);
			ragdoll.data->ragdoll->AddToPhysicsSystem(EActivation::Activate);
		}*/
		if(ragdoll.isDirty){
			ragdoll.isDirty = false;
			if(ragdoll.data != nullptr){
				ragdoll.data->ragdoll->RemoveFromPhysicsSystem();
				delete ragdoll.data->ragdoll;
			}
			ragdoll.data = new RagdollObject();
			JPH::Ref<RagdollSettings> settings = CreateRagdollSettings(info, trans, ragdoll, skinned.GetModel()->skeleton);
			ragdoll.data->ragdoll = settings->CreateRagdoll(0, 0, &physicsWorld->physicsSystem);
			for (size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
				BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
				bodyInterface.SetUserData(bodyID, static_cast<uint64_t>(ragdoll.parts[p].skinnedSkeletonIndex));
				LogInfo("Set Body %zd UserData to %d", p, ragdoll.parts[p].skinnedSkeletonIndex);
			}
			ragdoll.data->ragdoll->AddToPhysicsSystem(EActivation::Activate);
		}

		if(ragdoll.data != nullptr){
			/*SkinnedModelRendererComponent& skinned = scene->GetComponent<SkinnedModelRendererComponent>(entity);
			skinned.posePalette.resize(skinned.GetModel()->skeleton.GetRestPose().Size());
			skinned.finalPose = skinned.GetModel()->skeleton.GetRestPose();

			for(auto i: ragdoll.data->ragdoll->GetBodyIDs()){
				RVec3 pos;
				Quat rot;
				bodyInterface.GetPositionAndRotation(i, pos, rot);
				
				int boneIndex = static_cast<int>(bodyInterface.GetUserData(i));
				skinned.finalPose.SetGlobalTransform(boneIndex, Transform(
					trans.InverseTransformPoint(FromJolt(pos)), 
					math::inverse(math::quat_cast(trans.GetLocalModelMatrix())) * FromJolt(rot), //math::inverse(trans.Rotation()) * FromJolt(rot), 
					Vector3One
				));
			}	
			skinned.finalPose.GetMatrixPalette(skinned.posePalette, skinned.GetModel()->skeleton.GetInvBindPose());*/

			if(skinned.skeletonEntities.size() > 0){
				SkinnedModelRendererComponent& skinned = scene->GetComponent<SkinnedModelRendererComponent>(entity);
				//skinned.posePalette.resize(skinned.GetModel()->skeleton.GetRestPose().Size());
				skinned.finalPose = skinned.GetModel()->skeleton.GetRestPose();

				for(auto i: ragdoll.data->ragdoll->GetBodyIDs()){
					RVec3 pos;
					Quat rot;
					bodyInterface.GetPositionAndRotation(i, pos, rot);
					
					int boneIndex = static_cast<int>(bodyInterface.GetUserData(i));
					Assert(boneIndex != 0);
					TransformComponent& tt = scene->GetComponent<TransformComponent>(skinned.skeletonEntities[boneIndex]);
					tt.Position(FromJolt(pos));
					tt.Rotation(FromJolt(rot));
				}
				skinned.UpdateSkeletonEntitesIn(skinned.finalPose, *scene);
			}
		}
	}
}

void PhysicsSystem::OnDrawGizmos(Camera& cam){
	ShowDebugGizmos();
}

void PhysicsSystem::CheckForCollisionEvents(){

}

void PhysicsSystem::ShowDebugGizmos(){
	//if(GetScene()->Running() == false) return;
	//BodyInterface &bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();

	physicsWorld->physicsSystem.DrawBodies(JPH::BodyManager::DrawSettings(), physicsWorld->renderer);
	Graphics::DrawLinesComamnd({0, 1, 0}, 1);
}

// Example: Filter for non-moving objects (e.g., static walls)
class BroadPhaseLayerFilterImpl : public JPH::BroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override {
        return inLayer == BroadPhaseLayers::MOVING; // Replace with your layer
    }
};

struct MyObjectLayerFilter : public JPH::ObjectLayerFilter{
    LayerMask allowedMask; // your LayerMask.mask

    MyObjectLayerFilter(LayerMask inMask):allowedMask(inMask){}

    virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override{
		//return (inLayer & allowedMask.mask) != 0;;
		//bool r = (inLayer & allowedMask.mask) != 0;
		//return r;
        return (allowedMask.mask & inLayer) != 0;
    }
};

class ClosestHitRayCollector : public JPH::CastRayCollector {
public:
    ClosestHitRayCollector() : mHitFraction(1.0f) {}

    void AddHit(const JPH::RayCastResult& inResult) override {
        // Only keep the closest hit
        if(inResult.mFraction < mHitFraction){
            mHit = inResult;
            mHitFraction = inResult.mFraction;
        }
    }

    bool HadHit() const {
        return mHitFraction < 1.0f;
    }

    const JPH::RayCastResult& GetHit() const {
        return mHit;
    }

private:
    JPH::RayCastResult mHit;
    float mHitFraction;
};

bool PhysicsSystem::Raycast(Vector3 pos, Vector3 dir, RayResult& hit){
	Assert(physicsWorld != nullptr); 

	JPH::RRayCast ray;
	ray.mOrigin = ToJolt(pos);
	ray.mDirection = ToJolt(dir);

	JPH::RayCastSettings settings;
	settings.mBackFaceModeTriangles = JPH::EBackFaceMode::IgnoreBackFaces; // Ignore back-facing triangles
	settings.mBackFaceModeConvex = JPH::EBackFaceMode::IgnoreBackFaces;   // Ignore back-facing convex shapes
	settings.mTreatConvexAsSolid = false; // Treat convex shapes as solid

	JPH::BodyInterface& bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();
	ClosestHitRayCollector collector;

	//JPH::RayCastResult result;
	//if(physicsWorld->physicsSystem.GetNarrowPhaseQuery().CastRay(ray, result)){

	physicsWorld->physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector);
	if(collector.HadHit()){
		const JPH::RayCastResult& result = collector.GetHit();
		JPH::BodyID hitBodyID = result.mBodyID;
		float hitFraction = result.mFraction; // From 0.0 to 1.0
		JPH::Vec3 hitPosition = ray.mOrigin + ray.mDirection * hitFraction;

		// Get the body that was hit
		const JPH::BodyLockRead lock(physicsWorld->physicsSystem.GetBodyLockInterface(), result.mBodyID);
		if(!lock.Succeeded()) return false;

		const JPH::Body &body = lock.GetBody();
		JPH::Vec3 hitPoint = ray.GetPointOnRay(result.mFraction);

		hit.entity = static_cast<Entity>(body.GetUserData()); // safe cast
		hit.hitPoint = FromJolt(hitPoint);
		hit.hitPoint = FromJolt(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint));
		return true;
	}

    return false;
}

bool PhysicsSystem::Raycast(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask){
    Assert(physicsWorld != nullptr); 

	JPH::RRayCast ray;
	ray.mOrigin = ToJolt(pos);
	ray.mDirection = ToJolt(dir);

	JPH::RayCastSettings settings;
	settings.mBackFaceModeTriangles = JPH::EBackFaceMode::IgnoreBackFaces; // Ignore back-facing triangles
	settings.mBackFaceModeConvex = JPH::EBackFaceMode::IgnoreBackFaces;   // Ignore back-facing convex shapes
	settings.mTreatConvexAsSolid = false; // Treat convex shapes as solid

	JPH::BodyInterface& bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();
	ClosestHitRayCollector collector;

	//JPH::RayCastResult result;
	//if(physicsWorld->physicsSystem.GetNarrowPhaseQuery().CastRay(ray, result)){

	MyObjectLayerFilter objectLayerFilter(mask);

	MyObjectLayerFilter _objectLayerFilter({Layers::Layer0 | Layers::Layer1});
	Assert(_objectLayerFilter.ShouldCollide(Layers::Layer1) == true);
	Assert(_objectLayerFilter.ShouldCollide(Layers::Layer2) == false);

	physicsWorld->physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector, {}, objectLayerFilter);
	if(collector.HadHit()){
		const JPH::RayCastResult& result = collector.GetHit();
		JPH::BodyID hitBodyID = result.mBodyID;
		float hitFraction = result.mFraction; // From 0.0 to 1.0
		JPH::Vec3 hitPosition = ray.mOrigin + ray.mDirection * hitFraction;

		// Get the body that was hit
		const JPH::BodyLockRead lock(physicsWorld->physicsSystem.GetBodyLockInterface(), result.mBodyID);
		if(!lock.Succeeded()) return false;

		const JPH::Body &body = lock.GetBody();
		JPH::Vec3 hitPoint = ray.GetPointOnRay(result.mFraction);

		hit.entity = static_cast<Entity>(body.GetUserData()); // safe cast
		hit.hitPoint = FromJolt(hitPoint);
		hit.hitPoint = FromJolt(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint));
		return true;
	}

    return false;
}

void PhysicsSystem::Simulate(float step){
    
}

void PhysicsSystem::SynchronizeMotionStates(){
    
}

void PhysicsSystem::AddOnCollisionEnterCallback(OnCollisionCallback callback){ onCollisionEnterCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnCollisionEnterCallback(OnCollisionCallback callback){ 
    onCollisionEnterCallbacks.erase(std::remove(onCollisionEnterCallbacks.begin(), onCollisionEnterCallbacks.end(), callback), onCollisionEnterCallbacks.end()); 
}

void PhysicsSystem::AddOnCollisionExitCallback(OnCollisionCallback callback){ onCollisionExitCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnCollisionExitCallback(OnCollisionCallback callback){
    onCollisionExitCallbacks.erase(std::remove(onCollisionExitCallbacks.begin(), onCollisionExitCallbacks.end(), callback), onCollisionExitCallbacks.end());
}

void PhysicsSystem::AddOnTriggerEnterCallback(OnCollisionCallback callback){ onTriggerEnterCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnTriggerEnterCallback(OnCollisionCallback callback){
    onTriggerEnterCallbacks.erase(std::remove(onTriggerEnterCallbacks.begin(), onTriggerEnterCallbacks.end(), callback), onTriggerEnterCallbacks.end());
}

void PhysicsSystem::AddOnTriggerExitCallback(OnCollisionCallback callback){ onTriggerExitCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnTriggerExitCallback(OnCollisionCallback callback){
    onTriggerExitCallbacks.erase(std::remove(onTriggerExitCallbacks.begin(), onTriggerExitCallbacks.end(), callback), onTriggerExitCallbacks.end());
}

void PhysicsSystem::OnRemoveRigidbody(entt::registry& r, entt::entity e){
    RigidbodyComponent& rb = r.get<RigidbodyComponent>(e);
    if(rb.data == nullptr) return;

    PhysicsSystem* physicsSystem = r.ctx().get<PhysicsSystem*>();
    physicsSystem->RemoveRigidbody(e, rb);
    delete rb.data;
}

void PhysicsSystem::AddRigidbody(Entity entity, RigidbodyComponent& rb, TransformComponent& transform, InfoComponent& info){
    BodyInterface &bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();

    EMotionType type = EMotionType::Dynamic;
    if(rb.type == RigidbodyComponent::Type::Static) type = EMotionType::Static;
	if(rb.type == RigidbodyComponent::Type::Kinematic) type = EMotionType::Kinematic;

    JPH::Ref<Shape> shape = nullptr;
    if(rb.shape.type == CollisionShape::Type::Box){
		BoxShapeSettings shapeSettings(ToJolt(rb.shape.size * 0.5f));
		shapeSettings.SetDensity(rb.mass);
		shape = shapeSettings.Create().Get();
	} else if(rb.shape.type == CollisionShape::Type::Sphere){
		SphereShapeSettings shapeSettings(rb.shape.radius);
		shapeSettings.SetDensity(rb.mass);
		shape = shapeSettings.Create().Get();
	} else if(rb.shape.type == CollisionShape::Type::Capsule){
		float halfHeight = (rb.shape.height - 2.0f * rb.shape.radius) * 0.5f;
		if (halfHeight < 0.0f) {
			std::cerr << "Invalid capsule dimensions: height must be at least 2 * radius\n";
			Assert(false);
		}
		CapsuleShapeSettings shapeSettings(halfHeight, rb.shape.radius);
		shapeSettings.SetDensity(rb.mass);
		shape = shapeSettings.Create().Get();
	} else if(rb.shape.type == CollisionShape::Type::Mesh){
		//JPH::MeshShapeSettings shapeSettings(rb.shape.mesh->joltVertices, rb.shape.mesh->joltTriangles);
		//shapeSettings.SetDensity(rb.mass);
		//shape = shapeSettings.Create().Get();
		//shape = rb.shape.mesh->meshShape;// shapeSettings.Create().Get();

		Assert(rb.shape.mesh != nullptr);
		Assert(rb.shape.mesh->joltVertices.size() > 0);
		Assert(rb.shape.mesh->joltTriangles.size() > 0);
		Assert(rb.shape.mesh->convexPoints.size() > 0);

		if(rb.type == RigidbodyComponent::Type::Dynamic){
			// Use Convex Hull for dynamic
			JPH::ConvexHullShapeSettings shapeSettings(rb.shape.mesh->convexPoints);
			shapeSettings.SetDensity(rb.mass);

			auto result = shapeSettings.Create();
			if (!result.HasError()) {
				shape = result.Get();
			} else {
				std::cerr << "ConvexHullShape creation error: " << result.GetError() << std::endl;
				return;
			}
		} else {
			// Use MeshShape for static or kinematic
			JPH::MeshShapeSettings shapeSettings(rb.shape.mesh->joltVertices, rb.shape.mesh->joltTriangles);
			shapeSettings.SetEmbedded();
			auto result = shapeSettings.Create();
			if (!result.HasError()) {
				shape = result.Get();
			} else {
				std::cerr << "MeshShape creation error: " << result.GetError() << std::endl;
				return;
			}
		}
	}

	Assert(shape != nullptr);
	//RefConst<Shape> finalShape = new OffsetCenterOfMassShape(shape, ToJolt(rb.shape.center));

	RotatedTranslatedShapeSettings offsetShapeSettings(ToJolt(rb.shape.center), Quat::sIdentity(), shape);
	//RefConst<Shape> finalShape = offsetShapeSettings.Create().Get();

	auto offsetResult = offsetShapeSettings.Create();
	if (offsetResult.HasError()) {
        LogError("OffsetShape creation error for entity %s: %s", info.name.c_str(), offsetResult.GetError().c_str());
        return;
    }
    RefConst<Shape> finalShape = offsetResult.Get();

	BodyCreationSettings settings(
        finalShape, ToJolt(transform.Position()), ToJolt(transform.Rotation()), type, PhysicsLayers::MOVING
    );
	settings.mUserData = static_cast<uint64>(entity); // safe cast
	settings.mCollisionGroup = JPH::CollisionGroup(
		physicsWorld->groupFilter,
        info.layer,
        rb.mask.mask // stored in subgroup ID
    );
    rb.data->bodyID = bodyInterface.CreateAndAddBody(settings, rb.type == RigidbodyComponent::Type::Dynamic ? EActivation::Activate : EActivation::DontActivate);
	// Verify body creation
    if (!bodyInterface.IsAdded(rb.data->bodyID)) {
        LogError("Failed to add body for entity %s", info.name.c_str());
        return;
    }

	rb.SetAngularFactor(rb.angularFactor);
}

void PhysicsSystem::RemoveRigidbody(Entity entity, RigidbodyComponent& rb){
	BodyInterface &bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();
    bodyInterface.RemoveBody(rb.data->bodyID);
    bodyInterface.DestroyBody(rb.data->bodyID);
}

void PhysicsSystem::OnRemoveCollisionBody(entt::registry& r, entt::entity e){

}

void PhysicsSystem::AddCollisionBody(Entity entity, CollisionBodyComponent& c, TransformComponent& t, InfoComponent& info){
    
}

void PhysicsSystem::RemoveCollisionBody(Entity entity, CollisionBodyComponent& rb){
    
}

void PhysicsSystem::OnRemoveJoint(entt::registry& r, entt::entity e){
    
}

void PhysicsSystem::AddJoint(Scene* scene, Entity entity, JointComponent& c, TransformComponent& t, InfoComponent& info){

}

void PhysicsSystem::RemoveJoint(Entity entity, JointComponent& c){
   
}

#pragma endregion

}
#endif