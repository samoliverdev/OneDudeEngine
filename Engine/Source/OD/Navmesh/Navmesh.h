#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/Asset.h"
#include "OD/Graphics/Culling.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <DetourCrowd.h>
#include <Recast.h>

namespace cereal {

template<class Archive>
void serialize(Archive& ar, dtNavMeshParams& p) {
    ar(cereal::make_nvp("orig", p.orig),
       cereal::make_nvp("tileWidth", p.tileWidth),
       cereal::make_nvp("tileHeight", p.tileHeight),
       cereal::make_nvp("maxTiles", p.maxTiles),
       cereal::make_nvp("maxPolys", p.maxPolys));
}

} 

namespace OD{

class Scene;
class Mesh;

enum SamplePartitionType{
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS
};

struct OD_API BuildSettings{
	bool useTile = false;

	// Cell size in world units
	float cellSize = 0.3f;
	// Cell height in world units
	float cellHeight = 0.2f;
	// Agent height in world units
	float agentHeight = 2.0f;
	// Agent radius in world units
	float agentRadius = 0.3f;
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
	float tileSize = 256;

	//int maxTiles;
	//int maxPolysPerTile;

	template<class Archive>
    void serialize(Archive& ar){
		ArchiveDumpNVP(ar, useTile);
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
	NavMeshPathStatus status = NavMeshPathStatus::PathInvalid;
};

class OD_API Navmesh: public Asset{
	friend class NavmeshSystem;
public:
	DrawMode m_drawMode = DRAWMODE_NAVMESH;

	struct BakeData;

	~Navmesh();

	bool Bake(Scene* scene, AABB bounds, BuildSettings buildSettings, LayerMask layerMask = {});
	bool BakeSingle(Scene* scene, AABB bounds, BuildSettings buildSettings, LayerMask layerMask = {});
	bool BakeAllTiles(Scene* scene, AABB bounds, BuildSettings buildSettings, LayerMask layerMask = {});
	bool BakeTile(Scene* scene, AABB bounds, const Vector3 pos, BuildSettings buildSettings);
	bool RemoveTile(Scene* scene, AABB bounds, const Vector3 pos);

	void Cleanup(BakeData& data);

	void DrawDebug();
	bool FindPath(Vector3 startPos, Vector3 endPos, NavMeshPath& outPath);
	bool SamplePosition(Vector3 position, Vector3& outClosestPoint, float maxSearchRadius = 2.0f);

	bool LoadFromFile(const std::string& _path) override;
    bool SaveAs(const std::string& _path) override;
    std::vector<std::string> GetFileAssociations() override;

	bool InitBake(Scene* scene, AABB bounds, BuildSettings buildSettings, LayerMask layerMask = {});
	void UpdateTilesNear(Scene* scene, AABB bounds, Vector3 viewPos, int radiusInTiles);
	void BakeNextTile(Scene* scene, AABB bounds);

	// Returns true if a tile exists at tile coords (tx,ty) and contains data.
	bool HasTile(int tx, int ty) const;
    // Convert world position -> tile coords using explicit bounds and build settings.
    IVector2 WorldPosToTile(const Vector3& pos, AABB& bounds, const BuildSettings& settings) const;
    // Given tile coords, compute tile-world AABB (outMin/outMax are float[3])
    void GetTileWorldBounds(int tx, int ty, AABB& bounds, const BuildSettings& settings, float outMin[3], float outMax[3]) const;
    // Convenience: center point of the tile in world space.
    Vector3 TileCenterWorld(int tx, int ty, AABB& bounds, const BuildSettings& settings) const;

	/*template<class Archive> 
	void serialize(Archive& ar){
		ArchiveDumpNVP(ar, buildSettings);
		ArchiveDumpNVP(ar, mask);
	}*/

	/*
	template<class Archive>
	void save(Archive& ar) const {
		// Save build settings and mask
		ar(cereal::make_nvp("BuildSettings", buildSettings));
		ar(cereal::make_nvp("Mask", mask));

		if (!m_navMesh) {
			int tileCount = 0;
			ar(cereal::make_nvp("TileCount", tileCount));
			return;
		}

		// Save params
		const dtNavMeshParams* params = m_navMesh->getParams();
		ar(cereal::make_nvp("NavMeshParams", *params));

		// Count valid tiles
		int tileCount = 0;
		for (int i = 0; i < m_navMesh->getMaxTiles(); i++) {
			const dtMeshTile* tile = ((const dtNavMesh*)m_navMesh)->getTile(i);
			if (tile && tile->header && tile->dataSize > 0) {
				tileCount++;
			}
		}
		ar(cereal::make_nvp("TileCount", tileCount));

		// Serialize each tile
		for (int i = 0; i < m_navMesh->getMaxTiles(); i++) {
			const dtMeshTile* tile = ((const dtNavMesh*)m_navMesh)->getTile(i);
			if (!tile || !tile->header || !tile->dataSize) continue;

			std::vector<uint8_t> blob(tile->dataSize);
			std::memcpy(blob.data(), tile->data, tile->dataSize);

			ar(cereal::make_nvp("TileData", blob));
		}
	}

	template<class Archive>
	void load(Archive& ar) {
		ar(cereal::make_nvp("BuildSettings", buildSettings));
		ar(cereal::make_nvp("Mask", mask));

		int tileCount = 0;
		ar(cereal::make_nvp("TileCount", tileCount));

		if (tileCount <= 0) {
			m_navMesh = nullptr;
			return;
		}

		// Load params
		dtNavMeshParams params{};
		ar(cereal::make_nvp("NavMeshParams", params));

		dtNavMesh* navMesh = dtAllocNavMesh();
		if (!navMesh || dtStatusFailed(navMesh->init(&params))) {
			if (navMesh) dtFreeNavMesh(navMesh);
			m_navMesh = nullptr;
			return;
		}

		// Read all tiles
		for (int i = 0; i < tileCount; i++) {
			std::vector<uint8_t> blob;
			ar(cereal::make_nvp("TileData", blob));

			unsigned char* data = (unsigned char*)dtAlloc((int)blob.size(), DT_ALLOC_PERM);
			std::memcpy(data, blob.data(), blob.size());

			dtStatus status = navMesh->addTile(data, (int)blob.size(), DT_TILE_FREE_DATA, 0, nullptr);
			if (dtStatusFailed(status)) {
				dtFree(data);
			}
		}

		m_navMesh = navMesh;

		if (!m_navQuery) m_navQuery = dtAllocNavMeshQuery();
		m_navQuery->init(m_navMesh, 2048);
	}
	*/

private:
	bool TileInit(Scene* scene, AABB bounds);
	void _TileInit0(BakeData& data);

	static const int MAX_POLYS = 256*2;

	bool hasInitTile = false;

	BuildSettings buildSettings;
	LayerMask mask;

	struct BakeData{
		unsigned char* m_triareas = nullptr;
		rcHeightfield* m_solid = nullptr;
		rcCompactHeightfield* m_chf = nullptr;
		rcContourSet* m_cset = nullptr;
		rcPolyMesh* m_pmesh = nullptr;
		rcConfig m_cfg;	
		rcPolyMeshDetail* m_dmesh = nullptr;
		rcContext* m_ctx = nullptr;

		bool hasInitTile2 = false;

		~BakeData(){
			delete [] m_triareas;
			m_triareas = 0;
			rcFreeHeightField(m_solid);
			m_solid = 0;
			rcFreeCompactHeightfield(m_chf);
			m_chf = 0;
			rcFreeContourSet(m_cset);
			m_cset = 0;
			rcFreePolyMesh(m_pmesh);
			m_pmesh = 0;
			rcFreePolyMeshDetail(m_dmesh);
			m_dmesh = 0;
			delete m_ctx;
		}
	};

    /*unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;	
	rcPolyMeshDetail* m_dmesh;
	rcContext* m_ctx;
	*/
	
	BakeData bakeData;
	dtNavMesh* m_navMesh;
	class dtNavMeshQuery* m_navQuery;
	dtCrowd* m_crowd;

	int m_partitionType;
	unsigned char m_navMeshDrawFlags = 0;
	NavmeshBuildData builData;

	dtPolyRef m_polys[MAX_POLYS];
	float m_straightPath[MAX_POLYS*3];
	unsigned char m_straightPathFlags[MAX_POLYS];
	dtPolyRef m_straightPathPolys[MAX_POLYS];
	int m_nstraightPath = 0;
	int m_npolys = 0;

	float m_lastBuiltTileBmin[3];
	float m_lastBuiltTileBmax[3];

	struct TilePos { int x, y; };
	std::queue<TilePos> m_pendingBake;

	void RasterizeScene(BakeData& data, Scene& scene, AABB& bounds);
	bool RasterizeMesh(BakeData& data, const Matrix4& model, Ref<Mesh>& mesh);
	void GetTilePos(const float* pos, int& tx, int& ty);
	unsigned char* BuildTileMesh(BakeData& data, Scene* scene, const int tx, const int ty, const float* bmin, const float* bmax, int& dataSize);
};

struct OD_API NavmeshSkipTag{
	int id = 0;
	template<class Archive> void serialize(Archive& ar){}
};

struct OD_API NavmeshComponent{
	enum class AgentUpdateMode{FindPath, Crowd};

	//BuildSettings buildSettings;
	BuildSettings buildSettings;
	LayerMask mask;

	AgentUpdateMode agentUpdateMode;
	Vector3 size = {250, 250, 250};
	Ref<Navmesh> navmesh;

    static inline void OnGui(Entity& e, Scene& scene);

	template<class Archive> 
	void serialize(Archive& ar){
		//ArchiveDumpNVP(ar, buildSettings);
		ArchiveDumpNVP(ar, agentUpdateMode);
		ArchiveDumpNVP(ar, size);
		ArchiveDumpNVP(ar, buildSettings);
		ArchiveDumpNVP(ar, mask);
		//ArchiveDumpNVP(ar, navmesh);

		AssetRefSerialize<Navmesh> _navmesh(navmesh);
        ArchiveDump(ar, CEREAL_NVP(_navmesh));
	}
};

struct OD_API NavmeshAgentComponent{
	friend class NavmeshSystem;

	float speed = 2;
	float stopDistance = 0.25f;
	
	bool manualUpdate = false;

	Vector3 desiredVelocity;

	Vector3 GetDestination();
	void SetDestination(Vector3 d);

	void Reset(); 

	template<class Archive>
    void serialize(Archive& ar){
		ArchiveDumpNVP(ar, speed);
		ArchiveDumpNVP(ar, stopDistance);

		if constexpr(std::is_same_v<Archive, cereal::ImGuiArchive>){
            ArchiveDumpNVP(ar, destination);
			ArchiveDumpNVP(ar, lastPos);
			ArchiveDumpNVP(ar, isDirty);
			ArchiveDumpNVP(ar, curPathIndex);
			ArchiveDumpNVP(ar, reach);
			ArchiveDumpNVP(ar, hasInit);
		}
	}
private:
	Vector3 destination = {0, 0, 0};
	Vector3 lastPos = {0, 0, 0};
	bool isDirty = false;
	NavMeshPath path;
	int curPathIndex = -1;
	bool reach = false;
	bool hasInit = false;

	int crowdId = -1;
	Ref<Navmesh> navmesh = nullptr;
};

class OD_API NavmeshSystem: public System{
public:
	void OnInit(Scene& scene) override;
    void OnEnd(Scene& scene) override;
    
	//NavmeshSystem* Clone(Scene* inScene) const override{ return new NavmeshSystem(inScene); }
    virtual int Type() override { return SystemType::Late; }

    virtual void LateUpdate(Scene& scene) override;
	virtual void OnDrawGizmos(Scene& scene, Camera& cam) override;
	virtual void OnDrawGizmosSelected(Scene& scene, Camera& cam, Entity entity) override;
private:
	static void OnRemoveAgent(entt::registry& r, entt::entity e);

	//Scene* scene;
};

void NavmeshModuleInit();

}