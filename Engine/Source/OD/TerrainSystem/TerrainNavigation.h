#pragma once
#include "OD/Base.h"
#include "OD/Core/Math.h"
#include <functional>

namespace OD{
class Scene;
struct AABB;
struct LayerMask;
class Mesh;

// CPU geometry scoped to one bake region, unrelated to render LOD and GPU meshes.
OD_API void RasterizeTerrainNavigation(Scene& scene,
                                       AABB worldBounds,
                                       LayerMask mask,
                                       const std::function<void(const Matrix4&, Ref<Mesh>&)>& rasterize);
}
