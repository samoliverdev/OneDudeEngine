#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Graphics/Culling.h"
#include "OD/Scene/Scene.h"
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

namespace OD{

class Scene;
class Mesh;

struct OD_API BuildSettings{
	// Cell size in world units
	float cellSize;
	// Cell height in world units
	float cellHeight;
	// Agent height in world units
	float agentHeight;
	// Agent radius in world units
	float agentRadius;
	// Agent max climb in world units
	float agentMaxClimb;
	// Agent max slope in degrees
	float agentMaxSlope;
	// Region minimum size in voxels.
	// regionMinSize = sqrt(regionMinArea)
	float regionMinSize;
	// Region merge size in voxels.
	// regionMergeSize = sqrt(regionMergeArea)
	float regionMergeSize;
	// Edge max length in world units
	float edgeMaxLen;
	// Edge max error in voxels
	float edgeMaxError;
	float vertsPerPoly;
	// Detail sample distance in voxels
	float detailSampleDist;
	// Detail sample max error in voxel heights.
	float detailSampleMaxError;
	// Partition type, see SamplePartitionType
	int partitionType;
	// Bounds of the area to mesh
	float navMeshBMin[3];
	float navMeshBMax[3];
	// Size of the tiles in voxels
	float tileSize;
};

enum SamplePartitionType{
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS
};

enum SamplePolyAreas{
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP
};

struct OD_API NavmeshBuildData{
	static const int MAX_OFFMESH_CONNECTIONS = 256;
	float m_offMeshConVerts[MAX_OFFMESH_CONNECTIONS*3*2];
	float m_offMeshConRads[MAX_OFFMESH_CONNECTIONS];
	unsigned char m_offMeshConDirs[MAX_OFFMESH_CONNECTIONS];
	unsigned char m_offMeshConAreas[MAX_OFFMESH_CONNECTIONS];
	unsigned short m_offMeshConFlags[MAX_OFFMESH_CONNECTIONS];
	unsigned int m_offMeshConId[MAX_OFFMESH_CONNECTIONS];
	int m_offMeshConCount;
};

enum DrawMode{
	DRAWMODE_NAVMESH,
	DRAWMODE_NAVMESH_TRANS,
	DRAWMODE_NAVMESH_BVTREE,
	DRAWMODE_NAVMESH_NODES,
	DRAWMODE_NAVMESH_INVIS,
	DRAWMODE_MESH,
	DRAWMODE_VOXELS,
	DRAWMODE_VOXELS_WALKABLE,
	DRAWMODE_COMPACT,
	DRAWMODE_COMPACT_DISTANCE,
	DRAWMODE_COMPACT_REGIONS,
	DRAWMODE_REGION_CONNECTIONS,
	DRAWMODE_RAW_CONTOURS,
	DRAWMODE_BOTH_CONTOURS,
	DRAWMODE_CONTOURS,
	DRAWMODE_POLYMESH,
	DRAWMODE_POLYMESH_DETAIL,
	MAX_DRAWMODE
};

enum class NavMeshPathStatus{
	PathComplete,
	PathPartial,
	PathInvalid
};

struct OD_API NavMeshPath{
	std::vector<Vector3> corners;
	NavMeshPathStatus status;
};

class OD_API Navmesh{
public:
	BuildSettings buildSettings;
	DrawMode m_drawMode = DRAWMODE_NAVMESH;

	/*Navmesh(){ 
		LogWarning("Navmesh::Navmesh()"); 
	}*/
	/*~Navmesh(){ 
		LogWarning("Navmesh::~Navmesh()"); 
	}*/

	bool Bake(Scene* scene, AABB bounds);
	void Cleanup();
	void DrawDebug();
	bool FindPath(Vector3 startPos, Vector3 endPos, NavMeshPath& outPath);

private:
	static const int MAX_POLYS = 256*2;

    unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;	
	rcPolyMeshDetail* m_dmesh;
	rcContext* m_ctx;
	class dtNavMesh* m_navMesh;
	class dtNavMeshQuery* m_navQuery;
	int m_partitionType;
	unsigned char m_navMeshDrawFlags = 0;
	NavmeshBuildData builData;

	dtPolyRef m_polys[MAX_POLYS];
	float m_straightPath[MAX_POLYS*3];
	unsigned char m_straightPathFlags[MAX_POLYS];
	dtPolyRef m_straightPathPolys[MAX_POLYS];
	int m_nstraightPath = 0;
	int m_npolys = 0;

	bool RasterizeMesh(const Matrix4& model, Ref<Mesh>& mesh);
};

struct OD_API NavmeshComponent{
	Ref<Navmesh> navmesh;

	template<class Archive> void serialize(Archive& ar){}
    static inline void OnGui(Entity& e){
		if(ImGui::Button("Bake")){
			NavmeshComponent& navmeshComponent = e.GetComponent<NavmeshComponent>();
			if(navmeshComponent.navmesh != nullptr) navmeshComponent.navmesh->Bake(e.GetScene(), AABB(Vector3(0, 0, 0), 100, 100, 100)); 
		}
	}
};

struct OD_API NavmeshAgentComponent{
	friend class NavmeshSystem;

	float speed = 2;
	float stopDistance = 0.25f;

	Vector3 GetDestination(){ return destination; }
	void SetDestination(Vector3 d){
		if(d == destination) return;
		destination = d;
		isDirty = true;
	}

	template<class Archive> void serialize(Archive& ar){}
    static inline void OnGui(Entity& e){}

private:
	Vector3 destination = {0, 0, 0};
	Vector3 lastPos = {0, 0, 0};
	bool isDirty = false;
	NavMeshPath path;
	int curPathIndex = -1;
	bool reach = false;
};

class OD_API NavmeshSystem: public System{
public:
	NavmeshSystem(Scene* scene);
    ~NavmeshSystem() override;
    
	NavmeshSystem* Clone(Scene* inScene) const override{ return new NavmeshSystem(inScene); }
    virtual SystemType Type() override { return SystemType::Physics; }

    virtual void Update() override;
	virtual void OnDrawGizmos() override;
};

void NavmeshModuleInit();

}