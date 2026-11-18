#pragma once
#include "TerrainComponents.h"
#include "OD/Graphics/Camera.h"

namespace OD{
class OD_API TerrainEditor{
public:
    static bool IsEditing(Scene& scene, Entity selected);
    // Image rectangle is in screen pixels, including letterboxing/custom viewport sizes.
    static void
    DrawViewport(Scene& scene, Entity selected, const Camera& camera, Vector2 imageMin, Vector2 imageMax, bool flipY);
};
}
