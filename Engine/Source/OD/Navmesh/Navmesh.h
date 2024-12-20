#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Graphics/Culling.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

namespace OD{

class Scene;
class Mesh;

enum SamplePartitionType{
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS
};

struct OD_API BuildSettings{
	// Cell size in world units
	float cellSize = 0.3f;
	// Cell height in world units
	float cellHeight = 0.2f;
	// Agent height in world units
	float agentHeight = 2.0f;
	// Agent radius in world units
	float agentRadius = 0.6f;
	// Agent max climb in world units
	float agentMaxClimb = 0.9f;
	// Agent max slope in degrees
	float agentMaxSlope = 45.0f;
	// Region minimum size in voxels.
	// regionMinSize = sqrt(regionMinArea)
	float regionMinSize = 8;
	// Region merge size in voxels.
	// regionMergeSize = sqrt(regionMergeArea)
	float regionMergeSize = 20;
	// Edge max length in world units
	float edgeMaxLen = 12.0f;
	// Edge max error in voxels
	float edgeMaxError = 1.3f;
	float vertsPerPoly = 6.0f;
	// Detail sample distance in voxels
	float detailSampleDist = 6.0f;
	// Detail sample max error in voxel heights.
	float detailSampleMaxError = 1.0f;
	// Partition type, see SamplePartitionType
	int partitionType = SAMPLE_PARTITION_WATERSHED;
	// Bounds of the area to mesh
	float navMeshBMin[3];
	float navMeshBMax[3];
	// Size of the tiles in voxels
	float tileSize;

	template<class Archive>
    void serialize(Archive& ar){
		ArchiveDumpNVP(ar, cellSize);
		ArchiveDumpNVP(ar, cellHeight);
		ArchiveDumpNVP(ar, agentHeight);
		ArchiveDumpNVP(ar, agentRadius);
		ArchiveDumpNVP(ar, agentMaxClimb);
		ArchiveDumpNVP(ar, agentMaxSlope);
		ArchiveDumpNVP(ar, regionMinSize);
		ArchiveDumpNVP(ar, regionMergeSize);
		ArchiveDumpNVP(ar, edgeMaxLen);
		ArchiveDumpNVP(ar, edgeMaxError);
		ArchiveDumpNVP(ar, vertsPerPoly);
		ArchiveDumpNVP(ar, detailSampleDist);
		ArchiveDumpNVP(ar, detailSampleMaxError);
		//ArchiveDumpNVP(ar, partitionType);
		//ArchiveDumpNVP(ar, navMeshBMin);
		//ArchiveDumpNVP(ar, navMeshBMax);
		//ArchiveDumpNVP(ar, tileSize);
	}
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
	BuildSettings buildSettings;
	Vector3 size;
	Ref<Navmesh> navmesh;
	
    static inline void OnGui(Entity& e);

	template<class Archive> 
	void serialize(Archive& ar){
		ArchiveDumpNVP(ar, buildSettings);
		ArchiveDumpNVP(ar, size);
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

	template<class Archive>
    void serialize(Archive& ar){
		ArchiveDumpNVP(ar, speed);
		ArchiveDumpNVP(ar, stopDistance);
	}

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
    
	//NavmeshSystem* Clone(Scene* inScene) const override{ return new NavmeshSystem(inScene); }
    virtual SystemType Type() override { return SystemType::Physics; }

    virtual void Update() override;
	virtual void OnDrawGizmos() override;
};

void NavmeshModuleInit();

}