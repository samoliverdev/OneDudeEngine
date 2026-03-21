#include "OD/pch.h"
#include "Navmesh.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Model.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Graphics/Culling.h"
#include "OD/Graphics/Geometry.h"
#include "OD/Physics/PhysicsSystem.h"
#include <DebugDraw.h>
#include <DetourDebugDraw.h>
#include <DetourCommon.h>
#include <DetourCrowd.h>
#include <DetourObstacleAvoidance.h>

#include <taskflow/taskflow.hpp> 
#include <taskflow/algorithm/for_each.hpp>

namespace OD{

void NavmeshModuleInit(){
	SceneManager::Get().RegisterCoreComponent<NavmeshSkipTag>("NavmeshSkipTag", "AI");
	SceneManager::Get().RegisterCoreComponent<NavmeshComponent>("NavmeshComponent", "AI");
	SceneManager::Get().RegisterCoreComponent<NavmeshAgentComponent>("NavmeshAgentComponent", "AI");
	SceneManager::Get().RegisterSystem<NavmeshSystem>("NavmeshSystem");

	AssetTypesDB::Get().RegisterAssetType<Navmesh>(".navmesh", [](const std::string& path){ return AssetManager::Get().LoadAsset<Navmesh>(path); });
}

void NavmeshComponent::QuickBake(Scene& scene, Entity& e){
	TransformComponent& trans = scene.GetComponent<TransformComponent>(e);

	//if(navmesh == nullptr) 
	navmesh = CreateRef<Navmesh>();
	//if(navmesh != nullptr){
		navmesh->Bake(
			&scene, 
			AABB(
				trans.Position(), 
				size.x, size.y, size.z
			),
			buildSettings,
			mask
		);
	//}
}

void NavmeshComponent::OnGui(Entity& e, Scene& scene){
	TransformComponent& trans = scene.GetComponent<TransformComponent>(e);

	NavmeshComponent& navmeshComponent = scene.GetComponent<NavmeshComponent>(e);
	cereal::ImGuiArchive uiArchive;
	uiArchive(navmeshComponent);

	if(ImGui::Button("Bake")){
		if(navmeshComponent.navmesh == nullptr) navmeshComponent.navmesh = CreateRef<Navmesh>();
		if(navmeshComponent.navmesh != nullptr){
			//navmeshComponent.navmesh->buildSettings = navmeshComponent.buildSettings;
			navmeshComponent.navmesh->Bake(
				&scene, 
				AABB(
					trans.Position(), 
					navmeshComponent.size.x, navmeshComponent.size.y, navmeshComponent.size.z
				),
				navmeshComponent.buildSettings,
				navmeshComponent.mask
			);
			if(scene.PathIsValid() && scene.Running() == false){
				std::string savePath = scene.Path() + "_Navmesh_" + std::to_string((size_t)e) + ".navmesh";
				navmeshComponent.navmesh->Save(savePath, Asset::SaveType::AssetBinary);
			}
		}
	}
}

class DebugDrawGL : public duDebugDraw{
public:
    Ref<Material> shader;
    Ref<Mesh> mesh;

    DebugDrawGL(){
        shader = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Navmesh.glsl"));
        mesh = CreateRef<Mesh>();
    }

	virtual void depthMask(bool state){
        //glDepthMask(state ? GL_TRUE : GL_FALSE);
        //Graphics::SetDepthMask(state);
    }
	
    virtual void texture(bool state){
        /*if(state){
            glEnable(GL_TEXTURE_2D);
            g_tex.bind();
        } else{
            glDisable(GL_TEXTURE_2D);
        }*/
    }

	virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f){
        //LogWarning("DebugDrawGL::begin %d", prim);
        /*switch(prim){
            case DU_DRAW_POINTS:
                glPointSize(size);
                glBegin(GL_POINTS);
                break;
            case DU_DRAW_LINES:
                glLineWidth(size);
                glBegin(GL_LINES);
                break;
            case DU_DRAW_TRIS:
                glBegin(GL_TRIANGLES);
                break;
            case DU_DRAW_QUADS:
                glBegin(GL_QUADS);
                break;
        };*/

        mesh->vertices.clear();
        if(prim == DU_DRAW_POINTS) mesh->drawMode = MeshDrawMode::POINTS;
        if(prim == DU_DRAW_LINES) mesh->drawMode = MeshDrawMode::LINES;
        if(prim == DU_DRAW_TRIS) mesh->drawMode = MeshDrawMode::TRIANGLES;
        if(prim == DU_DRAW_QUADS) mesh->drawMode = MeshDrawMode::QUADS;
    }
	
    virtual void vertex(const float* pos, unsigned int color){
        //glColor4ubv((GLubyte*)&color);
	    //glVertex3fv(pos);
        mesh->vertices.push_back(Vector3(pos[0], pos[1], pos[2]));
    }
	
    virtual void vertex(const float x, const float y, const float z, unsigned int color){
        //glColor4ubv((GLubyte*)&color);
	    //glVertex3f(x,y,z);
        mesh->vertices.push_back(Vector3(x, y, z));
    }
	
    virtual void vertex(const float* pos, unsigned int color, const float* uv){
        //glColor4ubv((GLubyte*)&color);
        //glTexCoord2fv(uv);
        //glVertex3fv(pos);
        mesh->vertices.push_back(Vector3(pos[0], pos[1], pos[2]));
    }
	
    virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v){
        //glColor4ubv((GLubyte*)&color);
        //glTexCoord2f(u,v);
        //glVertex3f(x,y,z);
        mesh->vertices.push_back(Vector3(x, y, z));
    }
	
    virtual void end(){
        //glEnd();
        //glLineWidth(1.0f);
        //glPointSize(1.0f);
        if(mesh->vertices.size() == 0) return;

		//SubShader::Bind(*shader);
		//shader->SetFloat("alpha", 0.5f);

        mesh->Submit();
        Graphics::DrawMesh(*mesh, *shader, Matrix4Identity);
    }
};

DebugDrawGL* m_dd = nullptr;

inline unsigned int nextPow2(unsigned int v){
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

inline unsigned int ilog2(unsigned int v){
	unsigned int r;
	unsigned int shift;
	r = (v > 0xffff) << 4; v >>= r;
	shift = (v > 0xff) << 3; v >>= shift; r |= shift;
	shift = (v > 0xf) << 2; v >>= shift; r |= shift;
	shift = (v > 0x3) << 1; v >>= shift; r |= shift;
	r |= (v >> 1);
	return r;
}

enum SamplePolyFlags{
	SAMPLE_POLYFLAGS_WALK		= 0x01,		// Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM		= 0x02,		// Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR		= 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP		= 0x08,		// Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED	= 0x10,		// Disabled polygon
	SAMPLE_POLYFLAGS_ALL		= 0xffff	// All abilities.
};

Navmesh::~Navmesh(){
	/*delete [] m_triareas;
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
	m_dmesh = 0;*/

	dtFreeNavMeshQuery(m_navQuery);

	dtFreeCrowd(m_crowd);
	
	dtFreeNavMesh(m_navMesh);
	m_navMesh = 0;
	//delete m_ctx;
}

void Navmesh::Cleanup(BakeData& data){
    if(data.m_triareas != nullptr) delete [] data.m_triareas;
	data.m_triareas = 0;
	rcFreeHeightField(data.m_solid);
	data.m_solid = 0;
	rcFreeCompactHeightfield(data.m_chf);
	data.m_chf = 0;
	rcFreeContourSet(data.m_cset);
	data.m_cset = 0;
	rcFreePolyMesh(data.m_pmesh);
	data.m_pmesh = 0;
	rcFreePolyMeshDetail(data.m_dmesh);
	data.m_dmesh = 0;
	
	if(buildSettings.useTile == false){
		dtFreeNavMesh(m_navMesh);
		m_navMesh = 0;
		delete data.m_ctx;
	}
}

void Navmesh::RasterizeScene(BakeData& data, Scene& scene, AABB& bounds){
	auto meshView = scene.GetRegistry().view<MeshRendererComponent, TransformComponent, InfoComponent>(entt::exclude<NavmeshSkipTag>);
    for(auto e: meshView){
		auto& info = meshView.get<InfoComponent>(e);
		//if(!(info.layer & mask.mask)) continue;
		if(!(mask.mask & (1u << info.layer))) continue;  

        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        //if(c.material == nullptr) continue;

		Matrix4 targetMatrix = t.GlobalModelMatrix();
		AABB aabb = transform_aabb_optimized_abs_center_extents(c.boundingVolume, targetMatrix);
		//aabb.Expand(Vector3(1.1f));
		if(aabb.isOnAABB(bounds) == false) continue;

		//LogInfo("Navmesh::RasterizeScene::Entity: {}", info.name);

		if(c.mesh->vertices.size() <= 0){
			LogWarning("Entity: {}, Navmesh Try RasterizeMesh with Zero Vertices", info.name);
		}

        RasterizeMesh(data, targetMatrix, c.mesh);
    }

    auto meshRenderView = scene.GetRegistry().view<ModelRendererComponent, TransformComponent, InfoComponent>(entt::exclude<NavmeshSkipTag>);
    for(auto e: meshRenderView){
		auto& info = meshRenderView.get<InfoComponent>(e);
		//if(!(info.layer & mask.mask)) continue;
		if(!(mask.mask & (1u << info.layer))) continue;  

        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;
        //if(c.GetAABB().isOnFrustum(cam.frustum, t) == false) continue;

		//LogInfo("Navmesh::RasterizeScene::Entity: {}", info.name);
        for(auto i: c.GetModel()->renderTargets){
            auto targetMesh = c.GetModel()->meshs[i.meshIndex];
            auto targetMatrix =  t.GlobalModelMatrix() * c.localTransform.GetModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);

			AABB aabb = transform_aabb_optimized_abs_center_extents(c.GetAABB(), targetMatrix);
			//aabb.Expand(Vector3(1.1f));
			if(aabb.isOnAABB(bounds) == false) continue;

            RasterizeMesh(data, targetMatrix, targetMesh);
        }
    }
}

bool Navmesh::RasterizeMesh(BakeData& data, const Matrix4& model, Ref<Mesh>& mesh){
	if(mesh->vertices.size() <= 0){
		LogWarning("Navmesh Try RasterizeMesh with Zero Vertices");
	}

    std::vector<float> _verts;
    std::vector<int> _tris;

    for(Vector3 i: mesh->vertices){
        Vector3 v = model * Vector4(i.x, i.y, i.z, 1);
        _verts.push_back(v.x);
        _verts.push_back(v.y);
        _verts.push_back(v.z);
    }
    for(int i: mesh->indices){
        _tris.push_back(i);
    }

    const float* verts = _verts.data();
    const int nverts = _verts.size()/3;
	const int* tris = _tris.data();
	const int ntris = _tris.size()/3;

    // Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	data.m_triareas = new unsigned char[ntris];
	if(!data.m_triareas){
        LogError("buildNavigation: Out of memory 'm_triareas' ({}).", ntris);
		return false;
	}
	
    // Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(data.m_triareas, 0, ntris*sizeof(unsigned char));
	rcMarkWalkableTriangles(data.m_ctx, data.m_cfg.walkableSlopeAngle, verts, nverts, tris, ntris, data.m_triareas);
	if(!rcRasterizeTriangles(data.m_ctx, verts, nverts, tris, data.m_triareas, ntris, *data.m_solid, data.m_cfg.walkableClimb)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not rasterize triangles.");
        LogError("buildNavigation: Could not rasterize triangles.");
		return false;
	}

	/*int walkableCount = 0;
	for(int i = 0; i < ntris; ++i){
		if(data.m_triareas[i] == RC_WALKABLE_AREA) walkableCount++;
	}
	LogInfo("Walkable triangles: %d / %d", walkableCount, ntris);*/

	delete [] data.m_triareas;
	data.m_triareas = 0;

    return true;
}

bool Navmesh::Bake(Scene* scene, AABB bounds, BuildSettings inbuildSettings, LayerMask layerMask){
	OD_LOG_PROFILE("Navmesh::Bake");

	buildSettings = inbuildSettings;
	bounds.Expand(Vector3(0.5f));

	if(buildSettings.useTile) return BakeAllTiles(scene, bounds, buildSettings, layerMask);
	return BakeSingle(scene, bounds, buildSettings, layerMask);
}

bool Navmesh::BakeSingle(Scene* scene, AABB bounds, BuildSettings inbuildSettings, LayerMask layerMask){
	buildSettings = inbuildSettings;
	
	if(buildSettings.useTile == true) return false;
	mask = layerMask;

	Cleanup(bakeData);
	hasInitTile = false;

    bakeData.m_ctx = new rcContext();
    m_navQuery = dtAllocNavMeshQuery();
	
	Vector3 bmin = bounds.GetMin(); //m_geom->getNavMeshBoundsMin();
	Vector3 bmax = bounds.GetMax(); //m_geom->getNavMeshBoundsMax();
	
	//
	// Step 1. Initialize build config.
	//
	
	// Init build configuration from GUI
	memset(&bakeData.m_cfg, 0, sizeof(bakeData.m_cfg));
	bakeData.m_cfg.cs = buildSettings.cellSize;
	bakeData.m_cfg.ch = buildSettings.cellHeight;
	bakeData.m_cfg.walkableSlopeAngle = buildSettings.agentMaxSlope;
	bakeData.m_cfg.walkableHeight = (int)ceilf(buildSettings.agentHeight / bakeData.m_cfg.ch);
	bakeData.m_cfg.walkableClimb = (int)floorf(buildSettings.agentMaxClimb / bakeData.m_cfg.ch);
	bakeData.m_cfg.walkableRadius = (int)ceilf(buildSettings.agentRadius / bakeData.m_cfg.cs);
	bakeData.m_cfg.maxEdgeLen = (int)(buildSettings.edgeMaxLen / buildSettings.cellSize);
	bakeData.m_cfg.maxSimplificationError = buildSettings.edgeMaxError;
	bakeData.m_cfg.minRegionArea = (int)rcSqr(buildSettings.regionMinSize);		// Note: area = size*size
	bakeData.m_cfg.mergeRegionArea = (int)rcSqr(buildSettings.regionMergeSize);	// Note: area = size*size
	bakeData.m_cfg.maxVertsPerPoly = (int)buildSettings.vertsPerPoly;
	bakeData.m_cfg.detailSampleDist = buildSettings.detailSampleDist < 0.9f ? 0 : buildSettings.cellSize * buildSettings.detailSampleDist;
	bakeData.m_cfg.detailSampleMaxError = buildSettings.cellHeight * buildSettings.detailSampleMaxError;
	
	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcVcopy(bakeData.m_cfg.bmin, &bmin.x);
	rcVcopy(bakeData.m_cfg.bmax, &bmax.x);
	rcCalcGridSize(bakeData.m_cfg.bmin, bakeData.m_cfg.bmax, bakeData.m_cfg.cs, &bakeData.m_cfg.width, &bakeData.m_cfg.height);

	//
	// Step 2. Rasterize input polygon soup.
	//
	
	// Allocate voxel heightfield where we rasterize our input data to.
	bakeData.m_solid = rcAllocHeightfield();
	if(!bakeData.m_solid){
        //m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		LogError("buildNavigation: Out of memory 'solid'.");
		return false;
	}
	if(!rcCreateHeightfield(bakeData.m_ctx, *bakeData.m_solid, bakeData.m_cfg.width, bakeData.m_cfg.height, bakeData.m_cfg.bmin, bakeData.m_cfg.bmax, bakeData.m_cfg.cs, bakeData.m_cfg.ch)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
        LogError("buildNavigation: Could not create solid heightfield.");
		return false;
	}

	//AABB aabb = AABB(Vector3Zero, 10, 10, 10);
	RasterizeScene(bakeData, *scene, bounds);
	
	/*auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshView){
        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RasterizeMesh(t.GlobalModelMatrix(), c.mesh);
    }

    auto meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;
        //if(c.GetAABB().isOnFrustum(cam.frustum, t) == false) continue;

        for(auto i: c.GetModel()->renderTargets){
            auto targetMesh = c.GetModel()->meshs[i.meshIndex];
            auto targetMatrix =  t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);

            RasterizeMesh(targetMatrix, targetMesh);
        }
    }*/
	
	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	//if(m_filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(bakeData.m_ctx, bakeData.m_cfg.walkableClimb, *bakeData.m_solid);
	//if(m_filterLedgeSpans)
		rcFilterLedgeSpans(bakeData.m_ctx, bakeData.m_cfg.walkableHeight, bakeData.m_cfg.walkableClimb, *bakeData.m_solid);
	//if (m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(bakeData.m_ctx, bakeData.m_cfg.walkableHeight, *bakeData.m_solid);


	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	bakeData.m_chf = rcAllocCompactHeightfield();
	if(!bakeData.m_chf){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
        LogError("buildNavigation: Out of memory 'chf'.");
		return false;
	}
	if(!rcBuildCompactHeightfield(bakeData.m_ctx, bakeData.m_cfg.walkableHeight, bakeData.m_cfg.walkableClimb, *bakeData.m_solid, *bakeData.m_chf)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
        LogError("buildNavigation: Could not build compact data.");
		return false;
	}
    
    bool m_keepInterResults = false;
	if(!m_keepInterResults){
		rcFreeHeightField(bakeData.m_solid);
		bakeData.m_solid = 0;
	}
		
	// Erode the walkable area by agent radius.
	if(!rcErodeWalkableArea(bakeData.m_ctx, bakeData.m_cfg.walkableRadius, *bakeData.m_chf)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
        LogError("buildNavigation: Could not erode.");
		return false;
	}

	// (Optional) Mark areas.
	//const ConvexVolume* vols = m_geom->getConvexVolumes();
	//for(int i  = 0; i < m_geom->getConvexVolumeCount(); ++i)
	//	rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);

	
	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	// There are 3 martitioning methods, each with some pros and cons:
	// 1) Watershed partitioning
	//   - the classic Recast partitioning
	//   - creates the nicest tessellation
	//   - usually slowest
	//   - partitions the heightfield into nice regions without holes or overlaps
	//   - the are some corner cases where this method creates produces holes and overlaps
	//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
	//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
	//   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
	// 2) Monotone partioning
	//   - fastest
	//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
	//   - creates long thin polygons, which sometimes causes paths with detours
	//   * use this if you want fast navmesh generation
	// 3) Layer partitoining
	//   - quite fast
	//   - partitions the heighfield into non-overlapping regions
	//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
	//   - produces better triangles than monotone partitioning
	//   - does not have the corner cases of watershed partitioning
	//   - can be slow and create a bit ugly tessellation (still better than monotone)
	//     if you have large open areas with small obstacles (not a problem if you use tiles)
	//   * good choice to use for tiled navmesh with medium and small sized tiles
    
	
	if(m_partitionType == SAMPLE_PARTITION_WATERSHED){
		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if(!rcBuildDistanceField(bakeData.m_ctx, *bakeData.m_chf)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
            LogError("buildNavigation: Could not build distance field.");
			return false;
		}
		
		// Partition the walkable surface into simple regions without holes.
		if(!rcBuildRegions(bakeData.m_ctx, *bakeData.m_chf, 0, bakeData.m_cfg.minRegionArea, bakeData.m_cfg.mergeRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
            LogError("buildNavigation: Could not build watershed regions.");
			return false;
		}
	} else if(m_partitionType == SAMPLE_PARTITION_MONOTONE){
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if(!rcBuildRegionsMonotone(bakeData.m_ctx, *bakeData.m_chf, 0, bakeData.m_cfg.minRegionArea, bakeData.m_cfg.mergeRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
            LogError("buildNavigation: Could not build monotone regions.");
			return false;
		}
	} else{ // SAMPLE_PARTITION_LAYERS
		// Partition the walkable surface into simple regions without holes.
		if(!rcBuildLayerRegions(bakeData.m_ctx, *bakeData.m_chf, 0, bakeData.m_cfg.minRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
            LogError("buildNavigation: Could not build layer regions.");
			return false;
		}
	}
	
	//
	// Step 5. Trace and simplify region contours.
	//
	
	// Create contours.
	bakeData.m_cset = rcAllocContourSet();
	if(!bakeData.m_cset){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
        LogError("buildNavigation: Out of memory 'cset'.");
		return false;
	}
	if(!rcBuildContours(bakeData.m_ctx, *bakeData.m_chf, bakeData.m_cfg.maxSimplificationError, bakeData.m_cfg.maxEdgeLen, *bakeData.m_cset)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
        LogError("buildNavigation: Could not create contours.");
		return false;
	}
	
	//
	// Step 6. Build polygons mesh from contours.
	//
	
	// Build polygon navmesh from the contours.
	bakeData.m_pmesh = rcAllocPolyMesh();
	if(!bakeData.m_pmesh){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
        LogError("buildNavigation: Out of memory 'pmesh'.");
		return false;
	}
	if(!rcBuildPolyMesh(bakeData.m_ctx, *bakeData.m_cset, bakeData.m_cfg.maxVertsPerPoly, *bakeData.m_pmesh)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
        LogError("buildNavigation: Could not triangulate contours.");
		return false;
	}
	
	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//
	
	bakeData.m_dmesh = rcAllocPolyMeshDetail();
	if(!bakeData.m_dmesh){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
        LogError("buildNavigation: Out of memory 'pmdtl'.");
		return false;
	}

	if(!rcBuildPolyMeshDetail(bakeData.m_ctx, *bakeData.m_pmesh, *bakeData.m_chf, bakeData.m_cfg.detailSampleDist, bakeData.m_cfg.detailSampleMaxError, *bakeData.m_dmesh)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
        LogError("buildNavigation: Could not build detail mesh.");
		return false;
	}

	if(!m_keepInterResults){
		rcFreeCompactHeightfield(bakeData.m_chf);
		bakeData.m_chf = 0;
		rcFreeContourSet(bakeData.m_cset);
		bakeData.m_cset = 0;
	}

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.
	
	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//
	
	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
	if(bakeData.m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON){
		unsigned char* navData = 0;
		int navDataSize = 0;

		// Update poly flags from areas.
		for(int i = 0; i < bakeData.m_pmesh->npolys; ++i){
			if(bakeData.m_pmesh->areas[i] == RC_WALKABLE_AREA)
				bakeData.m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
				
			if(bakeData.m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				bakeData.m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				bakeData.m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
            {
				bakeData.m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			} else if(bakeData.m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER){
				bakeData.m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			} else if(bakeData.m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR){
				bakeData.m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}

		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = bakeData.m_pmesh->verts;
		params.vertCount = bakeData.m_pmesh->nverts;
		params.polys = bakeData.m_pmesh->polys;
		params.polyAreas = bakeData.m_pmesh->areas;
		params.polyFlags = bakeData.m_pmesh->flags;
		params.polyCount = bakeData.m_pmesh->npolys;
		params.nvp = bakeData.m_pmesh->nvp;
		params.detailMeshes = bakeData.m_dmesh->meshes;
		params.detailVerts = bakeData.m_dmesh->verts;
		params.detailVertsCount = bakeData.m_dmesh->nverts;
		params.detailTris = bakeData.m_dmesh->tris;
		params.detailTriCount = bakeData.m_dmesh->ntris;
		params.offMeshConVerts = builData.m_offMeshConVerts; //m_geom->getOffMeshConnectionVerts();
		params.offMeshConRad = builData.m_offMeshConRads;// m_geom->getOffMeshConnectionRads();
		params.offMeshConDir = builData.m_offMeshConDirs;// m_geom->getOffMeshConnectionDirs();
		params.offMeshConAreas = builData.m_offMeshConAreas;// m_geom->getOffMeshConnectionAreas();
		params.offMeshConFlags = builData.m_offMeshConFlags;// m_geom->getOffMeshConnectionFlags();
		params.offMeshConUserID = builData.m_offMeshConId;// m_geom->getOffMeshConnectionId();
		params.offMeshConCount = builData.m_offMeshConCount;// m_geom->getOffMeshConnectionCount();
		params.walkableHeight = buildSettings.agentHeight;
		params.walkableRadius = buildSettings.agentRadius;
		params.walkableClimb = buildSettings.agentMaxClimb;
		rcVcopy(params.bmin, bakeData.m_pmesh->bmin);
		rcVcopy(params.bmax, bakeData.m_pmesh->bmax);
		params.cs = bakeData.m_cfg.cs;
		params.ch = bakeData.m_cfg.ch;
		params.buildBvTree = true;
		
		if(!dtCreateNavMeshData(&params, &navData, &navDataSize)){
			//m_ctx->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
            LogError("Could not build Detour navmesh.");
			return false;
		}
		
		m_navMesh = dtAllocNavMesh();
		if(!m_navMesh){
			dtFree(navData);
			//m_ctx->log(RC_LOG_ERROR, "Could not create Detour navmesh");
            LogError("Could not create Detour navmesh");
			return false;
		}
		
		dtStatus status;
		
		status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if(dtStatusFailed(status)){
			dtFree(navData);
			//m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh");
			LogError("Could not init Detour navmesh");
            return false;
		}
		
		status = m_navQuery->init(m_navMesh, 2048*2);
		if(dtStatusFailed(status)){
			//m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
            LogError("Could not init Detour navmesh query");
			return false;
		}

		m_crowd = dtAllocCrowd();
		m_crowd->init(5000, buildSettings.agentRadius, m_navMesh);

		/*struct dtObstacleAvoidanceParams params;
		memcpy(&params, m_crowd->getObstacleAvoidanceParams(3), sizeof(dtObstacleAvoidanceParams));
		params.velBias = 0.5f;
		params.weightDesVel = 2.0f;
		params.weightCurVel = 0.4f;
		params.weightSide = 0.8f;
		params.weightToi = 2.5f;
		params.weightDir = 1.5f;
		params.adaptiveDivs = 7;
		params.adaptiveRings = 2;
		params.adaptiveDepth = 3;
		params.gridSize = 33;
		params.gridDepth = 7;
		m_crowd->setObstacleAvoidanceParams(3, &params);*/
	}
	
	bakeData.m_ctx->stopTimer(RC_TIMER_TOTAL);

	return true;
}

void Navmesh::_TileInit0(BakeData& data){
	if(data.m_ctx != nullptr) delete data.m_ctx;
	data.m_ctx = new rcContext();
}

bool Navmesh::TileInit(Scene* scene, AABB bounds){
	//Cleanup(data);
	hasInitTile = true;

    /*m_ctx = new rcContext();*/
    m_navQuery = dtAllocNavMeshQuery();
	Vector3 _bmin = bounds.GetMin(); //m_geom->getNavMeshBoundsMin();
	Vector3 _bmax = bounds.GetMax(); //m_geom->getNavMeshBoundsMax();

	dtFreeNavMesh(m_navMesh);
	m_navMesh = dtAllocNavMesh();
	if(!m_navMesh){
		//m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not allocate navmesh.");
		LogError("buildTiledNavigation: Could not allocate navmesh.");
		return false;
	}

	dtNavMeshParams params;
	rcVcopy(params.orig, &_bmin.x/*m_geom->getNavMeshBoundsMin()*/);
	params.tileWidth = buildSettings.tileSize*buildSettings.cellSize;
	params.tileHeight = buildSettings.tileSize*buildSettings.cellSize;
	//params.maxTiles = buildSettings.maxTiles;
	//params.maxPolys = buildSettings.maxPolysPerTile;

	int gw = 0, gh = 0;
	const float* bmin = &_bmin.x; //m_geom->getNavMeshBoundsMin();
	const float* bmax = &_bmax.x; //m_geom->getNavMeshBoundsMax();
	rcCalcGridSize(bmin, bmax, buildSettings.cellSize, &gw, &gh);
	const int ts = (int)buildSettings.tileSize;
	const int tw = (gw + ts-1) / ts;
	const int th = (gh + ts-1) / ts;
	// Max tiles and max polys affect how the tile IDs are caculated.
	// There are 22 bits available for identifying a tile and a polygon.
	int tileBits = rcMin((int)ilog2(nextPow2(tw*th)), 14);
	if(tileBits > 14) tileBits = 14;
	int polyBits = 22 - tileBits;
	params.maxTiles = 1 << tileBits;
	params.maxPolys = 1 << polyBits;

	dtStatus status;
	
	status = m_navMesh->init(&params);
	if(dtStatusFailed(status)){
		//m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not init navmesh.");
		LogError("buildTiledNavigation: Could not init navmesh.");
		return false;
	}
	
	status = m_navQuery->init(m_navMesh, 2048*2);
	if(dtStatusFailed(status)){
		//m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not init Detour navmesh query");
		LogError("buildTiledNavigation: Could not init Detour navmesh query");
		return false;
	}

	m_crowd = dtAllocCrowd();
	m_crowd->init(5000, buildSettings.agentRadius, m_navMesh);
	
	return true;
}

bool Navmesh::BakeAllTiles(Scene* scene, AABB bounds, BuildSettings inbuildSettings, LayerMask layerMask){
	buildSettings = inbuildSettings;
	if(buildSettings.useTile == false) return false;
	mask = layerMask;

	if(hasInitTile == false){
		TileInit(scene, bounds);
	} 
	//if (!m_geom) return;
	//if (!m_navMesh) return;

	Vector3 _min = bounds.GetMin();
	Vector3 _max = bounds.GetMax();
	float* bmin = &_min.x; //m_geom->getNavMeshBoundsMin();
 	float* bmax = &_max.x; //m_geom->getNavMeshBoundsMax();
	int gw = 0, gh = 0;
	rcCalcGridSize(bmin, bmax, buildSettings.cellSize, &gw, &gh);
	const int ts = (int)buildSettings.tileSize;
	const int tw = (gw + ts-1) / ts;
	const int th = (gh + ts-1) / ts;
	const float tcs = buildSettings.tileSize*buildSettings.cellSize;

	// Start the build process.
	//m_ctx->startTimer(RC_TIMER_TEMP);

	/*for(int y = 0; y < th; ++y){
		for(int x = 0; x < tw; ++x){
			m_lastBuiltTileBmin[0] = bmin[0] + x*tcs;
			m_lastBuiltTileBmin[1] = bmin[1];
			m_lastBuiltTileBmin[2] = bmin[2] + y*tcs;
			
			m_lastBuiltTileBmax[0] = bmin[0] + (x+1)*tcs;
			m_lastBuiltTileBmax[1] = bmax[1];
			m_lastBuiltTileBmax[2] = bmin[2] + (y+1)*tcs;
			
			int dataSize = 0;
			unsigned char* data = BuildTileMesh(scene, x, y, m_lastBuiltTileBmin, m_lastBuiltTileBmax, dataSize);
			if(data){
				// Remove any previous data (navmesh owns and deletes the data).
				m_navMesh->removeTile(m_navMesh->getTileRefAt(x,y,0),0,0);
				// Let the navmesh own the data.
				dtStatus status = m_navMesh->addTile(data,dataSize,DT_TILE_FREE_DATA,0,0);
				if(dtStatusFailed(status)){
					LogError("Erro on add tile");
					dtFree(data);
				}
			}
		}
	}*/

	tf::Executor executor;
    tf::Taskflow taskflow;
	for(int y = 0; y < th; ++y){
		for(int x = 0; x < tw; ++x){
			m_navMesh->removeTile(m_navMesh->getTileRefAt(x,y,0),0,0);
		}
	}
	struct Data{
		int x;
		int y;

		int dataSize = 0;
		unsigned char* data = nullptr;
		float m_lastBuiltTileBmin[3];
		float m_lastBuiltTileBmax[3];

		BakeData bakeData;
	};
	std::vector<std::vector<Data>> datas;
	datas.resize(th);
	for(int y = 0; y < th; ++y){
		datas[y].resize(tw);
	}
	for(int y = 0; y < th; ++y){
		for(int x = 0; x < tw; ++x){
			datas[y][x] = {x, y};

			datas[y][x].m_lastBuiltTileBmin[0] = bmin[0] + x*tcs;
			datas[y][x].m_lastBuiltTileBmin[1] = bmin[1];
			datas[y][x].m_lastBuiltTileBmin[2] = bmin[2] + y*tcs;
			
			datas[y][x].m_lastBuiltTileBmax[0] = bmin[0] + (x+1)*tcs;
			datas[y][x].m_lastBuiltTileBmax[1] = bmax[1];
			datas[y][x].m_lastBuiltTileBmax[2] = bmin[2] + (y+1)*tcs;

			taskflow.emplace([&, tileDataPtr = &datas[y][x]](){
				Cleanup(tileDataPtr->bakeData);
				_TileInit0(tileDataPtr->bakeData);
				tileDataPtr->data = BuildTileMesh(
					tileDataPtr->bakeData,
					scene, tileDataPtr->x, tileDataPtr->y,
					tileDataPtr->m_lastBuiltTileBmin,
					tileDataPtr->m_lastBuiltTileBmax,
					tileDataPtr->dataSize
				);
			});
		}
	}
	executor.run(taskflow).wait(); 

	for(int y = 0; y < th; ++y){
		for(int x = 0; x < tw; ++x){
			if(datas[x][y].data){
				dtStatus status = m_navMesh->addTile(datas[x][y].data, datas[x][y].dataSize, DT_TILE_FREE_DATA, 0, 0);
				if(dtStatusFailed(status)){
					LogError("Erro on add tile");
					dtFree(datas[x][y].data);
				}
			}
		}
	}
	
	// Start the build process.	
	//m_ctx->stopTimer(RC_TIMER_TEMP);

	//m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TEMP)/1000.0f;

	return true;
}

bool Navmesh::BakeTile(Scene* scene, AABB bounds, const Vector3 pos, BuildSettings inbuildSettings){
	buildSettings = inbuildSettings;

	if(buildSettings.useTile == false) return false;
	
	if(hasInitTile == false){
		_TileInit0(bakeData);
		TileInit(scene, bounds); 
		Cleanup(bakeData);
	}

	//if (!m_geom) return;
	if(!m_navMesh) return false;

	Vector3 _min = bounds.GetMin();
	Vector3 _max = bounds.GetMax();
	float* bmin = &_min.x; //m_geom->getNavMeshBoundsMin();
 	float* bmax = &_max.x; //m_geom->getNavMeshBoundsMax();
	
	float ts = buildSettings.tileSize * buildSettings.cellSize;
	int tx = (int)((pos[0] - bmin[0]) / ts);
	int ty = (int)((pos[2] - bmin[2]) / ts);
	
	m_lastBuiltTileBmin[0] = bmin[0] + tx*ts;
	m_lastBuiltTileBmin[1] = bmin[1];
	m_lastBuiltTileBmin[2] = bmin[2] + ty*ts;
	
	m_lastBuiltTileBmax[0] = bmin[0] + (tx+1)*ts;
	m_lastBuiltTileBmax[1] = bmax[1];
	m_lastBuiltTileBmax[2] = bmin[2] + (ty+1)*ts;

	/*tx = pos.x;
	ty = pos.z;
	m_lastBuiltTileBmin[0] = bmin[0];
	m_lastBuiltTileBmin[1] = bmin[1];
	m_lastBuiltTileBmin[2] = bmin[2];
	m_lastBuiltTileBmax[0] = bmax[0];
	m_lastBuiltTileBmax[1] = bmax[1];
	m_lastBuiltTileBmax[2] = bmax[2];*/
	
	//m_tileCol = duRGBA(255,255,255,64);
	
	bakeData.m_ctx->resetLog();
	
	int dataSize = 0;
	unsigned char* data = BuildTileMesh(bakeData, scene, tx, ty, m_lastBuiltTileBmin, m_lastBuiltTileBmax, dataSize);

	// Remove any previous data (navmesh owns and deletes the data).
	m_navMesh->removeTile(m_navMesh->getTileRefAt(tx,ty,0),0,0);

	// Add tile, or leave the location empty.
	if(data){
		// Let the navmesh own the data.
		dtStatus status = m_navMesh->addTile(data,dataSize,DT_TILE_FREE_DATA,0,0);
		if(dtStatusFailed(status)){
			LogError("Erro on add tile");
			dtFree(data);
		}
	}
	
	//m_ctx->log("Build Tile (%d,%d):", tx, ty);
	//LogWarning("Build Tile (%d,%d):", tx, ty);
	return true;
}

bool Navmesh::RemoveTile(Scene* scene, AABB bounds, const Vector3 pos){
	return false;
}

void Navmesh::GetTilePos(const float* pos, int& tx, int& ty){
	/*if(!m_geom) return;
	
	const float* bmin = m_geom->getNavMeshBoundsMin();
	
	const float ts = m_tileSize*m_cellSize;
	tx = (int)((pos[0] - bmin[0]) / ts);
	ty = (int)((pos[2] - bmin[2]) / ts);*/
}

bool Navmesh::InitBake(Scene* scene, AABB bounds, BuildSettings inbuildSettings, LayerMask layerMask){
	if(hasInitTile == false){
		buildSettings = inbuildSettings;
		mask = layerMask;
		Cleanup(bakeData);
		_TileInit0(bakeData);
		TileInit(scene, bounds); 
	}
	return true;
}

void Navmesh::UpdateTilesNear(Scene* scene, AABB bounds, Vector3 viewPos, int radiusInTiles){
	Assert(buildSettings.useTile == true);
	Assert(hasInitTile == true);
	
    float ts = buildSettings.tileSize * buildSettings.cellSize;
    Vector3 _min = bounds.GetMin();
    float* bmin = &_min.x;

    int cx = (int)((viewPos[0] - bmin[0]) / ts);
    int cy = (int)((viewPos[2] - bmin[2]) / ts);

    for(int dy = -radiusInTiles; dy <= radiusInTiles; ++dy){
        for(int dx = -radiusInTiles; dx <= radiusInTiles; ++dx){
            int tx = cx + dx;
            int ty = cy + dy;

            // check if already baked in navmesh
            dtTileRef ref = m_navMesh->getTileRefAt(tx, ty, 0);
            if(ref != 0){
                const dtMeshTile* tile = nullptr;
                m_navMesh->getTileAndPolyByRefUnsafe(ref, &tile, nullptr);
                if(tile && tile->data){
                    continue; // already baked → skip queuing
                }
            }

            m_pendingBake.push({tx, ty});
        }
    }
}

void Navmesh::BakeNextTile(Scene* scene, AABB bounds){
    if(m_pendingBake.empty()) return;

	Assert(buildSettings.useTile == true);
	Assert(hasInitTile == true);

	if(!m_navMesh) return;

    auto [tx, ty] = m_pendingBake.front();
    m_pendingBake.pop();

    float ts = buildSettings.tileSize * buildSettings.cellSize;
    Vector3 _min = bounds.GetMin();
    Vector3 _max = bounds.GetMax();
    float* bmin = &_min.x; 
    float* bmax = &_max.x;

    m_lastBuiltTileBmin[0] = bmin[0] + tx*ts;
    m_lastBuiltTileBmin[1] = bmin[1];
    m_lastBuiltTileBmin[2] = bmin[2] + ty*ts;

    m_lastBuiltTileBmax[0] = bmin[0] + (tx+1)*ts;
    m_lastBuiltTileBmax[1] = bmax[1];
    m_lastBuiltTileBmax[2] = bmin[2] + (ty+1)*ts;

    int dataSize = 0;
    unsigned char* data = BuildTileMesh(
        bakeData, scene, tx, ty,
        m_lastBuiltTileBmin, m_lastBuiltTileBmax, dataSize
    );

    if(data){
        // remove existing tile if any (rare, since we skip already baked)
        m_navMesh->removeTile(m_navMesh->getTileRefAt(tx,ty,0),0,0);

        dtStatus status = m_navMesh->addTile(data, dataSize, DT_TILE_FREE_DATA, 0, 0);
        if(dtStatusFailed(status)){
            LogError("Error on add tile");
            dtFree(data);
        }
    }
}

bool Navmesh::HasTile(int tx, int ty) const {
	/*if(hasInitTile == false) return false;
	if (!m_navMesh) return false;
    dtTileRef ref = m_navMesh->getTileRefAt(tx, ty, 0);
    if (ref == 0) return false;
    const dtMeshTile* tile = nullptr;
    // getTileAndPolyByRefUnsafe returns tile pointer (we don't need poly)
    m_navMesh->getTileAndPolyByRefUnsafe(ref, &tile, nullptr);
    return tile && tile->data;*/

	if (!hasInitTile) return false;
    if (!m_navMesh) return false;

    const dtMeshTile* tile = m_navMesh->getTileAt(tx, ty, 0);
    if (!tile || !tile->header) 
        return false;

    return true;
}

IVector2 Navmesh::WorldPosToTile(const Vector3& pos, AABB& bounds, const BuildSettings& settings) const{
	const float tileWorldSize = settings.tileSize * settings.cellSize;
    Vector3 mn = bounds.GetMin();
    // use floor to handle negative world coords robustly
    int tx = (int)std::floor((pos.x - mn.x) / tileWorldSize);
    int ty = (int)std::floor((pos.z - mn.z) / tileWorldSize);
    return {tx, ty};
}

void Navmesh::GetTileWorldBounds(int tx, int ty, AABB& bounds, const BuildSettings& settings, float outMin[3], float outMax[3]) const{
	const float tileWorldSize = settings.tileSize * settings.cellSize;
    Vector3 mn = bounds.GetMin();
    Vector3 mx = bounds.GetMax();

    outMin[0] = mn.x + tx * tileWorldSize;
    outMin[1] = mn.y;                     // keep full vertical range of global bounds
    outMin[2] = mn.z + ty * tileWorldSize;

    outMax[0] = mn.x + (tx + 1) * tileWorldSize;
    outMax[1] = mx.y;
    outMax[2] = mn.z + (ty + 1) * tileWorldSize;
}

Vector3 Navmesh::TileCenterWorld(int tx, int ty, AABB& bounds, const BuildSettings& settings) const{
	float bmin[3], bmax[3];
    GetTileWorldBounds(tx, ty, bounds, settings, bmin, bmax);
    return Vector3( (bmin[0] + bmax[0]) * 0.5f, (bmin[1] + bmax[1]) * 0.5f, (bmin[2] + bmax[2]) * 0.5f );
}

unsigned char* Navmesh::BuildTileMesh(BakeData& data, Scene* scene, const int tx, const int ty, const float* bmin, const float* bmax, int& dataSize){
	/*if(!m_geom || !m_geom->getMesh() || !m_geom->getChunkyMesh()){
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Input mesh is not specified.");
		return 0;
	}*/
	
	float m_tileMemUsage = 0;
	float m_tileBuildTime = 0;
	
	Cleanup(data);
	
	/*const float* verts = m_geom->getMesh()->getVerts();
	const int nverts = m_geom->getMesh()->getVertCount();
	const int ntris = m_geom->getMesh()->getTriCount();
	const rcChunkyTriMesh* chunkyMesh = m_geom->getChunkyMesh();*/
		
	// Init build configuration from GUI
	memset(&data.m_cfg, 0, sizeof(data.m_cfg));
	data.m_cfg.cs = buildSettings.cellSize;
	data.m_cfg.ch = buildSettings.cellHeight;
	data.m_cfg.walkableSlopeAngle = buildSettings.agentMaxSlope;
	data.m_cfg.walkableHeight = (int)ceilf(buildSettings.agentHeight / data.m_cfg.ch);
	data.m_cfg.walkableClimb = (int)floorf(buildSettings.agentMaxClimb / data.m_cfg.ch);
	data.m_cfg.walkableRadius = (int)ceilf(buildSettings.agentRadius / data.m_cfg.cs);
	data.m_cfg.maxEdgeLen = (int)(buildSettings.edgeMaxLen / buildSettings.cellSize);
	data.m_cfg.maxSimplificationError = buildSettings.edgeMaxError;
	data.m_cfg.minRegionArea = (int)rcSqr(buildSettings.regionMinSize);		// Note: area = size*size
	data.m_cfg.mergeRegionArea = (int)rcSqr(buildSettings.regionMergeSize);	// Note: area = size*size
	data.m_cfg.maxVertsPerPoly = (int)buildSettings.vertsPerPoly;
	data.m_cfg.tileSize = (int)buildSettings.tileSize;
	data.m_cfg.borderSize = data.m_cfg.walkableRadius + 3; // Reserve enough padding.
	data.m_cfg.width = data.m_cfg.tileSize + data.m_cfg.borderSize*2;
	data.m_cfg.height = data.m_cfg.tileSize + data.m_cfg.borderSize*2;
	data.m_cfg.detailSampleDist = buildSettings.detailSampleDist < 0.9f ? 0 : buildSettings.cellSize * buildSettings.detailSampleDist;
	data.m_cfg.detailSampleMaxError = buildSettings.cellHeight * buildSettings.detailSampleMaxError;
	
	// Expand the heighfield bounding box by border size to find the extents of geometry we need to build this tile.
	//
	// This is done in order to make sure that the navmesh tiles connect correctly at the borders,
	// and the obstacles close to the border work correctly with the dilation process.
	// No polygons (or contours) will be created on the border area.
	//
	// IMPORTANT!
	//
	//   :''''''''':
	//   : +-----+ :
	//   : |     | :
	//   : |     |<--- tile to build
	//   : |     | :  
	//   : +-----+ :<-- geometry needed
	//   :.........:
	//
	// You should use this bounding box to query your input geometry.
	//
	// For example if you build a navmesh for terrain, and want the navmesh tiles to match the terrain tile size
	// you will need to pass in data from neighbour terrain tiles too! In a simple case, just pass in all the 8 neighbours,
	// or use the bounding box below to only pass in a sliver of each of the 8 neighbours.
	rcVcopy(data.m_cfg.bmin, bmin);
	rcVcopy(data.m_cfg.bmax, bmax);
	data.m_cfg.bmin[0] -= data.m_cfg.borderSize*data.m_cfg.cs;
	data.m_cfg.bmin[2] -= data.m_cfg.borderSize*data.m_cfg.cs;
	data.m_cfg.bmax[0] += data.m_cfg.borderSize*data.m_cfg.cs;
	data.m_cfg.bmax[2] += data.m_cfg.borderSize*data.m_cfg.cs;
	
	// Reset build times gathering.
	data.m_ctx->resetTimers();
	
	// Start the build process.
	data.m_ctx->startTimer(RC_TIMER_TOTAL);
	
	//m_ctx->log(RC_LOG_PROGRESS, "Building navigation:");
	//m_ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg.width, m_cfg.height);
	//m_ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts/1000.0f, ntris/1000.0f);
	//LogWarning("Building navigation:");
	//LogWarning(" - %d x %d cells", m_cfg.width, m_cfg.height);
	
	// Allocate voxel heightfield where we rasterize our input data to.
	data.m_solid = rcAllocHeightfield();
	if(!data.m_solid){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		LogError("buildNavigation: Out of memory 'solid'.");
		return 0;
	}
	if(!rcCreateHeightfield(data.m_ctx, *data.m_solid, data.m_cfg.width, data.m_cfg.height, data.m_cfg.bmin, data.m_cfg.bmax, data.m_cfg.cs, data.m_cfg.ch)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		LogError("buildNavigation: Could not create solid heightfield.");
		return 0;
	}

	const float* _bmin = data.m_cfg.bmin;
	const float* _bmax = data.m_cfg.bmax;
	/*Vector3 center = Vector3(
		(_bmin[0] + _bmax[0]) * 0.5f,
		(_bmin[1] + _bmax[1]) * 0.5f,
		(_bmin[2] + _bmax[2]) * 0.5f
	);
	Vector3 halfExtents = Vector3(
		(_bmax[0] - _bmin[0]) * 0.5f,
		(_bmax[1] - _bmin[1]) * 0.5f,
		(_bmax[2] - _bmin[2]) * 0.5f
	);
	auto aabb = AABB(center, halfExtents.x, halfExtents.y, halfExtents.z);*/

	auto aabb = AABB({_bmin[0], _bmin[1], _bmin[2]}, {_bmax[0], _bmax[1], _bmax[2]});

	//auto aabb = AABB(Vector3Zero, 10, 10, 10);
	
	RasterizeScene(data, *scene, aabb);
	/*
	auto meshView = scene->GetRegistry().view<MeshRendererComponent, TransformComponent>();
    for(auto e: meshView){
        auto& c = meshView.get<MeshRendererComponent>(e);
        auto& t = meshView.get<TransformComponent>(e);
        if(c.mesh == nullptr) continue;
        if(c.material == nullptr) continue;

        RasterizeMesh(t.GlobalModelMatrix(), c.mesh);
    }

    auto meshRenderView = scene->GetRegistry().view<ModelRendererComponent, TransformComponent>();
    for(auto e: meshRenderView){
        auto& c = meshRenderView.get<ModelRendererComponent>(e);
        auto& t = meshRenderView.get<TransformComponent>(e);
        if(c.GetModel() == nullptr) continue;
        //if(c.GetAABB().isOnFrustum(cam.frustum, t) == false) continue;

        for(auto i: c.GetModel()->renderTargets){
            auto targetMesh = c.GetModel()->meshs[i.meshIndex];
            auto targetMatrix =  t.GlobalModelMatrix() * c.localTransform.GetLocalModelMatrix() * c.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);

            RasterizeMesh(targetMatrix, targetMesh);
        }
    }*/
	
	// Allocate array that can hold triangle flags.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	/*m_triareas = new unsigned char[chunkyMesh->maxTrisPerChunk];
	if(!m_triareas){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", chunkyMesh->maxTrisPerChunk);
		LogError("buildNavigation: Out of memory 'm_triareas' (%d).", chunkyMesh->maxTrisPerChunk);
		return 0;
	}
	
	float tbmin[2], tbmax[2];
	tbmin[0] = m_cfg.bmin[0];
	tbmin[1] = m_cfg.bmin[2];
	tbmax[0] = m_cfg.bmax[0];
	tbmax[1] = m_cfg.bmax[2];
	int cid[512];// TODO: Make grow when returning too many items.
	const int ncid = rcGetChunksOverlappingRect(chunkyMesh, tbmin, tbmax, cid, 512);
	if(!ncid) return 0;
	
	int m_tileTriCount = 0;
	
	for(int i = 0; i < ncid; ++i){
		const rcChunkyTriMeshNode& node = chunkyMesh->nodes[cid[i]];
		const int* ctris = &chunkyMesh->tris[node.i*3];
		const int nctris = node.n;
		
		m_tileTriCount += nctris;
		
		memset(m_triareas, 0, nctris*sizeof(unsigned char));
		rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle,
								verts, nverts, ctris, nctris, m_triareas);
		
		if (!rcRasterizeTriangles(m_ctx, verts, nverts, ctris, m_triareas, nctris, *m_solid, m_cfg.walkableClimb))
			return 0;
	}*/
	
	/*if(!m_keepInterResults){
		delete [] m_triareas;
		m_triareas = 0;
	}*/
	
	// Once all geometry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	//if(m_filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(data.m_ctx, data.m_cfg.walkableClimb, *data.m_solid);
	//if(m_filterLedgeSpans)
		rcFilterLedgeSpans(data.m_ctx, data.m_cfg.walkableHeight, data.m_cfg.walkableClimb, *data.m_solid);
	//if(m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(data.m_ctx, data.m_cfg.walkableHeight, *data.m_solid);
	
	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	data.m_chf = rcAllocCompactHeightfield();
	if(!data.m_chf){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		LogError("buildNavigation: Out of memory 'chf'.");
		return 0;
	}
	if(!rcBuildCompactHeightfield(data.m_ctx, data.m_cfg.walkableHeight, data.m_cfg.walkableClimb, *data.m_solid, *data.m_chf)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		LogError("buildNavigation: Could not build compact data.");
		return 0;
	}
	
	//if(!m_keepInterResults){
		rcFreeHeightField(data.m_solid);
		data.m_solid = 0;
	//}

	// Erode the walkable area by agent radius.
	if(!rcErodeWalkableArea(data.m_ctx, data.m_cfg.walkableRadius, *data.m_chf)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		LogError("buildNavigation: Could not erode.");
		return 0;
	}

	// (Optional) Mark areas.
	/*const ConvexVolume* vols = m_geom->getConvexVolumes();
	for(int i  = 0; i < m_geom->getConvexVolumeCount(); ++i)
		rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);*/
	
	
	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	// There are 3 martitioning methods, each with some pros and cons:
	// 1) Watershed partitioning
	//   - the classic Recast partitioning
	//   - creates the nicest tessellation
	//   - usually slowest
	//   - partitions the heightfield into nice regions without holes or overlaps
	//   - the are some corner cases where this method creates produces holes and overlaps
	//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
	//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
	//   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
	// 2) Monotone partioning
	//   - fastest
	//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
	//   - creates long thin polygons, which sometimes causes paths with detours
	//   * use this if you want fast navmesh generation
	// 3) Layer partitoining
	//   - quite fast
	//   - partitions the heighfield into non-overlapping regions
	//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
	//   - produces better triangles than monotone partitioning
	//   - does not have the corner cases of watershed partitioning
	//   - can be slow and create a bit ugly tessellation (still better than monotone)
	//     if you have large open areas with small obstacles (not a problem if you use tiles)
	//   * good choice to use for tiled navmesh with medium and small sized tiles
	
	if(m_partitionType == SAMPLE_PARTITION_WATERSHED){
		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if(!rcBuildDistanceField(data.m_ctx, *data.m_chf)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
			LogError("buildNavigation: Could not build distance field.");
			return 0;
		}
		
		// Partition the walkable surface into simple regions without holes.
		if(!rcBuildRegions(data.m_ctx, *data.m_chf, data.m_cfg.borderSize, data.m_cfg.minRegionArea, data.m_cfg.mergeRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
			LogError("buildNavigation: Could not build watershed regions.");
			return 0;
		}
	} else if(m_partitionType == SAMPLE_PARTITION_MONOTONE){
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if(!rcBuildRegionsMonotone(data.m_ctx, *data.m_chf, data.m_cfg.borderSize, data.m_cfg.minRegionArea, data.m_cfg.mergeRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
			LogError("buildNavigation: Could not build monotone regions.");
			return 0;
		}
	} else // SAMPLE_PARTITION_LAYERS
	{
		// Partition the walkable surface into simple regions without holes.
		if(!rcBuildLayerRegions(data.m_ctx, *data.m_chf, data.m_cfg.borderSize, data.m_cfg.minRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
			LogError("buildNavigation: Could not build layer regions.");
			return 0;
		}
	}
	 	
	// Create contours.
	data.m_cset = rcAllocContourSet();
	if(!data.m_cset){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
		LogError("buildNavigation: Out of memory 'cset'.");
		return 0;
	}
	if(!rcBuildContours(data.m_ctx, *data.m_chf, data.m_cfg.maxSimplificationError, data.m_cfg.maxEdgeLen, *data.m_cset)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
		LogError("buildNavigation: Could not create contours.");
		return 0;
	}
	
	if(data.m_cset->nconts == 0){
		return 0;
	}
	
	// Build polygon navmesh from the contours.
	data.m_pmesh = rcAllocPolyMesh();
	if(!data.m_pmesh){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		LogError("buildNavigation: Out of memory 'pmesh'.");
		return 0;
	}
	if(!rcBuildPolyMesh(data.m_ctx, *data.m_cset, data.m_cfg.maxVertsPerPoly, *data.m_pmesh)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		LogError("buildNavigation: Could not triangulate contours.");
		return 0;
	}
	
	// Build detail mesh.
	data.m_dmesh = rcAllocPolyMeshDetail();
	if(!data.m_dmesh){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'dmesh'.");
		LogError("buildNavigation: Out of memory 'dmesh'.");
		return 0;
	}
	
	if(!rcBuildPolyMeshDetail(
		data.m_ctx, *data.m_pmesh, *data.m_chf,
		data.m_cfg.detailSampleDist, data.m_cfg.detailSampleMaxError,
		*data.m_dmesh))
	{
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could build polymesh detail.");
		LogError("buildNavigation: Could build polymesh detail.");
		return 0;
	}
	
	//if(!m_keepInterResults){
		rcFreeCompactHeightfield(data.m_chf);
		data.m_chf = 0;
		rcFreeContourSet(data.m_cset);
		data.m_cset = 0;
	//}
	
	unsigned char* navData = 0;
	int navDataSize = 0;
	if(data.m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON){
		if(data.m_pmesh->nverts >= 0xffff){
			// The vertex indices are ushorts, and cannot point to more than 0xffff vertices.
			//m_ctx->log(RC_LOG_ERROR, "Too many vertices per tile %d (max: %d).", m_pmesh->nverts, 0xffff);
			LogError("Too many vertices per tile {} (max: {}).", data.m_pmesh->nverts, 0xffff);
			return 0;
		}
		
		// Update poly flags from areas.
		for(int i = 0; i < data.m_pmesh->npolys; ++i){
			if(data.m_pmesh->areas[i] == RC_WALKABLE_AREA)
				data.m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
			
			if(data.m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				data.m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				data.m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
			{
				data.m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			} else if(data.m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER){
				data.m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			} else if(data.m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR){
				data.m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}
		
		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = data.m_pmesh->verts;
		params.vertCount = data.m_pmesh->nverts;
		params.polys = data.m_pmesh->polys;
		params.polyAreas = data.m_pmesh->areas;
		params.polyFlags = data.m_pmesh->flags;
		params.polyCount = data.m_pmesh->npolys;
		params.nvp = data.m_pmesh->nvp;
		params.detailMeshes = data.m_dmesh->meshes;
		params.detailVerts = data.m_dmesh->verts;
		params.detailVertsCount = data.m_dmesh->nverts;
		params.detailTris = data.m_dmesh->tris;
		params.detailTriCount = data.m_dmesh->ntris;
		params.offMeshConVerts = builData.m_offMeshConVerts; //m_geom->getOffMeshConnectionVerts();
		params.offMeshConRad = builData.m_offMeshConRads;// m_geom->getOffMeshConnectionRads();
		params.offMeshConDir = builData.m_offMeshConDirs;// m_geom->getOffMeshConnectionDirs();
		params.offMeshConAreas = builData.m_offMeshConAreas;// m_geom->getOffMeshConnectionAreas();
		params.offMeshConFlags = builData.m_offMeshConFlags;// m_geom->getOffMeshConnectionFlags();
		params.offMeshConUserID = builData.m_offMeshConId;// m_geom->getOffMeshConnectionId();
		params.offMeshConCount = builData.m_offMeshConCount;// m_geom->getOffMeshConnectionCount();
		params.walkableHeight = buildSettings.agentHeight;
		params.walkableRadius = buildSettings.agentRadius;
		params.walkableClimb = buildSettings.agentMaxClimb;
		params.tileX = tx;
		params.tileY = ty;
		params.tileLayer = 0;
		rcVcopy(params.bmin, data.m_pmesh->bmin);
		rcVcopy(params.bmax, data.m_pmesh->bmax);
		params.cs = data.m_cfg.cs;
		params.ch = data.m_cfg.ch;
		params.buildBvTree = true;
		
		if(!dtCreateNavMeshData(&params, &navData, &navDataSize)){
			//m_ctx->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
			LogError("Could not build Detour navmesh.");
			return 0;
		}		
	}
	m_tileMemUsage = navDataSize/1024.0f;
	
	data.m_ctx->stopTimer(RC_TIMER_TOTAL);
	
	// Show performance stats.
	//duLogBuildTimes(*m_ctx, m_ctx->getAccumulatedTime(RC_TIMER_TOTAL));
	//m_ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);
	//LogWarning(">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);
	
	//m_tileBuildTime = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;

	dataSize = navDataSize;
	return navData;
}

void Navmesh::DrawDebug(){
	if(m_navMesh == nullptr) return;

    if(m_dd == nullptr) m_dd = new DebugDrawGL();

	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);*/

	const float texScale = 1.0f / (buildSettings.cellSize * 10.0f);

	if(m_navMesh && m_navQuery &&
		(m_drawMode == DRAWMODE_NAVMESH ||
		m_drawMode == DRAWMODE_NAVMESH_TRANS ||
		m_drawMode == DRAWMODE_NAVMESH_BVTREE ||
		 m_drawMode == DRAWMODE_NAVMESH_NODES ||
		m_drawMode == DRAWMODE_NAVMESH_INVIS))
	{
		if(m_drawMode != DRAWMODE_NAVMESH_INVIS)
			duDebugDrawNavMeshWithClosedList(m_dd, *m_navMesh, *m_navQuery, m_navMeshDrawFlags);
		if(m_drawMode == DRAWMODE_NAVMESH_BVTREE)
			duDebugDrawNavMeshBVTree(m_dd, *m_navMesh);
		if(m_drawMode == DRAWMODE_NAVMESH_NODES)
			duDebugDrawNavMeshNodes(m_dd, *m_navQuery);
		duDebugDrawNavMeshPolysWithFlags(m_dd, *m_navMesh, SAMPLE_POLYFLAGS_DISABLED, duRGBA(0,0,0,128));
	}
		
	/*glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);*/	
}

bool Navmesh::FindPath(Vector3 startPos, Vector3 endPos, NavMeshPath& outPath){
	Assert(m_navMesh != nullptr);
	Assert(m_navQuery != nullptr);
	if(!m_navMesh) return false;

	dtPolyRef m_startRef = 0;
	dtPolyRef m_endRef = 0;
	dtQueryFilter m_filter;
	m_filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED);
	m_filter.setExcludeFlags(0);
	/*m_filter.setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);
	m_filter.setAreaCost(SAMPLE_POLYAREA_WATER, 10.0f);
	m_filter.setAreaCost(SAMPLE_POLYAREA_ROAD, 1.0f);
	m_filter.setAreaCost(SAMPLE_POLYAREA_DOOR, 1.0f);
	m_filter.setAreaCost(SAMPLE_POLYAREA_GRASS, 2.0f);
	m_filter.setAreaCost(SAMPLE_POLYAREA_JUMP, 1.5f);*/

	float tolerance[3] = {2, 4, 2};
	float _startPos[3] = {startPos.x, startPos.y, startPos.z};
	float _endPos[3] = {endPos.x, endPos.y, endPos.z};

	int m_nstraightPath = 0;
	int m_straightPathOptions = 0;

	//LogWarning("x:%f y:%f z:%f", _startPos[0], _startPos[1], _startPos[2]);

	auto s1 = m_navQuery->findNearestPoly(_startPos, tolerance, &m_filter, &m_startRef, 0);
	auto s2 = m_navQuery->findNearestPoly(_endPos, tolerance, &m_filter, &m_endRef, 0);
	//Assert(m_navQuery->isValidPolyRef(m_startRef, &m_filter) == true);
	//Assert(m_navQuery->isValidPolyRef(m_endRef, &m_filter) == true);
	auto s3 = m_navQuery->findPath(m_startRef, m_endRef, _startPos, _endPos, &m_filter, m_polys, &m_npolys, MAX_POLYS);
	//LogInfo("%u", s1);

	m_nstraightPath = 0;
	if(m_npolys){
		// In case of partial path, make sure the end point is clamped to the last polygon.
		float epos[3];
		dtVcopy(epos, _endPos);
		if(m_polys[m_npolys-1] != m_endRef){
			m_navQuery->closestPointOnPoly(m_polys[m_npolys-1], _endPos, epos, 0);
			outPath.status = NavMeshPathStatus::PathPartial;
		} else {
			outPath.status = NavMeshPathStatus::PathComplete;
		}
		
		m_navQuery->findStraightPath(
			_startPos, epos, m_polys, m_npolys,
			m_straightPath, m_straightPathFlags,
			m_straightPathPolys, &m_nstraightPath, MAX_POLYS, m_straightPathOptions
		);

		outPath.corners.clear();
		for(int i = 0; i < m_nstraightPath*3; i += 3){
			outPath.corners.push_back(Vector3(m_straightPath[i], m_straightPath[i+1], m_straightPath[i+2]));
		}

		//outPath.status = NavMeshPathStatus::PathComplete;
		//LogWarningExtra("OK Count: %zd", outPath.corners.size());
		return true;
	}

	outPath.status = NavMeshPathStatus::PathInvalid;
	outPath.corners.clear();
	//LogWarningExtra("Not OK");
	return false;
}

bool Navmesh::SamplePosition(Vector3 position, Vector3& outClosestPoint, float maxSearchRadius){
	Assert(m_navMesh != nullptr);
	Assert(m_navQuery != nullptr);
	if(!m_navMesh) return false;

	float _position[3] = {position.x, position.y, position.z};

    // Create and initialize the query
    dtNavMeshQuery navQuery;
    navQuery.init(m_navMesh, 2048); // Max nodes

    // Define a simple filter (modify as needed)
    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    // Extents define the search box around the given position
    float searchExtents[3] = { maxSearchRadius, maxSearchRadius, maxSearchRadius };

    // Find nearest polygon
    dtPolyRef nearestPoly;
    float nearestPoint[3];

    dtStatus status = navQuery.findNearestPoly(_position, searchExtents, &filter, &nearestPoly, nearestPoint);

    if(dtStatusSucceed(status) && nearestPoly){
        outClosestPoint = {nearestPoint[0], nearestPoint[1], nearestPoint[2]};
        return true;
    }

    return false; // No valid position found
}

// Save the navmesh state to a file
bool Navmesh::Save(const std::string& outPath, SaveType type){
	if(type == SaveType::SettingOnly) return false;

	if(!m_navMesh) return false;

    FILE* fp = fopen(outPath.c_str(), "wb");
    if(!fp) return false;

    // Save navmesh params first
    const dtNavMeshParams* params = m_navMesh->getParams();
    fwrite(params, sizeof(dtNavMeshParams), 1, fp);

    // Save tiles
    for(int i = 0; i < m_navMesh->getMaxTiles(); i++){
        const dtMeshTile* tile = ((const dtNavMesh*)m_navMesh)->getTile(i);
        if(!tile || !tile->header || !tile->dataSize) continue;

        fwrite(&tile->dataSize, sizeof(int), 1, fp);
        fwrite(tile->data, tile->dataSize, 1, fp);
    }

    fclose(fp);
    this->path = outPath;
    return true;
}

// Load navmesh from a file
bool Navmesh::LoadFromFile(const std::string& path){
	Cleanup(bakeData);

    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) return false;

    // Read navmesh params
    dtNavMeshParams params{};
    if (fread(&params, sizeof(dtNavMeshParams), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    // Allocate + init navmesh
    dtNavMesh* navMesh = dtAllocNavMesh();
    if (!navMesh || dtStatusFailed(navMesh->init(&params))) {
        if (navMesh) dtFreeNavMesh(navMesh);
        fclose(fp);
        return false;
    }

    // Load all tiles
    while (true) {
        int dataSize = 0;
        if (fread(&dataSize, sizeof(int), 1, fp) != 1) break;
        if (dataSize <= 0) break;

        unsigned char* data = (unsigned char*)dtAlloc(dataSize, DT_ALLOC_PERM);
        if (!data) {
            fclose(fp);
            dtFreeNavMesh(navMesh);
            return false;
        }

        if (fread(data, dataSize, 1, fp) != 1) {
            dtFree(data);
            break;
        }

        dtStatus status = navMesh->addTile(data, dataSize, DT_TILE_FREE_DATA, 0, nullptr);
        if (dtStatusFailed(status)) {
            dtFree(data);
            fclose(fp);
            dtFreeNavMesh(navMesh);
            return false;
        }
    }

    fclose(fp);

    // Replace old navmesh
    m_navMesh = navMesh;

    // Init query
    if (!m_navQuery) m_navQuery = dtAllocNavMeshQuery();
    m_navQuery->init(m_navMesh, 2048);

    this->path = path;
    return true;
}

std::vector<std::string> Navmesh::GetFileAssociations(){
	return {".navmesh"};
}

Vector3 NavmeshAgentComponent::GetDestination(){ 
	return destination; 
}

void NavmeshAgentComponent::SetDestination(Vector3 d){
	if(d == destination && hasInit == true) return;
	destination = d;
	isDirty = true;
	hasInit = true;
}

void NavmeshAgentComponent::Reset(){
	isDirty = true;
	hasInit = true;
	//destination = Vector3Zero;
}

void NavmeshSystem::OnInit(Scene& scene){
	scene.GetRegistry().on_destroy<NavmeshAgentComponent>().connect<&OnRemoveAgent>();
}

void NavmeshSystem::OnEnd(Scene& scene){
	scene.GetRegistry().on_destroy<NavmeshAgentComponent>().disconnect<&OnRemoveAgent>();
}

void NavmeshSystem::OnRemoveAgent(entt::registry& r, entt::entity e){
	NavmeshAgentComponent& agent = r.get<NavmeshAgentComponent>(e);
	if(agent.crowdId != -1){
		agent.navmesh->m_crowd->removeAgent(agent.crowdId);
		agent.navmesh = nullptr;
		agent.crowdId = -1;
	}
}

void NavmeshSystem::LateUpdate(Scene& scene){
	OD_PROFILE_SCOPE("NavmeshSystem::Update");

	Ref<Navmesh> navmesh = nullptr;
	NavmeshComponent::AgentUpdateMode updateMode;

	int navmeshCount = 0;
	auto navmeshView = scene.GetRegistry().view<NavmeshComponent>();
	for(auto e: navmeshView){
		NavmeshComponent& navmeshComponent = navmeshView.get<NavmeshComponent>(e);
		navmesh = navmeshComponent.navmesh;
		updateMode = navmeshComponent.agentUpdateMode;
		navmeshCount += 1;
	}

	Assert(navmeshCount <= 1);
	if(navmesh == nullptr) return;

	if(updateMode == NavmeshComponent::AgentUpdateMode::FindPath){
		#if InternalSystemsMulthread
		scene.GetTaskflow().emplace([=, &scene](tf::Subflow& subflow){
		#endif
			auto navmeshAgentView = scene.GetRegistry().view<NavmeshAgentComponent, TransformComponent>();
			for(auto e: navmeshAgentView){
				NavmeshAgentComponent& navmeshComponent = navmeshAgentView.get<NavmeshAgentComponent>(e);
				TransformComponent& transform = navmeshAgentView.get<TransformComponent>(e);

				if(navmeshComponent.isDirty){
					navmeshComponent.isDirty = false;
					navmeshComponent.lastPos = transform.Position();
					navmesh->FindPath(transform.Position(), navmeshComponent.destination, navmeshComponent.path);
					navmeshComponent.curPathIndex = 1;//-1;
					navmeshComponent.reach = false;
				}

				if(scene.Running() == false) continue;

				#if InternalSystemsMulthread
				subflow.emplace([&](){ 
				#endif
					if(navmeshComponent.path.status == NavMeshPathStatus::PathComplete){
						if(navmeshComponent.path.corners.size() <= 1){
							navmeshComponent.reach = true;
							navmeshComponent.desiredVelocity = Vector3Zero;
							return;
						}

						Assert(navmeshComponent.path.corners.size() > 1);
						if(navmeshComponent.reach) return;

						Vector3 pos = transform.Position();
						Vector3 dir = navmeshComponent.path.corners[navmeshComponent.curPathIndex] - pos;
						if(math::length(dir) > 0.1f) dir = math::normalizeSafe(dir);
						Assert(Mathf::IsNan(dir) == false);

						float distance = math::distance(pos, navmeshComponent.path.corners[navmeshComponent.curPathIndex]);

						if(distance <= navmeshComponent.stopDistance){
							navmeshComponent.curPathIndex += 1;
							if(navmeshComponent.curPathIndex >= navmeshComponent.path.corners.size()){
								navmeshComponent.curPathIndex += navmeshComponent.path.corners.size()-1;
								navmeshComponent.reach = true;
								navmeshComponent.desiredVelocity = Vector3Zero;
								return;
							}
						}

						navmeshComponent.desiredVelocity = dir * navmeshComponent.speed;
						
						if(navmeshComponent.manualUpdate == false){
							transform.Position(pos + dir * (navmeshComponent.speed * Application::DeltaTime()));
						}
					} else {
						navmeshComponent.curPathIndex = -1;
						navmeshComponent.reach = false;
					}
				#if InternalSystemsMulthread
				});
				#endif
			}
		#if InternalSystemsMulthread
		});
		#endif
	}

	if(updateMode == NavmeshComponent::AgentUpdateMode::Crowd){
		auto navmeshAgentView = scene.GetRegistry().view<NavmeshAgentComponent, TransformComponent>();
		
		for(auto [entity, agent, trans] : navmeshAgentView.each()){
			if(agent.crowdId == -1){
				dtCrowdAgentParams ap;
				memset(&ap, 0, sizeof(ap));
				ap.radius = navmesh->buildSettings.agentRadius;// 0.3f;
				ap.height = navmesh->buildSettings.agentHeight;// 1.7f;
				ap.maxAcceleration = 10.0f;
				ap.maxSpeed = 3.0f;
				ap.collisionQueryRange = ap.radius * 12.0f;
				ap.pathOptimizationRange = ap.radius * 30.0f;
				ap.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OBSTACLE_AVOIDANCE;
				ap.obstacleAvoidanceType = 0;
				ap.separationWeight = 2.0f;

				ap.obstacleAvoidanceType = 3;
				ap.separationWeight = 1.0f; // experimente valores entre 0.5 e 2.0

				Vector3 pos = trans.Position();
				int idx = navmesh->m_crowd->addAgent(&pos.x, &ap);
				agent.crowdId = idx;
				agent.navmesh = navmesh;
			}

			// Only request new path if dirty and hasn't already reached
			if(agent.isDirty){
				agent.isDirty = false;

				agent.lastPos = trans.Position();
				navmesh->FindPath(trans.Position(), agent.destination, agent.path);
				agent.curPathIndex = 1;//-1;
				agent.reach = false;

				const dtCrowdAgent* crowdAgent = navmesh->m_crowd->getAgent(agent.crowdId);
				if(crowdAgent && crowdAgent->active){
					dtPolyRef ref;
					dtQueryFilter m_filter;
					m_filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED);
					m_filter.setExcludeFlags(0);
					float tolerance[3] = {2, 4, 2};
					float destination[3] = {agent.destination.x, agent.destination.y, agent.destination.z};

					navmesh->m_navQuery->findNearestPoly(destination, tolerance, &m_filter, &ref, nullptr);
					navmesh->m_crowd->requestMoveTarget(agent.crowdId, ref, destination);
				}
			}

			if(agent.manualUpdate == true/*&& scene->HasComponent<RigidbodyComponent>(entity)*/){
				dtCrowdAgent* ca = navmesh->m_crowd->getEditableAgent(agent.crowdId);
				if(ca){
					//auto& rb = scene->GetComponent<RigidbodyComponent>(entity);

					Vector3 pos = trans.Position();
					//Vector3 vel = rb.Velocity();

					ca->npos[0] = pos.x;
					ca->npos[1] = pos.y;
					ca->npos[2] = pos.z;

					/*ca->vel[0] = vel.x;
					ca->vel[1] = vel.y;
					ca->vel[2] = vel.z;*/
				}
			}
		}

		// Update the crowd simulation
		navmesh->m_crowd->update(Application::DeltaTime(), nullptr);

		// Apply positions and check arrival
		for(auto [entity, agent, trans] : navmeshAgentView.each()){
			const dtCrowdAgent* a = navmesh->m_crowd->getAgent(agent.crowdId);
			if(a && a->active){
				//agent.path = a->targetPathqRef;

				agent.desiredVelocity = Vector3(a->dvel[0], a->dvel[1], a->dvel[2]);

				// Update entity transform
				if(agent.manualUpdate == false){
					trans.Position(Vector3(a->npos[0], a->npos[1], a->npos[2]));
				}/*else {
					auto* editable = navmesh->m_crowd->getEditableAgent(agent.crowdId);
					if(editable){
						Vector3 curPos = trans.Position();
						editable->npos[0] = curPos.x;
						editable->npos[1] = curPos.y;
						editable->npos[2] = curPos.z;
					}
				}*/
			
				// Check if agent reached destination
				const float distSq = math::distance2(
					Vector3(a->npos[0], a->npos[1], a->npos[2]),
					agent.destination
				);

				const float reachThreshold = agent.stopDistance;
				if(distSq <= (reachThreshold * reachThreshold)){
					if(!agent.reach){
						agent.reach = true;
						navmesh->m_crowd->resetMoveTarget(agent.crowdId);
						auto* editable = navmesh->m_crowd->getEditableAgent(agent.crowdId);
						editable->vel[0] = 0; editable->vel[1] = 0; editable->vel[2] = 0;
					}
				}
			}
		}
	}

	/*if(updateMode == NavmeshComponent::AgentUpdateMode::Crowd){
		auto navmeshAgentView = scene->GetRegistry().view<NavmeshAgentComponent, TransformComponent>();
		for(auto [entity, agent, trans]: navmeshAgentView.each()){
			if(agent.crowdId == -1){
				dtCrowdAgentParams ap;
				memset(&ap, 0, sizeof(ap));
				ap.radius = 0.3f; //agent.radius;
				ap.height = 1.7f; //agent.height;
				ap.maxAcceleration = 10;// agent.maxAccel;
				ap.maxSpeed = 3; //agent.maxSpeed;
				ap.collisionQueryRange = ap.radius * 12.0f;// agent.radius * 12.0f;
				ap.pathOptimizationRange = ap.radius * 30.0f; //agent.radius * 30.0f;
				ap.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OBSTACLE_AVOIDANCE;
				ap.obstacleAvoidanceType = 0;
				ap.separationWeight = 2.0f;

				Vector3 pos = trans.Position();
				int idx = navmesh->m_crowd->addAgent(&pos.x, &ap);
				agent.crowdId = idx;
			}

			if(!agent.reach){
				const dtCrowdAgent* crowdAgent = navmesh->m_crowd->getAgent(agent.crowdId);
				if(crowdAgent && crowdAgent->active){
					dtPolyRef ref;
					dtQueryFilter m_filter;
					m_filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED);
					m_filter.setExcludeFlags(0);
					float tolerance[3] = {2, 4, 2};
					float destination[3] = {agent.destination.x, agent.destination.y, agent.destination.z};
					navmesh->m_navQuery->findNearestPoly(destination, tolerance, &m_filter, &ref, 0);
					navmesh->m_crowd->requestMoveTarget(agent.crowdId, ref, destination);
				}
			}
		}

		navmesh->m_crowd->update(Application::DeltaTime(), nullptr);

		for(auto [entity, agent, trans]: navmeshAgentView.each()){
			const dtCrowdAgent* a = navmesh->m_crowd->getAgent(agent.crowdId);
			if(a && a->active){
				if(agent.manualUpdate == false) trans.Position(Vector3(a->npos[0], a->npos[1], a->npos[2]));
				agent.desiredVelocity = Vector3(a->vel[0], a->vel[1], a->vel[2]);

				// Check if the agent reached the destination
				const float distSq = math::distance2(
					Vector3(a->npos[0], a->npos[1], a->npos[2]),
					agent.destination
				);
				
				const float reachThreshold = agent.stopDistance;
				if(distSq < (reachThreshold * reachThreshold)){
					agent.reach = true;
					navmesh->m_crowd->resetMoveTarget(agent.crowdId);
				}
			}
		}
	}
	*/
	
	return;

	auto navmeshAgentView = scene.GetRegistry().view<NavmeshAgentComponent, TransformComponent>();
	for(auto e: navmeshAgentView){
		NavmeshAgentComponent& navmeshComponent = navmeshAgentView.get<NavmeshAgentComponent>(e);
		TransformComponent& transform = navmeshAgentView.get<TransformComponent>(e);

		if(navmeshComponent.isDirty /*|| (transform.Position() != navmeshComponent.lastPos)*/){
			navmeshComponent.isDirty = false;
			navmeshComponent.lastPos = transform.Position();
			navmesh->FindPath(transform.Position(), navmeshComponent.destination, navmeshComponent.path);
			navmeshComponent.curPathIndex = 1;//-1;
			navmeshComponent.reach = false;
		}

		if(scene.Running() == false) continue;

		if(navmeshComponent.path.status == NavMeshPathStatus::PathComplete){
			Assert(navmeshComponent.path.corners.size() > 1);
			/*if(navmeshComponent.curPathIndex == -1){
				navmeshComponent.curPathIndex = 0;
				navmeshComponent.reach = false;
			}*/

			if(navmeshComponent.reach) continue;

			Vector3 pos = transform.Position();
			Vector3 dir = navmeshComponent.path.corners[navmeshComponent.curPathIndex] - pos;
			if(math::length(dir) > 0.1f) dir = math::normalizeSafe(dir);
        	Assert(Mathf::IsNan(dir) == false);

			float distance = math::distance(pos, navmeshComponent.path.corners[navmeshComponent.curPathIndex]);

			if(distance <= navmeshComponent.stopDistance){
				navmeshComponent.curPathIndex += 1;
				if(navmeshComponent.curPathIndex >= navmeshComponent.path.corners.size()){
					navmeshComponent.curPathIndex += navmeshComponent.path.corners.size()-1;
					navmeshComponent.reach = true;
					navmeshComponent.desiredVelocity = Vector3Zero;
					continue;
				}
			}

			navmeshComponent.desiredVelocity = dir * navmeshComponent.speed;
			
			if(navmeshComponent.manualUpdate == false){
				transform.Position(pos + dir * (navmeshComponent.speed * Application::DeltaTime()));
			}
		} else {
			navmeshComponent.curPathIndex = -1;
			navmeshComponent.reach = false;
		}
	}
}

void NavmeshSystem::OnDrawGizmos(Scene& scene, Camera& cam){
	//return;
	auto navmeshAgentView = scene.GetRegistry().view<NavmeshAgentComponent>();
	for(auto e: navmeshAgentView){
		NavmeshAgentComponent& navmeshComponent = navmeshAgentView.get<NavmeshAgentComponent>(e);
		if(navmeshComponent.path.status == NavMeshPathStatus::PathInvalid) continue;
		if(navmeshComponent.path.corners.size() < 2) continue;

		for(int i = 0; i < navmeshComponent.path.corners.size()-1; i++){
			Graphics::DrawLine(
				navmeshComponent.path.corners[i] + Vector3(0, 0.1f, 0), 
				navmeshComponent.path.corners[i+1] + Vector3(0, 0.1f, 0), 
				Vector3(1, 0, 0), 
				1
			);
		}
	}

	auto navmeshView = scene.GetRegistry().view<NavmeshComponent, TransformComponent>();
	for(auto e: navmeshView){
		NavmeshComponent& navmeshComponent = navmeshView.get<NavmeshComponent>(e);
		TransformComponent& trans = navmeshView.get<TransformComponent>(e);

		Transform t;
		t.Position(trans.Position());
		t.Scale(navmeshComponent.size);
		Graphics::DrawWireCube(t.GetModelMatrix(), {1,1,1}, 1);
		
		continue;

		if(navmeshComponent.navmesh != nullptr && navmeshComponent.navmesh->buildSettings.useTile){
			AABB bounds = AABB(
				trans.Position(), 
				navmeshComponent.size.x*0.5f, navmeshComponent.size.z*0.5f, navmeshComponent.size.z*0.5f
			);

			Vector3 _min = bounds.GetMin();
			Vector3 _max = bounds.GetMax();
			float* bmin = &_min.x; //m_geom->getNavMeshBoundsMin();
			float* bmax = &_max.x; //m_geom->getNavMeshBoundsMax();
			int gw = 0, gh = 0;
			rcCalcGridSize(bmin, bmax, navmeshComponent.navmesh->buildSettings.cellSize, &gw, &gh);
			const int ts = (int)navmeshComponent.navmesh->buildSettings.tileSize;
			const int tw = (gw + ts-1) / ts;
			const int th = (gh + ts-1) / ts;
			const float tcs = navmeshComponent.navmesh->buildSettings.tileSize*navmeshComponent.navmesh->buildSettings.cellSize;

			for(int y = 0; y < th; ++y){
				for(int x = 0; x < tw; ++x){
					auto min0 = bmin[0] + x*tcs;
					auto min1 = bmin[1];
					auto min2 = bmin[2] + y*tcs;
					
					auto max0 = bmin[0] + (x+1)*tcs;
					auto max1 = bmax[1];
					auto max2 = bmin[2] + (y+1)*tcs;

					AABB a = AABB({min0, min1, min2}, {max0, max1, max2});

					Transform t;
					t.Position(a.center);
					t.Scale(a.extents * 2.0f);
					Graphics::DrawWireCube(t.GetModelMatrix(), {1,1,1}, 1);
				}
			}
		}
	}
}

void NavmeshSystem::OnDrawGizmosSelected(Scene& scene, Camera& cam, Entity entity){
	if(scene.HasComponent<NavmeshComponent>(entity) == false) return;

	auto& n = scene.GetComponent<NavmeshComponent>(entity);
	if(n.navmesh == nullptr) return;
    
	n.navmesh->DrawDebug();
}

}