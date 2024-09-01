#include "Culling.h"
#include "OD/Core/Math.h"

namespace OD{

/*Frustum CreateFrustumFromCamera(Transform& cam, float aspect, float fovY, float zNear, float zFar){
    Vector3 forward = -cam.Forward();
    Vector3 right = cam.Right();
    Vector3 up = cam.Up();
    Vector3 position = cam.LocalPosition();

    Frustum frustum;
    float halfVSide = zFar * tanf(fovY * .5f);
    float halfHSide = halfVSide * aspect;
    Vector3 frontMultFar = zFar * forward;

    frustum.nearFace = { position + zNear * forward, forward };
    frustum.farFace = { position + frontMultFar, -forward };
    frustum.rightFace = { position, math::cross(frontMultFar - right * halfHSide, up) };
    frustum.leftFace = { position, math::cross(up, frontMultFar + right * halfHSide) };
    frustum.topFace = { position, math::cross(right, frontMultFar - up * halfVSide) };
    frustum.bottomFace = { position, math::cross(frontMultFar + up * halfVSide, right) };

    return frustum;
}

Frustum CreateFrustumFromOthor(Transform& cam, float orthographicSize, float aspect, float zNear, float zFar){
    Vector3 forward = -cam.Forward();
    Vector3 right = cam.Right();
    Vector3 left = -right;
    Vector3 up = cam.Up();
    Vector3 down = -up;
    Vector3 position = cam.LocalPosition();

    float size = aspect * orthographicSize;

    Frustum frustum;
 
    frustum.nearFace = { position + zNear * forward, forward };
    frustum.farFace = { position + zFar * forward, -forward };
    frustum.rightFace = { position + right * size, right};
    frustum.leftFace = { position + left * size, left };
    frustum.topFace = { position + up * size, up };
    frustum.bottomFace = { position + down * size, down };

    return frustum;
}

Frustum CreateFrustumFromMatrix(const Matrix4& viewMatrix, const Matrix4& projectionMatrix){
    const Matrix4& v = viewMatrix;
    const Matrix4& p = projectionMatrix;
 
    Matrix4 clipMatrix;

    Frustum out;
 
    clipMatrix[0][0] = v[0][0 ]*p[0][0]+v[0][1]*p[1][0]+v[0][2]*p[2][0]+v[0][3]*p[3][0];
    clipMatrix[1][0] = v[0][0]*p[0][1]+v[0][1]*p[1][1]+v[0][2]*p[2][1]+v[0][3]*p[3][1];
    clipMatrix[2][0] = v[0][0]*p[0][2]+v[0][1]*p[1][2]+v[0][2]*p[2][2]+v[0][3]*p[3][2];
    clipMatrix[3][0] = v[0][0]*p[0][3]+v[0][1]*p[1][3]+v[0][2]*p[2][3]+v[0][3]*p[3][3];
    clipMatrix[0][1] = v[1][0]*p[0][0]+v[1][1]*p[1][0]+v[1][2]*p[2][0]+v[1][3]*p[3][0];
    clipMatrix[1][1] = v[1][0]*p[0][1]+v[1][1]*p[1][1]+v[1][2]*p[2][1]+v[1][3]*p[3][1];
    clipMatrix[2][1] = v[1][0]*p[0][2]+v[1][1]*p[1][2]+v[1][2]*p[2][2]+v[1][3]*p[3][2];
    clipMatrix[3][1] = v[1][0]*p[0][3]+v[1][1]*p[1][3]+v[1][2]*p[2][3]+v[1][3]*p[3][3];
    clipMatrix[0][2] = v[2][0]*p[0][0]+v[2][1]*p[1][0]+v[2][2]*p[2][0]+v[2][3]*p[3][0];
    clipMatrix[1][2] = v[2][0]*p[0][1]+v[2][1]*p[1][1]+v[2][2]*p[2][1]+v[2][3]*p[3][1];
    clipMatrix[2][2] = v[2][0]*p[0][2]+v[2][1]*p[1][2]+v[2][2]*p[2][2]+v[2][3]*p[3][2];
    clipMatrix[3][2] = v[2][0]*p[0][3]+v[2][1]*p[1][3]+v[2][2]*p[2][3]+v[2][3]*p[3][3];
    clipMatrix[0][3] = v[3][0]*p[0][0]+v[3][1]*p[1][0]+v[3][2]*p[2][0]+v[3][3]*p[3][0];
    clipMatrix[1][3] = v[3][0]*p[0][1]+v[3][1]*p[1][1]+v[3][2]*p[2][1]+v[3][3]*p[3][1];
    clipMatrix[2][3] = v[3][0]*p[0][2]+v[3][1]*p[1][2]+v[3][2]*p[2][2]+v[3][3]*p[3][2];
    clipMatrix[3][3] = v[3][0]*p[0][3]+v[3][1]*p[1][3]+v[3][2]*p[2][3]+v[3][3]*p[3][3];
 
    out.rightFace.normal.x = clipMatrix[3][0]-clipMatrix[0][0];
    out.rightFace.normal.y = clipMatrix[3][1]-clipMatrix[0][1];
    out.rightFace.normal.z = clipMatrix[3][2]-clipMatrix[0][2];
    out.rightFace.distance = clipMatrix[3][3]-clipMatrix[0][3];
    out.rightFace.normal = -out.rightFace.normal;
 
    out.leftFace.normal.x = clipMatrix[3][0]+clipMatrix[0][0];
    out.leftFace.normal.y = clipMatrix[3][1]+clipMatrix[0][1];
    out.leftFace.normal.z = clipMatrix[3][2]+clipMatrix[0][2];
    out.leftFace.distance = clipMatrix[3][3]+clipMatrix[0][3];
    out.leftFace.normal = -out.leftFace.normal;
 
    out.bottomFace.normal.x = clipMatrix[3][0]+clipMatrix[1][0];
    out.bottomFace.normal.y = clipMatrix[3][1]+clipMatrix[1][1];
    out.bottomFace.normal.z = clipMatrix[3][2]+clipMatrix[1][2];
    out.bottomFace.distance = clipMatrix[3][3]+clipMatrix[1][3];
    out.bottomFace.normal = -out.bottomFace.normal;
 
    out.topFace.normal.x = clipMatrix[3][0]-clipMatrix[1][0];
    out.topFace.normal.y = clipMatrix[3][1]-clipMatrix[1][1];
    out.topFace.normal.z = clipMatrix[3][2]-clipMatrix[1][2];
    out.topFace.distance = clipMatrix[3][3]-clipMatrix[1][3];
    out.topFace.normal = -out.topFace.normal;
 
    out.nearFace.normal.x = clipMatrix[3][0]-clipMatrix[2][0];
    out.nearFace.normal.y = clipMatrix[3][1]-clipMatrix[2][1];
    out.nearFace.normal.z = clipMatrix[3][2]-clipMatrix[2][2];
    out.nearFace.distance = clipMatrix[3][3]-clipMatrix[2][3];
    out.nearFace.normal = -out.nearFace.normal;
 
    out.farFace.normal.x = clipMatrix[3][0]+clipMatrix[2][0];
    out.farFace.normal.y = clipMatrix[3][1]+clipMatrix[2][1];
    out.farFace.normal.z = clipMatrix[3][2]+clipMatrix[2][2];
    out.farFace.distance = clipMatrix[3][3]+clipMatrix[2][3];
    out.farFace.normal = -out.farFace.normal;

    out.rightFace.normalize();
    out.leftFace.normalize();
    out.bottomFace.normalize();
    out.topFace.normalize();
    out.nearFace.normalize();
    out.farFace.normalize();

    return out;
}
*/

// i extract planes from projection matrix using this class:
// mat is column major, mat[i] is ith column of matrix
// create frustum from  matrix
// if extracted from projection matrix only, planes will be in eye-space
// if extracted from view*projection, planes will be in world space
// if extracted from model*view*projection planes will be in model space
Frustum CreateFrustumFromMatrix2(const Matrix4& mat, bool normalizePlanes){
    Frustum out;

    out.leftFace = Plane(mat[3]+mat[0]);       // left
    out.rightFace = Plane(mat[3]-mat[0]);       // right
    out.topFace = Plane(mat[3]-mat[1]);       // top
    out.bottomFace = Plane(mat[3]+mat[1]);       // bottom
    out.nearFace = Plane(mat[3]+mat[2]);       // near
    out.farFace = Plane(mat[3]-mat[2]);       // far
    // normalize the plane equations, if requested
    if(normalizePlanes){
        out.leftFace.normalize();
        out.rightFace.normalize();
        out.topFace.normalize();
        out.bottomFace.normalize();
        out.nearFace.normalize();
        out.farFace.normalize();
    }

    return out;
}

bool BoundingVolume::isOnFrustum(Frustum& camFrustum) const{
    return (isOnOrForwardPlane(camFrustum.leftFace) &&
        isOnOrForwardPlane(camFrustum.rightFace) &&
        isOnOrForwardPlane(camFrustum.topFace) &&
        isOnOrForwardPlane(camFrustum.bottomFace) &&
        isOnOrForwardPlane(camFrustum.nearFace) &&
        isOnOrForwardPlane(camFrustum.farFace));
};

bool Sphere::isOnOrForwardPlane(Plane& plane) const{
    return plane.getSignedDistanceToPlane(center) > -radius;
}

bool Sphere::isOnFrustum(Frustum& camFrustum, Transform& transform) const{
    //Get global scale thanks to our transform
    Vector3 globalScale = transform.LocalScale();

    //Get our global center with process it with the global model matrix of our transform
    Vector3 globalCenter{ transform.GetLocalModelMatrix() * glm::vec4(center, 1.f) };

    //To wrap correctly our shape, we need the maximum scale scalar.
    const float maxScale = std::max(std::max(globalScale.x, globalScale.y), globalScale.z);

    //Max scale is assuming for the diameter. So, we need the half to apply it to our radius
    Sphere globalSphere(globalCenter, radius * (maxScale * 0.5f));

    //Check Firstly the result that have the most chance to failure to avoid to call all functions.
    return (globalSphere.isOnOrForwardPlane(camFrustum.leftFace) &&
        globalSphere.isOnOrForwardPlane(camFrustum.rightFace) &&
        globalSphere.isOnOrForwardPlane(camFrustum.farFace) &&
        globalSphere.isOnOrForwardPlane(camFrustum.nearFace) &&
        globalSphere.isOnOrForwardPlane(camFrustum.topFace) &&
        globalSphere.isOnOrForwardPlane(camFrustum.bottomFace));
};

bool SquareAABB::isOnOrForwardPlane(Plane& plane) const{
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    const float r = extent * (math::abs(plane.normal.x) + math::abs(plane.normal.y) + math::abs(plane.normal.z));
    return r <= plane.getSignedDistanceToPlane(center);
}

bool SquareAABB::isOnFrustum(Frustum& camFrustum, Transform& transform) const{
    //Get global scale thanks to our transform
    const Vector3 globalCenter{ transform.GetLocalModelMatrix() * glm::vec4(center, 1.f) };

    // Scaled orientation
    const Vector3 right = transform.Right() * extent;
    const Vector3 up = transform.Up() * extent;
    const Vector3 forward = transform.Forward() * extent;

    const float newIi = math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, forward));

    const float newIj = math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, forward));

    const float newIk = math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, forward));

    const SquareAABB globalAABB(globalCenter, math::max(math::max(newIi, newIj), newIk));

    return (globalAABB.isOnOrForwardPlane(camFrustum.leftFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.rightFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.topFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.bottomFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.nearFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.farFace));
};

void AABB::Expand(Vector3 amount){ 
    extents.x *= amount.x; 
    extents.y *= amount.y; 
    extents.z *= amount.z; 
}

AABB AABB::Scaled(Vector3 s){
    AABB result = *this;
    result.Expand(s);
    return result; 
}

std::array<Vector3, 8> AABB::getVertice() const{
    std::array<Vector3, 8> vertice;
    vertice[0] = { center.x - extents.x, center.y - extents.y, center.z - extents.z };
    vertice[1] = { center.x + extents.x, center.y - extents.y, center.z - extents.z };
    vertice[2] = { center.x - extents.x, center.y + extents.y, center.z - extents.z };
    vertice[3] = { center.x + extents.x, center.y + extents.y, center.z - extents.z };
    vertice[4] = { center.x - extents.x, center.y - extents.y, center.z + extents.z };
    vertice[5] = { center.x + extents.x, center.y - extents.y, center.z + extents.z };
    vertice[6] = { center.x - extents.x, center.y + extents.y, center.z + extents.z };
    vertice[7] = { center.x + extents.x, center.y + extents.y, center.z + extents.z };
    return vertice;
}

//see https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
bool AABB::isOnOrForwardPlane(Plane& plane) const{
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    const float r = extents.x * math::abs(plane.normal.x) + extents.y * math::abs(plane.normal.y) +
        extents.z * math::abs(plane.normal.z);

    return -r <= plane.getSignedDistanceToPlane(center);
}

bool AABB::isOnFrustum(Frustum& camFrustum, Transform& transform) const{
    //Get global scale thanks to our transform
    const Vector3 globalCenter{ transform.GetLocalModelMatrix() * Vector4(center, 1.f) };

    // Scaled orientation
    const Vector3 right = transform.Right() * extents.x;
    const Vector3 up = transform.Up() * extents.y;
    const Vector3 forward = transform.Forward() * extents.z;

    const float newIi = math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, forward));

    const float newIj = math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, forward));

    const float newIk = math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, forward));

    AABB globalAABB(globalCenter, newIi, newIj, newIk);
    globalAABB.Expand(transform.LocalScale());

    return (globalAABB.isOnOrForwardPlane(camFrustum.leftFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.rightFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.topFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.bottomFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.nearFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.farFace));
};

bool AABB::isOnFrustum(Frustum& camFrustum){
    return (isOnOrForwardPlane(camFrustum.leftFace) &&
        isOnOrForwardPlane(camFrustum.rightFace) &&
        isOnOrForwardPlane(camFrustum.topFace) &&
        isOnOrForwardPlane(camFrustum.bottomFace) &&
        isOnOrForwardPlane(camFrustum.nearFace) &&
        isOnOrForwardPlane(camFrustum.farFace));
}

bool AABB::isOnAABB(AABB& other){
    Vector3 aMin = GetMin();
    Vector3 aMax = GetMax();
    Vector3 bMin = other.GetMin();
    Vector3 bMax = other.GetMax();

    return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
        (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
        (aMin.z <= bMax.z && aMax.z >= bMin.z);
}

}