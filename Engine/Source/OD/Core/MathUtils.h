#pragma once
#include "Math.h"

namespace glm{

template<typename T> constexpr T sqr(T v) { return v * v; }

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

namespace OD{

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