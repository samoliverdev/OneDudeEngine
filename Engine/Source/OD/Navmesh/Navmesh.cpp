#include "Navmesh.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Scene/Scene.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/Platform/GL.h"
#include "OD/Core/Application.h"
#include <DebugDraw.h>
#include <DetourDebugDraw.h>
#include <DetourCommon.h>
#include "OD/Scene/SceneManager.h"

namespace OD{

void NavmeshModuleInit(){
	SceneManager::Get().RegisterCoreComponent<NavmeshComponent>("NavmeshComponent");
	SceneManager::Get().RegisterCoreComponent<NavmeshAgentComponent>("NavmeshAgentComponent");
	SceneManager::Get().RegisterSystem<NavmeshSystem>("NavmeshSystem");
}

void NavmeshComponent::OnGui(Entity& e){
	NavmeshComponent& navmeshComponent = e.GetComponent<NavmeshComponent>();
	cereal::ImGuiArchive uiArchive;
	uiArchive(navmeshComponent);

	if(ImGui::Button("Bake")){
		if(navmeshComponent.navmesh == nullptr) navmeshComponent.navmesh = CreateRef<Navmesh>();
		if(navmeshComponent.navmesh != nullptr){
			navmeshComponent.navmesh->Bake(
				e.GetScene(), AABB(Vector3(0, 0, 0), 
				navmeshComponent.size.x, navmeshComponent.size.y, navmeshComponent.size.z
			)); 
		}
	}
}

class DebugDrawGL : public duDebugDraw{
public:
    Ref<Shader> shader;
    Ref<Mesh> mesh;

    DebugDrawGL(){
        shader = Shader::CreateFromFile("Engine/Shaders/Gizmos.glsl");
        mesh = CreateRef<Mesh>();
    }

	virtual void depthMask(bool state){
        //glDepthMask(state ? GL_TRUE : GL_FALSE);
        Graphics::SetDepthMask(state);
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

		Shader::Bind(*shader);
		shader->SetFloat("alpha", 0.5f);

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

void Navmesh::Cleanup(){
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
	
	if(useTile == false){
		dtFreeNavMesh(m_navMesh);
		m_navMesh = 0;
		delete m_ctx;
	}
}

bool Navmesh::RasterizeMesh(const Matrix4& model, Ref<Mesh>& mesh){
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

    const float* verts = &_verts[0];
    const int nverts = _verts.size()/3;
	const int* tris = &_tris[0];
	const int ntris = _tris.size()/3;

    // Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	m_triareas = new unsigned char[ntris];
	if(!m_triareas){
        LogError("buildNavigation: Out of memory 'm_triareas' (%d).", ntris);
		return false;
	}
	
    // Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(m_triareas, 0, ntris*sizeof(unsigned char));
	rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
	if(!rcRasterizeTriangles(m_ctx, verts, nverts, tris, m_triareas, ntris, *m_solid, m_cfg.walkableClimb)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not rasterize triangles.");
        LogError("buildNavigation: Could not rasterize triangles.");
		return false;
	}

	delete [] m_triareas;
	m_triareas = 0;

    return true;
}

bool Navmesh::Bake(Scene* scene, AABB bounds){
	Cleanup();

    m_ctx = new rcContext();
    m_navQuery = dtAllocNavMeshQuery();
	Vector3 bmin = bounds.GetMin(); //m_geom->getNavMeshBoundsMin();
	Vector3 bmax = bounds.GetMax(); //m_geom->getNavMeshBoundsMax();
	
	//
	// Step 1. Initialize build config.
	//
	
	// Init build configuration from GUI
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = buildSettings.cellSize;
	m_cfg.ch = buildSettings.cellHeight;
	m_cfg.walkableSlopeAngle = buildSettings.agentMaxSlope;
	m_cfg.walkableHeight = (int)ceilf(buildSettings.agentHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(buildSettings.agentMaxClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(buildSettings.agentRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(buildSettings.edgeMaxLen / buildSettings.cellSize);
	m_cfg.maxSimplificationError = buildSettings.edgeMaxError;
	m_cfg.minRegionArea = (int)rcSqr(buildSettings.regionMinSize);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(buildSettings.regionMergeSize);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)buildSettings.vertsPerPoly;
	m_cfg.detailSampleDist = buildSettings.detailSampleDist < 0.9f ? 0 : buildSettings.cellSize * buildSettings.detailSampleDist;
	m_cfg.detailSampleMaxError = buildSettings.cellHeight * buildSettings.detailSampleMaxError;
	
	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcVcopy(m_cfg.bmin, &bmin.x);
	rcVcopy(m_cfg.bmax, &bmax.x);
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	//
	// Step 2. Rasterize input polygon soup.
	//
	
	// Allocate voxel heightfield where we rasterize our input data to.
	m_solid = rcAllocHeightfield();
	if(!m_solid){
        //m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		LogError("buildNavigation: Out of memory 'solid'.");
		return false;
	}
	if(!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
        LogError("buildNavigation: Could not create solid heightfield.");
		return false;
	}
	
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
    }
	
	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	//if(m_filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
	//if(m_filterLedgeSpans)
		rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	//if (m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);


	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = rcAllocCompactHeightfield();
	if(!m_chf){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
        LogError("buildNavigation: Out of memory 'chf'.");
		return false;
	}
	if(!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
        LogError("buildNavigation: Could not build compact data.");
		return false;
	}
    
    bool m_keepInterResults = false;
	if(!m_keepInterResults){
		rcFreeHeightField(m_solid);
		m_solid = 0;
	}
		
	// Erode the walkable area by agent radius.
	if(!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf)){
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
		if(!rcBuildDistanceField(m_ctx, *m_chf)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
            LogError("buildNavigation: Could not build distance field.");
			return false;
		}
		
		// Partition the walkable surface into simple regions without holes.
		if(!rcBuildRegions(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
            LogError("buildNavigation: Could not build watershed regions.");
			return false;
		}
	} else if(m_partitionType == SAMPLE_PARTITION_MONOTONE){
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if(!rcBuildRegionsMonotone(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
            LogError("buildNavigation: Could not build monotone regions.");
			return false;
		}
	} else{ // SAMPLE_PARTITION_LAYERS
		// Partition the walkable surface into simple regions without holes.
		if(!rcBuildLayerRegions(m_ctx, *m_chf, 0, m_cfg.minRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
            LogError("buildNavigation: Could not build layer regions.");
			return false;
		}
	}
	
	//
	// Step 5. Trace and simplify region contours.
	//
	
	// Create contours.
	m_cset = rcAllocContourSet();
	if(!m_cset){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
        LogError("buildNavigation: Out of memory 'cset'.");
		return false;
	}
	if(!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
        LogError("buildNavigation: Could not create contours.");
		return false;
	}
	
	//
	// Step 6. Build polygons mesh from contours.
	//
	
	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if(!m_pmesh){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
        LogError("buildNavigation: Out of memory 'pmesh'.");
		return false;
	}
	if(!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
        LogError("buildNavigation: Could not triangulate contours.");
		return false;
	}
	
	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//
	
	m_dmesh = rcAllocPolyMeshDetail();
	if(!m_dmesh){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
        LogError("buildNavigation: Out of memory 'pmdtl'.");
		return false;
	}

	if(!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
        LogError("buildNavigation: Could not build detail mesh.");
		return false;
	}

	if(!m_keepInterResults){
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_cset);
		m_cset = 0;
	}

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.
	
	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//
	
	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
	if(m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON){
		unsigned char* navData = 0;
		int navDataSize = 0;

		// Update poly flags from areas.
		for(int i = 0; i < m_pmesh->npolys; ++i){
			if(m_pmesh->areas[i] == RC_WALKABLE_AREA)
				m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
				
			if(m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
            {
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			} else if(m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER){
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			} else if(m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR){
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}

		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = m_pmesh->verts;
		params.vertCount = m_pmesh->nverts;
		params.polys = m_pmesh->polys;
		params.polyAreas = m_pmesh->areas;
		params.polyFlags = m_pmesh->flags;
		params.polyCount = m_pmesh->npolys;
		params.nvp = m_pmesh->nvp;
		params.detailMeshes = m_dmesh->meshes;
		params.detailVerts = m_dmesh->verts;
		params.detailVertsCount = m_dmesh->nverts;
		params.detailTris = m_dmesh->tris;
		params.detailTriCount = m_dmesh->ntris;
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
		rcVcopy(params.bmin, m_pmesh->bmin);
		rcVcopy(params.bmax, m_pmesh->bmax);
		params.cs = m_cfg.cs;
		params.ch = m_cfg.ch;
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
	}
	
	m_ctx->stopTimer(RC_TIMER_TOTAL);

	return true;
}

bool Navmesh::TileInit(Scene* scene, AABB bounds){
	Cleanup();

    m_ctx = new rcContext();
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
	
	return true;
}

bool Navmesh::BakeAllTiles(Scene* scene, AABB bounds){
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
	m_ctx->startTimer(RC_TIMER_TEMP);

	for(int y = 0; y < th; ++y){
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
	}
	
	// Start the build process.	
	m_ctx->stopTimer(RC_TIMER_TEMP);

	//m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TEMP)/1000.0f;

	return true;
}

bool Navmesh::BakeTile(Scene* scene, AABB bounds, const Vector3 pos){
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
	
	m_ctx->resetLog();
	
	int dataSize = 0;
	unsigned char* data = BuildTileMesh(scene, tx, ty, m_lastBuiltTileBmin, m_lastBuiltTileBmax, dataSize);

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
	LogWarning("Build Tile (%d,%d):", tx, ty);
	return true;
}

bool Navmesh::RemoveTile(Scene* scene, AABB bounds, const Vector3 pos){
	return false;
}

void Navmesh::GetTilePos(const float* pos, int& tx, int& ty){
	//if(!m_geom) return;
	
	/*const float* bmin = m_geom->getNavMeshBoundsMin();
	
	const float ts = m_tileSize*m_cellSize;
	tx = (int)((pos[0] - bmin[0]) / ts);
	ty = (int)((pos[2] - bmin[2]) / ts);*/
}

unsigned char* Navmesh::BuildTileMesh(Scene* scene, const int tx, const int ty, const float* bmin, const float* bmax, int& dataSize){
	/*if(!m_geom || !m_geom->getMesh() || !m_geom->getChunkyMesh()){
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Input mesh is not specified.");
		return 0;
	}*/
	
	float m_tileMemUsage = 0;
	float m_tileBuildTime = 0;
	
	Cleanup();
	
	/*const float* verts = m_geom->getMesh()->getVerts();
	const int nverts = m_geom->getMesh()->getVertCount();
	const int ntris = m_geom->getMesh()->getTriCount();
	const rcChunkyTriMesh* chunkyMesh = m_geom->getChunkyMesh();*/
		
	// Init build configuration from GUI
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = buildSettings.cellSize;
	m_cfg.ch = buildSettings.cellHeight;
	m_cfg.walkableSlopeAngle = buildSettings.agentMaxSlope;
	m_cfg.walkableHeight = (int)ceilf(buildSettings.agentHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(buildSettings.agentMaxClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(buildSettings.agentRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(buildSettings.edgeMaxLen / buildSettings.cellSize);
	m_cfg.maxSimplificationError = buildSettings.edgeMaxError;
	m_cfg.minRegionArea = (int)rcSqr(buildSettings.regionMinSize);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(buildSettings.regionMergeSize);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)buildSettings.vertsPerPoly;
	m_cfg.tileSize = (int)buildSettings.tileSize;
	m_cfg.borderSize = m_cfg.walkableRadius + 3; // Reserve enough padding.
	m_cfg.width = m_cfg.tileSize + m_cfg.borderSize*2;
	m_cfg.height = m_cfg.tileSize + m_cfg.borderSize*2;
	m_cfg.detailSampleDist = buildSettings.detailSampleDist < 0.9f ? 0 : buildSettings.cellSize * buildSettings.detailSampleDist;
	m_cfg.detailSampleMaxError = buildSettings.cellHeight * buildSettings.detailSampleMaxError;
	
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
	rcVcopy(m_cfg.bmin, bmin);
	rcVcopy(m_cfg.bmax, bmax);
	m_cfg.bmin[0] -= m_cfg.borderSize*m_cfg.cs;
	m_cfg.bmin[2] -= m_cfg.borderSize*m_cfg.cs;
	m_cfg.bmax[0] += m_cfg.borderSize*m_cfg.cs;
	m_cfg.bmax[2] += m_cfg.borderSize*m_cfg.cs;
	
	// Reset build times gathering.
	m_ctx->resetTimers();
	
	// Start the build process.
	m_ctx->startTimer(RC_TIMER_TOTAL);
	
	//m_ctx->log(RC_LOG_PROGRESS, "Building navigation:");
	//m_ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg.width, m_cfg.height);
	//m_ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts/1000.0f, ntris/1000.0f);
	LogWarning("Building navigation:");
	LogWarning(" - %d x %d cells", m_cfg.width, m_cfg.height);
	
	// Allocate voxel heightfield where we rasterize our input data to.
	m_solid = rcAllocHeightfield();
	if(!m_solid){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		LogError("buildNavigation: Out of memory 'solid'.");
		return 0;
	}
	if(!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		LogError("buildNavigation: Could not create solid heightfield.");
		return 0;
	}

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
    }
	
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
		rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
	//if(m_filterLedgeSpans)
		rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	//if(m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);
	
	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = rcAllocCompactHeightfield();
	if(!m_chf){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		LogError("buildNavigation: Out of memory 'chf'.");
		return 0;
	}
	if(!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		LogError("buildNavigation: Could not build compact data.");
		return 0;
	}
	
	//if(!m_keepInterResults){
		rcFreeHeightField(m_solid);
		m_solid = 0;
	//}

	// Erode the walkable area by agent radius.
	if(!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf)){
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
		if(!rcBuildDistanceField(m_ctx, *m_chf)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
			LogError("buildNavigation: Could not build distance field.");
			return 0;
		}
		
		// Partition the walkable surface into simple regions without holes.
		if(!rcBuildRegions(m_ctx, *m_chf, m_cfg.borderSize, m_cfg.minRegionArea, m_cfg.mergeRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
			LogError("buildNavigation: Could not build watershed regions.");
			return 0;
		}
	} else if(m_partitionType == SAMPLE_PARTITION_MONOTONE){
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if(!rcBuildRegionsMonotone(m_ctx, *m_chf, m_cfg.borderSize, m_cfg.minRegionArea, m_cfg.mergeRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
			LogError("buildNavigation: Could not build monotone regions.");
			return 0;
		}
	} else // SAMPLE_PARTITION_LAYERS
	{
		// Partition the walkable surface into simple regions without holes.
		if(!rcBuildLayerRegions(m_ctx, *m_chf, m_cfg.borderSize, m_cfg.minRegionArea)){
			//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
			LogError("buildNavigation: Could not build layer regions.");
			return 0;
		}
	}
	 	
	// Create contours.
	m_cset = rcAllocContourSet();
	if(!m_cset){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
		LogError("buildNavigation: Out of memory 'cset'.");
		return 0;
	}
	if(!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
		LogError("buildNavigation: Could not create contours.");
		return 0;
	}
	
	if(m_cset->nconts == 0){
		return 0;
	}
	
	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if(!m_pmesh){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		LogError("buildNavigation: Out of memory 'pmesh'.");
		return 0;
	}
	if(!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh)){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		LogError("buildNavigation: Could not triangulate contours.");
		return 0;
	}
	
	// Build detail mesh.
	m_dmesh = rcAllocPolyMeshDetail();
	if(!m_dmesh){
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'dmesh'.");
		LogError("buildNavigation: Out of memory 'dmesh'.");
		return 0;
	}
	
	if(!rcBuildPolyMeshDetail(
		m_ctx, *m_pmesh, *m_chf,
		m_cfg.detailSampleDist, m_cfg.detailSampleMaxError,
		*m_dmesh))
	{
		//m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could build polymesh detail.");
		LogError("buildNavigation: Could build polymesh detail.");
		return 0;
	}
	
	//if(!m_keepInterResults){
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_cset);
		m_cset = 0;
	//}
	
	unsigned char* navData = 0;
	int navDataSize = 0;
	if(m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON){
		if(m_pmesh->nverts >= 0xffff){
			// The vertex indices are ushorts, and cannot point to more than 0xffff vertices.
			//m_ctx->log(RC_LOG_ERROR, "Too many vertices per tile %d (max: %d).", m_pmesh->nverts, 0xffff);
			LogError("Too many vertices per tile %d (max: %d).", m_pmesh->nverts, 0xffff);
			return 0;
		}
		
		// Update poly flags from areas.
		for(int i = 0; i < m_pmesh->npolys; ++i){
			if(m_pmesh->areas[i] == RC_WALKABLE_AREA)
				m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
			
			if(m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			} else if(m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER){
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			} else if(m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR){
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}
		
		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = m_pmesh->verts;
		params.vertCount = m_pmesh->nverts;
		params.polys = m_pmesh->polys;
		params.polyAreas = m_pmesh->areas;
		params.polyFlags = m_pmesh->flags;
		params.polyCount = m_pmesh->npolys;
		params.nvp = m_pmesh->nvp;
		params.detailMeshes = m_dmesh->meshes;
		params.detailVerts = m_dmesh->verts;
		params.detailVertsCount = m_dmesh->nverts;
		params.detailTris = m_dmesh->tris;
		params.detailTriCount = m_dmesh->ntris;
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
		rcVcopy(params.bmin, m_pmesh->bmin);
		rcVcopy(params.bmax, m_pmesh->bmax);
		params.cs = m_cfg.cs;
		params.ch = m_cfg.ch;
		params.buildBvTree = true;
		
		if(!dtCreateNavMeshData(&params, &navData, &navDataSize)){
			//m_ctx->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
			LogError("Could not build Detour navmesh.");
			return 0;
		}		
	}
	m_tileMemUsage = navDataSize/1024.0f;
	
	m_ctx->stopTimer(RC_TIMER_TOTAL);
	
	// Show performance stats.
	//duLogBuildTimes(*m_ctx, m_ctx->getAccumulatedTime(RC_TIMER_TOTAL));
	//m_ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);
	LogWarning(">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);
	
	//m_tileBuildTime = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;

	dataSize = navDataSize;
	return navData;
}

void Navmesh::DrawDebug(){
    if(m_dd == nullptr) m_dd = new DebugDrawGL();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

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
		
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);	
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

	LogWarning("x:%f y:%f z:%f", _startPos[0], _startPos[1], _startPos[2]);

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
		if(m_polys[m_npolys-1] != m_endRef)
			m_navQuery->closestPointOnPoly(m_polys[m_npolys-1], _endPos, epos, 0);
		
		m_navQuery->findStraightPath(
			_startPos, epos, m_polys, m_npolys,
			m_straightPath, m_straightPathFlags,
			m_straightPathPolys, &m_nstraightPath, MAX_POLYS, m_straightPathOptions
		);

		outPath.corners.clear();
		for(int i = 0; i < m_nstraightPath*3; i += 3){
			outPath.corners.push_back(Vector3(m_straightPath[i], m_straightPath[i+1], m_straightPath[i+2]));
		}

		outPath.status = NavMeshPathStatus::PathComplete;
		LogWarningExtra("OK Count: %zd", outPath.corners.size());
		return true;
	}

	outPath.status = NavMeshPathStatus::PathInvalid;
	outPath.corners.clear();
	LogWarningExtra("Not OK");
	return false;
}

NavmeshSystem::NavmeshSystem(Scene* inScene):System(inScene){}
NavmeshSystem::~NavmeshSystem(){}

void NavmeshSystem::Update(){
	Ref<Navmesh> navmesh = nullptr;

	auto navmeshView = scene->GetRegistry().view<NavmeshComponent>();
	for(auto e: navmeshView){
		NavmeshComponent& navmeshComponent = navmeshView.get<NavmeshComponent>(e);
		navmesh = navmeshComponent.navmesh;
	}

	if(navmesh == nullptr) return;

	auto navmeshAgentView = scene->GetRegistry().view<NavmeshAgentComponent, TransformComponent>();
	for(auto e: navmeshAgentView){
		NavmeshAgentComponent& navmeshComponent = navmeshAgentView.get<NavmeshAgentComponent>(e);
		TransformComponent& transform = navmeshAgentView.get<TransformComponent>(e);

		if(navmeshComponent.isDirty /*|| (transform.Position() != navmeshComponent.lastPos)*/){
			navmeshComponent.isDirty = false;
			navmeshComponent.lastPos = transform.Position();
			navmesh->FindPath(transform.Position(), navmeshComponent.destination, navmeshComponent.path);
			navmeshComponent.curPathIndex = -1;
			navmeshComponent.reach = false;
		}

		if(scene->Running() == false) continue;

		if(navmeshComponent.path.status == NavMeshPathStatus::PathComplete){
			if(navmeshComponent.curPathIndex == -1){
				navmeshComponent.curPathIndex = 0;
				navmeshComponent.reach = false;
			}

			if(navmeshComponent.reach) return;

			Vector3 pos = transform.Position();
			Vector3 dir = navmeshComponent.path.corners[navmeshComponent.curPathIndex] - pos;
			if(math::length(dir) > 0.1f) dir = math::normalize(dir);
        	Assert(Mathf::IsNan(dir) == false);

			float distance = math::distance(pos, navmeshComponent.path.corners[navmeshComponent.curPathIndex]);

			if(distance <= navmeshComponent.stopDistance){
				navmeshComponent.curPathIndex += 1;
				if(navmeshComponent.curPathIndex >= navmeshComponent.path.corners.size()){
					navmeshComponent.curPathIndex += navmeshComponent.path.corners.size()-1;
					navmeshComponent.reach = true;
					continue;
				}
			}
			
			transform.Position(pos + dir * (navmeshComponent.speed * Application::DeltaTime()));
		} else {
			navmeshComponent.curPathIndex = -1;
			navmeshComponent.reach = false;
		}
	}
}

void NavmeshSystem::OnDrawGizmos(){
	auto navmeshAgentView = scene->GetRegistry().view<NavmeshAgentComponent>();
	for(auto e: navmeshAgentView){
		NavmeshAgentComponent& navmeshComponent = navmeshAgentView.get<NavmeshAgentComponent>(e);
		if(navmeshComponent.path.status == NavMeshPathStatus::PathInvalid) continue;
		if(navmeshComponent.path.corners.size() < 2) continue;

		for(int i = 0; i < navmeshComponent.path.corners.size()-1; i++){
			Graphics::DrawLine(
				navmeshComponent.path.corners[i] + Vector3(0, 0.1f, 0), 
				navmeshComponent.path.corners[i+1] + Vector3(0, 0.1f, 0), 
				Vector3(0, 1, 0), 
				1
			);
		}
	}
}

}