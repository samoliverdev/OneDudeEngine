#include "OD/pch.h"
#include "TerrainComponents.h"
#include "TerrainRendering.h"
#include "TerrainPhysics.h"
#include "OD/Scene/SceneManager.h"
#include "OD/RenderPipeline/RenderContext.h"

namespace OD{
void TerrainSystemModuleInit(){
    auto& manager = SceneManager::Get();
    manager.RegisterCoreComponent<TerrainDataComponent>("TerrainDataComponent", "Terrain System");
    manager.RegisterCoreComponent<TerrainRendererComponent>("TerrainRendererComponent", "Terrain System");
    manager.RegisterCoreComponent<TerrainCollisionComponent>("TerrainCollisionComponent", "Terrain System");
    manager.RegisterCoreComponent<TerrainNavmeshDataComponent>("TerrainNavmeshDataComponent", "Terrain System");
    manager.RegisterCoreComponent<TerrainEditorComponent>("TerrainEditorComponent", "Terrain System");
    manager.RegisterSystem<TerrainCollisionSystem>("TerrainCollisionSystem");
    ResourceTypesDB::Get().RegisterAssetType<TerrainPaintMap>(".terrainpaint", [](const std::string& path){
        return ResourceManager::Get().LoadByPath<TerrainPaintMap>(path);
    });
    RendererFeatureContext::RegisterRenderFeature<TerrainQuadtreeRenderer>();
}
}
