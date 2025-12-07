#pragma once
#include "OD/Core/Transform.h"
#include <array>

namespace OD{

/*struct OD_API alignas(16) Plane{
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
};*/

struct OD_API alignas(16) Plane{
    glm::vec4 n = glm::vec4(0, 1, 0, 0); // SIMD layout: (nx, ny, nz, distance)
    glm::vec3 absN = glm::vec3(0, 1, 0); // Precomputed |normal|

    Plane() = default;

    // Construct from point + normal
    Plane(const glm::vec3& p1, const glm::vec3& norm){
        glm::vec3 nn = glm::normalize(norm);
        n = glm::vec4(nn, glm::dot(nn, p1));
        absN = glm::abs(nn);
    }

    // Construct from (a,b,c,d)
    Plane(const glm::vec4& abcd){
        n = abcd;
        absN = glm::abs(glm::vec3(abcd));
    }

    // Signed distance: dot(normal, point) + distance
    inline float getSignedDistanceToPlane(const glm::vec3& point) const {
        return glm::dot(glm::vec3(n), point) + n.w;
    }

    // Normalize plane (normal and distance)
    inline void normalize(){
        float mag = glm::length(glm::vec3(n));
        n /= mag;     // divides xyz and w
        absN = glm::abs(glm::vec3(n));
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

	AABB(): center(0.0f), extents(0.0f) {}

	AABB(const Vector3& min, const Vector3& max): 
		BoundingVolume{}, center{ (max + min) * 0.5f }, extents{ max.x - center.x, max.y - center.y, max.z - center.z }{}

	AABB(const Vector3& inCenter, float iI, float iJ, float iK): 
		BoundingVolume{}, center{ inCenter }, extents{ iI, iJ, iK }{}

	// Expand to include another AABB
    inline void Encapsulate(const AABB& other) {
		if (extents == Vector3(0)) { // treat as empty box
			center = other.center;
			extents = other.extents;
			return;
		}
		
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
		if (extents == Vector3(0)) {
			center = point;
			return; // first point initializes box
		}
		
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

struct OD_API OBB {
    glm::vec3 center;
    glm::vec3 halfSize;

    //union{
        glm::vec3 axes[3];
        //glm::mat3 orientation;
    //};

    OBB() = default;
    OBB(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& size);

	glm::quat GetRotation() const;

    bool ContainsPoint(const glm::vec3& p) const;

	bool IntersectSegment(
        const glm::vec3& p0,
        const glm::vec3& p1
    ) const;

    bool IntersectSegment(
        const glm::vec3& p0,
        const glm::vec3& p1,
        float& outTmin,
        glm::vec3& outPoint
    ) const;
};


}