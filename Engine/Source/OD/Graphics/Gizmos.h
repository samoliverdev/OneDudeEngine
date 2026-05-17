#pragma once
#include "OD/Defines.h"
#include "Geometry.h"

namespace OD{

class OD_API Gizmos {
public:
    static void DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color = Vector3(1,1,1));
};

}