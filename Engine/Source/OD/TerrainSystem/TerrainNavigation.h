#pragma once
#include "TerrainComponents.h"
#include "OD/Graphics/Mesh.h"
#include <functional>

namespace OD{
// CPU geometry scoped to one bake region, unrelated to render LOD and GPU meshes.
OD_API void RasterizeTerrainNavigation(Scene& scene,
                                       AABB worldBounds,
                                       LayerMask mask,
                                       const std::function<void(const Matrix4&, Ref<Mesh>&)>& rasterize);
}
