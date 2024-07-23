#include "Navmesh.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Scene/Scene.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/Platform/GL.h"
#include <DebugDraw.h>
#include <DetourDebugDraw.h>

namespace OD{

class DebugDrawGL : public duDebugDraw{
public:
    Ref<Shader> shader;
    Ref<Mesh> mesh;

    DebugDrawGL(){
        shader = Shader::CreateFromFile("res/Engine/Shaders/Gizmos.glsl");
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
        mesh->UpdateMesh();
        Graphics::DrawMesh(*mesh, *shader, Matrix4Identity);
    }
};

DebugDrawGL* m_dd = nullptr;

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
	dtFreeNavMesh(m_navMesh);
	m_navMesh = 0;

    delete m_ctx;
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
		/*for(int i = 0; i < m_pmesh->npolys; ++i){
			if(m_pmesh->areas[i] == RC_WALKABLE_AREA)
				m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
				
			if(m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
            {
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			} else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER){
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			} else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR){
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}*/


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
		
		status = m_navQuery->init(m_navMesh, 2048);
		if(dtStatusFailed(status)){
			//m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
            LogError("Could not init Detour navmesh query");
			return false;
		}
	}
	
	m_ctx->stopTimer(RC_TIMER_TOTAL);

	return true;
}

void Navmesh::DrawDebug(){
    if(m_dd == nullptr) m_dd = new DebugDrawGL();

	glEnable(GL_FOG);
	glDepthMask(GL_TRUE);

	const float texScale = 1.0f / (buildSettings.cellSize * 10.0f);

	glDisable(GL_FOG);
	glDepthMask(GL_FALSE);

	if(m_navMesh && m_navQuery &&
		(m_drawMode == DRAWMODE_NAVMESH ||
		m_drawMode == DRAWMODE_NAVMESH_TRANS ||
		m_drawMode == DRAWMODE_NAVMESH_BVTREE ||
		 m_drawMode == DRAWMODE_NAVMESH_NODES ||
		m_drawMode == DRAWMODE_NAVMESH_INVIS))
	{
		if (m_drawMode != DRAWMODE_NAVMESH_INVIS)
			duDebugDrawNavMeshWithClosedList(m_dd, *m_navMesh, *m_navQuery, m_navMeshDrawFlags);
		if (m_drawMode == DRAWMODE_NAVMESH_BVTREE)
			duDebugDrawNavMeshBVTree(m_dd, *m_navMesh);
		if (m_drawMode == DRAWMODE_NAVMESH_NODES)
			duDebugDrawNavMeshNodes(m_dd, *m_navQuery);
		duDebugDrawNavMeshPolysWithFlags(m_dd, *m_navMesh, SAMPLE_POLYFLAGS_DISABLED, duRGBA(0,0,0,128));
	}
		
	glDepthMask(GL_TRUE);
	
	/*if (m_chf && m_drawMode == DRAWMODE_COMPACT)
		duDebugDrawCompactHeightfieldSolid(&m_dd, *m_chf);

	if (m_chf && m_drawMode == DRAWMODE_COMPACT_DISTANCE)
		duDebugDrawCompactHeightfieldDistance(&m_dd, *m_chf);
	if (m_chf && m_drawMode == DRAWMODE_COMPACT_REGIONS)
		duDebugDrawCompactHeightfieldRegions(&m_dd, *m_chf);
	if (m_solid && m_drawMode == DRAWMODE_VOXELS)
	{
		glEnable(GL_FOG);
		duDebugDrawHeightfieldSolid(&m_dd, *m_solid);
		glDisable(GL_FOG);
	}
	if (m_solid && m_drawMode == DRAWMODE_VOXELS_WALKABLE)
	{
		glEnable(GL_FOG);
		duDebugDrawHeightfieldWalkable(&m_dd, *m_solid);
		glDisable(GL_FOG);
	}
	if (m_cset && m_drawMode == DRAWMODE_RAW_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawRawContours(&m_dd, *m_cset);
		glDepthMask(GL_TRUE);
	}
	if (m_cset && m_drawMode == DRAWMODE_BOTH_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawRawContours(&m_dd, *m_cset, 0.5f);
		duDebugDrawContours(&m_dd, *m_cset);
		glDepthMask(GL_TRUE);
	}
	if (m_cset && m_drawMode == DRAWMODE_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawContours(&m_dd, *m_cset);
		glDepthMask(GL_TRUE);
	}
	if (m_chf && m_cset && m_drawMode == DRAWMODE_REGION_CONNECTIONS)
	{
		duDebugDrawCompactHeightfieldRegions(&m_dd, *m_chf);
			
		glDepthMask(GL_FALSE);
		duDebugDrawRegionConnections(&m_dd, *m_cset);
		glDepthMask(GL_TRUE);
	}
	if (m_pmesh && m_drawMode == DRAWMODE_POLYMESH)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawPolyMesh(&m_dd, *m_pmesh);
		glDepthMask(GL_TRUE);
	}
	if (m_dmesh && m_drawMode == DRAWMODE_POLYMESH_DETAIL)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawPolyMeshDetail(&m_dd, *m_dmesh);
		glDepthMask(GL_TRUE);
	}
	
	m_geom->drawConvexVolumes(&m_dd);

	if (m_tool) m_tool->handleRender();
	renderToolStates();*/

	glDepthMask(GL_TRUE);
}

}