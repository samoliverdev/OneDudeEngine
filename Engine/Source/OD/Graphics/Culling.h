#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/Transform.h"
#include <array>

namespace OD{

struct OD_API alignas(16) Plane{
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

struct OD_API alignas(16) Frustum{
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

struct OD_API alignas(16) AABB: public BoundingVolume{
	Vector3 center = Vector3(0);
	Vector3 extents = Vector3(0);

	AABB(){}

	AABB(const Vector3& min, const Vector3& max): 
		BoundingVolume{}, center{ (max + min) * 0.5f }, extents{ max.x - center.x, max.y - center.y, max.z - center.z }{}

	AABB(const Vector3& inCenter, float iI, float iJ, float iK): 
		BoundingVolume{}, center{ inCenter }, extents{ iI, iJ, iK }{}

	// Expand to include another AABB
    inline void Encapsulate(const AABB& other) {
        Vector3 min = GetMin();
        Vector3 max = GetMax();
        Vector3 otherMin = other.GetMin();
        Vector3 otherMax = other.GetMax();

        min.x = std::min(min.x, otherMin.x);
        min.y = std::min(min.y, otherMin.y);
        min.z = std::min(min.z, otherMin.z);

        max.x = std::max(max.x, otherMax.x);
        max.y = std::max(max.y, otherMax.y);
        max.z = std::max(max.z, otherMax.z);

        center = (max + min) * 0.5f;
        extents = max - center;
    }

    // Expand to include a single point
    inline void Encapsulate(const Vector3& point) {
        Vector3 min = GetMin();
        Vector3 max = GetMax();

        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);

        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);

        center = (max + min) * 0.5f;
        extents = max - center;
    }

	inline Vector3 GetMin() const {
		return center - extents;

		/*Vector3 p1 = center + extents;
		Vector3 p2 = center - extents;
		return Vector3(
			fminf(p1.x, p2.x),
			fminf(p1.y, p2.y),
			fminf(p1.z, p2.z)
		);*/
	}

	inline Vector3 GetMax() const {
		return center + extents;

		/*Vector3 p1 = center + extents;
		Vector3 p2 = center - extents;
		return Vector3(
			fmaxf(p1.x, p2.x),
			fmaxf(p1.y, p2.y),
			fmaxf(p1.z, p2.z)
		);*/
	}

	void Expand(Vector3 amount);
	void Expand2(Vector3 amount);
	AABB Scaled(Vector3 s);

	std::array<Vector3, 8> getVertice() const;

	//see https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
	bool isOnOrForwardPlane(Plane& plane) const override;
	bool isOnFrustum(Frustum& camFrustum, Transform& transform) const override;
	bool isOnFrustum(Frustum& camFrustum);
	bool isOnAABB(AABB& other);

	//bool isOnFrustum(Frustum& camFrustum);

	template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, center);
        ArchiveDumpNVP(ar, extents);
    }
};

}