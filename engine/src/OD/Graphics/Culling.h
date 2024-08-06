#pragma once

#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/Transform.h"
#include <array>

namespace OD{

struct OD_API Plane{
    Vector3 normal = Vector3Up;
    float distance = 0;

    Plane() = default;

	Plane(const Vector3& p1, const Vector3& norm): 
        normal(math::normalize(norm)),
		distance(math::dot(normal, p1)){}

	Plane(const Vector4& abcd): 
		normal(abcd.x, abcd.y, abcd.z), distance(abcd.w){}

	inline float getSignedDistanceToPlane(const Vector3& point) const{
		return math::dot(normal, point) + distance;
	}

	inline void normalize(){
		float mag = glm::length(normal);
		normal /= mag;
		distance /= mag;
	}
};

struct OD_API Frustum{
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

/*Frustum CreateFrustumFromCamera(Transform& cam, float aspect, float fovY, float zNear, float zFar);
Frustum CreateFrustumFromOthor(Transform& cam, float orthographicSize, float aspect, float zNear, float zFar);
Frustum CreateFrustumFromMatrix(const Matrix4& viewMatrix, const Matrix4& projectionMatrix);*/
Frustum CreateFrustumFromMatrix2(const Matrix4& mat, bool normalizePlanes = true);

struct OD_API BoundingVolume{
    virtual bool isOnFrustum(Frustum& camFrustum, Transform& transform) const = 0;
	virtual bool isOnOrForwardPlane(Plane& plane) const = 0;

	bool isOnFrustum(Frustum& camFrustum) const;
};

struct OD_API Sphere: public BoundingVolume{
    Vector3 center{ 0.f, 0.f, 0.f };
	float radius{ 0.f };

	Sphere(){}

	Sphere(Vector3& inCenter, float inRadius): 
        BoundingVolume{}, center{ inCenter }, radius{ inRadius }{}

	bool isOnOrForwardPlane(Plane& plane) const override;
	bool isOnFrustum(Frustum& camFrustum, Transform& transform) const override;
};

struct OD_API SquareAABB: public BoundingVolume{
	Vector3 center{ 0.f, 0.f, 0.f };
	float extent{ 0.f };

	SquareAABB(const Vector3& inCenter, float inExtent): 
		BoundingVolume{}, center{ inCenter }, extent{ inExtent }{}

	bool isOnOrForwardPlane(Plane& plane) const override;
	bool isOnFrustum(Frustum& camFrustum, Transform& transform) const override;
};

struct OD_API AABB: public BoundingVolume{
	Vector3 center{ 0.f, 0.f, 0.f };
	Vector3 extents{ 0.f, 0.f, 0.f };

	AABB(){}

	AABB(const Vector3& min, const Vector3& max): 
		BoundingVolume{}, center{ (max + min) * 0.5f }, extents{ max.x - center.x, max.y - center.y, max.z - center.z }{}

	AABB(const Vector3& inCenter, float iI, float iJ, float iK): 
		BoundingVolume{}, center{ inCenter }, extents{ iI, iJ, iK }{}

	inline Vector3 GetMin(){
		Vector3 p1 = center + extents;
		Vector3 p2 = center - extents;
		return Vector3(
			fminf(p1.x, p2.x),
			fminf(p1.y, p2.y),
			fminf(p1.z, p2.z)
		);
	}

	inline Vector3 GetMax(){
		Vector3 p1 = center + extents;
		Vector3 p2 = center - extents;
		return Vector3(
			fmaxf(p1.x, p2.x),
			fmaxf(p1.y, p2.y),
			fmaxf(p1.z, p2.z)
		);
	}

	void Expand(Vector3 amount);
	AABB Scaled(Vector3 s);

	std::array<Vector3, 8> getVertice() const;

	//see https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
	bool isOnOrForwardPlane(Plane& plane) const override;
	bool isOnFrustum(Frustum& camFrustum, Transform& transform) const override;
	bool isOnAABB(AABB& other);

	//bool isOnFrustum(Frustum& camFrustum);
};

}