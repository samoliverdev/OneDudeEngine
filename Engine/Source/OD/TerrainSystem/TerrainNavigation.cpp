#include "OD/pch.h"
#include "TerrainNavigation.h"
#include "OD/Graphics/Geometry.h"
#include "OD/Navmesh/Navmesh.h"

namespace OD{
void RasterizeTerrainNavigation(Scene& scene,
                                AABB worldBounds,
                                LayerMask mask,
                                const std::function<void(const Matrix4&, Ref<Mesh>&)>& rasterize){
    auto view =
        scene.GetRegistry().view<TerrainDataComponent, TerrainNavmeshDataComponent, TransformComponent, InfoComponent>(
            entt::exclude<SelfDisable, NavmeshSkipTag>);
    for(auto e : view){
        auto& data = view.get<TerrainDataComponent>(e);
        auto& source = view.get<TerrainNavmeshDataComponent>(e);
        auto& transform = view.get<TransformComponent>(e);
        auto& info = view.get<InfoComponent>(e);
        if(!source.enabled || !info.enable || !mask.Contains(info.layer) || !data.View().Valid())
            continue;
        source.Validate();
        Matrix4 root = transform.GlobalModelMatrix();
        if(std::abs(math::determinant(root)) < 1e-10f)
            continue;
        auto terrainBounds = transform_aabb_optimized_abs_center_extents(
            AABB(data.size * 0.5f, data.size.x * 0.5f, data.size.y * 0.5f, data.size.z * 0.5f), root);
        if(!terrainBounds.isOnAABB(worldBounds))
            continue;
        Matrix4 inverse = math::inverse(root);
        Vector3 lo(std::numeric_limits<float>::max()), hi(-std::numeric_limits<float>::max());
        auto min = worldBounds.GetMin(), max = worldBounds.GetMax();
        for(int z = 0; z < 2; ++z)
            for(int y = 0; y < 2; ++y)
                for(int x = 0; x < 2; ++x){
                    Vector3 p = Vector3(inverse * Vector4(x ? max.x : min.x, y ? max.y : min.y, z ? max.z : min.z, 1));
                    lo = math::min(lo, p);
                    hi = math::max(hi, p);
                }
        float spacing =
            (std::max)(source.sampleSpacing, (std::max)(hi.x - lo.x, hi.z - lo.z) / float(source.maxCellsPerAxis - 2));
        float x0 = (std::max)(0.f, std::floor(lo.x / spacing) * spacing),
              z0 = (std::max)(0.f, std::floor(lo.z / spacing) * spacing);
        float x1 = (std::min)(data.size.x, std::ceil(hi.x / spacing) * spacing),
              z1 = (std::min)(data.size.z, std::ceil(hi.z / spacing) * spacing);
        if(x1 <= x0 || z1 <= z0)
            continue;
        int nx = std::clamp(int(std::ceil((x1 - x0) / spacing)), 1, source.maxCellsPerAxis);
        int nz = std::clamp(int(std::ceil((z1 - z0) / spacing)), 1, source.maxCellsPerAxis);
        auto mesh = CreateRef<Mesh>();
        auto field = data.View();
        mesh->vertices.reserve(size_t(nx + 1) * (nz + 1));
        mesh->indices.reserve(size_t(nx) * nz * 6);
        for(int z = 0; z <= nz; ++z)
            for(int x = 0; x <= nx; ++x){
                float px = (std::min)(x1, x0 + x * spacing), pz = (std::min)(z1, z0 + z * spacing);
                mesh->vertices.emplace_back(px, field.Sample(px, pz), pz);
            }
        for(int z = 0; z < nz; ++z)
            for(int x = 0; x < nx; ++x){
                unsigned a = unsigned(z * (nx + 1) + x), b = a + unsigned(nx + 1);
                mesh->indices.insert(mesh->indices.end(), {a, b, b + 1, a, b + 1, a + 1});
            }
        rasterize(root, mesh);
    }
}
}
