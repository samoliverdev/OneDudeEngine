#include "Culling.h"

namespace OD{

Frustum CreateFrustumFromCamera(Transform& cam, float aspect, float fovY, float zNear, float zFar){
    Vector3 forward = -cam.forward();
    Vector3 right = -cam.right();
    Vector3 up = -cam.up();
    Vector3 position = cam.localPosition();

    Frustum frustum;
    float halfVSide = zFar * tanf(fovY * .5f);
    float halfHSide = halfVSide * aspect;
    Vector3 frontMultFar = zFar * forward;

    frustum.nearFace = { position + zNear * forward, forward };
    frustum.farFace = { position + frontMultFar, -forward };
    frustum.rightFace = { position, math::cross(frontMultFar - right * halfHSide, up) };
    frustum.leftFace = { position, math::cross(up,frontMultFar + right * halfHSide) };
    frustum.topFace = { position, math::cross(right, frontMultFar - up * halfVSide) };
    frustum.bottomFace = { position, math::cross(frontMultFar + up * halfVSide, right) };

    return frustum;
}

Frustum CreateFrustumFromCamera(TransformComponent& cam, float aspect, float fovY, float zNear, float zFar){
    Vector3 forward = -cam.forward();
    Vector3 right = -cam.right();
    Vector3 up = -cam.up();
    Vector3 position = cam.position();

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

bool Sphere::isOnFrustum(Frustum& camFrustum, TransformComponent& transform) const{
    //Get global scale thanks to our transform
    Vector3 globalScale = transform.localScale();

    //Get our global center with process it with the global model matrix of our transform
    Vector3 globalCenter{ transform.globalModelMatrix() * glm::vec4(center, 1.f) };

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
    return -r <= plane.getSignedDistanceToPlane(center);
}

bool SquareAABB::isOnFrustum(Frustum& camFrustum, TransformComponent& transform) const{
    //Get global scale thanks to our transform
    const Vector3 globalCenter{ transform.globalModelMatrix() * glm::vec4(center, 1.f) };

    // Scaled orientation
    const Vector3 right = transform.right() * extent;
    const Vector3 up = transform.up() * extent;
    const Vector3 forward = transform.forward() * extent;

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

bool AABB::isOnFrustum(Frustum& camFrustum, TransformComponent& transform) const{
    //Get global scale thanks to our transform
    const Vector3 globalCenter{ transform.globalModelMatrix() * Vector4(center, 1.f) };

    // Scaled orientation
    const Vector3 right = transform.right() * extents.x;
    const Vector3 up = transform.up() * extents.y;
    const Vector3 forward = transform.forward() * extents.z;

    const float newIi = math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 1.f, 0.f, 0.f }, forward));

    const float newIj = math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 1.f, 0.f }, forward));

    const float newIk = math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, right)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, up)) +
        math::abs(math::dot(Vector3{ 0.f, 0.f, 1.f }, forward));

    const AABB globalAABB(globalCenter, newIi, newIj, newIk);

    return (globalAABB.isOnOrForwardPlane(camFrustum.leftFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.rightFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.topFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.bottomFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.nearFace) &&
        globalAABB.isOnOrForwardPlane(camFrustum.farFace));
};

AABB generateAABB(const Model& model){
	Vector3 minAABB = Vector3(std::numeric_limits<float>::max());
	Vector3 maxAABB = Vector3(std::numeric_limits<float>::min());
	for(auto&& mesh : model.meshs){
		for(auto& vertex : mesh->vertices){
        
			minAABB.x = std::min(minAABB.x, vertex.x);
			minAABB.y = std::min(minAABB.y, vertex.y);
			minAABB.z = std::min(minAABB.z, vertex.z);

			maxAABB.x = std::max(maxAABB.x, vertex.x);
			maxAABB.y = std::max(maxAABB.y, vertex.y);
			maxAABB.z = std::max(maxAABB.z, vertex.z);
		}
	}
	return AABB(minAABB, maxAABB);
}

Sphere generateSphereBV(const Model& model){
	Vector3 minAABB = Vector3(std::numeric_limits<float>::max());
	Vector3 maxAABB = Vector3(std::numeric_limits<float>::min());
	for(auto&& mesh : model.meshs){
		for(auto& vertex : mesh->vertices){
			minAABB.x = std::min(minAABB.x, vertex.x);
			minAABB.y = std::min(minAABB.y, vertex.y);
			minAABB.z = std::min(minAABB.z, vertex.z);

			maxAABB.x = std::max(maxAABB.x, vertex.x);
			maxAABB.y = std::max(maxAABB.y, vertex.y);
			maxAABB.z = std::max(maxAABB.z, vertex.z);
		}
	}

	Vector3 c = (maxAABB + minAABB) * 0.5f;
	float r = (minAABB - maxAABB).length();
	return Sphere(c, r);
}

}