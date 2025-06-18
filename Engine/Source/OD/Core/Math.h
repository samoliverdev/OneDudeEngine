#pragma once
//#include <math.h>
//#include <float.h>
#include <cmath>
#include <cfloat>

//#define GLM_FORCE_PURE
//#define GLM_FORCE_AVX2
#define GLM_FORCE_SSE2
#define GLM_FORCE_ALIGNED
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#define GLM_FORCE_QUAT_DATA_XYZW
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/projection.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <memory>

template<typename T, size_t Alignment>
struct AlignedAllocator {
    using value_type = T;
    T* allocate(size_t n) {
        #ifdef _MSC_VER
        return static_cast<T*>(_aligned_malloc(n * sizeof(T), Alignment));
        #else
        return static_cast<T*>(std::aligned_alloc(Alignment, n * sizeof(T)));
        #endif
    }
    void deallocate(T* p, size_t) noexcept {
        #ifdef _MSC_VER
        _aligned_free(p);
        #else
        std::free(p);
        #endif
    }
    template<typename U> struct rebind { using other = AlignedAllocator<U, Alignment>; };
};

#include <vector>

// Template alias for aligned vectors
template<typename T>
using AlignedVector = std::vector<T, AlignedAllocator<T, 16>>;

namespace glm{

inline vec3 projectOnPlane(vec3 vector, vec3 planeNormal){
    return proj(vector, planeNormal);
}

inline vec3 normalizeSafe(vec3 v){
    if(v == vec3(0, 0, 0)) return v;
    return normalize(v);
}

inline quat fromTo(const vec3& from, const vec3& to){
    vec3 f = normalize(from);
    vec3 t = normalize(to);
    if(f == t){
        return quat();
    }else if(f == t * -1.0f){
        vec3 ortho = vec3(1, 0, 0);
        if(fabsf(f.y) <fabsf(f.x)){
            ortho = vec3(0, 1, 0);
        }
        if (fabsf(f.z)<fabs(f.y) && fabs(f.z)<fabsf(f.x)){
        ortho = vec3(0, 0, 1);
    }
    vec3 axis = normalize(cross(f, ortho));
        return quat(axis.x, axis.y, axis.z, 0);
    }

    vec3 half = normalize(f + t);
    vec3 axis = cross(f, half);
    return quat(axis.x, axis.y, axis.z, dot(f, half));
}

}

namespace OD {

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Quaternion = glm::quat;
using Matrix4 = glm::mat4;
using IVector2 = glm::ivec2;
using IVector3 = glm::ivec3;
using IVector4 = glm::ivec4;

namespace math = glm;

const Vector2 Vector2Left(-1.0f, 0.0f);
const Vector2 Vector2Right(1.0f, 0.0f);
const Vector2 Vector2Up(0.0f, 1.0f);
const Vector2 Vector2Down(0.0f, -1.0f);
const Vector2 Vector2One(1.0f, 1.0f);
const Vector2 Vector2Zero(0.0f, 0.0f);

const Vector3 Vector3Forward(0.0f, 0.0f, 1.0f);
const Vector3 Vector3Back(0.0f, 0.0f, -1.0f);
const Vector3 Vector3Left(-1.0f, 0.0f, 0.0f);
const Vector3 Vector3Right(1.0f, 0.0f, 0.0f);
const Vector3 Vector3Up(0.0f, 1.0f, 0.0f);
const Vector3 Vector3Down(0.0f, -1.0f, 0.0f);
const Vector3 Vector3One(1.0f, 1.0f, 1.0f);
const Vector3 Vector3Zero(0.0f, 0.0f, 0.0f);

const Vector4 Vector4One(0.0f, 0.0f, 0.0f, 0.0f);
const Vector4 Vector4Zero(0.0f, 0.0f, 0.0f, 0.0f);

const Quaternion QuaternionIdentity = Quaternion(0,0,0,1);

const Matrix4 Matrix4Identity(1.0f);
const Matrix4 Matrix4Zero(0.0f);

namespace Mathf{
    inline Vector4 ToVector4(Vector3 v){ return Vector4(v.x, v.y, v.z, 1);}

    inline float Deg2Rad(float a){ return math::radians(a); }
    inline float Rad2Deg(float a){ return math::degrees(a); }

    inline Vector3 Deg2Rad(Vector3 a){ return Vector3(math::radians(a.x), math::radians(a.y), math::radians(a.z)); }
    inline Vector3 Rad2Deg(Vector3 a){ return Vector3(math::degrees(a.x), math::degrees(a.y), math::degrees(a.z)); }

    inline float* Raw(Matrix4& m){ return &(m[0].x); }

    inline static Matrix4 TRS(Vector3 pos, Quaternion q, Vector3 s){
        return math::translate(Matrix4Identity, pos) * math::mat4_cast(q) * math::scale(Matrix4Identity, s);
    }

    inline static Quaternion mix(const Quaternion& from, const Quaternion& to, float t) {
        return from * (1.0f - t) + to * t;
    }

    inline static Quaternion nlerp(const Quaternion& from, const Quaternion& to, float t) {
        return math::normalize(from + (to - from) * t);
    }

    inline static Vector3 lerp(const Vector3& s, const Vector3& e, float t) {
        return Vector3(
            s.x + (e.x - s.x) * t,
            s.y + (e.y - s.y) * t,
            s.z + (e.z - s.z) * t
        );
    }

    inline static bool IsNan(const Vector3& v){
        if(isnan(v.x)) return true;
        if(isnan(v.y)) return true;
        if(isnan(v.z)) return true;
        return false;
    }
}

}