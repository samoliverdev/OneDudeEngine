#include "OD/pch.h"
#include "PhysicsSystem.h"

#if defined(UseJoltPhysics)
#include "OD/Core/Application.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Time.h"
#include "OD/Core/Input.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/GlobalSettings.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Serialization/ImGuiArchive.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Mesh.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/Editor/Editor.h"
#include <cstdarg>

//#define JPH_DEBUG_RENDERER

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
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
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

namespace OD{

//constexpr float fixedTimeStep = 1.0f / 60.0f; // 60 Hz physics update
constexpr int maxSubSteps = 5;
constexpr int cCollisionSteps = 1;

//constexpr bool EnableFixedRate = true;
constexpr bool EnableInterpolation = true; //true; //true;
constexpr bool EnableFixedPostPhysicUpdate = false;//true;

#pragma region Core

void DrawLayerCollisionMatrix(
	std::array<std::array<bool, LayerCount>, LayerCount>& collisionMatrix,
	std::vector<std::string>& layerNames
){
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_BordersInnerV
        | ImGuiTableFlags_BordersOuter
        | ImGuiTableFlags_ScrollY
        | ImGuiTableFlags_ScrollX;

    ImGuiStyle& style = ImGui::GetStyle();
    style.TableAngledHeadersAngle = 0;
    style.TableAngledHeadersTextAlign = ImVec2(0.5f, 0.5f);

    // 🧩 Collect valid layers (skip default-named ones)
    std::vector<int> visibleLayers;
    for(int i = 0; i < LayerCount; ++i){
        std::string defaultName = "Layer" + std::to_string(i);
        if(strcmp(layerNames[i].c_str(), defaultName.c_str()) != 0 && layerNames[i][0] != '\0'){
            visibleLayers.push_back(i);
		}
    }

    if(visibleLayers.empty()) return; // Nothing to draw

    int visibleCount = (int)visibleLayers.size();

    if(ImGui::BeginTable("LayerCollisionMatrix", visibleCount + 1, tableFlags)){
        ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide);

        // 🔁 Reversed order (for angled headers)
        for(int idx = visibleCount - 1; idx >= 0; --idx){
            int layerIndex = visibleLayers[idx];
            ImGui::TableSetupColumn(layerNames[layerIndex].c_str(),
                ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed);
        }

        ImGui::TableAngledHeadersRow();

        // Draw visible layer rows
        for(int r = 0; r < visibleCount; ++r){
            int row = visibleLayers[r];
            ImGui::PushID(row);
            ImGui::TableNextRow();

            for(int c = 0; c < visibleCount + 1; ++c){
                ImGui::TableSetColumnIndex(c);

                if(c == 0){
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(layerNames[row].c_str());
                } else {
                    int realCol = visibleLayers[visibleCount - c];

                    if(realCol < row) continue;

                    ImGui::PushID(c);
                    bool& val = collisionMatrix[row][realCol];
                    ImGui::Checkbox("", &val);
                    if(row != realCol)
                        collisionMatrix[realCol][row] = val;
                    ImGui::PopID();
                }
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

bool DrawLayerCollisionMatrix2(
    std::array<std::array<bool, LayerCount>, LayerCount>& collisionMatrix,
    std::array<std::string, LayerCount>& layerNames,
    bool skipDefaultNamedLayers = true,
    bool reverseHeaders = true
){
    bool changed = false;

	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	/*static ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Hideable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_HighlightHoveredColumn;
	static ImGuiTableColumnFlags column_flags = ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed;
	static int frozen_cols = 1;
	static int frozen_rows = 2;
	ImGui::CheckboxFlags("_ScrollX", &table_flags, ImGuiTableFlags_ScrollX);
	ImGui::CheckboxFlags("_ScrollY", &table_flags, ImGuiTableFlags_ScrollY);
	ImGui::CheckboxFlags("_Resizable", &table_flags, ImGuiTableFlags_Resizable);
	ImGui::CheckboxFlags("_NoBordersInBody", &table_flags, ImGuiTableFlags_NoBordersInBody);
	ImGui::CheckboxFlags("_HighlightHoveredColumn", &table_flags, ImGuiTableFlags_HighlightHoveredColumn);
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
	ImGui::SliderInt("Frozen columns", &frozen_cols, 0, 2);
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
	ImGui::SliderInt("Frozen rows", &frozen_rows, 0, 2);
	ImGui::CheckboxFlags("Disable header contributing to column width", &column_flags, ImGuiTableColumnFlags_NoHeaderWidth);

	if (ImGui::TreeNode("Style settings"))
	{
		ImGui::SameLine();
		//HelpMarker("Giving access to some ImGuiStyle value in this demo for convenience.");
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
		ImGui::SliderAngle("style.TableAngledHeadersAngle", &ImGui::GetStyle().TableAngledHeadersAngle, -50.0f, +50.0f);
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
		ImGui::SliderFloat2("style.TableAngledHeadersTextAlign", (float*)&ImGui::GetStyle().TableAngledHeadersTextAlign, 0.0f, 1.0f, "%.2f");
		ImGui::TreePop();
	}*/

    /*ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_BordersInnerV
        | ImGuiTableFlags_BordersOuter
        | ImGuiTableFlags_ScrollY
        | ImGuiTableFlags_ScrollX;*/

	ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit 
		| ImGuiTableFlags_ScrollX 
		| ImGuiTableFlags_ScrollY 
		| ImGuiTableFlags_BordersOuter 
		| ImGuiTableFlags_BordersInnerH 
		| ImGuiTableFlags_Hideable 
		| ImGuiTableFlags_Resizable 
		| ImGuiTableFlags_Reorderable 
		| ImGuiTableFlags_HighlightHoveredColumn;

	/*ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuter 
		| ImGuiTableFlags_BordersInnerH 
		| ImGuiTableFlags_SizingFixedFit 
		| ImGuiTableFlags_ScrollX;*/

    ImGuiStyle& style = ImGui::GetStyle();
    style.TableAngledHeadersAngle = 0; //50;
    style.TableAngledHeadersTextAlign = ImVec2(0.5f, 0.5f);

    // 🧩 Collect visible layers (skip default ones if requested)
    std::vector<int> visibleLayers;
    visibleLayers.reserve(LayerCount);

    for(int i = 0; i < LayerCount; ++i){
        const std::string& name = layerNames[i];
        if(!skipDefaultNamedLayers){
            visibleLayers.push_back(i);
		} else {
            std::string defaultName = "Layer" + std::to_string(i);
            if(!name.empty() && name != defaultName)
                visibleLayers.push_back(i);
        }
    }

    if(visibleLayers.empty()) return false; // Nothing to draw

    int visibleCount = (int)visibleLayers.size();

    if(ImGui::BeginTable("LayerCollisionMatrix", visibleCount + 1, tableFlags/*, ImVec2(0.0f, TEXT_BASE_HEIGHT * 52)*/)){
        ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide);

        // 🔁 Setup headers (optionally reversed)
        if(reverseHeaders){
            for(int idx = visibleCount - 1; idx >= 0; --idx){
                int layerIndex = visibleLayers[idx];
                ImGui::TableSetupColumn(
                    layerNames[layerIndex].c_str(),
                    ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed
                );
            }
        } else {
            for(int idx = 0; idx < visibleCount; ++idx){
                int layerIndex = visibleLayers[idx];
                ImGui::TableSetupColumn(
                    layerNames[layerIndex].c_str(),
                    ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed
                );
            }
        }

		//ImGui::TableSetupScrollFreeze(frozen_cols, frozen_rows);
        ImGui::TableAngledHeadersRow();
		//ImGui::TableHeadersRow();       // Draw remaining headers and allow access to context-menu and other functions.

        // 🧱 Draw matrix rows
        for(int r = 0; r < visibleCount; ++r){
            int row = visibleLayers[r];
            ImGui::PushID(row);
            ImGui::TableNextRow();

            for(int c = 0; c < visibleCount + 1; ++c){
                ImGui::TableSetColumnIndex(c);

                if(c == 0){
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(layerNames[row].c_str());
                } else {
                    int realCol = reverseHeaders
                        ? visibleLayers[visibleCount - c]
                        : visibleLayers[c - 1];

                    if(realCol < row) continue;

                    ImGui::PushID(c);
                    bool oldVal = collisionMatrix[row][realCol];
                    bool val = oldVal;
                    if(ImGui::Checkbox("", &val)){
                        collisionMatrix[row][realCol] = val;
                        if(row != realCol)
                            collisionMatrix[realCol][row] = val;
                        changed = true;
                    }
                    ImGui::PopID();
                }
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    return changed;
}

struct PhysicsSettings {
    Vector3 gravity = {0, -9.81f, 0};
    int solverIterations = 8;

	std::array<std::array<bool, LayerCount>, LayerCount> collisionMatrix{};
	std::array<uint32_t, LayerCount> layerMasks{};

	PhysicsSettings(){
		for(int i = 0; i < LayerCount; i++){
			layerMasks[i] = AllLayersMask;
			for(int j = 0; j < LayerCount; j++){
				collisionMatrix[i][j] = true; // Everything collides with everything initially
			}
		}
	}

    template<class Archive>
    void serialize(Archive& ar) {
        ArchiveDumpNVP(ar, gravity);
        ArchiveDumpNVP(ar, solverIterations);
		ArchiveDumpNVP(ar, collisionMatrix);

		UpdateLayerMasks();
    }

	void UpdateLayerMasks(){
		for(int row = 0; row < LayerCount; row++){
			uint32_t mask = 0;
			for(int col = 0; col < LayerCount; col++){
				if(collisionMatrix[row][col])
					mask |= (1u << col);
			}
			layerMasks[row] = mask;
		}
	}

    void OnImGuiRender() {
		ImGui::PushID("PhysicsSettings");
        ImGui::DragFloat3("Gravity", &gravity.x, 0.1f);
        ImGui::DragInt("Solver Iterations", &solverIterations, 1, 1, 64);

		if(DrawLayerCollisionMatrix2(collisionMatrix, GetGlobalSceneData().layerNames, true, true)){
			UpdateLayerMasks();
		}

		ImGui::PopID();
	}
};

void PhysicsModuleInit(){
    SceneManager::Get().RegisterCoreComponent<RigidbodyComponent>("RigidbodyComponent", "Physics");
	SceneManager::Get().RegisterCoreComponent<RagdollComponent>("RagdollComponent", "Physics");
    SceneManager::Get().RegisterCoreComponent<JointComponent>("JointComponent", "Physics");
    SceneManager::Get().RegisterCoreComponent<HeightmapColliderComponent>("HeightmapColliderComponent", "Physics");
    SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");

	SceneManager::Get().RegisterCoreComponent<MotorTest>("MotorTest", "Physics");
	SceneManager::Get().RegisterCoreComponent<VehiclePhysic>("VehiclePhysic", "Physics");

	OD::GlobalSettings::Get().Register<PhysicsSettings>("Physics");
}

uint64_t EncodeUserData(uint32_t u, int32_t i) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(i)) << 32) |
           static_cast<uint64_t>(u);
}

void DecodeUserData(uint64_t packed, uint32_t &u, int32_t &i){
    u = static_cast<uint32_t>(packed & 0xFFFFFFFFull);
    i = static_cast<int32_t>((packed >> 32) & 0xFFFFFFFFull);
}

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

using namespace JPH;// All Jolt symbols are in the JPH namespace
using namespace JPH::literals;// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace std;// We're also using STL classes in this example

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
/*namespace PhysicsLayers{
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING = 1;
	static constexpr ObjectLayer NUM_LAYERS = 2;
};*/

/*constexpr int layerCount = (int)Layers::LayerCount; //IM_ARRAYSIZE(layerNames);
static bool collisionMatrix[layerCount][layerCount] = {};
static uint32_t layerMasks[layerCount] = {};

void UpdateLayerMasks(){
    for (int row = 0; row < layerCount; row++){
        uint32_t mask = 0;
        for(int col = 0; col < layerCount; col++){
            if(collisionMatrix[row][col])
                mask |= (1u << col);
        }
        layerMasks[row] = mask;
    }
}

void InitLayerMasks(){
    for(int i = 0; i < layerCount; i++){
        layerMasks[i] = AllLayersMask;
        for(int j = 0; j < layerCount; j++)
            collisionMatrix[i][j] = true; // Everything collides with everything initially
    }
}*/

PhysicsSettings* currentSettings = nullptr;

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter{
public:
	virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override{
		return (currentSettings->layerMasks[inObject1] & (1u << inObject2)) != 0;
		//return (layerMasks[inObject1] & (1u << inObject2)) != 0;

		/*switch (inObject1)
		{
		case PhysicsLayers::NON_MOVING:
			return inObject2 == PhysicsLayers::MOVING; // Non moving only collides with moving
		case PhysicsLayers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}*/
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
		mObjectToBroadPhase[0] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[1] = BroadPhaseLayers::MOVING;
	}

	virtual uint GetNumBroadPhaseLayers() const override{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer	GetBroadPhaseLayer(ObjectLayer inLayer) const override{
		return BroadPhaseLayers::MOVING;
		JPH_ASSERT(inLayer < Layers::LayerCount);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
		return "MOVING";

		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer mObjectToBroadPhase[BroadPhaseLayers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter{
public:
	virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override{
		return true;

		/*switch (inLayer1)
		{
		case PhysicsLayers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case PhysicsLayers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}*/
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
		return false;
		return true;

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
        /*std::cout << "CanCollide: aLayer=" << aLayer << ", aMask=" << aMask
                  << ", bLayer=" << bLayer << ", bMask=" << bMask
                  << ", Result=" << canCollide << std::endl;*/

		/*if(canCollide == false){
			int test = 20;
			LogInfo("Test: %d", test);
		}*/		  

        return canCollide;
    }
};

class MyDebugRenderer: public DebugRendererSimple {
public:
	bool useLineCommand = true;
	Scene* scene = nullptr;

    virtual void DrawLine(JPH::RVec3 from, JPH::RVec3 to, JPH::Color color) override {
		/*if(scene != nullptr){
            TransformComponent& cam = scene->GetComponent<TransformComponent>(scene->GetMainCamera());
            if(math::distance(cam.Position(), FromJolt(from)) > 50) return;
        }*/

		//static int counter = 0;
		//if (++counter % 8 != 0) return; // desenha só 25%
		//if (++counter % 256 != 0) return; // desenha só 25%

		if(useLineCommand){
			Graphics::AddDrawLineCommand(
				Vector3(FromJolt(from)), 
				Vector3(FromJolt(to))
			);
		} else{ 
			Graphics::DrawLine(
				FromJolt(from), 
				FromJolt(to), 
				Vector3(color.r, color.g, color.b),
				2
			);
		}
    }

	virtual void DrawText3D(JPH::RVec3Arg inPosition, const string_view &inString, JPH::ColorArg inColor, float inHeight) override{}
};

class MeshShapeData{
public:
    MeshShapeData() = default;
	//JPH::Ref<Shape> meshShape; // Jolt usa RefConst

	JPH::Array<JPH::Vec3> convexPoints;
	JPH::Array<JPH::Float3> joltVertices;
	JPH::Array<JPH::IndexedTriangle> joltTriangles;
};

Ref<MeshShapeData> CreateMeshShapeData(Model& model){
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

	for(auto i: model.renderTargets){
		auto targetMesh = model.meshs[i.meshIndex].get();
		auto targetMatrix = model.skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
		AppedFrom(*targetMesh, targetMatrix);
	}

	return CreateMeshShapeData(vertices, indices);
}

Ref<MeshShapeData> CreateMeshShapeData(const Mesh& mesh){
    return CreateMeshShapeData(mesh.vertices, mesh.indices); // Usa a função abaixo
}

Ref<MeshShapeData> CreateMeshShapeData(const std::vector<Vector3>& vertices, const std::vector<unsigned int> indices){
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
		uint32_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
		JPH::Vec3 v0 = ToJolt(vertices[i0]);
		JPH::Vec3 v1 = ToJolt(vertices[i1]);
		JPH::Vec3 v2 = ToJolt(vertices[i2]);
		if ((v1 - v0).Cross(v2 - v0).Length() >= 1e-6f) {
			out->joltTriangles.push_back(JPH::IndexedTriangle(i0, i1, i2));
		} else {
			LogWarning("Skipped degenerate triangle: %u, %u, %u", i0, i1, i2);
		}

        /*out->joltTriangles.push_back(JPH::IndexedTriangle(
            indices[i],
            indices[i + 1],
            indices[i + 2]
        ));*/
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
    /*bool hasDegenerate = false;
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
    if(hasDegenerate){
        return nullptr; // Stop if any degenerate triangles are found
    }*/

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
	MyContactListener* contactListener;

    TempAllocatorImpl* tempAllocator;
    JobSystemThreadPool jobSystem;
	//JobSystemSingleThreaded jobSystem = JobSystemSingleThreaded(cMaxPhysicsJobs);

	MyDebugRenderer* renderer = nullptr;
	PhysicsSystem* system = nullptr;

    ~PhysicsWorld(){
		delete contactListener;
		//delete renderer;
        delete tempAllocator;
    }
};

// An example contact listener
class MyContactListener : public ContactListener{
public:
	Scene* scene = nullptr;
	PhysicsSystem* physic = nullptr;

	Mutex mutex;

	// See: ContactListener
	virtual ValidateResult	OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override{
		//cout << "Contact validate callback" << endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override{
		return;
		lock_guard lock(mutex);

		//cout << "A contact was added" << endl;
		//InfoComponent& e1 = scene->GetComponent<InfoComponent>(static_cast<Entity>(inBody1.GetUserData()));
		//InfoComponent& e2 = scene->GetComponent<InfoComponent>(static_cast<Entity>(inBody2.GetUserData()));
		//LogInfo("OnContactAdded e1: %s, e2: %s", e1.name.c_str(), e2.name.c_str());

		/*const Vec3 contactPoint = inManifold.GetWorldSpaceContactPointOn1(0); // or average of points if you prefer
		Vec3 v1 = inBody1.GetLinearVelocity() + inBody1.GetAngularVelocity().Cross(contactPoint - inBody1.GetCenterOfMassPosition());
		Vec3 v2 = inBody2.GetLinearVelocity() + inBody2.GetAngularVelocity().Cross(contactPoint - inBody2.GetCenterOfMassPosition());*/

		Collision collision;
		collision.e1 = static_cast<Entity>(inBody1.GetUserData());
		collision.e2 = static_cast<Entity>(inBody2.GetUserData());
		collision.normal = FromJolt(inManifold.mWorldSpaceNormal);
		collision.penetrationDepth = inManifold.mPenetrationDepth;
		//collision.relativeVelocity = FromJolt(v2 - v1);
		collision.relativeVelocity = FromJolt(inBody2.GetLinearVelocity() - inBody1.GetLinearVelocity());
		collision.relativeContactPointOn1 = FromJolt(inManifold.GetWorldSpaceContactPointOn1(0));
		collision.relativeContactPointOn2 = FromJolt(inManifold.GetWorldSpaceContactPointOn2(0)); 

		if(inBody1.IsSensor() || inBody2.IsSensor()){
			for(auto& i: physic->onTriggerEnterCallbacks) i(*scene, collision);
			
			/*if(inBody1.IsSensor()){
				for(auto& i: physic->onTriggerEnterCallbacks) i(*scene, collision);
			}
			if(inBody2.IsSensor()){
				for(auto& i: physic->onTriggerEnterCallbacks)i(*scene, collision);
			}*/
		} else {
			for(auto& i: physic->onCollisionEnterCallbacks) 
				i(*scene, collision);
		}
	}

	virtual void OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override{
		//cout << "A contact was persisted" << endl;
	}

	virtual void OnContactRemoved(const SubShapeIDPair &inSubShapePair) override{
		return;
		lock_guard lock(mutex);

    	const BodyLockRead lock1(physic->physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), inSubShapePair.GetBody1ID());
		if(!lock1.Succeeded()) return;
	
		const BodyLockRead lock2(physic->physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), inSubShapePair.GetBody2ID());
		if(!lock2.Succeeded()) return;

		const Body& inBody1 = lock1.GetBody();
		const Body& inBody2 = lock2.GetBody();

		Collision collision;
		collision.e1 = static_cast<Entity>(inBody1.GetUserData());
		collision.e2 = static_cast<Entity>(inBody2.GetUserData());

		if(inBody1.IsSensor() || inBody2.IsSensor()){
			//for(auto& i: physic->onTriggerExitCallbacks) i(*scene, static_cast<Entity>(inBody1.GetUserData()), static_cast<Entity>(inBody2.GetUserData()), Vector3Zero);
			if(inBody1.IsSensor()){
				for(auto& i: physic->onTriggerExitCallbacks) i(*scene, collision);
			}
			if(inBody2.IsSensor()){
				for(auto& i: physic->onTriggerExitCallbacks) i(*scene, collision);
			}
		} else {
			for(auto& i: physic->onCollisionExitCallbacks) i(*scene, collision);
		}
	}
};

class PhysicObject{
public:
    BodyID bodyID = BodyID();
	PhysicsWorld* world = nullptr;
};

class JointObject{
public:
	Constraint* constraint = nullptr;
};

class VehiclePhysicData{
public:
	VehicleConstraint* vehicleConstraint = nullptr;
};

struct RagdollObject{
	JPH::Ref<Ragdoll> ragdoll;
	PhysicsWorld* world = nullptr;
};

class SelectedBodyDrawFilter: public JPH::BodyDrawFilter{
public:	
	std::unordered_set<JPH::BodyID> selectedBodies;

	void UpdateSelected(){
		selectedBodies.clear();
		Ref<Scene> scene = SceneManager::Get().GetActiveScene();
		if(scene == nullptr) return;

		Editor* editor = Application::GetModuleByType<Editor>();
		if(editor == nullptr) return;
		
		for(auto& e: editor->GetSelectedEntities()){
			if(scene->HasComponent<RigidbodyComponent>(e)){
				RigidbodyComponent& rb = scene->GetComponent<RigidbodyComponent>(e);
				if(rb.data == nullptr) continue;

				selectedBodies.insert(rb.data->bodyID);
			}

			if(scene->HasComponent<RagdollComponent>(e)){
				RagdollComponent& rb = scene->GetComponent<RagdollComponent>(e);
				if(rb.data == nullptr) continue;

				for(auto& i: rb.data->ragdoll->GetBodyIDs()){
					selectedBodies.insert(i);
				}
			}
		}
	}

	bool ShouldDraw(const JPH::Body& inBody) const override {
		//return true;
		return selectedBodies.count(inBody.GetID());
	}
};

//constexpr float fixedTimeStep = 1.0f / 60.0f; // 60 Hz physics update

#pragma endregion

#pragma region RagdollComponent

void RagdollComponent::OnGui(Entity& e, Scene& scene){
	RagdollComponent& ragdoll = scene.GetComponent<RagdollComponent>(e);

	ImGui::Checkbox("SyncWithFinalPose", &ragdoll.syncWithFinalPose);
	ImGui::Checkbox("SyncFromTheHips", &ragdoll.syncFromTheHips);
	ImGui::Checkbox("UseTorqueControl", &ragdoll.useTorqueControl);

	ImGui::DragFloat("GlobalMass", &ragdoll.globalMass);
	ImGui::DragFloat("LinearDamping", &ragdoll.linearDamping);

	ImGui::DragFloat("Gain", &ragdoll.gain);
	ImGui::DragFloat("Damping", &ragdoll.damping);
	ImGui::DragFloat("Stiffness", &ragdoll.stiffness);
	ImGui::Spacing();

    //ImGui::DrawEnumCombo<RagdollComponent::Type>("Type", &ragdoll.type);
	
	if(ImGui::DrawEnumCombo<RagdollComponent::Type>("Type", &ragdoll.type)){
		ragdoll.SetType(ragdoll.type);
	}

	ImGui::DrawEnumCombo<RagdollComponent::MotorType>("MotorType", &ragdoll.motorType);

	ImGui::Checkbox("Interpolate", &ragdoll.interpolate);
	//ImGui::DrawEnumCombo<Layers>("Layer", &ragdoll.layer);
	ImGui::DrawLayer("Layer", ragdoll.layer, GetGlobalSceneData().layerNames, true);
	ImGui::DrawLayerMask("Mask", ragdoll.mask);
	ImGui::Spacing();

	Skeleton* skeleton = nullptr;
	if(scene.HasComponent<SkinnedModelRendererComponent>(e)){
		SkinnedModelRendererComponent& skinned = scene.GetComponent<SkinnedModelRendererComponent>(e);
		if(skinned.GetModel() != nullptr) skeleton = &skinned.GetModel()->skeleton; 
	}

    int index = 0;
    for(auto& part : ragdoll.parts){
        ImGui::PushID(index);
        if(ImGui::TreeNodeEx(("Part " + std::to_string(index)).c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth)){
            //Vector3 oldPos = part.pos;
            //Quaternion oldRot = part.rot;

			/*if (ImGui::DragInt("Parent Index", &part.parent, 1))
                ragdoll.isDirty = true;

            if (ImGui::DragInt("Skinned Index", &part.skinnedSkeletonIndex, 1))
                ragdoll.isDirty = true;*/

			if(skeleton){
				auto& boneNames = skeleton->GetJointNames();
				int boneCount = static_cast<int>(boneNames.size());

				// Create C-string array from std::string list
				std::vector<const char*> boneNameCStrs;
				boneNameCStrs.reserve(boneNames.size());
				for(auto& name : boneNames)
					boneNameCStrs.push_back(name.c_str());

				// Parent Bone DropDown
				int currentParent = part.parent;
				int partCount = static_cast<int>(ragdoll.parts.size());
				if(currentParent >= partCount || currentParent < -1)
					currentParent = -1;

				std::string labelParent = "Parent Part##" + std::to_string(index);
				std::string previewParent = (currentParent >= 0 && currentParent < partCount)
					? ("[" + std::to_string(currentParent) + "] Part") : "<None>";

				if(ImGui::BeginCombo(labelParent.c_str(), previewParent.c_str())){
					if(ImGui::Selectable("<None>", currentParent == -1)){
						part.parent = -1;
						ragdoll.isDirty = true;
					}

					for(int i = 0; i < partCount; ++i){
						if (i == index) continue; // evita selecionar a si mesmo como pai
						std::string name = "[" + std::to_string(i) + "] Part";
						bool selected = (part.parent == i);
						if(ImGui::Selectable(name.c_str(), selected)){
							part.parent = i;
							ragdoll.isDirty = true;
						}
					}

					ImGui::EndCombo();
				}

				// Skinned Bone DropDown
				int currentSkin = part.skinnedSkeletonIndex;
				if(currentSkin >= boneCount || currentSkin < -1)
					currentSkin = -1;

				std::string labelSkin = "Skinned Bone##" + std::to_string(index);
				std::string previewSkin = (currentSkin >= 0 && currentSkin < boneCount) 
					? boneNames[currentSkin] : "<None>";

				if(ImGui::BeginCombo(labelSkin.c_str(), previewSkin.c_str())){
					if(ImGui::Selectable("<None>", currentSkin == -1)){
						part.skinnedSkeletonIndex = -1;
						ragdoll.isDirty = true;
					}

					for(int i = 0; i < boneCount; ++i){
						bool selected = (part.skinnedSkeletonIndex == i);
						if(ImGui::Selectable(boneNames[i].c_str(), selected)){
							part.skinnedSkeletonIndex = i;
							ragdoll.isDirty = true;
						}
					}

					ImGui::EndCombo();
				}

			} else {
				// Fallback
				if(ImGui::DragInt("Parent Index", &part.parent, 1))
					ragdoll.isDirty = true;
				if(ImGui::DragInt("Skinned Index", &part.skinnedSkeletonIndex, 1))
					ragdoll.isDirty = true;
			}

			ImGui::DrawEnumCombo<RagdollComponent::Part::OverrideType>("OverrideType", &part.overrideType);

			if(ragdoll.syncWithFinalPose){
				if(ImGui::Checkbox("DisableSync", &part.disableSync))
                	ragdoll.isDirty = true;
				if(ImGui::Checkbox("IsHips", &part.isHips))
                	ragdoll.isDirty = true;
			}

			if(ImGui::DragFloat("stiffnessMult", &part.stiffnessMult)){
				ragdoll.isDirty = true;
			}

			if(ImGui::DragFloat("overrideLinearDamping", &part.overrideLinearDamping)){
				ragdoll.isDirty = true;
			}

			auto ExtractFreezeStates = 
			[](JPH::EAllowedDOFs allowedDOFs, 
				bool& freezePosX, bool& freezePosY, bool& freezePosZ,
				bool& freezeRotX, bool& freezeRotY, bool& freezeRotZ)
			{
				// A DOF is frozen if it is NOT set in allowedDOFs
				freezePosX = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::TranslationX) == 0);
				freezePosY = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::TranslationY) == 0);
				freezePosZ = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::TranslationZ) == 0);
				freezeRotX = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::RotationX) == 0);
				freezeRotY = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::RotationY) == 0);
				freezeRotZ = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::RotationZ) == 0);
			};

			if(ImGui::CollapsingHeader("Position Constraints")){
				// Extract freeze states
				bool freezePosX, freezePosY, freezePosZ, freezeRotX, freezeRotY, freezeRotZ;
				ExtractFreezeStates(static_cast<JPH::EAllowedDOFs>(part.constraints), freezePosX, freezePosY, freezePosZ, freezeRotX, freezeRotY, freezeRotZ);

				bool changed = false;
				if(ImGui::Checkbox("Freeze Position X", &freezePosX)) changed = true;
				if(ImGui::Checkbox("Freeze Position Y", &freezePosY)) changed = true;
				if(ImGui::Checkbox("Freeze Position Z", &freezePosZ)) changed = true;
				if(ImGui::Checkbox("Freeze Rotation X", &freezeRotX)) changed = true;
				if(ImGui::Checkbox("Freeze Rotation Y", &freezeRotY)) changed = true;
				if(ImGui::Checkbox("Freeze Rotation Z", &freezeRotZ)) changed = true;

				// Update DOFs if any checkbox changed
				if(changed){
					JPH::EAllowedDOFs newDOFs = JPH::EAllowedDOFs::None;
					// Enable DOFs for non-frozen axes
					if (!freezePosX) newDOFs |= JPH::EAllowedDOFs::TranslationX;
					if (!freezePosY) newDOFs |= JPH::EAllowedDOFs::TranslationY;
					if (!freezePosZ) newDOFs |= JPH::EAllowedDOFs::TranslationZ;
					if (!freezeRotX) newDOFs |= JPH::EAllowedDOFs::RotationX;
					if (!freezeRotY) newDOFs |= JPH::EAllowedDOFs::RotationY;
					if (!freezeRotZ) newDOFs |= JPH::EAllowedDOFs::RotationZ;

					ragdoll.Constraints(index, static_cast<RigidbodyConstraints>(newDOFs));
				}
			}

            /*if(ImGui::DragFloat3("Position", &part.pos.x, 0.01f))
                ragdoll.isDirty = true;

            if(ImGui::DragFloat4("Rotation (Quat)", &part.rot.x, 0.01f))
                ragdoll.isDirty = true;*/

			if(ImGui::TreeNode("Limits")){
				if(ImGui::DragFloat3("Constraint Pos", &part.constraintPos.x, 0.01f))
					ragdoll.isDirty = true;

				if(ImGui::DragFloat3("Twist Axis", &part.twistAxis.x, 0.01f))
					ragdoll.isDirty = true;

				if(ImGui::DragFloat("Twist Min", &part.twistAngleMin, 0.1f))
					ragdoll.isDirty = true;

				if(ImGui::DragFloat("Twist Max", &part.twistAngleMax, 0.1f))
					ragdoll.isDirty = true;

				if(ImGui::DragFloat("Normal Angle", &part.normalAngle, 0.1f))
					ragdoll.isDirty = true;

				if(ImGui::DragFloat("Plane Angle", &part.planeAngle, 0.1f))
					ragdoll.isDirty = true;

				ImGui::TreePop();
			}

            if(ImGui::TreeNode("Collision Shape")){
                auto prevType = part.shape.type;
                if(ImGui::DrawEnumCombo<CollisionShape::Type>("Shape Type", &part.shape.type)){
                    if(part.shape.type != prevType)
                        ragdoll.isDirty = true;
                }

                if(ImGui::DragFloat3("Center", &part.shape.center.x, 0.01f))
                    ragdoll.isDirty = true;

				if(ImGui::DragFloat3("Rotation", &part.shape.rotation.x, 0.01f))
                    ragdoll.isDirty = true;

                if(part.shape.type == CollisionShape::Type::Box){
                    if(ImGui::DragFloat3("Size", &part.shape.size.x, 0.01f))
                        ragdoll.isDirty = true;
                } else if(part.shape.type == CollisionShape::Type::Sphere){
                    if (ImGui::DragFloat("Radius", &part.shape.radius, 0.01f))
                        ragdoll.isDirty = true;
                } else if(part.shape.type == CollisionShape::Type::Capsule){
                    if(ImGui::DragFloat("Radius", &part.shape.radius, 0.01f))
                        ragdoll.isDirty = true;
                    if(ImGui::DragFloat("Height", &part.shape.height, 0.01f))
                        ragdoll.isDirty = true;
                }

                ImGui::TreePop();
            }
			ImGui::TreePop();
        }
        ImGui::PopID();
        index++;
    }

    if(ImGui::Button("Add Part")){
        ragdoll.parts.emplace_back();
        ragdoll.isDirty = true;
    }

	ImGui::Checkbox("IsDirty", &ragdoll.isDirty);
}

float RagdollComponent::Mass(int boneIndex){
	const BodyLockRead lock(data->world->physicsSystem.GetBodyLockInterfaceNoLock(), data->ragdoll->GetBodyIDs()[boneIndex]);
	const Body &body = lock.GetBody();
	float mass = body.GetMotionProperties()->GetInverseMass() > 0.0f ? 1.0f / body.GetMotionProperties()->GetInverseMass() : 0.0f;
	return mass;
}

Vector3 RagdollComponent::CenterOfMass(int boneIndex){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return Vector3Zero;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetCenterOfMassPosition(bodyID));
}

Vector3 RagdollComponent::Position(int boneIndex){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return Vector3Zero;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetPosition(bodyID));
}

Vector3 RagdollComponent::PositionInterpoled(int boneIndex){
	if constexpr(EnableInterpolation == false) return Position(boneIndex); 

	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return Vector3Zero;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	
	float alpha = data->world->system->InterpolationAlpha();
	Vector3 interpolatedPos = math::mix(parts[boneIndex].previousPosition, FromJolt(bodyInterface.GetPosition(bodyID)), alpha);
	return interpolatedPos;
}

void RagdollComponent::Position(int boneIndex, Vector3 position){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.SetPosition(bodyID, ToJolt(position), EActivation::Activate);
}

Quaternion RagdollComponent::Rotation(int boneIndex){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return QuaternionIdentity;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetRotation(bodyID));
}

void RagdollComponent::Rotation(int boneIndex, Quaternion rotation){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.SetRotation(bodyID, ToJolt(rotation), EActivation::Activate);
}

Vector3 RagdollComponent::Velocity(int boneIndex){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return Vector3Zero;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetLinearVelocity(bodyID));
}

void RagdollComponent::Velocity(int boneIndex, Vector3 v){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.SetLinearVelocity(bodyID, ToJolt(v));
}

Vector3 RagdollComponent::AngularVelocity(int boneIndex){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return Vector3Zero;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetAngularVelocity(bodyID));
}

void RagdollComponent::AngularVelocity(int boneIndex, Vector3 v){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.SetAngularVelocity(bodyID, ToJolt(v));
}

void RagdollComponent::ApplyForce(int boneIndex, Vector3 v){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.AddForce(bodyID, ToJolt(v));
}

void RagdollComponent::ApplyTorque(int boneIndex, Vector3 v){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.AddTorque(bodyID, ToJolt(v));
}

void RagdollComponent::ApplyImpulse(int boneIndex, Vector3 v){
	if(data == nullptr || boneIndex < 0 || boneIndex >= data->ragdoll->GetBodyIDs().size()) return;

	BodyID bodyID = data->ragdoll->GetBodyIDs()[boneIndex];
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.AddImpulse(bodyID, ToJolt(v));
}

void RagdollComponent::AddExplosionImpulse(float force, Vector3 explosionPosition, float radius, float upwardsModifier){
	if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();

	for(int i = 0; i < data->ragdoll->GetBodyIDs().size(); i++){
		JPH::Vec3 explosionCenter = ToJolt(explosionPosition);

		// Get the body's center of mass
		JPH::Vec3 centerOfMass = bodyInterface.GetCenterOfMassPosition(data->ragdoll->GetBodyIDs()[i]);

		// Calculate distance from explosion center to center of mass
		JPH::Vec3 direction = centerOfMass - explosionCenter;
		float distance = direction.Length();

		// Skip bodies outside the radius
		if(distance > radius) return;

		// Handle case where body is at the explosion center
		if(distance < 0.0001f){
			direction = JPH::Vec3(0, 1, 0); // Default to up direction (assuming Y is up)
			distance = 0.0f;
		} else {
			direction /= distance; // Normalize direction
		}

		// Apply upwards modifier
		if (upwardsModifier != 0.0f) {
			JPH::Vec3 up = JPH::Vec3(0, 1, 0); // Assuming Y is up
			direction += up * upwardsModifier;
			direction = direction.NormalizedOr(up); // Normalize or fallback to up if zero
		}

		// Calculate impulse magnitude with linear falloff
		float distanceFactor = JPH::Clamp(1.0f - (distance / radius), 0.0f, 1.0f);
		float impulseMagnitude = (force) * distanceFactor;

		// Calculate impulse vector
		JPH::Vec3 impulse = direction * impulseMagnitude;

		// 🔎 Print the mass of this body
		/*const BodyLockRead lock(data->world->physicsSystem.GetBodyLockInterfaceNoLock(), data->ragdoll->GetBodyIDs()[i]);
		const Body &body = lock.GetBody();
		float mass = body.GetMotionProperties()->GetInverseMass() > 0.0f ? 1.0f / body.GetMotionProperties()->GetInverseMass() : 0.0f;
		LogInfo("Body %d mass = %f\n", i, mass);*/

		// Apply impulse to the body's center of mass
		bodyInterface.AddImpulse(data->ragdoll->GetBodyIDs()[i], impulse, centerOfMass);
	}
}

RigidbodyConstraints RagdollComponent::Constraints(int boneIndex){
	return parts[boneIndex].constraints;
}

void RagdollComponent::Constraints(int boneIndex, RigidbodyConstraints constraints){
	if(parts[boneIndex].constraints == constraints) return;

	parts[boneIndex].constraints = constraints;
	
	//UpdateSettings();

	if(data == nullptr) return;
	BodyInterface& bodyInterface = data->world->physicsSystem.GetBodyInterface();
    BodyLockWrite lock(data->world->physicsSystem.GetBodyLockInterfaceNoLock(), data->ragdoll->GetBodyID(boneIndex));
    Assert(lock.Succeeded());

    Body& body = lock.GetBody();
    MotionProperties* motionProps = body.GetMotionProperties();
	motionProps->SetMassProperties(static_cast<JPH::EAllowedDOFs>(constraints), body.GetShape()->GetMassProperties());
}

RagdollComponent::Type RagdollComponent::GetType(){
	return type;
}

void RagdollComponent::SetType(Type intype){
	type = intype;
	isDirty = true;

	/*if(data == nullptr) return;
	physicSystem->RemoveRagdoll(entity, *this);
	physicSystem->AddRagdoll(
		entity, *this, 
		scene->GetComponent<TransformComponent>(entity),
		scene->GetComponent<InfoComponent>(entity),
		scene->GetComponent<SkinnedModelRendererComponent>(entity)
	);*/
}

void RagdollComponent::UpdateInternalData(TransformComponent& trans, InfoComponent& info, SkinnedModelRendererComponent& skinned){
	if(physicSystem == nullptr) return;
	physicSystem->RemoveRagdoll(entity, *this);
	physicSystem->AddRagdoll(
		entity, *this, 
		trans, info, skinned
	);
}

RagdollSettings* CreateRagdollSettings(InfoComponent& info, TransformComponent& trans, RagdollComponent& ragdoll, Skeleton& skinnedSkeleton, JPH::GroupFilter* filter, Pose* customSetupPose = nullptr){
	auto GetShape = [](CollisionShape shape) -> Shape* {
		if(shape.type == CollisionShape::Type::Box){
			auto* r = new BoxShape(ToJolt(shape.size * 0.5f));
			return r;
		}
		if(shape.type == CollisionShape::Type::Sphere){
			auto* r = new SphereShape(shape.radius);
			return r;
		}
		if(shape.type == CollisionShape::Type::Capsule){
			auto* r = new CapsuleShape(shape.height * 0.5f, shape.radius);
			return r;
		}
		
		Assert(false);
		return nullptr;
	};

	auto GetShape2 = [](CollisionShape shape, float mass) -> JPH::Ref<JPH::Shape> {
		if(shape.type == CollisionShape::Type::Box){
			BoxShapeSettings s(ToJolt(shape.size * 0.5f));
			s.SetDensity(mass);
			return s.Create().Get();
		}
		if(shape.type == CollisionShape::Type::Sphere){
			SphereShapeSettings s(shape.radius);
			s.SetDensity(mass);
			return s.Create().Get();
		}
		if(shape.type == CollisionShape::Type::Capsule){
			CapsuleShapeSettings s(shape.height * 0.5f, shape.radius);
			s.SetDensity(mass);
			return s.Create().Get();
		}
		
		Assert(false);
		return nullptr;
	};

	JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;

	for(int i = 0; i < ragdoll.parts.size(); i++){
		if(ragdoll.parts[i].skinnedSkeletonIndex < 0) continue;
		Assert(ragdoll.parts[i].skinnedSkeletonIndex >= 0);
		//Assert(ragdoll.parts[i].parent > 0);

		if(ragdoll.parts[i].parent >= 0/* && ragdoll.type != RagdollComponent::Type::Trigger*/){
			skeleton->AddJoint(skinnedSkeleton.GetJointName(ragdoll.parts[i].skinnedSkeletonIndex), ragdoll.parts[i].parent);
		} else {
			skeleton->AddJoint(skinnedSkeleton.GetJointName(ragdoll.parts[i].skinnedSkeletonIndex));
		}
	}

	Pose& setupPose = skinnedSkeleton.GetRestPose();
	if(customSetupPose != nullptr) setupPose = *customSetupPose;
	ragdoll.startPose = setupPose;

	// Create ragdoll settings
	RagdollSettings *settings = new RagdollSettings;
	settings->mSkeleton = skeleton;
	settings->mParts.resize(skeleton->GetJointCount());
	for(int p = 0; p < skeleton->GetJointCount(); ++p){	
		auto shapes = GetShape(ragdoll.parts[p].shape);
		Transform boneTrans = setupPose.GetGlobalTransform(ragdoll.parts[p].skinnedSkeletonIndex);
		auto positions = ToJolt(trans.TransformPoint(boneTrans.Position()/* + ragdoll.parts[p].shape.center*/));
		auto rotations = ToJolt(math::quat_cast(trans.GetLocalModelMatrix()) * boneTrans.Rotation()); //ToJolt(trans.Rotation() * boneTrans.LocalRotation());
		auto constraint_positions = ToJolt(trans.TransformPoint(boneTrans.TransformPoint(ragdoll.parts[p].constraintPos)));

		auto twist_axis = ToJolt(trans.TransformDirection(math::normalizeSafe(ragdoll.parts[p].twistAxis)));
		auto planeAxisWorld = ToJolt(trans.TransformDirection(FromJolt(Vec3::sAxisZ()))); //TODO: Make this editable in the ragdoll part

		//auto twist_axis = ToJolt(trans.TransformDirection(boneTrans.TransformDirection(ragdoll.parts[p].twistAxis)));
		//auto planeAxisWorld = ToJolt(trans.TransformDirection(boneTrans.TransformDirection({0, 0, 1})));
		
		//auto twist_angle = ragdoll.parts[p].twistAngle;
		auto normal_angle = ragdoll.parts[p].normalAngle;
		auto plane_angle = ragdoll.parts[p].planeAngle;

		RotatedTranslatedShapeSettings offsetShapeSettings(ToJolt(ragdoll.parts[p].shape.center), ToJolt(Quaternion(Mathf::Deg2Rad(ragdoll.parts[p].shape.rotation))), shapes);
		RefConst<Shape> finalShape = offsetShapeSettings.Create().Get();

		RagdollSettings::Part &part = settings->mParts[p];
		part.SetShape(finalShape /*shapes*/);

		JPH::MassProperties msp;
		msp.ScaleToMass(ragdoll.globalMass / skeleton->GetJointCount()); //actual mass in kg
		part.mMassPropertiesOverride = msp;
		part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;

		//part.mLinearDamping = ragdoll.parts[p].overrideLinearDamping >= 0 ? ragdoll.parts[p].overrideLinearDamping : ragdoll.linearDamping;
		part.mAngularDamping = ragdoll.parts[p].overrideLinearDamping >= 0 ? ragdoll.parts[p].overrideLinearDamping : ragdoll.linearDamping;
		
		//part.mMassPropertiesOverride.mInertia = finalShape->GetMassProperties().mInertia;
		//part.mMassPropertiesOverride.mMass = finalShape->GetMassProperties().mMass;
		//part.mOverrideMassProperties = EOverrideMassProperties::MassAndInertiaProvided;
		//part.mNumVelocityStepsOverride = 20; //16;
		//part.mNumPositionStepsOverride = 10; //8;
		part.mMotionQuality = EMotionQuality::Discrete;// EMotionQuality::LinearCast;
		part.mPosition = positions;
		part.mRotation = rotations;
		part.mMotionType =  EMotionType::Dynamic;
		if(ragdoll.type == RagdollComponent::Type::Kinematic) part.mMotionType = EMotionType::Kinematic;
		if(ragdoll.type == RagdollComponent::Type::Trigger) part.mMotionType = EMotionType::Kinematic; 
		if(ragdoll.type == RagdollComponent::Type::Static) part.mMotionType = EMotionType::Static; 
		//if(ragdoll.type == RagdollComponent::Type::Dynamic && p == 0) part.mMotionType = EMotionType::Kinematic;
		if(ragdoll.parts[p].overrideType != RagdollComponent::Part::OverrideType::None){
			if(ragdoll.parts[p].overrideType != RagdollComponent::Part::OverrideType::Dynamic) part.mMotionType = EMotionType::Dynamic;
			if(ragdoll.parts[p].overrideType != RagdollComponent::Part::OverrideType::Kinematic) part.mMotionType = EMotionType::Kinematic;
			if(ragdoll.parts[p].overrideType != RagdollComponent::Part::OverrideType::Static) part.mMotionType = EMotionType::Static;
		} else {
			part.mIsSensor = ragdoll.type == RagdollComponent::Type::Trigger;
		}
		part.mObjectLayer = ragdoll.layer; //PhysicsLayers::MOVING;
		/*part.mCollisionGroup = JPH::CollisionGroup(
			filter,
			ragdoll.layer,
			ragdoll.mask.mask // stored in subgroup ID
		);*/
		//part.mAngularDamping = 0;
		//part.mLinearDamping = 0;
		//TODO: Fix this, add the root object id
		//part.mUserData = static_cast<uint64_t>(ragdoll.parts[p].skinnedSkeletonIndex); //static_cast<uint64>(ragdoll.parts[p].skinnedSkeletonIndex);

		/*if(ragdoll.parts[p].parent < 0){
			part.mAllowedDOFs = EAllowedDOFs::RotationY | EAllowedDOFs::TranslationX | EAllowedDOFs::TranslationY | EAllowedDOFs::TranslationZ; 
		}*/

		part.mAllowedDOFs = static_cast<EAllowedDOFs>(ragdoll.parts[p].constraints);
		
		part.mFriction = 1;

		//part.mAngularDamping = 10;

		// First part is the root, doesn't have a parent and doesn't have a constraint
		if(p > 0 /*&& ragdoll.type != RagdollComponent::Type::Trigger*/){
			SwingTwistConstraintSettings *constraint = new SwingTwistConstraintSettings;
			constraint->mDrawConstraintSize = 0.1f;
			constraint->mPosition1 = constraint->mPosition2 = constraint_positions;
			constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis;
			constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = planeAxisWorld;
			constraint->mTwistMinAngle = math::clamp<float>(DegreesToRadians(ragdoll.parts[p].twistAngleMin), -math::pi<float>(), math::pi<float>()); //-DegreesToRadians(twist_angle);
			constraint->mTwistMaxAngle = math::clamp<float>(DegreesToRadians(ragdoll.parts[p].twistAngleMax), -math::pi<float>(), math::pi<float>()); //DegreesToRadians(twist_angle);
			constraint->mNormalHalfConeAngle = math::clamp<float>(DegreesToRadians(normal_angle), -math::pi<float>(), math::pi<float>());
			constraint->mPlaneHalfConeAngle = math::clamp<float>(DegreesToRadians(plane_angle), -math::pi<float>(), math::pi<float>());
			part.mToParent = constraint;
		}
	}

	settings->Stabilize();// Optional: Stabilize the inertia of the limbs
	settings->DisableParentChildCollisions();// Disable parent child collisions so that we don't get collisions between constrained bodies
	settings->CalculateBodyIndexToConstraintIndex();// Calculate the map needed for GetBodyIndexToConstraintIndex()

	return settings;
}

void PhysicsSystem::OnRemoveRagdoll(entt::registry& r, entt::entity e){
	RagdollComponent& ragdoll = r.get<RagdollComponent>(e);
	PhysicsSystem* physicsSystem = r.ctx().get<PhysicsSystem*>();
	physicsSystem->RemoveRagdoll(e, ragdoll);
    
	/*if(ragdoll.data == nullptr) return;
	ragdoll.isDirty = false;
	ragdoll.data->ragdoll->RemoveFromPhysicsSystem();
	delete ragdoll.data->ragdoll;
	delete ragdoll.data;
	ragdoll.data = nullptr;*/
}

void PhysicsSystem::AddRagdoll(Entity entity, RagdollComponent& ragdoll, TransformComponent& trans, InfoComponent& info, SkinnedModelRendererComponent& skinned){
	ragdoll.isDirty = false;
	ragdoll.physicSystem = this;
	ragdoll.scene = scene;
	ragdoll.entity = entity;

	if(ragdoll.type == RagdollComponent::Type::Disable){
		return;
	}
	
	SetJointsAsDirtyIfBodyIsDirty(entity);
    BodyInterface &bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();

	ragdoll.data = new RagdollObject();
	ragdoll.data->world = physicsWorld;
	JPH::Ref<RagdollSettings> settings = CreateRagdollSettings(
		info, trans, ragdoll, 
		skinned.GetModel()->skeleton, 
		nullptr, //physicsWorld->groupFilter, 
		nullptr //skinned.finalPose.Size() > 0 ? &skinned.finalPose : nullptr
	);
	ragdoll.data->ragdoll = settings->CreateRagdoll(/*ragdoll.layer*/ 0, static_cast<uint64>(entity), &physicsWorld->physicsSystem);
	for(int i = 0; i < ragdoll.data->ragdoll->GetBodyCount(); ++i){
		BodyID bodyID = ragdoll.data->ragdoll->GetBodyID(i);
		BodyInterface& bi = bodyInterface; //physicsWorld->physicsSystem.GetBodyInterface();

		// Setar manualmente o CollisionGroup correto
		/*bi.SetCollisionGroup(bodyID, JPH::CollisionGroup(
			physicsWorld->groupFilter,
			ragdoll.layer,
			ragdoll.mask.mask
		));*/

		ragdoll.parts[i].initedRot = FromJolt(bi.GetRotation(bodyID));

		if(skinned.finalPose.Size() > 0){
			auto positions = ToJolt(trans.TransformPoint(skinned.finalPose.GetGlobalTransform(ragdoll.parts[i].skinnedSkeletonIndex).Position()));
			auto rotations = ToJolt(math::quat_cast(trans.GetLocalModelMatrix()) * skinned.finalPose.GetGlobalTransform(ragdoll.parts[i].skinnedSkeletonIndex).Rotation()); 
			bi.SetPositionAndRotation(bodyID, positions, rotations, JPH::EActivation::Activate);
		}

		if(ragdoll.overrideStartVelocity != Vector3Zero){
			bi.SetLinearVelocity(bodyID, ToJolt(ragdoll.overrideStartVelocity));
		}

		Quat rot;
		RVec3 pos;
		bodyInterface.GetPositionAndRotation(bodyID, pos, rot);
		ragdoll.parts[i].previousPosition = FromJolt(pos);
		ragdoll.parts[i].previousRotation = FromJolt(rot);
	}

	for(size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
		BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
		bodyInterface.SetUserData(bodyID, EncodeUserData(static_cast<uint32_t>(entity), p));
		//LogInfo("Set Body %zd UserData to %d", p, ragdoll.parts[p].skinnedSkeletonIndex);
	}
	ragdoll.data->ragdoll->AddToPhysicsSystem(EActivation::Activate);
}

void PhysicsSystem::RemoveRagdoll(Entity entity, RagdollComponent& ragdoll){
	if(ragdoll.data == nullptr) return;

	ragdoll.data->ragdoll->RemoveFromPhysicsSystem();
	//ragdoll.data->ragdoll = nullptr;
	delete ragdoll.data;
	ragdoll.data = nullptr;
	ragdoll.isDirty = true;
}

#pragma endregion

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

	ImGui::Checkbox("interpolate", &rb.interpolate);

    float mass = rb.Mass();
    if(ImGui::DragFloat("mass", &mass)){
        rb.Mass(mass);
    }

	float friction = rb.Friction();
    if(ImGui::DragFloat("friction", &friction)){
        rb.Friction(friction);
    }

	float linearDamping = rb.linearDamping;
	if(ImGui::DragFloat("linearDamping", &linearDamping)){
		rb.LinearDamping(linearDamping);
	}
	float angularDamping = rb.angularDamping;
	if(ImGui::DragFloat("angularDamping", &angularDamping)){
		rb.AngularDamping(angularDamping);
	}

	ImGui::Checkbox("overrideCenterOfMass", &rb.overrideCenterOfMass);
	if(rb.overrideCenterOfMass){
		ImGui::DragFloat3("centerOfMassOffset", &rb.centerOfMass.x);
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

	if(rb.shape.type == CollisionShape::Type::Model){
        bool update = false;

		if(ImGui::DrawAsset<Model>("modelSource", shape.modelSource)){
			update = true;
		}

		if(ImGui::DragInt("modelSourceMeshIndex", &shape.modelSourceMeshIndex)){
			update = true;
		}

		float _center[] = {shape.center.x, shape.center.y, shape.center.z};
        if(ImGui::DragFloat3("center", _center)){
            shape.center = Vector3(_center[0], _center[1], _center[2]);
            update = true;
        }

		if(update){
			shape.meshData = nullptr;// CreateMeshShapeData(*shape.modelSource->meshs[shape.modelSourceMeshIndex]);
			rb.SetShape(shape);
		}
	}

	ImGui::Spacing();

	PhysicMotionQuality motionQuality = rb.MotionQuality();
    if(ImGui::DrawEnumCombo<PhysicMotionQuality>("motionQuality", &motionQuality)){
        rb.MotionQuality(motionQuality);
    }

	auto ExtractFreezeStates = 
	[](JPH::EAllowedDOFs allowedDOFs, 
        bool& freezePosX, bool& freezePosY, bool& freezePosZ,
        bool& freezeRotX, bool& freezeRotY, bool& freezeRotZ)
	{
		// A DOF is frozen if it is NOT set in allowedDOFs
		freezePosX = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::TranslationX) == 0);
		freezePosY = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::TranslationY) == 0);
		freezePosZ = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::TranslationZ) == 0);
		freezeRotX = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::RotationX) == 0);
		freezeRotY = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::RotationY) == 0);
		freezeRotZ = (static_cast<uint8>(allowedDOFs & JPH::EAllowedDOFs::RotationZ) == 0);
	};

	if(ImGui::CollapsingHeader("Position Constraints")){
		// Extract freeze states
   		bool freezePosX, freezePosY, freezePosZ, freezeRotX, freezeRotY, freezeRotZ;
    	ExtractFreezeStates(static_cast<JPH::EAllowedDOFs>(rb.constraints), freezePosX, freezePosY, freezePosZ, freezeRotX, freezeRotY, freezeRotZ);

		bool changed = false;
		if(ImGui::Checkbox("Freeze Position X", &freezePosX)) changed = true;
		if(ImGui::Checkbox("Freeze Position Y", &freezePosY)) changed = true;
		if(ImGui::Checkbox("Freeze Position Z", &freezePosZ)) changed = true;
		if(ImGui::Checkbox("Freeze Rotation X", &freezeRotX)) changed = true;
		if(ImGui::Checkbox("Freeze Rotation Y", &freezeRotY)) changed = true;
		if(ImGui::Checkbox("Freeze Rotation Z", &freezeRotZ)) changed = true;

		// Update DOFs if any checkbox changed
		if(changed){
			JPH::EAllowedDOFs newDOFs = JPH::EAllowedDOFs::None;
			// Enable DOFs for non-frozen axes
			if (!freezePosX) newDOFs |= JPH::EAllowedDOFs::TranslationX;
			if (!freezePosY) newDOFs |= JPH::EAllowedDOFs::TranslationY;
			if (!freezePosZ) newDOFs |= JPH::EAllowedDOFs::TranslationZ;
			if (!freezeRotX) newDOFs |= JPH::EAllowedDOFs::RotationX;
			if (!freezeRotY) newDOFs |= JPH::EAllowedDOFs::RotationY;
			if (!freezeRotZ) newDOFs |= JPH::EAllowedDOFs::RotationZ;

			rb.Constraints(static_cast<RigidbodyConstraints>(newDOFs));
		}
	}
}

void RigidbodyComponent::SetShape(CollisionShape inShape){
    shape = inShape;
	isDirt = true;
}

void RigidbodyComponent::Mass(float m){
	mass = m;
	isDirt = true;
}

void RigidbodyComponent::Friction(float f){
	friction = f;
	isDirt = true;
}

void RigidbodyComponent::SetType(RigidbodyComponent::Type value){
    type = value;
	isDirt = true;
	return;

	if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	EMotionType _type = EMotionType::Dynamic;
    if(type == RigidbodyComponent::Type::Static) _type = EMotionType::Static;
	if(type == RigidbodyComponent::Type::Kinematic) _type = EMotionType::Kinematic;
	if(type == RigidbodyComponent::Type::Trigger) _type = EMotionType::Kinematic;
	bodyInterface.SetMotionType(data->bodyID, _type, EActivation::Activate);
}

void RigidbodyComponent::NeverSleep(bool value){

}

Vector3 RigidbodyComponent::CenterOfMass(){
	if(data == nullptr) return Vector3Zero;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetCenterOfMassPosition(data->bodyID));
}

Vector3 RigidbodyComponent::Position(){
	if(data == nullptr) return Vector3Zero;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	return FromJolt(bodyInterface.GetPosition(data->bodyID));
}

Vector3 RigidbodyComponent::PositionInterpoled(){
	if constexpr(EnableInterpolation == false) return Position();

	if(data == nullptr) return Vector3Zero;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();

	float alpha = data->world->system->InterpolationAlpha(); 
	Vector3 interpolatedPos = math::mix(previousPosition, FromJolt(bodyInterface.GetPosition(data->bodyID)), alpha);
	
	return interpolatedPos;
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

void RigidbodyComponent::SetTransform(const Vector3& pos, const Quaternion& rot){
    if (data == nullptr) return;
    BodyInterface& bodyInterface = data->world->physicsSystem.GetBodyInterface();
    bodyInterface.SetPositionAndRotation(
        data->bodyID,
        ToJolt(pos),    // current position
        ToJolt(rot),    // new rotation
        EActivation::Activate
    );
}

Vector3 RigidbodyComponent::Velocity(){
    if(data == nullptr || type != Type::Dynamic) return Vector3Zero;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterfaceNoLock();
	return FromJolt(bodyInterface.GetLinearVelocity(data->bodyID));
}

void RigidbodyComponent::Velocity(Vector3 v){
    if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterfaceNoLock();
	bodyInterface.SetLinearVelocity(data->bodyID, ToJolt(v));
	//bodyInterface.ActivateBody(data->bodyID);
	
	//bodyInterface.SetFriction(data->bodyID, 0.0f);
	//bodyInterface.SetLinearDamping(data->bodyID, 0.0f);
}

Vector3 RigidbodyComponent::AngularVelocity(){
	if(data == nullptr) return Vector3Zero;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterfaceNoLock();
	return FromJolt(bodyInterface.GetAngularVelocity(data->bodyID));
}

void RigidbodyComponent::AngularVelocity(Vector3 v){
	if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();
	bodyInterface.SetAngularVelocity(data->bodyID, ToJolt(v));
}

void RigidbodyComponent::ApplyForce(Vector3 v){
    if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterfaceNoLock(); //data->world->physicsSystem.GetBodyInterface();
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

void RigidbodyComponent::AddExplosionImpulse(float force, Vector3 explosionPosition, float radius, float upwardsModifier){
	if(data == nullptr) return;
	BodyInterface &bodyInterface = data->world->physicsSystem.GetBodyInterface();

	JPH::Vec3 explosionCenter = ToJolt(explosionPosition);

	// Get the body's center of mass
	JPH::Vec3 centerOfMass = bodyInterface.GetCenterOfMassPosition(data->bodyID);

	// Calculate distance from explosion center to center of mass
	JPH::Vec3 direction = centerOfMass - explosionCenter;
	float distance = direction.Length();

	// Skip bodies outside the radius
	if(distance > radius) return;

	// Handle case where body is at the explosion center
	if(distance < 0.0001f){
		direction = JPH::Vec3(0, 1, 0); // Default to up direction (assuming Y is up)
		distance = 0.0f;
	} else {
		direction /= distance; // Normalize direction
	}

	// Apply upwards modifier
	if (upwardsModifier != 0.0f) {
		JPH::Vec3 up = JPH::Vec3(0, 1, 0); // Assuming Y is up
		direction += up * upwardsModifier;
		direction = direction.NormalizedOr(up); // Normalize or fallback to up if zero
	}

	// Calculate impulse magnitude with linear falloff
	float distanceFactor = JPH::Clamp(1.0f - (distance / radius), 0.0f, 1.0f);
	float impulseMagnitude = force * distanceFactor;

	// Calculate impulse vector
	JPH::Vec3 impulse = direction * impulseMagnitude;

	// Apply impulse to the body's center of mass
	bodyInterface.AddImpulse(data->bodyID, impulse, centerOfMass);
}

void RigidbodyComponent::SetAngularFactor(Vector3 v){
    angularFactor = v;
    if(data == nullptr) return;
	if(type != RigidbodyComponent::Type::Dynamic) return;

    BodyInterface& bodyInterface = data->world->physicsSystem.GetBodyInterface();
    BodyLockWrite lock(data->world->physicsSystem.GetBodyLockInterfaceNoLock(), data->bodyID);
    Assert(lock.Succeeded());

    Body& body = lock.GetBody();
    MotionProperties* motionProps = body.GetMotionProperties();
	//motionProps->SetMassProperties(JPH::EAllowedDOFs::TranslationY, body.GetShape()->GetMassProperties());
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

float RigidbodyComponent::LinearDamping(){
	return linearDamping;
}

void RigidbodyComponent::LinearDamping(float v){
	linearDamping = v;
	if(data == nullptr) return;

	BodyInterface& bodyInterface = data->world->physicsSystem.GetBodyInterfaceNoLock();
    BodyLockWrite lock(data->world->physicsSystem.GetBodyLockInterfaceNoLock(), data->bodyID);
	Assert(lock.Succeeded());

    Body& body = lock.GetBody();
    MotionProperties* motionProps = body.GetMotionProperties();
	motionProps->SetLinearDamping(linearDamping);
}

float RigidbodyComponent::AngularDamping(){
	return angularDamping;
}

void RigidbodyComponent::AngularDamping(float v){
	angularDamping = v;
	if(data == nullptr) return;

	BodyInterface& bodyInterface = data->world->physicsSystem.GetBodyInterface();
    BodyLockWrite lock(data->world->physicsSystem.GetBodyLockInterfaceNoLock(), data->bodyID);
    Assert(lock.Succeeded());

    Body& body = lock.GetBody();
    MotionProperties* motionProps = body.GetMotionProperties();
	motionProps->SetAngularDamping(angularDamping);
}

PhysicMotionQuality RigidbodyComponent::MotionQuality(){
	return motionQuality;
}

void RigidbodyComponent::MotionQuality(PhysicMotionQuality v){
	motionQuality = v;
	isDirt = true;
}

RigidbodyConstraints RigidbodyComponent::Constraints(){
	return constraints;
}

void RigidbodyComponent::Constraints(RigidbodyConstraints inconstraints){
	if(constraints == inconstraints) return;

	constraints = inconstraints;
	
	//UpdateSettings();

	if(data == nullptr) return;
	BodyInterface& bodyInterface = data->world->physicsSystem.GetBodyInterface();
    BodyLockWrite lock(data->world->physicsSystem.GetBodyLockInterfaceNoLock(), data->bodyID);
    Assert(lock.Succeeded());

    Body& body = lock.GetBody();
    MotionProperties* motionProps = body.GetMotionProperties();
	motionProps->SetMassProperties(static_cast<JPH::EAllowedDOFs>(constraints), body.GetShape()->GetMassProperties());
}

void PhysicsSystem::OnRemoveRigidbody(entt::registry& r, entt::entity e){
    RigidbodyComponent& rb = r.get<RigidbodyComponent>(e);
    if(rb.data == nullptr) return;

	if(r.any_of<VehiclePhysic>(e)){
		OnRemoveVehicle(r, e);
	}

    PhysicsSystem* physicsSystem = r.ctx().get<PhysicsSystem*>();
    physicsSystem->RemoveRigidbody(e, rb);
    delete rb.data;
}

void PhysicsSystem::AddRigidbody(Entity entity, RigidbodyComponent& rb, TransformComponent& transform, InfoComponent& info){
	rb.data = new PhysicObject();
	rb.data->world = physicsWorld;
	rb.isDirt = false;

	SetJointsAsDirtyIfBodyIsDirty(entity);

    BodyInterface &bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();

    EMotionType type = EMotionType::Dynamic;
    if(rb.type == RigidbodyComponent::Type::Static) type = EMotionType::Static;
	if(rb.type == RigidbodyComponent::Type::Kinematic) type = EMotionType::Kinematic;
	if(rb.type == RigidbodyComponent::Type::Trigger) type = EMotionType::Kinematic;

    JPH::Ref<Shape> shape = nullptr;
    if(rb.shape.type == CollisionShape::Type::Box){
		BoxShapeSettings shapeSettings(ToJolt(rb.shape.size * 0.5f));
		//shapeSettings.SetDensity(rb.mass);
		shape = shapeSettings.Create().Get();
	} else if(rb.shape.type == CollisionShape::Type::Sphere){
		SphereShapeSettings shapeSettings(rb.shape.radius);
		//shapeSettings.SetDensity(rb.mass);
		shape = shapeSettings.Create().Get();
	} else if(rb.shape.type == CollisionShape::Type::Capsule){
		float halfHeight = (rb.shape.height - 2.0f * rb.shape.radius) * 0.5f;
		if (halfHeight < 0.0f) {
			std::cerr << "Invalid capsule dimensions: height must be at least 2 * radius\n";
			Assert(false);
		}
		CapsuleShapeSettings shapeSettings(halfHeight, rb.shape.radius);
		//shapeSettings.SetDensity(rb.mass);
		shape = shapeSettings.Create().Get();
	} else if(rb.shape.type == CollisionShape::Type::Mesh || rb.shape.type == CollisionShape::Type::Model){
		//JPH::MeshShapeSettings shapeSettings(rb.shape.mesh->joltVertices, rb.shape.mesh->joltTriangles);
		//shapeSettings.SetDensity(rb.mass);
		//shape = shapeSettings.Create().Get();
		//shape = rb.shape.mesh->meshShape;// shapeSettings.Create().Get();

		if(rb.shape.type == CollisionShape::Type::Model && rb.shape.meshData == nullptr){
			Assert(rb.shape.modelSource != nullptr);
			/*Assert(rb.shape.modelSourceMeshIndex < rb.shape.modelSource->meshs.size());
			Assert(rb.shape.modelSourceMeshIndex >= 0);
			rb.shape.mesh = CreateMeshShapeData(*rb.shape.modelSource->meshs[rb.shape.modelSourceMeshIndex]);*/
			if(rb.shape.modelSourceMeshIndex < 0){
				rb.shape.meshData = CreateMeshShapeData(*rb.shape.modelSource);
			} else if(rb.shape.modelSourceMeshIndex < rb.shape.modelSource->meshs.size()){
				rb.shape.meshData = CreateMeshShapeData(*rb.shape.modelSource->meshs[rb.shape.modelSourceMeshIndex]);
			} else {
				Assert(false);
			}
		}

		if(rb.shape.meshData == nullptr) return;
		if(rb.shape.meshData->joltVertices.size() <= 0) return;
		if(rb.shape.meshData->joltTriangles.size() <= 0) return;
		if(rb.shape.meshData->convexPoints.size() <= 0) return;

		Assert(rb.shape.meshData != nullptr);
		Assert(rb.shape.meshData->joltVertices.size() > 0);
		Assert(rb.shape.meshData->joltTriangles.size() > 0);
		Assert(rb.shape.meshData->convexPoints.size() > 0);

		if(rb.type == RigidbodyComponent::Type::Dynamic){
			// Use Convex Hull for dynamic
			JPH::ConvexHullShapeSettings shapeSettings(rb.shape.meshData->convexPoints);
			//shapeSettings.SetDensity(rb.mass);

			auto result = shapeSettings.Create();
			if(!result.HasError()){
				shape = result.Get();
			} else {
				std::cerr << "ConvexHullShape creation error: " << result.GetError() << std::endl;
				return;
			}
		} else {
			// Use MeshShape for static or kinematic
			JPH::MeshShapeSettings shapeSettings(rb.shape.meshData->joltVertices, rb.shape.meshData->joltTriangles);
			shapeSettings.SetEmbedded();
			auto result = shapeSettings.Create();
			if(!result.HasError()){
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

	// Apply local collider offset
	if(rb.overrideCenterOfMass){
		//Vec3 offset = finalShape->GetCenterOfMass() - ToJolt(rb.centerOfMass);
		Vec3 offset = (Vec3(0, 0, 0) - finalShape->GetCenterOfMass()) + ToJolt(rb.centerOfMass);
		finalShape = new OffsetCenterOfMassShape(finalShape, offset);

		auto cm = finalShape->GetCenterOfMass();
		LogInfo("CenterOfMass: (%f, %f, %f)", cm.GetX(), cm.GetY(), cm.GetZ());
	}

	BodyCreationSettings settings(
        finalShape, ToJolt(transform.Position()), ToJolt(transform.Rotation()), type, info.layer //PhysicsLayers::MOVING 
    );
	settings.mLinearDamping = rb.linearDamping;
	settings.mAngularDamping = rb.angularDamping;
	settings.mAllowedDOFs = static_cast<EAllowedDOFs>(rb.constraints);
	settings.mUserData = EncodeUserData(static_cast<uint32_t>(entity), -1);// static_cast<uint64>(entity); // safe cast
	/*settings.mCollisionGroup = JPH::CollisionGroup(//Deprecated
		physicsWorld->groupFilter,
        info.layer,
        rb.mask.mask // stored in subgroup ID
    );*/
	//settings.mMotionQuality = rb.motionQuality == PhysicMotionQuality::LinearCast ? EMotionQuality::LinearCast : EMotionQuality::Discrete;
	//settings.mMotionQuality = EMotionQuality::LinearCast;
	
	/*settings.mNumVelocityStepsOverride = 50;
	settings.mNumPositionStepsOverride = 50;
	*/

	settings.mFriction = rb.friction;

	JPH::MassProperties msp;
	msp.ScaleToMass(rb.mass); //actual mass in kg
	settings.mMassPropertiesOverride = msp;
	settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;

	settings.mIsSensor = rb.type == RigidbodyComponent::Type::Trigger;

    rb.data->bodyID = bodyInterface.CreateAndAddBody(settings, rb.type == RigidbodyComponent::Type::Dynamic ? EActivation::Activate : EActivation::DontActivate);
	// Verify body creation
    if (!bodyInterface.IsAdded(rb.data->bodyID)) {
        LogError("Failed to add body for entity %s", info.name.c_str());
        return;
    }

	rb.SetAngularFactor(rb.angularFactor);
}

void PhysicsSystem::RemoveRigidbody(Entity entity, RigidbodyComponent& rb){
	if(rb.data == nullptr) return;

	SetJointsAsDirtyIfBodyIsDirty(entity);

	BodyInterface &bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();
    bodyInterface.RemoveBody(rb.data->bodyID);
    bodyInterface.DestroyBody(rb.data->bodyID);

	delete rb.data;
	rb.data = nullptr;
	rb.isDirt = true;
}

#pragma endregion

#pragma region JointComponent

void JointComponent::OnGui(Entity& e, Scene& scene){
	JointComponent& c = scene.GetComponent<JointComponent>(e);

	ImGui::DragScalar("bodyA", ImGuiDataType_U32, &c.bodyA);
	ImGui::DragScalar("bodyASubIndex", ImGuiDataType_S32, &c.bodyASubIndex);

	ImGui::DragScalar("bodyB", ImGuiDataType_U32, &c.bodyB);
	ImGui::DragScalar("bodyBSubIndex", ImGuiDataType_S32, &c.bodyBSubIndex);

	ImGui::DrawEnumCombo<JointSpace>("jointSpace", &c.jointSpace);

	ImGui::DrawEnumCombo<JointComponent::Type>("type", &c.type);
	
	if(c.type == JointComponent::Type::Fixed){
		if(ImGui::DragFloat3("point1", &c.fixedSettings.point1.x)) c.CreateFixed(c.fixedSettings);
		if(ImGui::DragFloat3("point2", &c.fixedSettings.point2.x)) c.CreateFixed(c.fixedSettings);
	}

	if(c.type == JointComponent::Type::Distance){
		if(ImGui::DragFloat3("point1", &c.distanceSettings.point1.x)) c.CreateDistance(c.distanceSettings);
		if(ImGui::DragFloat3("point2", &c.distanceSettings.point2.x)) c.CreateDistance(c.distanceSettings);
		if(ImGui::DragFloat("minDistance", &c.distanceSettings.minDistance)) c.SetDistance(c.distanceSettings.minDistance, c.distanceSettings.maxDistance);
		if(ImGui::DragFloat("maxDistance", &c.distanceSettings.maxDistance)) c.SetDistance(c.distanceSettings.minDistance, c.distanceSettings.maxDistance);
		if(ImGui::DragFloat("springFequency", &c.distanceSettings.springFequency)) c.CreateDistance(c.distanceSettings);
		if(ImGui::DragFloat("springDamping", &c.distanceSettings.springDamping)) c.CreateDistance(c.distanceSettings);
	}
}

void JointComponent::SetTargets(Entity inbodyA, int inbodyASubIndex, Entity inbodyB, int inbodyBSubIndex){
	bodyA = inbodyA;
	bodyASubIndex = inbodyASubIndex;
	bodyB = inbodyB;
	bodyBSubIndex = inbodyBSubIndex;
	isDirty = true;
}

JointSpace JointComponent::GetJointSpace(){
	return jointSpace;
}

void JointComponent::SetJointSpace(JointSpace injointSpace){
	jointSpace = injointSpace;
	isDirty = true;
}

void JointComponent::CreateFixed(const FixedSettings& settings){
	isDirty = true;
	type = JointComponent::Type::Fixed;
	fixedSettings = settings;
}

void JointComponent::CreateDistance(const DistanceSettings& settings){
	isDirty = true;
	type = JointComponent::Type::Distance;
	distanceSettings = settings;
}

void JointComponent::SetDistance(float min, float max){
	distanceSettings.minDistance = min;
	distanceSettings.maxDistance = max;

	if(type != JointComponent::Type::Distance) return;
	if(data == nullptr) return;

	DistanceConstraint* c = static_cast<DistanceConstraint*>(data->constraint);
	c->SetDistance(min, max);
}

Vector3 JointComponent::GetWorldSpacePoint1Pos(){
	if(type != JointComponent::Type::Distance) return Vector3Zero;
	if(data == nullptr) return Vector3Zero;

	DistanceConstraint* constraint = static_cast<DistanceConstraint*>(data->constraint);
	RMat44 transform = constraint->GetBody1()->GetCenterOfMassTransform();
	Vec3 localAnchor = constraint->GetConstraintToBody1Matrix().GetTranslation();

	//return FromJolt(transform.GetTranslation());
	return FromJolt(transform * localAnchor);
	//return FromJolt(localAnchor);
}

Vector3 JointComponent::GetWorldSpacePoint2Pos(){
	if(type != JointComponent::Type::Distance) return Vector3Zero;
	if(data == nullptr) return Vector3Zero;

	DistanceConstraint* constraint = static_cast<DistanceConstraint*>(data->constraint);
	RMat44 transform = constraint->GetBody2()->GetCenterOfMassTransform();
	Vec3 localAnchor = constraint->GetConstraintToBody2Matrix().GetTranslation();

	//return FromJolt(transform.GetTranslation());
	return FromJolt(transform * localAnchor);
	//return FromJolt(localAnchor);
}

void PhysicsSystem::OnRemoveJoint(entt::registry& r, entt::entity e){
    JointComponent& c = r.get<JointComponent>(e);
    if(c.data == nullptr) return;

    PhysicsSystem* physicsSystem = r.ctx().get<PhysicsSystem*>();
    physicsSystem->RemoveJoint(e, c);
}

void PhysicsSystem::AddJoint(Scene* scene, Entity entity, JointComponent& joint, TransformComponent& trans, InfoComponent& info){
	auto getBodyId = [&](Entity target, int subBodyIndex){
		if(subBodyIndex >= 0 && scene->HasComponent<RagdollComponent>(target)){
			return scene->GetComponent<RagdollComponent>(target).data->ragdoll->GetBodyID(subBodyIndex);
		} else if(scene->HasComponent<RigidbodyComponent>(target)){
			return scene->GetComponent<RigidbodyComponent>(target).data->bodyID;
		}

		return BodyID(BodyID::cInvalidBodyID);
	};

	BodyID idA = getBodyId(joint.bodyA, joint.bodyASubIndex);
	BodyID idB = getBodyId(joint.bodyB, joint.bodyBSubIndex);

	Assert(idA.IsInvalid() == false);
	Assert(idB.IsInvalid() == false);

	if(idA.IsInvalid() == true) return;
	if(idB.IsInvalid() == true) return;

	joint.data = new JointObject();

	BodyLockWrite lockA(physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), idA);
	Assert(lockA.Succeeded());
	JPH::Body& body1 = lockA.GetBody();

	BodyLockWrite lockB(physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), idB);
	Assert(lockB.Succeeded());
	JPH::Body& body2 = lockB.GetBody();

	if(joint.type == JointComponent::Type::Fixed){
		FixedConstraintSettings fixedSettings;

		if(joint.jointSpace == JointSpace::WorldSpace){
			fixedSettings.mPoint1 = ToJolt(trans.TransformPoint(joint.distanceSettings.point1));
			fixedSettings.mPoint2 = ToJolt(trans.TransformPoint(joint.distanceSettings.point2));
			fixedSettings.mSpace = EConstraintSpace::WorldSpace;
		} else {
			fixedSettings.mPoint1 = ToJolt(joint.distanceSettings.point1);
			fixedSettings.mPoint2 = ToJolt(joint.distanceSettings.point2);
			fixedSettings.mSpace = EConstraintSpace::LocalToBodyCOM;
		}

		FixedConstraint* c = new FixedConstraint(body1, body2, fixedSettings);
		joint.data->constraint = c;
	}

	if(joint.type == JointComponent::Type::Distance){
		DistanceConstraintSettings distanceSettings;

		if(joint.jointSpace == JointSpace::WorldSpace){
			/*distanceSettings.mPoint1 = ToJolt(trans.TransformPoint(joint.distanceSettings.point1));
			distanceSettings.mPoint2 = ToJolt(trans.TransformPoint(joint.distanceSettings.point2));*/
			distanceSettings.mPoint1 = ToJolt(joint.distanceSettings.point1);
			distanceSettings.mPoint2 = ToJolt(joint.distanceSettings.point2);
			distanceSettings.mSpace = EConstraintSpace::WorldSpace;

			distanceSettings.mLimitsSpringSettings.mFrequency = joint.distanceSettings.springFequency;// 5.0f;
			distanceSettings.mLimitsSpringSettings.mDamping = joint.distanceSettings.springDamping;// 0.9f*2;

		} else {
			Vec3 worldP1 = ToJolt(trans.TransformPoint(joint.distanceSettings.point1));// Get world-space points (the way user or editor defines them)
			Vec3 worldP2 = ToJolt(trans.TransformPoint(joint.distanceSettings.point2));
			Vec3 localP1 = body1.GetInverseCenterOfMassTransform() * worldP1;// Convert to local COM-space relative to each body
			Vec3 localP2 = body2.GetInverseCenterOfMassTransform() * worldP2;

			distanceSettings.mPoint1 = localP1;
			distanceSettings.mPoint2 = localP2;
			distanceSettings.mSpace = EConstraintSpace::LocalToBodyCOM;

			distanceSettings.mLimitsSpringSettings.mFrequency = joint.distanceSettings.springFequency; //5.0f;
			distanceSettings.mLimitsSpringSettings.mDamping = joint.distanceSettings.springDamping; //0.9f;

			/*float dist = (worldP1 - worldP2).Length();
			joint.distanceSettings.minDistance = dist;
			joint.distanceSettings.maxDistance = dist;*/
		}

		distanceSettings.mMinDistance = joint.distanceSettings.minDistance;
		distanceSettings.mMaxDistance = joint.distanceSettings.maxDistance;

		/*Vec3 worldP1 = ToJolt(trans.TransformPoint(joint.distanceSettings.point1));// Get world-space points (the way user or editor defines them)
		Vec3 worldP2 = ToJolt(trans.TransformPoint(joint.distanceSettings.point2));
		float dist = (worldP1 - worldP2).Length();
		distanceSettings.mMinDistance = dist;
		distanceSettings.mMaxDistance = dist;*/

		DistanceConstraint* c = new DistanceConstraint(body1, body2, distanceSettings);
		joint.data->constraint = c;
	}

	physicsWorld->physicsSystem.AddConstraint(joint.data->constraint);

	if(body1.IsDynamic()) physicsWorld->physicsSystem.GetBodyInterfaceNoLock().ActivateBody(idA);
	if(body2.IsDynamic()) physicsWorld->physicsSystem.GetBodyInterfaceNoLock().ActivateBody(idB);
}

void PhysicsSystem::RemoveJoint(Entity entity, JointComponent& joint){
	if(joint.data != nullptr && joint.data->constraint != nullptr){
		physicsWorld->physicsSystem.RemoveConstraint(joint.data->constraint);
	}

	delete joint.data;
}

void PhysicsSystem::SetJointsAsDirtyIfBodyIsDirty(Entity e){
	auto jointView = scene->GetRegistry().view<JointComponent>();
	for(auto [entity, joint]: jointView.each()){
		if(joint.bodyA == e){
			joint.isDirty = true;
			break;
		}
		if(joint.bodyB == e){
			joint.isDirty = true;
			break;
		}
	}
}

#pragma endregion

#pragma region VehiclePhysic

void PhysicsSystem::OnRemoveVehicle(entt::registry& r, entt::entity e){
	VehiclePhysic& rb = r.get<VehiclePhysic>(e);
    if(rb.data == nullptr) return;

    PhysicsSystem* physicsSystem = r.ctx().get<PhysicsSystem*>();
    physicsSystem->RemoveVehicle(e, rb);
    delete rb.data;
}

void PhysicsSystem::AddVehicle(Entity entity, VehiclePhysic& veh, RigidbodyComponent& c, TransformComponent& t, InfoComponent& info){
	BodyLockWrite lock(physicsWorld->physicsSystem.GetBodyLockInterface(), c.data->bodyID);
	if(!lock.Succeeded()) return;

	Body& mCarBody = lock.GetBody();

	const float wheel_radius = 0.3f;
	const float wheel_radius2 = 0.4f;
	const float wheel_width = 0.1f;
	const float half_vehicle_length = 2.0f;
	const float half_vehicle_width = 0.9f;
	const float half_vehicle_height = 0.2f;

	const float			sInitialRollAngle = 0;
	const float			sMaxRollAngle = DegreesToRadians(60.0f);
	const float			sMaxSteeringAngle = DegreesToRadians(30.0f);
	const int			sCollisionMode = 2;
	const bool			sFourWheelDrive = false;
	const bool			sAntiRollbar = true;
	const bool			sLimitedSlipDifferentials = true;
	const bool			sOverrideGravity = false;					///< If true, gravity is overridden to always oppose the ground normal
	const float			sMaxEngineTorque = 500.0f;
	const float			sClutchStrength = 10.0f;
	const float			sFrontCasterAngle = 0.0f;
	const float			sFrontKingPinAngle = 0.0f;
	const float			sFrontCamber = 0.0f;
	const float			sFrontToe = 0.0f;
	const float			sFrontSuspensionForwardAngle = 0.0f;
	const float			sFrontSuspensionSidewaysAngle = 0.0f;
	const float			sFrontSuspensionMinLength = 0.3f;
	const float			sFrontSuspensionMaxLength = 0.5f;
	const float			sFrontSuspensionFrequency = 1.5f;
	const float			sFrontSuspensionDamping = 0.5f;
	const float			sRearSuspensionForwardAngle = 0.0f;
	const float			sRearSuspensionSidewaysAngle = 0.0f;
	const float			sRearCasterAngle = 0.0f;
	const float			sRearKingPinAngle = 0.0f;
	const float			sRearCamber = 0.0f;
	const float			sRearToe = 0.0f;
	const float			sRearSuspensionMinLength = 0.3f;
	const float			sRearSuspensionMaxLength = 0.5f;
	const float			sRearSuspensionFrequency = 1.5f;
	const float			sRearSuspensionDamping = 0.5f;

	//Body *						mCarBody;									///< The vehicle
	//Ref<VehicleConstraint>		mVehicleConstraint;							///< The vehicle constraint
	//Ref<VehicleCollisionTester>	mTesters[3];								///< Collision testers for the wheel
	//RMat44						mCameraPivot = RMat44::sIdentity();			///< The camera pivot, recorded before the physics update to align with the drawn world

	// Player input
	float						mForward = 0.0f;
	float						mPreviousForward = 1.0f;					///< Keeps track of last car direction so we know when to brake and when to accelerate
	float						mRight = 0.0f;
	float						mBrake = 0.0f;
	float						mHandBrake = 0.0f;

	veh.data = new VehiclePhysicData();

	// Create vehicle constraint
	VehicleConstraintSettings vehicle;
	vehicle.mDrawConstraintSize = 0.1f;
	vehicle.mMaxPitchRollAngle = math::radians(veh.maxRollAngle); sMaxRollAngle;

	// Suspension direction
	Vec3 front_suspension_dir = Vec3(Tan(sFrontSuspensionSidewaysAngle), -1, Tan(sFrontSuspensionForwardAngle)).Normalized();
	Vec3 front_steering_axis = Vec3(-Tan(sFrontKingPinAngle), 1, -Tan(sFrontCasterAngle)).Normalized();
	Vec3 front_wheel_up = Vec3(Sin(sFrontCamber), Cos(sFrontCamber), 0);
	Vec3 front_wheel_forward = Vec3(-Sin(sFrontToe), 0, Cos(sFrontToe));
	Vec3 rear_suspension_dir = Vec3(Tan(sRearSuspensionSidewaysAngle), -1, Tan(sRearSuspensionForwardAngle)).Normalized();
	Vec3 rear_steering_axis = Vec3(-Tan(sRearKingPinAngle), 1, -Tan(sRearCasterAngle)).Normalized();
	Vec3 rear_wheel_up = Vec3(Sin(sRearCamber), Cos(sRearCamber), 0);
	Vec3 rear_wheel_forward = Vec3(-Sin(sRearToe), 0, Cos(sRearToe));
	Vec3 flip_x(-1, 1, 1);

	Assert(veh.wheels.size() == 4);

	// Wheels, left front
	WheelSettingsWV *w1 = new WheelSettingsWV;
	//w1->mPosition = Vec3(half_vehicle_width, -0.9f * half_vehicle_height, half_vehicle_length - 2.0f * wheel_radius);
	w1->mPosition = ToJolt(veh.wheels[0].pos);

	w1->mSuspensionDirection = front_suspension_dir;
	w1->mSteeringAxis = front_steering_axis;
	w1->mWheelUp = front_wheel_up;
	w1->mWheelForward = front_wheel_forward;
	w1->mSuspensionMinLength = veh.wheels[0].suspensionMinLength;// sFrontSuspensionMinLength;
	w1->mSuspensionMaxLength = veh.wheels[0].suspensionMaxLength;// sFrontSuspensionMaxLength;
	w1->mSuspensionSpring.mFrequency = veh.wheels[0].suspensionFrequency;// sFrontSuspensionFrequency;
	w1->mSuspensionSpring.mDamping = veh.wheels[0].suspensionDamping;// sFrontSuspensionDamping;
	w1->mMaxSteerAngle = math::radians(veh.maxSteeringAngle); //sMaxSteeringAngle;
	w1->mMaxHandBrakeTorque = 0.0f; // Front wheel doesn't have hand brake
	w1->mRadius = veh.wheels[0].radius;
	w1->mWidth = veh.wheels[0].width;

	// Right front
	WheelSettingsWV *w2 = new WheelSettingsWV;
	//w2->mPosition = Vec3(-half_vehicle_width, -0.9f * half_vehicle_height, half_vehicle_length - 2.0f * wheel_radius);
	w2->mPosition = ToJolt(veh.wheels[1].pos);

	w2->mSuspensionDirection = flip_x * front_suspension_dir;
	w2->mSteeringAxis = flip_x * front_steering_axis;
	w2->mWheelUp = flip_x * front_wheel_up;
	w2->mWheelForward = flip_x * front_wheel_forward;
	w2->mSuspensionMinLength = veh.wheels[1].suspensionMinLength;// sFrontSuspensionMinLength;
	w2->mSuspensionMaxLength = veh.wheels[1].suspensionMaxLength;// sFrontSuspensionMaxLength;
	w2->mSuspensionSpring.mFrequency = veh.wheels[1].suspensionFrequency;// sFrontSuspensionFrequency;
	w2->mSuspensionSpring.mDamping = veh.wheels[1].suspensionDamping;// sFrontSuspensionDamping;
	w2->mMaxSteerAngle = math::radians(veh.maxSteeringAngle); //sMaxSteeringAngle;
	w2->mMaxHandBrakeTorque = 0.0f; // Front wheel doesn't have hand brake
	w2->mRadius = veh.wheels[1].radius;
	w2->mWidth = veh.wheels[1].width;

	// Left rear
	WheelSettingsWV *w3 = new WheelSettingsWV;
	//w3->mPosition = Vec3(half_vehicle_width, -0.9f * half_vehicle_height, -half_vehicle_length + 2.0f * wheel_radius);
	w3->mPosition = ToJolt(veh.wheels[2].pos);

	w3->mSuspensionDirection = rear_suspension_dir;
	w3->mSteeringAxis = rear_steering_axis;
	w3->mWheelUp = rear_wheel_up;
	w3->mWheelForward = rear_wheel_forward;
	w3->mSuspensionMinLength = veh.wheels[2].suspensionMinLength;// sRearSuspensionMinLength;
	w3->mSuspensionMaxLength = veh.wheels[2].suspensionMaxLength;// sRearSuspensionMaxLength;
	w3->mSuspensionSpring.mFrequency = veh.wheels[2].suspensionFrequency;// sRearSuspensionFrequency;
	w3->mSuspensionSpring.mDamping = veh.wheels[2].suspensionDamping;// sRearSuspensionDamping;
	w3->mMaxSteerAngle = 0.0f;
	w3->mRadius = veh.wheels[2].radius;
	w3->mWidth = veh.wheels[2].width;

	// Right rear
	WheelSettingsWV *w4 = new WheelSettingsWV;
	//w4->mPosition = Vec3(-half_vehicle_width, -0.9f * half_vehicle_height, -half_vehicle_length + 2.0f * wheel_radius);
	w4->mPosition = ToJolt(veh.wheels[3].pos);

	w4->mSuspensionDirection = flip_x * rear_suspension_dir;
	w4->mSteeringAxis = flip_x * rear_steering_axis;
	w4->mWheelUp = flip_x * rear_wheel_up;
	w4->mWheelForward = flip_x * rear_wheel_forward;
	w4->mSuspensionMinLength = veh.wheels[3].suspensionMinLength;// sRearSuspensionMinLength;
	w4->mSuspensionMaxLength = veh.wheels[3].suspensionMaxLength;// sRearSuspensionMaxLength;
	w4->mSuspensionSpring.mFrequency = veh.wheels[3].suspensionFrequency;// sRearSuspensionFrequency;
	w4->mSuspensionSpring.mDamping = veh.wheels[3].suspensionDamping;// sRearSuspensionDamping;
	w4->mMaxSteerAngle = 0.0f;
	w4->mRadius = veh.wheels[3].radius;
	w4->mWidth = veh.wheels[3].width;

	vehicle.mWheels = { w1, w2, w3, w4 };

	/*for(WheelSettings *w : vehicle.mWheels){
		w->mRadius = wheel_radius;
		w->mWidth = wheel_width;
	}*/

	WheeledVehicleControllerSettings *controller = new WheeledVehicleControllerSettings;
	vehicle.mController = controller;

	// Differential
	controller->mDifferentials.resize(sFourWheelDrive ? 2 : 1);
	controller->mDifferentials[0].mLeftWheel = 0;
	controller->mDifferentials[0].mRightWheel = 1;
	if(sFourWheelDrive){
		controller->mDifferentials[1].mLeftWheel = 2;
		controller->mDifferentials[1].mRightWheel = 3;

		//controller->mDifferentials[0].mDifferentialRatio = 1.93f * 40.0f / 16.0f; // Combining primary and final drive (back divided by front sprockets) from: https://www.blocklayer.com/rpm-gear-bikes

		// Split engine torque
		controller->mDifferentials[0].mEngineTorqueRatio = controller->mDifferentials[1].mEngineTorqueRatio = 0.5f;
	}

	// Anti rollbars
	if(sAntiRollbar){
		vehicle.mAntiRollBars.resize(2);
		vehicle.mAntiRollBars[0].mLeftWheel = 0;
		vehicle.mAntiRollBars[0].mRightWheel = 1;
		vehicle.mAntiRollBars[1].mLeftWheel = 2;
		vehicle.mAntiRollBars[1].mRightWheel = 3;

		vehicle.mAntiRollBars[0].mStiffness = veh.antiRollBarsStiffness;
		vehicle.mAntiRollBars[1].mStiffness = veh.antiRollBarsStiffness;
	}

	veh.data->vehicleConstraint = new VehicleConstraint(mCarBody, vehicle);

	float longMult = veh.longitudinalImpulseMultplier;
	float latMult  = veh.lateralImpulseMultplier;

	// The vehicle settings were tweaked with a buggy implementation of the longitudinal tire impulses, this meant that PhysicsSettings::mNumVelocitySteps times more impulse
	// could be applied than intended. To keep the behavior of the vehicle the same we increase the max longitudinal impulse by the same factor. In a future version the vehicle
	// will be retweaked.
	static_cast<WheeledVehicleController *>(veh.data->vehicleConstraint->GetController())->SetTireMaxImpulseCallback(
		[longMult, latMult]
		(uint, float &outLongitudinalImpulse, float &outLateralImpulse, float inSuspensionImpulse, float inLongitudinalFriction, float inLateralFriction, float, float, float){
			outLongitudinalImpulse = /*10.0f **/ longMult * inLongitudinalFriction * inSuspensionImpulse;
			outLateralImpulse = /*2 **/ latMult * inLateralFriction * inSuspensionImpulse;
		}
	);

	VehicleCollisionTester* vehicle_tester = new VehicleCollisionTesterRay(info.layer);
	veh.data->vehicleConstraint->SetVehicleCollisionTester(vehicle_tester);

	physicsWorld->physicsSystem.AddConstraint(veh.data->vehicleConstraint);
	physicsWorld->physicsSystem.AddStepListener(veh.data->vehicleConstraint);
}

void PhysicsSystem::RemoveVehicle(Entity entity, VehiclePhysic& c){
	physicsWorld->physicsSystem.RemoveStepListener(c.data->vehicleConstraint);
	physicsWorld->physicsSystem.RemoveConstraint(c.data->vehicleConstraint);
}

#pragma endregion

#pragma region PhysicsSystem

Transform VehiclePhysic::GetWheelWorldTransform(int wheelIndex, Vector3 up, Vector3 right){
	if(data == nullptr) return Transform();

	Mat44 wheelmat = data->vehicleConstraint->GetWheelWorldTransform(wheelIndex, ToJolt(right), ToJolt(up));
	Transform out;
	out.Position(FromJolt(wheelmat.GetTranslation()));
	out.Rotation(FromJolt(wheelmat.GetQuaternion()));
	return out;
}

Transform VehiclePhysic::GetWheelLocalTransform(int wheelIndex, Vector3 up, Vector3 right){
	if(data == nullptr) return Transform();

	Mat44 wheelmat = data->vehicleConstraint->GetWheelLocalTransform(wheelIndex, ToJolt(right), ToJolt(up));
	Transform out;
	out.Position(FromJolt(wheelmat.GetTranslation()));
	out.Rotation(FromJolt(wheelmat.GetQuaternion()));
	return out;
}

bool PhysicsSystem::IsSimulationEnable(){ return true; /*return GetScene()->Running();*/ }

MyDebugRenderer* debugRenderer = nullptr;

void PhysicsSystem::OnInit(Scene& inScene){
	scene = &inScene;
	currentSettings = &GlobalSettings::Get().Get<PhysicsSettings>();

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
	const uint cMaxBodies = 8192*2; //65536; //8192;// 1024;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodyPairs = 65536*2; //262144; //65536;// 1024;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const uint cMaxContactConstraints = 10240*2; //65536; //10240; //1024;

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

	physicsWorld->contactListener = new MyContactListener();
	physicsWorld->contactListener->scene = scene;
	physicsWorld->contactListener->physic = this;
	physicsWorld->physicsSystem.SetContactListener(physicsWorld->contactListener);

	if(debugRenderer == nullptr){
		debugRenderer = new MyDebugRenderer();
	}

    physicsWorld->tempAllocator = new TempAllocatorImpl((10 * 1024 * 1024)*2);
    physicsWorld->jobSystem.Init(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
	physicsWorld->renderer = debugRenderer;// new MyDebugRenderer();
	//physicsWorld->renderer->scene = scene;
	physicsWorld->system = this;

	JPH::Ref<MyGroupFilter> groupFilter = new MyGroupFilter();
	physicsWorld->groupFilter = groupFilter;

	physicsWorld->physicsSystem.SetGravity(ToJolt(currentSettings->gravity));

	//JPH::DebugRenderer::sInstance = physicsWorld->renderer;

	this->scene->GetRegistry().on_destroy<RagdollComponent>().connect<&OnRemoveRagdoll>();
    this->scene->GetRegistry().on_destroy<RigidbodyComponent>().connect<&OnRemoveRigidbody>();
	this->scene->GetRegistry().on_destroy<JointComponent>().connect<&OnRemoveJoint>();
	this->scene->GetRegistry().on_destroy<HeightmapColliderComponent>().connect<&OnRemoveHeightmap>();
	this->scene->GetRegistry().on_destroy<VehiclePhysic>().connect<&OnRemoveVehicle>();
    this->scene->GetRegistry().ctx().emplace<PhysicsSystem*>(this);
}

void PhysicsSystem::OnEnd(Scene& inScene){
    scene->GetRegistry().on_destroy<RigidbodyComponent>().disconnect<&OnRemoveRigidbody>();
	scene->GetRegistry().on_destroy<RagdollComponent>().disconnect<&OnRemoveRagdoll>();
	this->scene->GetRegistry().on_destroy<JointComponent>().disconnect<&OnRemoveJoint>();
	scene->GetRegistry().on_destroy<VehiclePhysic>().disconnect<&OnRemoveVehicle>();
	scene->GetRegistry().on_destroy<HeightmapColliderComponent>().disconnect<&OnRemoveHeightmap>();

    UnregisterTypes();
	//JPH::DebugRenderer::sInstance = nullptr;
    delete Factory::sInstance;
	Factory::sInstance = nullptr;
    delete physicsWorld;
}

PhysicsSystem::~PhysicsSystem(){
	/*UnregisterTypes();
	JPH::DebugRenderer::sInstance = nullptr;
    delete Factory::sInstance;
	Factory::sInstance = nullptr;
    delete physicsWorld;*/
}

void* PhysicsSystem::GetInternlWorld(){
    return nullptr;
}

inline Vec3 ClampVectorLength(const Vec3& v, float maxLen){
    float len = v.Length();
    if(len > maxLen) return v * (maxLen / len);
    return v;
}

void ApplyTargetRotationMotorOld(
    BodyInterface& bodyInterface,
    BodyID bodyID,
    const Quat& currentRot,
    const Quat& targetRot,
    float kp,
    float kd,
    bool useTorqueControl
){
    Quat deltaRot = targetRot * currentRot.Conjugated();
	Vec3 axis;
	float angle;
	deltaRot.GetAxisAngle(axis, angle);

	Vec3 currentAngularVelocity = bodyInterface.GetAngularVelocity(bodyID);
	Vec3 torque = kp * axis * angle - kd * currentAngularVelocity;
	Vec3 angularVel = axis * angle * kp;

	torque = ClampVectorLength(torque, kp * 10);
	angularVel = ClampVectorLength(angularVel, kp * 10);

	if(useTorqueControl)
		bodyInterface.AddTorque(bodyID, torque);
	else
		bodyInterface.SetAngularVelocity(bodyID, angularVel);
}

void ApplyTargetRotationMotor(
    BodyInterface& bodyInterface,
    BodyID bodyID,
    const Quat& currentRot,
    const Quat& targetRot,
    float kp,
    float kd,
    bool useTorqueControl
){
    // ---------------------------------------------
    // 1. Compute quaternion error q_err = q_target * q_current^-1
    // ---------------------------------------------
    Quat q_err = (targetRot * currentRot.Conjugated()).Normalized();

    // ---------------------------------------------
    // 2. Convert quaternion error → angular error vector
    // angularError = 2 * sign(w) * xyz
    // This gives a stable rotation vector representing how much to rotate,
    // without weird reversals near 180 degrees.
    // ---------------------------------------------
    float signW = (q_err.GetW() >= 0.0f ? 1.0f : -1.0f);
    Vec3 angularError = Vec3(q_err.GetX(), q_err.GetY(), q_err.GetZ()) * (2.0f * signW);

    // ---------------------------------------------
    // 3. PD control: torque = kp * error - kd * angularVelocity
    // ---------------------------------------------
    Vec3 currentAngularVelocity = bodyInterface.GetAngularVelocity(bodyID);

    Vec3 torque = kp * angularError - kd * currentAngularVelocity;

    // Safety clamp (optional)
    torque = ClampVectorLength(torque, kp * 10.0f);

    // ---------------------------------------------
    // 4. Apply
    // ---------------------------------------------
    if(useTorqueControl){
        bodyInterface.AddTorque(bodyID, torque);
    } else {
        // Alternative: velocity control for very tight motors
        Vec3 angularVel = angularError * kp;
        angularVel = ClampVectorLength(angularVel, kp * 10.0f);
        bodyInterface.SetAngularVelocity(bodyID, angularVel);
    }
}

void ApplyServoMotor(
    BodyInterface& bodyInterface,
    BodyID id,
    const Quat& currentRot,
    const Quat& targetRot,
    float followSpeed,      // how strong animation pulls
    float muscleStrength    // torque applied to reach target velocity
){
    // quaternion error
    Quat qerr = (targetRot * currentRot.Conjugated()).Normalized();
    float signW = qerr.GetW() >= 0.0f ? 1.0f : -1.0f;

    // convert to angular error vector
    Vec3 angularError = Vec3(qerr.GetX(), qerr.GetY(), qerr.GetZ()) * (2.0f * signW);

    // this is the velocity we WANT
    Vec3 targetAV = angularError * followSpeed;

    // current angular velocity
    Vec3 currentAV = bodyInterface.GetAngularVelocity(id);

    // error
    Vec3 avError = targetAV - currentAV;

    // servo torque
    Vec3 torque = avError * muscleStrength;

    torque = ClampVectorLength(torque, muscleStrength * 10.0f);

    bodyInterface.AddTorque(id, torque);
}

void ApplyStableServo(
    BodyInterface& bi,
    BodyID id,
    Quat targetRot,
    Quat currentRot,
    float followSpeed,
    float strength,
    Quat &prevTargetRot
){
    // normalize everything
    targetRot = targetRot.Normalized();
    currentRot = currentRot.Normalized();

    // smooth the target: critical for jitter-free motion
    targetRot = targetRot; //math::slerp(prevTargetRot, targetRot, 0.25f);
    prevTargetRot = targetRot;

    // q error
    Quat qerr = (targetRot * currentRot.Conjugated()).Normalized();

    float signW = qerr.GetW() >= 0 ? 1 : -1;
    Vec3 angularError = Vec3(qerr.GetX(), qerr.GetY(), qerr.GetZ()) * (2.0f * signW);

    // clamp angle to avoid instability
    float maxAngle = 0.5f; // 30°
    float mag = angularError.Length();
    if (mag > maxAngle)
        angularError *= (maxAngle / mag);

    // compute target angular velocity
    Vec3 targetAV = angularError * followSpeed;

    Vec3 currentAV = bi.GetAngularVelocity(id);
    Vec3 avError = targetAV - currentAV;

    Vec3 torque = avError * strength;
    torque = ClampVectorLength(torque, strength * 10.0f);

    bi.AddTorque(id, torque);
}

float PhysicsSystem::InterpolationAlpha(){
	//return (scene->FixedUpdateAccumulator() / Time::UnscaledDeltaTime()) * Time::TimeScale(); 
	return scene->FixedUpdateAccumulator() / Time::FixedDelta();
}

int PhysicsSystem::ExecutionSortPriority(SystemType type){
	if(type == SystemType::PrePhysics) return 25000;
	if(type == SystemType::FixedPhysics) return 25000;
	if(type == SystemType::PostPhysics) return -25000;
	return 1; 
};

void PhysicsSystem::PrePhysicsUpdate(Scene& inscene){
	OD_PROFILE_SCOPE("PhysicsSystem::PrePhysicsUpdate");

	BodyInterface& bodyInterface = physicsWorld->physicsSystem.GetBodyInterfaceNoLock(); //physicsWorld->physicsSystem.GetBodyInterface();

	if constexpr(EnableFixedPostPhysicUpdate == true) return;

	auto rbView = scene->GetRegistry().view<RigidbodyComponent, TransformComponent>();
    for(auto e: rbView){
        RigidbodyComponent& rb = rbView.get<RigidbodyComponent>(e);
		if(rb.data == nullptr) continue;

        TransformComponent& transform = rbView.get<TransformComponent>(e);
		
        if(rb.GetType() != RigidbodyComponent::Type::Dynamic){
            bodyInterface.SetPosition(rb.data->bodyID, ToJolt(transform.Position()), EActivation::Activate);
            bodyInterface.SetRotation(rb.data->bodyID, ToJolt(transform.Rotation()), EActivation::Activate);
        }
    }
}

void PhysicsSystem::FixedPhysicsUpdate(Scene& inscene){
	//OD_PROFILE_SCOPE("PhysicsSystem::FixedPhysicsUpdate");

	Assert(&GlobalSettings::Get().Get<PhysicsSettings>() == currentSettings);
	if(scene->Running() == false) return;

	BodyInterface& bodyInterface = physicsWorld->physicsSystem.GetBodyInterfaceNoLock();

	auto rbView = scene->GetRegistry().view<RigidbodyComponent, TransformComponent, InfoComponent>();
	auto ragdollModelView = scene->GetRegistry().view<SkinnedModelRendererComponent, RagdollComponent, TransformComponent, InfoComponent>();

	auto PreInterpolate = [&](){
		for(auto [entity, rb, trans, info]: rbView.each()){
			if(rb.type == RigidbodyComponent::Type::Dynamic && rb.interpolate && rb.data != nullptr){
				BodyID bodyID = rb.data->bodyID;
				Quat rot;
				RVec3 pos;
				bodyInterface.GetPositionAndRotation(bodyID, pos, rot);
				rb.previousPosition = FromJolt(pos);
				rb.previousRotation = FromJolt(rot);
			}
		}

		for(auto [entity, skinned, ragdoll, trans, info]: ragdollModelView.each()){
			if(ragdoll.data != nullptr && scene->Running() == true && ragdoll.type == RagdollComponent::Type::Dynamic){
				if(ragdoll.type == RagdollComponent::Type::Dynamic && ragdoll.interpolate){
					for(size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
						//if(ragdoll.parts[p].previousPosition == Vector3Zero) continue;

						BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
						Quat rot;
						RVec3 pos;
						bodyInterface.GetPositionAndRotation(bodyID, pos, rot);
						ragdoll.parts[p].previousPosition = FromJolt(pos);
						ragdoll.parts[p].previousRotation = FromJolt(rot);
					}
				}
			}
		}
	};

	if constexpr(EnableFixedPostPhysicUpdate == true){
		auto rbView = scene->GetRegistry().view<RigidbodyComponent, TransformComponent>();
		for(auto e: rbView){
			RigidbodyComponent& rb = rbView.get<RigidbodyComponent>(e);
			if(rb.data == nullptr) continue;

			TransformComponent& transform = rbView.get<TransformComponent>(e);
			
			if(rb.GetType() != RigidbodyComponent::Type::Dynamic){
				bodyInterface.SetPosition(rb.data->bodyID, ToJolt(transform.Position()), EActivation::Activate);
				bodyInterface.SetRotation(rb.data->bodyID, ToJolt(transform.Rotation()), EActivation::Activate);
			}
		}
	}

	auto ApplyBoneMotor = [&](BodyInterface& bodyInterface, BodyID bodyID, const Quat& targetRot, float stiffness, float damping, float dt){
		Quat currentRot;
		RVec3 pos;
		bodyInterface.GetPositionAndRotation(bodyID, pos, currentRot);

		// delta rotation
		Quat delta = targetRot * currentRot.Conjugated();
		delta = delta.Normalized();

		Vec3 axis;
		float angle;
		delta.GetAxisAngle(axis, angle);

		if(angle > JPH_PI)
			angle -= 2.0f * JPH_PI;
		angle = math::clamp<float>(angle, -JPH_PI / 4.0f, JPH::JPH_PI / 4.0f);

		Vec3 angVel = bodyInterface.GetAngularVelocity(bodyID);

		// PD controller
		Vec3 torque = (stiffness * angle / dt) * axis - damping * angVel;

		// Clamp
		const float maxTorque = 100.0f;
		if(torque.LengthSq() > maxTorque * maxTorque)
			torque = torque.Normalized() * maxTorque;

		bodyInterface.AddTorque(bodyID, torque);
	};

	for(auto [entity, skinned, ragdoll, trans, info]: ragdollModelView.each()){
		scene->GetTaskflow().emplace([entity, &skinned, &ragdoll, &trans, &info, &bodyInterface, this](){

		if(ragdoll.data != nullptr && scene->Running() == true && ragdoll.type == RagdollComponent::Type::Dynamic && ragdoll.syncWithFinalPose){
			float gain = ragdoll.gain;
			float damping = ragdoll.damping;     // Novo: adicionar na struct
			float stiffness = ragdoll.stiffness;

			if(ragdoll.type == RagdollComponent::Type::Dynamic && skinned.finalPose.Size() > 0 && ragdoll.isDirty == false){ //TODO: this "ragdoll.isDirty == false" look fix a ragdoll sometime strecht bug, check if other place needs this check
				int hipIndex = -1;

				for(size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
					if(ragdoll.parts[p].isHips){
						hipIndex = p; 
						break;
					}
				}
				Assert(hipIndex == 0);
				Assert(ragdoll.parts.size() == ragdoll.data->ragdoll->GetBodyIDs().size());
				Assert(ragdoll.startPose.Size() == skinned.finalPose.Size());

				for(size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
					//TODO: Fix the instability and Freeze pose

					if(ragdoll.motorType != RagdollComponent::MotorType::Jolt){
						if(ragdoll.parts[p].parent >= 0){
							SwingTwistConstraint* c = static_cast<SwingTwistConstraint*>(ragdoll.data->ragdoll->GetConstraint(p-1));
							c->SetSwingMotorState(EMotorState::Off);
							c->SetTwistMotorState(EMotorState::Off);
						}
					}
					
					if(ragdoll.motorType == RagdollComponent::MotorType::Jolt){
						if(ragdoll.parts[p].parent >= 0){
							SwingTwistConstraint* c = static_cast<SwingTwistConstraint*>(ragdoll.data->ragdoll->GetConstraint(p-1));
							//c->SetSwingMotorState(EMotorState::Off); //INFO: Call this every frame bug
							//c->SetTwistMotorState(EMotorState::Off);

							if(ragdoll.parts[p].disableSync) continue;
							if(ragdoll.syncWithFinalPose == false) continue;
							if(ragdoll.stiffness <= 1) continue;

							//float breakVelocityThreshold = (stiffness * ragdoll.parts[p].stiffnessMult);
							//Vec3 vel = bodyInterface.GetLinearVelocity(ragdoll.data->ragdoll->GetBodyIDs()[p]);
							//float speed = vel.Length();
							float scale = 1.0f;
							//if(speed > breakVelocityThreshold) scale = math::clamp<float>(breakVelocityThreshold / speed, 0, 1); // smoothly reduce

							float motorFrequency = gain * scale;    // or convert/gain mapping as you prefer
							float motorDamping   = damping * scale; // damping term
							float maxMotorTorque = (stiffness * ragdoll.parts[p].stiffnessMult) * 10; // scale as needed

							//c->SetMaxFrictionTorque(200); // in N·m
							//c->SetNumPositionStepsOverride(8);
							//c->SetNumVelocityStepsOverride(8);
							c->GetSwingMotorSettings() = MotorSettings(motorFrequency, motorDamping);
							c->GetTwistMotorSettings() = MotorSettings(motorFrequency, motorDamping);
							c->SetSwingMotorState(EMotorState::Position);
							c->SetTwistMotorState(EMotorState::Position);
							c->SetMaxFrictionTorque(maxMotorTorque);
							//c->SetTwistMaxAngle(math::radians(90.0f));
							//c->SetTwistMinAngle(math::radians(-90.0f));
							//c->SetSwingLimits(math::radians(30.0f), math::radians(30.0f)); // Adjust as needed
							//c->SetTwistLimits(math::radians(-45.0f), math::radians(45.0f)); // Adjust as needed

							int parentIndex = ragdoll.parts[ragdoll.parts[p].parent].skinnedSkeletonIndex;
							int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
							Transform animParentGlobal = Transform::Combine(trans.ToTransform(), skinned.finalPose.GetGlobalTransform(parentIndex));
							Transform animGlobal = Transform::Combine(trans.ToTransform(), skinned.finalPose.GetGlobalTransform(boneIndex));
							
							auto boneTargetLocal = math::conjugate(animParentGlobal.Rotation()) * animGlobal.Rotation();
							//auto boneTargetLocal = math::conjugate(math::normalize(animParentGlobal.Rotation())) * math::normalize(animGlobal.Rotation());

							if(!isfinite(boneTargetLocal.x) || !isfinite(boneTargetLocal.y) || !isfinite(boneTargetLocal.z) || !isfinite(boneTargetLocal.w)) continue;

							c->SetTargetOrientationBS(ToJolt(boneTargetLocal));
							//Assert(ToJolt(math::normalize(boneTargetLocal)).Normalized().IsNormalized());
							//c->SetTargetOrientationBS(ToJolt(math::normalize(boneTargetLocal)).Normalized());

							//const float maxVel = 50.0f;
							//Vec3 vel = bodyInterface.GetLinearVelocity(ragdoll.data->ragdoll->GetBodyIDs()[p]);
							//if(vel.LengthSq() > maxVel * maxVel){
							//	bodyInterface.SetLinearVelocity(ragdoll.data->ragdoll->GetBodyIDs()[p], vel.Normalized() * maxVel);
							//}
						} else {
							if(ragdoll.parts[p].disableSync) continue;
							
							/*// Work, but in world space
							BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
							int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
							if(boneIndex < 0) continue;

							Transform targetTransform = skinned.finalPose.GetGlobalTransform(boneIndex);
							Quat targetRot = ToJolt(targetTransform.Rotation());

							Quat currentRot;
							RVec3 currentPos;
							bodyInterface.GetPositionAndRotation(bodyID, currentPos, currentRot);

							Quat deltaRot = targetRot.Normalized() * currentRot.Normalized().Conjugated();
							Vec3 axis;
							float angle;
							deltaRot.GetAxisAngle(axis, angle);

							Vec3 currentAngularVelocity = bodyInterface.GetAngularVelocity(bodyID);
							Vec3 torque = (stiffness * ragdoll.parts[p].stiffnessMult) * axis * angle - damping * currentAngularVelocity;

							if(ragdoll.useTorqueControl)
								bodyInterface.AddTorque(bodyID, torque);
							else
								bodyInterface.SetAngularVelocity(bodyID, axis * angle * (stiffness * ragdoll.parts[p].stiffnessMult));*/

							//Work, but in local space
							BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
							int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
							if(boneIndex < 0) continue;

							Transform targetTransform = Transform(trans.GlobalModelMatrix() * skinned.finalPose.GetGlobalMatrix(boneIndex));
							Quat targetRot = ToJolt(targetTransform.Rotation());
						
							Quat currentRot;
							RVec3 currentPos;
							bodyInterface.GetPositionAndRotation(bodyID, currentPos, currentRot);

							Quat deltaRot = targetRot.Normalized() * currentRot.Normalized().Conjugated();
							Vec3 axis;
							float angle;
							deltaRot.GetAxisAngle(axis, angle);

							Vec3 currentAngularVelocity = bodyInterface.GetAngularVelocity(bodyID);
							Vec3 torque = (stiffness * ragdoll.parts[p].stiffnessMult) * axis * angle - damping * currentAngularVelocity;
							Vec3 angularVel = axis * angle * (stiffness * ragdoll.parts[p].stiffnessMult);

							torque = ClampVectorLength(torque, (stiffness * ragdoll.parts[p].stiffnessMult) * 10);
							angularVel = ClampVectorLength(angularVel, (stiffness * ragdoll.parts[p].stiffnessMult) * 10);

							if(ragdoll.useTorqueControl)
								bodyInterface.AddTorque(bodyID, torque);
							else
								bodyInterface.SetAngularVelocity(bodyID, angularVel);

						}
					}
					
					if(ragdoll.motorType == RagdollComponent::MotorType::TargetRot){
						if(ragdoll.syncFromTheHips && hipIndex != -1 && ragdoll.parts[p].isHips == false){ //&& ragdoll.parts[p].isHips == false
							if(ragdoll.parts[p].disableSync) continue;
							Assert(hipIndex == 0);
							BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
							int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
							if (boneIndex <= 0) continue;

							int hipPartIndex = hipIndex;
							BodyID hipBodyID = ragdoll.data->ragdoll->GetBodyIDs()[hipPartIndex];
							int hipBoneIndex = ragdoll.parts[hipPartIndex].skinnedSkeletonIndex;

							// 1. Get local rotation from animation (relative to animated hip)
							Transform boneAnimGlobal = skinned.finalPose.GetGlobalTransform(boneIndex);

							// 2. Get current hip transform from physics
							Quat hipRot;
							RVec3 hipPos;
							bodyInterface.GetPositionAndRotation(hipBodyID, hipPos, hipRot);
							Transform hipPhysTransform = Transform(FromJolt(hipPos), FromJolt(hipRot), Vector3One);

							// 3. Rebuild the target bone transform in world space using local anim pose in current hip space
							Transform boneTargetWorld = Transform::Combine(hipPhysTransform, boneAnimGlobal);
							Quat targetRot = ToJolt(boneTargetWorld.Rotation());

							// 4. Get current bone rotation from physics
							Quat currentRot;
							RVec3 currentPos;
							bodyInterface.GetPositionAndRotation(bodyID, currentPos, currentRot);

							// 5. Compute delta rotation and torque
							Quat deltaRot = targetRot * currentRot.Conjugated();
							Vec3 axis;
							float angle;
							deltaRot.GetAxisAngle(axis, angle);

							Vec3 currentAngularVelocity = bodyInterface.GetAngularVelocity(bodyID);
							Vec3 torque = (stiffness * ragdoll.parts[p].stiffnessMult) * axis * angle - damping * currentAngularVelocity;
							Vec3 angularVel = axis * angle * (stiffness * ragdoll.parts[p].stiffnessMult);

							torque = ClampVectorLength(torque, (stiffness * ragdoll.parts[p].stiffnessMult) * 10);
							angularVel = ClampVectorLength(angularVel, (stiffness * ragdoll.parts[p].stiffnessMult) * 10);

							if(ragdoll.useTorqueControl)
								bodyInterface.AddTorque(bodyID, torque);
							else
								bodyInterface.SetAngularVelocity(bodyID, angularVel);
						} else {
							if(ragdoll.parts[p].disableSync) continue;
							// Work, but in world space
							BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
							int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
							//if (boneIndex <= 0) continue;
							if(boneIndex < 0) continue;

							//Transform targetTransform = skinned.finalPose.GetGlobalTransform(boneIndex);
							Transform targetTransform = Transform(trans.GlobalModelMatrix() * skinned.finalPose.GetGlobalMatrix(boneIndex));
							Quat targetRot = ToJolt(targetTransform.Rotation());
							//Quat targetRot = ToJolt(targetTransform.LocalRotation()) * ToJolt(ragdoll.parts[p].initedRot);
							//Quat targetRot = ToJolt(targetTransform.LocalRotation()) * ToJolt(ragdoll.startPose.GetGlobalTransform(boneIndex).LocalRotation());


							Quat currentRot;
							RVec3 currentPos;
							bodyInterface.GetPositionAndRotation(bodyID, currentPos, currentRot);

							//targetRot = currentRot * ToJolt(targetTransform.Rotation());

							//Transform bindGlobalTransform = ragdoll.startPose.GetGlobalTransform(boneIndex); 
							//Quat bindRot = ToJolt(ragdoll.parts[p].initedRot); //ToJolt(bindGlobalTransform.LocalRotation());

							Quat deltaRot = targetRot.Normalized() * currentRot.Normalized().Conjugated();
							Vec3 axis;
							float angle;
							deltaRot.GetAxisAngle(axis, angle);

							// Atual: velocidade angular do corpo
							Vec3 currentAngularVelocity = bodyInterface.GetAngularVelocity(bodyID);

							// PD controller: torque = P * erro - D * velocidade
							Vec3 torque = (stiffness * ragdoll.parts[p].stiffnessMult) * axis * angle - damping * currentAngularVelocity;
							Vec3 angularVel = axis * angle * (stiffness * ragdoll.parts[p].stiffnessMult);

							torque = ClampVectorLength(torque, (stiffness * ragdoll.parts[p].stiffnessMult) * 10);
							angularVel = ClampVectorLength(angularVel, (stiffness * ragdoll.parts[p].stiffnessMult) * 10);

							//bodyInterface.SetAngularVelocity(bodyID, axis * (angle / Application::DeltaTime()));

							if(ragdoll.useTorqueControl)
								bodyInterface.AddTorque(bodyID, torque);
							else
								bodyInterface.SetAngularVelocity(bodyID, angularVel);
						}
					}

					/*if(ragdoll.motorType == RagdollComponent::MotorType::TargetRot){
						BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
						int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
						if(boneIndex < 0) continue;

						Quat targetRot;

						if(ragdoll.syncFromTheHips && hipIndex != -1 && !ragdoll.parts[p].isHips){
							if(ragdoll.parts[p].disableSync) continue;

							// Animated bone world rotation reconstructed using physics hip as root
							BodyID hipBodyID = ragdoll.data->ragdoll->GetBodyIDs()[hipIndex];

							Quat hipPhysRot;
							RVec3 hipPhysPos;
							bodyInterface.GetPositionAndRotation(hipBodyID, hipPhysPos, hipPhysRot);

							Transform hipPhys = Transform(FromJolt(hipPhysPos), FromJolt(hipPhysRot), Vector3One);
							Transform animBone = skinned.finalPose.GetGlobalTransform(boneIndex);

							Transform boneTargetWorld = Transform::Combine(hipPhys, animBone);
							targetRot = ToJolt(boneTargetWorld.Rotation());
						} else {
							if(ragdoll.parts[p].disableSync) continue;

							// World-space target (simple case)
							Transform t = Transform(trans.GlobalModelMatrix() * skinned.finalPose.GetGlobalMatrix(boneIndex));
							targetRot = ToJolt(t.Rotation());
						}

						// Current rotation
						Quat currentRot;
						RVec3 currentPos;
						bodyInterface.GetPositionAndRotation(bodyID, currentPos, currentRot);

						// Apply qPD motor
						float kp = stiffness * ragdoll.parts[p].stiffnessMult;
						float kd = damping;

						ApplyTargetRotationMotorOld(
							bodyInterface,
							bodyID,
							currentRot,
							targetRot,
							kp,
							kd,
							ragdoll.useTorqueControl
						);
						/ApplyTargetRotationMotor(
							bodyInterface,
							bodyID,
							currentRot,
							targetRot,
							kp,
							kd,
							ragdoll.useTorqueControl
						);
						ApplyServoMotor(
							bodyInterface,
							bodyID,
							currentRot,
							targetRot,
							8, //8–12
							25 //75–150
						);
						ApplyStableServo(
							bodyInterface,
							bodyID,
							currentRot,
							targetRot,
							8/4, //8–12
							75/4, //75–150
							ToJolt(QuaternionIdentity)
						);/
					}*/

					if(ragdoll.motorType == RagdollComponent::MotorType::TargetRotLocal){
						Assert(false);
						if(ragdoll.parts[p].disableSync) continue;

						const int boneIndex   = ragdoll.parts[p].skinnedSkeletonIndex;
						const int parentIndex = ragdoll.parts[p].parent;

						if(boneIndex < 0 || parentIndex < 0) continue;

						BodyID body       = ragdoll.data->ragdoll->GetBodyIDs()[p];
						BodyID parentBody = ragdoll.data->ragdoll->GetBodyIDs()[parentIndex];

						// -------------------------
						// 1. Animation local rotation (TARGET)
						// -------------------------
						Transform animParentGlobal = 
							Transform::Combine(trans.ToTransform(),
											skinned.finalPose.GetGlobalTransform(parentIndex));

						Transform animBoneGlobal = 
							Transform::Combine(trans.ToTransform(),
											skinned.finalPose.GetGlobalTransform(boneIndex));

						// local target = inverse(parentAnim) * boneAnim
						Quaternion targetLocal = math::conjugate(animParentGlobal.Rotation()) * animBoneGlobal.Rotation();


						// -------------------------
						// 2. Physics local rotation (CURRENT)
						// -------------------------
						Quat parentPhysRot, bonePhysRot;
						RVec3 tmpPos;

						bodyInterface.GetPositionAndRotation(parentBody, tmpPos, parentPhysRot);
						bodyInterface.GetPositionAndRotation(body,       tmpPos, bonePhysRot);

						//Quat parentPhysRot = FromJolt(parentPhysRotJ);
						//Quat bonePhysRot   = FromJolt(bonePhysRotJ);

						// local current = inverse(parentPhys) * bonePhys
						Quat currentLocal = parentPhysRot.Conjugated() * bonePhysRot; //math::conjugate(parentPhysRot) * bonePhysRot;


						// -------------------------
						// 3. Delta rotation in LOCAL SPACE
						// -------------------------
						Quat delta = ToJolt(targetLocal) * currentLocal.Conjugated();

						Vec3 axis;
						float angle;
						delta.GetAxisAngle(axis, angle);

						if(!isfinite(angle)) continue;


						// -------------------------
						// 4. Apply rotation (PD or angular velocity)
						// -------------------------
						Vec3 angVel = bodyInterface.GetAngularVelocity(body);

						float Kp = stiffness * ragdoll.parts[p].stiffnessMult;
						float Kd = damping;

						Vec3 torque = axis * (angle * Kp) - angVel * Kd;

						if(ragdoll.useTorqueControl)
							bodyInterface.AddTorque(body, torque);
						else
							bodyInterface.SetAngularVelocity(body, axis * (angle * Kp));
					}
				
				}
			}
			
		} 
		});
		
	}

	scene->RunAllTaskAndSync();

	if(scene->Running() == true){
		if(EnableInterpolation) PreInterpolate();

		physicsWorld->physicsSystem.Update(
			Time::FixedDelta(),
			cCollisionSteps,
			physicsWorld->tempAllocator,
			&physicsWorld->jobSystem
		);
	}

	//////////////////////////////////////////////////

	if constexpr (EnableFixedPostPhysicUpdate){
		_PostPhysicsUpdate(false, false);
	}
}

void PhysicsSystem::PostPhysicsUpdate(Scene& inScene){
	OD_PROFILE_SCOPE("PhysicsSystem::PostPhysicsUpdate");

	if constexpr (EnableFixedPostPhysicUpdate){
		_PostPhysicsUpdate(true, true);
	} else {
		_PostPhysicsUpdate(false, true);
	}
}

//INFO: Just a test
void PhysicsSystem::_SyncRagdollToPose(Scene& scene, SkinnedModelRendererComponent& skinned, RagdollComponent& ragdoll, TransformComponent& trans, InfoComponent& info, BodyInterface& bodyInterface, bool canInterpolate){
	if(ragdoll.isDirty == true) return;

	//if(skinned.finalPose.Size() <= 0) return;
	skinned.posePalette.resize(skinned.GetModel()->skeleton.GetRestPose().Size());
	skinned.finalPose = skinned.GetModel()->skeleton.GetRestPose();

	auto& pose = skinned.finalPose;

	for(size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
		BodyID i = ragdoll.data->ragdoll->GetBodyIDs()[p];
		RVec3 pos;
		Quat rot;
		bodyInterface.GetPositionAndRotation(i, pos, rot);

		if(ragdoll.interpolate && EnableInterpolation && canInterpolate){
			float alpha = Time::TimeScale() <= 0 ? 1 : scene.FixedUpdateAccumulator() / Time::FixedDelta();
			Vector3 interpolatedPos = math::mix(ragdoll.parts[p].previousPosition, FromJolt(pos), alpha);
			Quaternion interpolatedRot = math::slerp(ragdoll.parts[p].previousRotation, FromJolt(rot), alpha);

			int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
			pose.SetGlobalTransform(boneIndex, Transform(
				trans.InverseTransformPoint(interpolatedPos), 
				math::inverse(math::quat_cast(trans.GetLocalModelMatrix())) * interpolatedRot, //math::inverse(trans.Rotation()) * FromJolt(rot), 
				Vector3One
			));
		} else{
		
			int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
			pose.SetGlobalTransform(boneIndex, Transform(
				trans.InverseTransformPoint(FromJolt(pos)), 
				math::inverse(math::quat_cast(trans.GetLocalModelMatrix())) * FromJolt(rot), 
				//math::inverse(trans.Rotation()) * FromJolt(rot), 
				Vector3One
			));
		}
	}	
	pose.GetMatrixPalette(skinned.posePalette, skinned.GetModel()->skeleton.GetInvBindPose());
};

//INFO: Just a test
void PhysicsSystem::_SyncRagdollToPose2::operator()(){
	if(ragdoll.isDirty == true) return;

	//if(skinned.finalPose.Size() <= 0) return;
	skinned.posePalette.resize(skinned.GetModel()->skeleton.GetRestPose().Size());
	skinned.finalPose = skinned.GetModel()->skeleton.GetRestPose();

	auto& pose = skinned.finalPose;

	for(size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
		BodyID i = ragdoll.data->ragdoll->GetBodyIDs()[p];
		RVec3 pos;
		Quat rot;
		bodyInterface.GetPositionAndRotation(i, pos, rot);

		if(ragdoll.interpolate && EnableInterpolation && canInterpolate){
			float alpha = Time::TimeScale() <= 0 ? 1 : scene.FixedUpdateAccumulator() / Time::FixedDelta();
			Vector3 interpolatedPos = math::mix(ragdoll.parts[p].previousPosition, FromJolt(pos), alpha);
			Quaternion interpolatedRot = math::slerp(ragdoll.parts[p].previousRotation, FromJolt(rot), alpha);

			int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
			pose.SetGlobalTransform(boneIndex, Transform(
				trans.InverseTransformPoint(interpolatedPos), 
				math::inverse(math::quat_cast(trans.GetLocalModelMatrix())) * interpolatedRot, //math::inverse(trans.Rotation()) * FromJolt(rot), 
				Vector3One
			));
		} else{
		
			int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
			pose.SetGlobalTransform(boneIndex, Transform(
				trans.InverseTransformPoint(FromJolt(pos)), 
				math::inverse(math::quat_cast(trans.GetLocalModelMatrix())) * FromJolt(rot), 
				//math::inverse(trans.Rotation()) * FromJolt(rot), 
				Vector3One
			));
		}
	}	
	pose.GetMatrixPalette(skinned.posePalette, skinned.GetModel()->skeleton.GetInvBindPose());
}

//INFO: I think this can be a litter performace impruvment instead of using lambda
#define FIELD(type, name) type name;
#define DEFINE_TASK(TaskName, FIELD_LIST, BODY) \
struct TaskName {                               \
    FIELD_LIST                                  \
    void operator()() const BODY                \
};

DEFINE_TASK(RagdollTask,
	FIELD(Scene&, scene)
    FIELD(SkinnedModelRendererComponent&, skinned)
    FIELD(RagdollComponent&, ragdoll)
    FIELD(TransformComponent&, trans)
    FIELD(InfoComponent&, info)
    FIELD(BodyInterface&, bodyInterface)
    FIELD(bool, canInterpolate),
    {
        PhysicsSystem::_SyncRagdollToPose(scene, skinned, ragdoll, trans, info, bodyInterface, canInterpolate);
    }
);

void PhysicsSystem::_PostPhysicsUpdate(bool onlyPostSync, bool canInterpolate){
	Assert(&GlobalSettings::Get().Get<PhysicsSettings>() == currentSettings);
	if(scene->Running() == false) return;

	BodyInterface& bodyInterface = physicsWorld->physicsSystem.GetBodyInterfaceNoLock();

	auto SyncRagdollToPose = [&](SkinnedModelRendererComponent& skinned, RagdollComponent& ragdoll, TransformComponent& trans, InfoComponent& info, BodyInterface& bodyInterface, bool canInterpolate){
		if(ragdoll.isDirty == true) return;

		//if(skinned.finalPose.Size() <= 0) return;
		skinned.posePalette.resize(skinned.GetModel()->skeleton.GetRestPose().Size());
		skinned.finalPose = skinned.GetModel()->skeleton.GetRestPose();

		auto& pose = skinned.finalPose;

		for(size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
			BodyID i = ragdoll.data->ragdoll->GetBodyIDs()[p];
			RVec3 pos;
			Quat rot;
			bodyInterface.GetPositionAndRotation(i, pos, rot);

			if(ragdoll.interpolate && EnableInterpolation && canInterpolate){
				float alpha = Time::TimeScale() <= 0 ? 1 : InterpolationAlpha();
				Vector3 interpolatedPos = math::mix(ragdoll.parts[p].previousPosition, FromJolt(pos), alpha);
				Quaternion interpolatedRot = math::slerp(ragdoll.parts[p].previousRotation, FromJolt(rot), alpha);

				int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
				pose.SetGlobalTransform(boneIndex, Transform(
					trans.InverseTransformPoint(interpolatedPos), 
					math::inverse(math::quat_cast(trans.GetLocalModelMatrix())) * interpolatedRot, //math::inverse(trans.Rotation()) * FromJolt(rot), 
					Vector3One
				));
			} else{
			
				int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
				pose.SetGlobalTransform(boneIndex, Transform(
					trans.InverseTransformPoint(FromJolt(pos)), 
					math::inverse(math::quat_cast(trans.GetLocalModelMatrix())) * FromJolt(rot), 
					//math::inverse(trans.Rotation()) * FromJolt(rot), 
					Vector3One
				));
			}
		}	
		pose.GetMatrixPalette(skinned.posePalette, skinned.GetModel()->skeleton.GetInvBindPose());
	};

	auto SyncPoseToRagdoll = [&](SkinnedModelRendererComponent& skinned, RagdollComponent& ragdoll, TransformComponent& trans, BodyInterface& bodyInterface){
		for(size_t p = 0; p < ragdoll.data->ragdoll->GetBodyIDs().size(); ++p){
			BodyID bodyID = ragdoll.data->ragdoll->GetBodyIDs()[p];
			int boneIndex = ragdoll.parts[p].skinnedSkeletonIndex;
			Assert(boneIndex != 0);

			Quaternion tRot;
			Vector3 tPos;
			math::extractPosRot(
				math::simdMul(trans.GlobalModelMatrix(), skinned.finalPose[boneIndex].GetModelMatrix()),
				tPos, 
				tRot
			);
			bodyInterface.SetPosition(bodyID, ToJolt(tPos), EActivation::Activate);
			bodyInterface.SetRotation(bodyID, ToJolt(tRot), EActivation::Activate);
		}
	};

	auto SyncRbToTrans = [&](RigidbodyComponent& rb, TransformComponent& trans){
		RVec3 pos;
		Quat rot;
		bodyInterface.GetPositionAndRotation(rb.data->bodyID, pos, rot);

		if(rb.interpolate && rb.previousPosition != Vector3Zero && EnableInterpolation && canInterpolate){
			float alpha = Time::TimeScale() <= 0 ? 1 : InterpolationAlpha();
			Vector3 interpolatedPos = math::mix(rb.previousPosition, FromJolt(pos), alpha);
			Quaternion interpolatedRot = math::slerp(rb.previousRotation, FromJolt(rot), alpha);
			trans.Position(interpolatedPos);
			trans.Rotation(interpolatedRot);
		} else {
			trans.Position(FromJolt(pos));
			trans.Rotation(FromJolt(rot));
		}
	};

	if(onlyPostSync){
		auto rbView = scene->GetRegistry().view<RigidbodyComponent, TransformComponent, InfoComponent>();
		for(auto [e, rb, transform, info]: rbView.each()){
			Assert(rb.data != nullptr);

			if(rb.GetType() == RigidbodyComponent::Type::Dynamic/* || rb.GetType() == RigidbodyComponent::Type::Static*/){
				SyncRbToTrans(rb, transform);
			}
		}

		auto ragdollModelView = scene->GetRegistry().view<SkinnedModelRendererComponent, RagdollComponent, TransformComponent, InfoComponent>();
		for(auto [entity, skinned, ragdoll, trans, info]: ragdollModelView.each()){
			if(ragdoll.data != nullptr && scene->Running() == true && ragdoll.type == RagdollComponent::Type::Dynamic){
				/*scene->GetTaskflow().emplace([&skinned, &ragdoll, &trans, &info, &bodyInterface, canInterpolate, SyncRagdollToPose, this](){
					SyncRagdollToPose(skinned, ragdoll, trans, info, bodyInterface, canInterpolate);
				});*/
				scene->GetTaskflow().emplace(
					_SyncRagdollToPose2{*scene, skinned, ragdoll, trans, info, bodyInterface, canInterpolate}
				);
			}
		}

		//Info: Just for safety
		scene->RunAllTaskAndSync();

		return;
	}

	auto rbModelView = scene->GetRegistry().view<RigidbodyComponent, ModelRendererComponent, TransformComponent>();
    for(auto e: rbModelView){
		RigidbodyComponent& rb = rbModelView.get<RigidbodyComponent>(e);
        TransformComponent& transform = rbModelView.get<TransformComponent>(e);
        ModelRendererComponent& mesh = rbModelView.get<ModelRendererComponent>(e);

		if(rb.shape.type == CollisionShape::Type::Mesh && rb.shape.meshData == nullptr){
			rb.shape.meshData = mesh.GetModel()->modelShapeData == nullptr ? CreateMeshShapeData(*mesh.GetModel()) : mesh.GetModel()->modelShapeData;
		}
	}

	/*auto rbMeshView = scene->GetRegistry().view<RigidbodyComponent, MeshRendererComponent, TransformComponent>();
    for(auto e: rbMeshView){
		RigidbodyComponent& rb = rbMeshView.get<RigidbodyComponent>(e);
        TransformComponent& transform = rbMeshView.get<TransformComponent>(e);
        MeshRendererComponent& mesh = rbMeshView.get<MeshRendererComponent>(e);
	}*/

    auto rbView = scene->GetRegistry().view<RigidbodyComponent, TransformComponent, InfoComponent>();
    for(auto e: rbView){
        RigidbodyComponent& rb = rbView.get<RigidbodyComponent>(e);
        TransformComponent& transform = rbView.get<TransformComponent>(e);
        InfoComponent& info = rbView.get<InfoComponent>(e);

		if(rb.isDirt){
			RemoveRigidbody(e, rb);
			AddRigidbody(e, rb, transform, info);
		}

        Assert(rb.data != nullptr);

        if(rb.GetType() == RigidbodyComponent::Type::Dynamic/* || rb.GetType() == RigidbodyComponent::Type::Static*/){
            SyncRbToTrans(rb, transform);
        } else if(rb.GetType() == RigidbodyComponent::Type::Kinematic){
            //bodyInterface.SetPosition(rb.data->bodyID, ToJolt(transform.Position()), EActivation::Activate);
            //bodyInterface.SetRotation(rb.data->bodyID, ToJolt(transform.Rotation()), EActivation::Activate);
        }
    }

	auto rbMotorView = scene->GetRegistry().view<RigidbodyComponent, MotorTest, TransformComponent>();
	for(auto [entity, rb, motor, trans]: rbMotorView.each()){
		if(rb.data == nullptr) continue;
		if(motor.inited == true) continue;

		motor.inited = true;

		JPH::HingeConstraintSettings settings;
		settings.mPoint1 = settings.mPoint2 = ToJolt(trans.Position());   // pivot point
		settings.mHingeAxis1 = settings.mHingeAxis2 = Vec3::sAxisY(); // Y axis

		//settings.mMotorSettings.mFrequency = 60.0f; // stiffness
		//settings.mMotorSettings.mDamping = 1.0f;   // damping
		settings.mMotorSettings.mMinTorqueLimit = -10000.0f;
		settings.mMotorSettings.mMaxTorqueLimit =  10000.0f;
		settings.mMotorSettings.mSpringSettings.mFrequency = 120 * 3; //60.0f;
		settings.mMotorSettings.mSpringSettings.mDamping = 0.5f; //1.0f;

		BodyLockWrite lock(physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), rb.data->bodyID);
		JPH::Body& body = lock.GetBody();

		JPH::Ref<JPH::HingeConstraint> hinge = static_cast<JPH::HingeConstraint*>(settings.Create(body, JPH::Body::sFixedToWorld));

		hinge->SetMotorState(EMotorState::Velocity);
		hinge->SetTargetAngularVelocity(JPH::DegreesToRadians(90.0f * 5));
		physicsWorld->physicsSystem.AddConstraint(hinge);
	}

	auto ragdollModelView = scene->GetRegistry().view<SkinnedModelRendererComponent, RagdollComponent, TransformComponent, InfoComponent>();
	for(auto [entity, skinned, ragdoll, trans, info]: ragdollModelView.each()){
		if(ragdoll.isDirty && skinned.GetModel() != nullptr){
			RemoveRagdoll(entity, ragdoll);
			AddRagdoll(entity, ragdoll, trans, info, skinned);
		}
		
		if(ragdoll.type != RagdollComponent::Type::Dynamic && skinned.finalPose.Size() > 0 && ragdoll.type != RagdollComponent::Type::Disable){
			scene->GetTaskflow().emplace([&](){
				SyncPoseToRagdoll(skinned, ragdoll, trans, bodyInterface);
			});
		}
		
		if(ragdoll.data != nullptr && scene->Running() == true && ragdoll.type == RagdollComponent::Type::Dynamic){
			/*scene->GetTaskflow().emplace([&skinned, &ragdoll, &trans, &info, &bodyInterface, canInterpolate, SyncRagdollToPose, this](){
				SyncRagdollToPose(skinned, ragdoll, trans, info, bodyInterface, canInterpolate);
			});*/
			scene->GetTaskflow().emplace(
				_SyncRagdollToPose2{*scene, skinned, ragdoll, trans, info, bodyInterface, canInterpolate}
			);
		}
	}

	//TODO: Update This, make handle dirty and organaze the code
	auto heightView = scene->GetRegistry().view<HeightmapColliderComponent, TransformComponent, InfoComponent>();
	for(auto e : heightView){
		auto& rb = heightView.get<HeightmapColliderComponent>(e);
		auto& transform = heightView.get<TransformComponent>(e);
		auto& info = heightView.get<InfoComponent>(e);

		if(rb.data == nullptr){
			rb.data = new PhysicObject();
			// Create heightfield shape
			uint32 sampleCount = rb.width; // width == length, pois é quadrado
			JPH::Vec3 offset = JPH::Vec3(
				0, // para centralizar em X
				0,
				0 // para centralizar em -Z
			);
			JPH::Vec3 scale = ToJolt(transform.Scale());// Aplica escala do Transform

			JPH::HeightFieldShapeSettings heightFieldSettings(
				rb.heights.data(),   // float* inSamples
				offset,              // Vec3Arg inOffset
				scale,               // Vec3Arg inScale
				sampleCount,         // uint32 inSampleCount
				nullptr,             // material indices (optional)
				JPH::PhysicsMaterialList() // default material list
			);

			// opcional: ajustar block size ou bits por sample para performance
			heightFieldSettings.mBlockSize = 4;
			heightFieldSettings.mBitsPerSample = 8;
			heightFieldSettings.mMinHeightValue = rb.minHeight;
			heightFieldSettings.mMaxHeightValue = rb.maxHeight;

			JPH::ShapeSettings::ShapeResult shapeResult = heightFieldSettings.Create();
			if(shapeResult.HasError()){
				Assert(false);
				continue;
			}

			RefConst<Shape> finalShape = shapeResult.Get();

			// Create body
			JPH::BodyCreationSettings bodySettings(
				finalShape,
				ToJolt(transform.Position()),
				ToJolt(transform.Rotation()),
				JPH::EMotionType::Static, // Heightfields are usually static
				info.layer // Your custom collision layer
			);

			rb.data->bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::DontActivate);
		} else {
			bodyInterface.SetPosition(rb.data->bodyID, ToJolt(transform.Position()), EActivation::Activate);
            bodyInterface.SetRotation(rb.data->bodyID, ToJolt(transform.Rotation()), EActivation::Activate);

			// For scale, you must recreate the shape with new scale (Jolt doesn't support dynamic scaling).
		}
	}

	auto jointView = scene->GetRegistry().view<JointComponent, TransformComponent, InfoComponent>();
	for(auto [entity, joint, trans, info]: jointView.each()){
		if(joint.isDirty){
			joint.isDirty = false;
			RemoveJoint(entity, joint);
			AddJoint(scene, entity, joint, trans, info);
		}
	}

	auto vehicleView = scene->GetRegistry().view<VehiclePhysic, RigidbodyComponent, TransformComponent, InfoComponent>();
	for(auto [entity, veh, rb, trans, info]: vehicleView.each()){
		if(rb.data != nullptr && veh.data == nullptr){
			AddVehicle(entity, veh, rb, trans, info);
		}

		static bool sLimitedSlipDifferentials = true;

		if(rb.data != nullptr && veh.data != nullptr){
			BodyLockWrite lock(physicsWorld->physicsSystem.GetBodyLockInterface(), rb.data->bodyID);
			if(!lock.Succeeded()) continue;

			Body& mCarBody = lock.GetBody();

			// On user input, assure that the car is active
			if(veh.rightInput != 0.0f || veh.forwardInput != 0.0f || veh.brakeInput != 0.0f || veh.brakeInput != 0.0f)
				bodyInterface.ActivateBody(mCarBody.GetID());

			WheeledVehicleController *controller = static_cast<WheeledVehicleController *>(veh.data->vehicleConstraint->GetController());

			// Update vehicle statistics
			controller->GetEngine().mMaxTorque = veh.maxEngineTorque;// sMaxEngineTorque;
			controller->GetTransmission().mClutchStrength = veh.clutchStrength;// sClutchStrength;

			// Set slip ratios to the same for everything
			float limited_slip_ratio = sLimitedSlipDifferentials? 1.4f : FLT_MAX;
			controller->SetDifferentialLimitedSlipRatio(limited_slip_ratio);
			for (VehicleDifferentialSettings &d : controller->GetDifferentials())
				d.mLimitedSlipRatio = limited_slip_ratio;

			// Pass the input on to the constraint
			controller->SetDriverInput(veh.forwardInput, veh.rightInput, veh.brakeInput, veh.handBrakeInput);
		}
	
		if(rb.data != nullptr && veh.data != nullptr && veh.handleDebugInputs){
			BodyLockWrite lock(physicsWorld->physicsSystem.GetBodyLockInterface(), rb.data->bodyID);
			if(!lock.Succeeded()) continue;

			Body& mCarBody = lock.GetBody();

			// Determine acceleration and brake
			veh.forwardInput = 0.0f;
			if(Input::IsKey(KeyCode::W))
				veh.forwardInput = 1.0f;
			else if(Input::IsKey(KeyCode::S))
				veh.forwardInput = -1.0f;

			// Check if we're reversing direction
			veh.brakeInput = 0.0f;
			if(veh.previousForward * veh.forwardInput < 0.0f){
				// Get vehicle velocity in local space to the body of the vehicle
				float velocity = (mCarBody.GetRotation().Conjugated() * mCarBody.GetLinearVelocity()).GetZ();
				if((veh.forwardInput > 0.0f && velocity < -0.1f) || (veh.forwardInput < 0.0f && velocity > 0.1f)){
					// Brake while we've not stopped yet
					veh.forwardInput = 0.0f;
					veh.brakeInput = 1.0f;
				} else {
					// When we've come to a stop, accept the new direction
					veh.previousForward = veh.forwardInput;
				}
			}

			// Hand brake will cancel gas pedal
			veh.handBrakeInput = 0.0f;
			if(Input::IsKey(KeyCode::Space)){
				veh.forwardInput = 0.0f;
				veh.handBrakeInput = 1.0f;
			}

			// Steering
			veh.rightInput = 0.0f;
			if(Input::IsKey(KeyCode::A))
				veh.rightInput = -1.0f;
			else if(Input::IsKey(KeyCode::D))
				veh.rightInput = 1.0f;
		}
	}
}	

void PhysicsSystem::OnDrawGizmos(Scene& inScene, Camera& cam){
	ShowDebugGizmos();
}

void PhysicsSystem::CheckForCollisionEvents(){

}

void PhysicsSystem::ShowDebugGizmos(){
	//if(GetScene()->Running() == false) return;
	//BodyInterface &bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();

	SelectedBodyDrawFilter selectedBodyDrawFilter;
	selectedBodyDrawFilter.UpdateSelected();

	physicsWorld->renderer->scene = scene;
	physicsWorld->renderer->useLineCommand = true;
	physicsWorld->physicsSystem.DrawBodies(JPH::BodyManager::DrawSettings(), physicsWorld->renderer, &selectedBodyDrawFilter);
	Graphics::DrawLinesComamnd({0, 1, 0}, 1);

	physicsWorld->renderer->useLineCommand = false;
	physicsWorld->physicsSystem.DrawConstraints(physicsWorld->renderer);
	physicsWorld->physicsSystem.DrawConstraintLimits(physicsWorld->renderer);
}

// Example: Filter for non-moving objects (e.g., static walls)
class BroadPhaseLayerFilterImpl : public JPH::BroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override {
		return true;
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
        //return (allowedMask.mask & inLayer) != 0;
		return (allowedMask.mask & (1u << inLayer)) != 0;
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

class IgnoreSensorBodyFilter : public JPH::BodyFilter {
public:
    IgnoreSensorBodyFilter(PhysicsWorld* world):physicsWorld(world){}

    bool ShouldCollide(const JPH::BodyID& inBodyID) const override {
		const JPH::BodyLockRead lock(physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), inBodyID);
        Assert(lock.Succeeded());
        return lock.GetBody().IsSensor() == false;
    }
private:
    PhysicsWorld* physicsWorld;
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

	physicsWorld->physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector);
	if(collector.HadHit()){
		const JPH::RayCastResult& result = collector.GetHit();
		JPH::BodyID hitBodyID = result.mBodyID;
		float hitFraction = result.mFraction; // From 0.0 to 1.0
		JPH::Vec3 hitPosition = ray.mOrigin + ray.mDirection * hitFraction;

		// Get the body that was hit
		const JPH::BodyLockRead lock(physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), result.mBodyID);
		Assert(lock.Succeeded());

		const JPH::Body &body = lock.GetBody();
		JPH::Vec3 hitPoint = ray.GetPointOnRay(result.mFraction);

		//hit.entity = static_cast<Entity>(body.GetUserData()); // safe cast
		uint32_t _entity;
		int32_t _index;
		DecodeUserData(body.GetUserData(), _entity, _index);
		hit.entity = static_cast<Entity>(_entity);
		hit.subBodyIndex = _index;
		hit.hitPoint = FromJolt(hitPoint);	
		hit.hitNormal = FromJolt(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint));
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

	/*MyObjectLayerFilter _objectLayerFilter({Layers::Layer0 | Layers::Layer1});
	Assert(_objectLayerFilter.ShouldCollide(Layers::Layer1) == true);
	Assert(_objectLayerFilter.ShouldCollide(Layers::Layer2) == false);*/

	physicsWorld->physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector, {}, objectLayerFilter);
	if(collector.HadHit()){
		const JPH::RayCastResult& result = collector.GetHit();
		JPH::BodyID hitBodyID = result.mBodyID;
		float hitFraction = result.mFraction; // From 0.0 to 1.0
		JPH::Vec3 hitPosition = ray.mOrigin + ray.mDirection * hitFraction;

		// Get the body that was hit
		const JPH::BodyLockRead lock(physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), result.mBodyID);
		Assert(lock.Succeeded());

		const JPH::Body &body = lock.GetBody();
		JPH::Vec3 hitPoint = ray.GetPointOnRay(result.mFraction);

		//hit.entity = static_cast<Entity>(body.GetUserData()); // safe cast
		uint32_t _entity;
		int32_t _index;
		DecodeUserData(body.GetUserData(), _entity, _index);
		hit.entity = static_cast<Entity>(_entity);
		hit.subBodyIndex = _index;
		hit.hitPoint = FromJolt(hitPoint);
		hit.hitNormal = FromJolt(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint));
		return true;
	}

    return false;
}

bool PhysicsSystem::RaycastIgnoreSensor(Vector3 pos, Vector3 dir, RayResult& hit, LayerMask mask){
	Assert(physicsWorld != nullptr); 

	JPH::RRayCast ray;
	ray.mOrigin = ToJolt(pos);
	ray.mDirection = ToJolt(dir);

	JPH::RayCastSettings settings;
	settings.mBackFaceModeTriangles = JPH::EBackFaceMode::IgnoreBackFaces; // Ignore back-facing triangles
	settings.mBackFaceModeConvex = JPH::EBackFaceMode::IgnoreBackFaces;   // Ignore back-facing convex shapes
	settings.mTreatConvexAsSolid = false; //false; // Treat convex shapes as solid

	JPH::BodyInterface& bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();
	ClosestHitRayCollector collector;
	MyObjectLayerFilter objectLayerFilter(mask);
	IgnoreSensorBodyFilter bodyFilter(physicsWorld);
	
	physicsWorld->physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector, {}, objectLayerFilter, bodyFilter);
	if(collector.HadHit()){
		const JPH::RayCastResult& result = collector.GetHit();
		JPH::BodyID hitBodyID = result.mBodyID;
		float hitFraction = result.mFraction; // From 0.0 to 1.0
		JPH::Vec3 hitPosition = ray.mOrigin + ray.mDirection * hitFraction;

		// Get the body that was hit
		const JPH::BodyLockRead lock(physicsWorld->physicsSystem.GetBodyLockInterfaceNoLock(), result.mBodyID);
		Assert(lock.Succeeded());

		const JPH::Body& body = lock.GetBody();
		JPH::Vec3 hitPoint = ray.GetPointOnRay(result.mFraction);

		//hit.entity = static_cast<Entity>(body.GetUserData()); // safe cast
		uint32_t _entity;
		int32_t _index;
		DecodeUserData(body.GetUserData(), _entity, _index);
		hit.entity = static_cast<Entity>(_entity);
		hit.subBodyIndex = _index;
		hit.hitPoint = FromJolt(hitPoint);
		hit.hitNormal = FromJolt(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint));
		return true;
	}

    return false;
}

std::vector<RayResult> PhysicsSystem::OverlapSphere(Vector3 center, float radius) {
    Assert(physicsWorld != nullptr);

    std::vector<RayResult> results;

    // Convert input to Jolt types
    JPH::Vec3 sphereCenter = ToJolt(center);
    JPH::SphereShape sphereShape(radius);
    JPH::CollideShapeSettings settings;
    settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideOnlyWithActive;
    settings.mCollisionTolerance = 0.0f; // Exact collision
    settings.mMaxSeparationDistance = 0.0f; // No penetration allowed

    // Collector to gather unique hits
    class SphereOverlapCollector : public JPH::CollideShapeCollector {
    public:
        std::vector<RayResult>& mResults;
        JPH::Vec3 mCenter;
        const JPH::PhysicsSystem& mPhysicsSystem;
        std::set<JPH::BodyID> mHitBodyIDs; // Track unique BodyIDs using std::set

        SphereOverlapCollector(std::vector<RayResult>& results, JPH::Vec3 center, const JPH::PhysicsSystem& physicsSystem)
            : mResults(results), mCenter(center), mPhysicsSystem(physicsSystem) {}

        void AddHit(const JPH::CollideShapeResult& inResult) override {
            // Only process the first hit for each body
            if(mHitBodyIDs.find(inResult.mBodyID2) != mHitBodyIDs.end()) {
                return; // Skip if this body was already processed
            }
            mHitBodyIDs.insert(inResult.mBodyID2);

            RayResult hit;

            // Lock the body to get its data
            JPH::BodyLockRead lock(mPhysicsSystem.GetBodyLockInterface(), inResult.mBodyID2);
            if(!lock.Succeeded()) return;

            const JPH::Body& body = lock.GetBody();
            //hit.entity = static_cast<Entity>(body.GetUserData());
			uint32_t _entity;
			int32_t _index;
			DecodeUserData(body.GetUserData(), _entity, _index);
			hit.entity = static_cast<Entity>(_entity);
			hit.subBodyIndex = _index;

            // Use the closest point on the hit shape as the hit point
            hit.hitPoint = FromJolt(inResult.mContactPointOn2);

            // Calculate the normal at the contact point
            hit.hitNormal = FromJolt(body.GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, inResult.mContactPointOn2));

            mResults.push_back(hit);
        }
    };

    // Perform the sphere overlap query
    JPH::BodyInterface& bodyInterface = physicsWorld->physicsSystem.GetBodyInterface();
    SphereOverlapCollector collector(results, sphereCenter, physicsWorld->physicsSystem);

    // Use CollideShape to test the sphere against all bodies
    physicsWorld->physicsSystem.GetNarrowPhaseQuery().CollideShape(
        &sphereShape,           // The sphere shape
        JPH::Vec3::sReplicate(1.0f), // Scale of the shape (no scaling)
        JPH::Mat44::sTranslation(sphereCenter), // Transform of the sphere
        settings,               // Collision settings
        sphereCenter,          // Base offset
        collector              // Collector for results
    );

    return results;
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

void PhysicsSystem::AddOnTriggerExitCallback(OnCollisionCallback callback){ 
	onTriggerExitCallbacks.push_back(callback); 
}

void PhysicsSystem::RemoveOnTriggerExitCallback(OnCollisionCallback callback){
    onTriggerExitCallbacks.erase(std::remove(onTriggerExitCallbacks.begin(), onTriggerExitCallbacks.end(), callback), onTriggerExitCallbacks.end());
}

void PhysicsSystem::OnRemoveHeightmap(entt::registry& r, entt::entity e){
	HeightmapColliderComponent& shape = r.get<HeightmapColliderComponent>(e);
    if(shape.data == nullptr) return;

	PhysicsSystem* physicsSystem = r.ctx().get<PhysicsSystem*>();
    BodyInterface &bodyInterface = physicsSystem->physicsWorld->physicsSystem.GetBodyInterface();
    bodyInterface.RemoveBody(shape.data->bodyID);
    bodyInterface.DestroyBody(shape.data->bodyID);
	delete shape.data;
}

#pragma endregion

}

#endif