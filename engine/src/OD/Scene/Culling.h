#pragma once

#include "OD/Core/Math.h"
#include "OD/Renderer/Model.h"
#include "Scene.h"
#include <array>

namespace OD{

struct Plane{
    Vector3 normal = Vector3Up;
    float distance = 0;

    Plane() = default;

	Plane(const Vector3& p1, const Vector3& norm): 
        normal(math::normalize(norm)),
		distance(math::dot(normal, p1)){}

	inline float getSignedDistanceToPlane(const Vector3& point) const{
		return math::dot(normal, point) - distance;
	}
};

struct Frustum{
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

Frustum CreateFrustumFromCamera(Transform& cam, float aspect, float fovY, float zNear, float zFar);
Frustum CreateFrustumFromCamera(TransformComponent& cam, float aspect, float fovY, float zNear, float zFar);

struct BoundingVolume{
    virtual bool isOnFrustum(Frustum& camFrustum, TransformComponent& transform) const = 0;
	virtual bool isOnOrForwardPlane(Plane& plane) const = 0;

	bool isOnFrustum(Frustum& camFrustum) const;
};

struct Sphere: public BoundingVolume{
    Vector3 center{ 0.f, 0.f, 0.f };
	float radius{ 0.f };

	Sphere(){}

	Sphere(Vector3& inCenter, float inRadius): 
        BoundingVolume{}, center{ inCenter }, radius{ inRadius }{}

	bool isOnOrForwardPlane(Plane& plane) const;
	bool isOnFrustum(Frustum& camFrustum, TransformComponent& transform) const;
};

struct SquareAABB: public BoundingVolume{
	Vector3 center{ 0.f, 0.f, 0.f };
	float extent{ 0.f };

	SquareAABB(const Vector3& inCenter, float inExtent): 
		BoundingVolume{}, center{ inCenter }, extent{ inExtent }{}

	bool isOnOrForwardPlane(Plane& plane) const;
	bool isOnFrustum(Frustum& camFrustum, TransformComponent& transform) const;
};

struct AABB: public BoundingVolume{
	Vector3 center{ 0.f, 0.f, 0.f };
	Vector3 extents{ 0.f, 0.f, 0.f };

	AABB(){}

	AABB(const Vector3& min, const Vector3& max): 
		BoundingVolume{}, center{ (max + min) * 0.5f }, extents{ max.x - center.x, max.y - center.y, max.z - center.z }{}

	AABB(const Vector3& inCenter, float iI, float iJ, float iK): 
		BoundingVolume{}, center{ inCenter }, extents{ iI, iJ, iK }{}

	inline void Expand(Vector3 amount){ 
		extents.x *= amount.x; 
		extents.y *= amount.y; 
		extents.z *= amount.z; 
	}

	inline AABB Scaled(Vector3 s){
		AABB result = *this;
		result.Expand(s);
		return result; 
	}

	std::array<Vector3, 8> getVertice() const;

	//see https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
	bool isOnOrForwardPlane(Plane& plane) const;

	bool isOnFrustum(Frustum& camFrustum, TransformComponent& transform) const;
};

AABB generateAABB(const Model& model);
Sphere generateSphereBV(const Model& model);

}