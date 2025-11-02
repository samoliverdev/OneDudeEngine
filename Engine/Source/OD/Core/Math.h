#pragma once
//#include <math.h>
//#include <float.h>
#include <cmath>
#include <cfloat>

//#define GLM_FORCE_PURE
//#define GLM_FORCE_AVX2
//#define GLM_FORCE_SSE2
#define GLM_FORCE_CXX11
//#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_SSE2
//#define GLM_FORCE_ALIGNED
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#define GLM_FORCE_QUAT_DATA_XYZW
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtc/type_aligned.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <glm/simd/matrix.h>
#include <glm/simd/common.h>


#include <memory>

/*template<typename T, size_t Alignment>
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
};*/

#include <cstdlib>
#include <new>
#include <type_traits>
#ifdef _MSC_VER
#  include <malloc.h> // _aligned_malloc/_aligned_free
#endif

template<typename T, std::size_t Alignment>
struct AlignedAllocator {
    static_assert(Alignment >= alignof(void*), "Alignment must be >= alignof(void*)");
    static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be a power of two");

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    AlignedAllocator() noexcept = default;

    // templated converting constructor required by the standard library
    template<typename U>
    constexpr AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

    // allocate n elements of T, rounding size up to a multiple of Alignment for std::aligned_alloc
    T* allocate(size_type n) {
        if (n == 0) return nullptr;

        size_type size = n * sizeof(T);

    #ifdef _MSC_VER
        void* p = _aligned_malloc(size, Alignment);
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    #else
        // std::aligned_alloc requires size % alignment == 0
        size_type aligned_size = ((size + Alignment - 1) / Alignment) * Alignment;
        void* p = std::aligned_alloc(Alignment, aligned_size);
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    #endif
    }

    void deallocate(T* p, size_type) noexcept {
        if (!p) return;
    #ifdef _MSC_VER
        _aligned_free(p);
    #else
        std::free(p);
    #endif
    }

    // rebind for older allocator usage (kept for compatibility)
    template<typename U>
    struct rebind { using other = AlignedAllocator<U, Alignment>; };

    // equality — many containers rely on this
    template<typename U>
    bool operator==(const AlignedAllocator<U, Alignment>&) const noexcept { return true; }

    template<typename U>
    bool operator!=(const AlignedAllocator<U, Alignment>&) const noexcept { return false; }
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

inline mat4 simdMul(const glm::mat4& a, const glm::mat4& b){
    glm::mat4 r;
    glm_mat4_mul(&a[0].data, &b[0].data, &r[0].data);
    return r;
}

inline vec4 simdMul(const glm::mat4& m, const glm::vec4& v){
    vec4 r;
    r.data = glm_mat4_mul_vec4(&m[0].data, v.data);
    return r;
}

inline void extractPosRot(const glm::mat4& m, glm::vec3& outPos, glm::quat& outRot){
    // Extract translation directly
    outPos = glm::vec3(m[3]);

    // Extract rotation (ignore scale)
    glm::mat3 rotMat = glm::mat3(
        glm::vec3(m[0]),  // X axis
        glm::vec3(m[1]),  // Y axis
        glm::vec3(m[2])   // Z axis
    );

    // Normalize each axis in case there’s a small scale
    rotMat[0] = glm::normalize(rotMat[0]);
    rotMat[1] = glm::normalize(rotMat[1]);
    rotMat[2] = glm::normalize(rotMat[2]);

    outRot = glm::quat_cast(rotMat);
}

}

namespace OD {

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Quaternion = glm::quat;
using Matrix4 = glm::aligned_mat4;// glm::mat4;
using IVector2 = glm::ivec2;
using IVector3 = glm::ivec3;
using IVector4 = glm::ivec4;

struct Matrix4x3{
    Vector4 v0;
    Vector4 v1;
    Vector4 v2;
};

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
        /*Matrix4 out(1.0f);
        glm_mat4_mul(
            &math::translate(Matrix4Identity, pos)[0].data,
            &math::mat4_cast(q) [0].data,
            &out[0].data
        );
        glm_mat4_mul(
            &out[0].data,
            &math::scale(Matrix4Identity, s)[0].data,
            &out[0].data
        );
        return out;*/

        glm::mat4 m = glm::mat4_cast(q);
        m[0] *= s.x;
        m[1] *= s.y;
        m[2] *= s.z;
        m[3] = glm::vec4(pos, 1.0f);
        return m;

        //return math::translate(Matrix4Identity, pos) * math::mat4_cast(q) * math::scale(Matrix4Identity, s);
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