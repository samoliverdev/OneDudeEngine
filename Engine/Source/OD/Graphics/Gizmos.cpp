#include "Gizmos.h"
#include "Graphics.h"

namespace OD{

Vector3 _Plane3Intersect(Plane p1, Plane p2, Plane p3){ //get the intersection point of 3 planes
    return ( ( -p1.n.w * math::cross( Vector3(p2.n), Vector3(p3.n) ) ) +
            ( -p2.n.w * math::cross( Vector3(p3.n), Vector3(p1.n) ) ) +
            ( -p3.n.w * math::cross( Vector3(p1.n), Vector3(p2.n) ) ) ) /
        ( math::dot( Vector3(p1.n), math::cross( Vector3(p2.n), Vector3(p3.n) ) ) );
}

//https://forum.unity.com/threads/drawfrustum-is-drawing-incorrectly.208081/
void Gizmos::DrawFrustum(Frustum frustum, Matrix4 model, Vector3 color){
    Vector3 nearCorners[4]; //Approx'd nearplane corners
    Vector3 farCorners[4]; //Approx'd farplane corners
    Plane camPlanes[6];
    camPlanes[0] = frustum.leftFace;
    camPlanes[1] = frustum.rightFace;
    camPlanes[2] = frustum.bottomFace;
    camPlanes[3] = frustum.topFace;
    camPlanes[4] = frustum.nearFace;
    camPlanes[5] = frustum.farFace;

    Plane temp = camPlanes[1]; camPlanes[1] = camPlanes[2]; camPlanes[2] = temp; //swap [1] and [2] so the order is better for the loop

    for(int i = 0; i < 4; i++){
        nearCorners[i] = _Plane3Intersect(camPlanes[4], camPlanes[i], camPlanes[(i + 1) % 4]); //near corners on the created projection matrix
        farCorners[i] = _Plane3Intersect(camPlanes[5], camPlanes[i], camPlanes[(i + 1) % 4]); //far corners on the created projection matrix
    }

    for(int i = 0; i < 4; i++){
        Graphics::DrawLine(model, nearCorners[i], nearCorners[( i + 1 ) % 4], color, 1); //near corners on the created projection matrix
        Graphics::DrawLine(model, farCorners[i], farCorners[( i + 1 ) % 4], color, 1); //far corners on the created projection matrix
        Graphics::DrawLine(model, nearCorners[i], farCorners[i], color, 1); //sides of the created projection matrix
    }
}

}