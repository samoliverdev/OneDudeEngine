#pragma once

#define GLM_FORCE_QUAT_DATA_XYZW

#include "OD/Core/Application.h"

#include <math.h>
#include <float.h>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include <string>

#include "glm/geometric.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtx/orthonormalize.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/matrix_query.hpp"
#include "glm/gtx/vector_angle.hpp"



namespace OD {

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Quaternion = glm::quat;
using Matrix4 = glm::mat4;

/*typedef glm::vec2 Vector2;
typedef glm::vec3 Vector3;
typedef glm::vec4 Vector4;
typedef glm::quat Quaternion;
typedef glm::mat4 Matrix4;*/

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
    inline float Deg2Rad(float a){ return math::radians(a); }
    inline float Rad2Deg(float a){ return math::degrees(a); }

    inline Vector3 Deg2Rad(Vector3 a){ return Vector3(math::radians(a.x), math::radians(a.y), math::radians(a.z)); }
    inline Vector3 Rad2Deg(Vector3 a){ return Vector3(math::degrees(a.x), math::degrees(a.y), math::degrees(a.z)); }

    inline float* Raw(Matrix4& m){ return &(m[0].x); }

    inline static Matrix4 TRS(Vector3 pos, Quaternion q, Vector3 s){
        Matrix4 translate = math::translate(pos);
        Matrix4 rotate = math::mat4_cast(q);
        Matrix4 scale = math::scale(s);
        return translate * rotate * scale;
    }
}

/*
using vec2 = glm::vec2;
namespace math = glm;

namespace mathf{

inline vec2 normalized(vec2 v){
    vec2 result = v;
    return math::normalize(result);
}

inline float Test(){ return math::radians<float>(20); }

}

struct Vector2: glm::vec2{
    Vector2(glm::vec2 v);
    Vector2(float x = 0.0f, float y = 0.0f);

    static const Vector2 down;
    static const Vector2 left;
    static const Vector2 one;
    static const Vector2 right;
    static const Vector2 up;
    static const Vector2 zero;

    float magnitude() const;
    Vector2 normalized() const;
    float sqrMagnitude() const;

    void Normalize();
    void Set(float new_x, float new_y);

    static float Angle(Vector2 from, Vector2 to);
    static Vector2 ClampMagnitude(Vector2 vector, float maxLength);
    static float Distance(Vector2 a, Vector2 b);
    static float Dot(Vector2 lhs, Vector2 rhs);
    static Vector2 Lerp(Vector2 a, Vector2 b, float t);
    static Vector2 LerpUnclamped(Vector2 a, Vector2 b, float t);
    static Vector2 Max(Vector2 lhs, Vector2 rhs);
    static Vector2 Min(Vector2 lhs, Vector2 rhs);
    static Vector2 MoveTowards(Vector2 current, Vector2 target, float maxDistanceDelta);
    static Vector2 Reflect(Vector2 inDirection, Vector2 inNormal);
    static Vector2 Scale(Vector2 a, Vector2 b);
};

struct Vector3: glm::vec3{
    Vector3(glm::vec3 v): glm::vec3(v){}
    Vector3(float x = 0.0f, float y = 0.0f, float z = 0.0f): glm::vec3(x, y, z){}

    static const Vector3 back;
    static const Vector3 down;
    static const Vector3 forward;
    static const Vector3 left;
    static const Vector3 one;
    static const Vector3 right;
    static const Vector3 up;
    static const Vector3 zero;

    float magnitude() const;
    Vector3 normalized() const;
    float sqrMagnitude() const;

    void Normalize();
    void Set(float new_x, float new_y, float new_z);

    static float Angle(Vector3 from, Vector3 to);
    static Vector3 ClampMagnitude(Vector3 vector, float maxLength);
    static float Distance(Vector3 a, Vector3 b);
    static float Dot(Vector3 lhs, Vector3 rhs);
    static Vector3 Cross(Vector3 lhs, Vector3 rhs);
    static Vector3 Lerp(Vector3 a, Vector3 b, float t);
    static Vector3 LerpUnclamped(Vector3 a, Vector3 b, float t);
    static Vector3 Max(Vector3 lhs, Vector3 rhs);
    static Vector3 Min(Vector3 lhs, Vector3 rhs);
    static Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta);
    static void OrthoNormalize(Vector3 *normal, Vector3 *tangent);
    static Vector3 Project(Vector3 vector, Vector3 onNormal);
    static Vector3 ProjectOnPlane(Vector3 vector, Vector3 planeNormal);
    static Vector3 Reflect(Vector3 inDirection, Vector3 inNormal);
    static Vector3 Scale(Vector3 a, Vector3 b);
    static Vector3 Slerp(Vector3 a, Vector3 b, float t);
    static Vector3 SlerpUnclamped(Vector3 a, Vector3 b, float t);   
};

struct Vector4: glm::vec4{
    Vector4(glm::vec4 v);
    Vector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f);
    Vector4(Vector3 v);

    static const Vector4 one;
    static const Vector4 zero;

    float magnitude() const;
    Vector4 normalized() const;
    float sqrMagnitude() const;

    void Normalize();
    void Set(float new_x, float new_y, float new_z, float new_w);

    static float Distance(Vector4 a, Vector4 b);
    static float Dot(Vector4 lhs, Vector4 rhs);
    static Vector4 Lerp(Vector4 a, Vector4 b, float t);
    static Vector4 LerpUnclamped(Vector4 a, Vector4 b, float t);
    static Vector4 Max(Vector4 lhs, Vector4 rhs);
    static Vector4 Min(Vector4 lhs, Vector4 rhs);
    static Vector4 MoveTowards(Vector4 current, Vector4 target, float maxDistanceDelta);
    static Vector4 Project(Vector4 a, Vector4 b);
    static Vector4 Scale(Vector4 a, Vector4 b);
};

struct Quaternion: glm::quat{
    Quaternion(glm::quat q);
    Quaternion(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f);

    static const Quaternion identity;

    Vector3 eulerAngles() const;
    void eulerAngles(Vector3 euler);
    Quaternion normalized();

    void Set(float new_x, float new_y, float new_z, float new_w);
    void SetFromToRotation(Vector3 fromDirection, Vector3 toDirection);
    void SetLookRotation(Vector3 view, Vector3 up = Vector3::up);
    void ToAngleAxis(float *angle, Vector3 *axis);

    static float Angle(Quaternion a, Quaternion b);
    static Quaternion AngleAxis(float angle, Vector3 axis);
    static float Dot(Quaternion a, Quaternion b);
    static Quaternion Euler(Vector3 euler);
    static Quaternion FromToRotation(Vector3 fromDirection, Vector3 toDirection);
    static Quaternion Inverse(Quaternion rotation);
    static Quaternion Lerp(Quaternion a, Quaternion b, float t);
    static Quaternion LerpUnclamped(Quaternion a, Quaternion b, float t);
    static Quaternion LookRotation(Vector3 forward, Vector3 upwards = Vector3::up);
    static Quaternion RotateTowards(Quaternion from, Quaternion to, float maxDegreesDelta);
    static Quaternion Slerp(Quaternion a, Quaternion b, float t);
    static Quaternion SlerpUnclamped(Quaternion a, Quaternion b, float t);
};

struct Matrix4: glm::mat4{
    Matrix4(glm::mat4 m);
    Matrix4(float f = 1.0f);

    inline float* raw(){ return &((*this)[0].x); }

    static const Matrix4 identity;
    static const Matrix4 zero;

    float determinant() const;
    Matrix4 inverse() const;
    Matrix4 transpose() const;

    Vector4 GetColumn(int i) const;
    Vector4 GetRow(int i) const;
    Vector3 MultiplyPoint(Vector3 v) const;
    Vector3 MultiplyPoint3x4(Vector3 v) const;
    Vector3 MultiplyVector(Vector3 v) const;
    void SetColumn(int i, Vector4 v);
    void SetRow(int i, Vector4 v);
    void SetTRS(Vector3 pos, Quaternion q, Vector3 s);

    //static Matrix4 Inverse(Matrix4 m){ return glm::inverse(m); }
    static Matrix4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar);
    static Matrix4 Perspective(float fov, float aspect, float zNear, float zFar);
    static Matrix4 TRS(Vector3 pos, Quaternion q, Vector3 s);
    static Matrix4 LookAt(Vector3 eye, Vector3 center, Vector3 up);
    static Matrix4 Rotate(float euler, Vector3 axis);
    static Matrix4 Translate(Vector3 position);
    static Matrix4 Scale(Vector3 scale);

    static Matrix4 ToMatrix(Quaternion quaternion);
};

struct Mathf{
    //static const float Deg2Rad;
    static const float Epsilon;
    static const float Infinity;
    static const float NegativeInfinity;
    static const float PI;
    //static const float Rad2Deg;

    static inline float Deg2Rad(float a){ return glm::radians(a); }
    static inline float Rad2Deg(float a){ return glm::degrees(a); }

    static inline float Abs(float f){ return glm::abs(f); }
    static float Acos(float f){ return glm::acos(f); }
    static inline bool Approximately(float a, float b){ return glm::epsilonEqual(a, b, Epsilon); }
    static inline float Asin(float f){ return glm::asin(f); }
    static inline float Atan(float f){ return glm::atan(f); }
    static inline float Atan2(float y, float x){ return glm::atan(y, x); }
    static inline float Ceil(float f) { return glm::ceil(f); }
    static inline int CeilToInt(float f){ return int(glm::ceil(f)); }
    static inline float Clamp(float value, float min, float max){ return glm::clamp(value, min, max); }
    static inline float Clamp01(float value){ return glm::clamp(value, 0.0f, 1.0f); }
    static inline float Cos(float f){ return glm::cos(f); }

    static inline float DeltaAngle(float current, float target){
        float num = Mathf::Repeat(target - current, 360.0f);
        if (num > 180.0f)
        {
            num -= 360.0f;
        }
        return num;
    }

    static inline float Exp(float power){ return glm::exp(power); }
    static inline float Floor(float f){ return glm::floor(f); }
    static inline int FloorToInt(float f){ return int(glm::floor(f)); }

    static inline float InverseLerp(float a, float b, float value){
        if (a < b){
            if (value < a){
                return 0.0f;
            }
            if (value > b){
                return 1.0f;
            }
            value -= a;
            value /= b - a;
            return value;
        } else {
            if (a <= b){
                return 0.0f;
            }
            if (value < b){
                return 1.0f;
            }
            if (value > a){
                return 0.0f;
            }
            return 1.0f - (value - b) / (a - b);
        }
    }

    static inline bool IsPowerOfTwo(int value){
        return (value & (value-1)) == 0;
    }

    static inline float Lerp(float a, float b, float t){
        t = glm::clamp(t, 0.0f, 1.0f);
        return glm::mix(a, b, t);
    }

    static inline float LerpAngle(float a, float b, float t){
        float num = Mathf::Repeat(b - a, 360.0f);
        if (num > 180.0f){
            num -= 360.0f;
        }
        return a + num * Mathf::Clamp01(t);
    }

    static inline float LerpUnclamped(float a, float b, float t){ return glm::mix(a, b, t); }
    static inline float Log10(float f){ return glm::log(f); }
    static inline float Max(float a, float b){ return glm::max(a, b); }
    static float Max(const std::vector<float>& values);
    static inline float Min(float a, float b){ return glm::min(a, b); }
    static float Min(const std::vector<float>& values);

    static inline float MoveTowards(float current, float target, float maxDelta){
        float delta = target - current;
        float result = (maxDelta >= delta) ? target : current + maxDelta;
        return result;
    }

    static inline float MoveTowardsAngle(float current, float target, float maxDelta){
        target = current + Mathf::DeltaAngle(current, target);
        return Mathf::MoveTowards(current, target, maxDelta);
    }

    static inline int NextPowerOfTwo(int value){
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        return value+1;
    }

    static float PerlinNoise(float x, float y);

    static inline float PingPong(float t, float length){
        float result = t;
        if (t >= 0.0f && t < length){
            length *= 2.0f;
            t = glm::modf(t, length);
            result = length - t;
        }
        return result;
    }

    static inline float Pow(float f, float p){ return glm::pow(f, p); }
    static inline float Repeat(float t, float length){ return glm::modf(t, length); }
    static inline float Round(float f){ return glm::round(f); }
    static inline int RoundToInt(float f){ return int(glm::round(f)); }
    static inline float Sign(float f){ return glm::sign(f); }
    static inline float Sin(float f){ return glm::sin(f); }

    static float SmoothDamp(float current,
                            float target,
                            float *currentVelocity,
                            float smoothTime,
                            float maxSpeed = Mathf::Infinity,
                            float deltaTime = Application::deltaTime());

    static float SmoothDampAngle(float current,
                                    float target,
                                    float *currentVelocity,
                                    float smoothTime,
                                    float maxSpeed = Mathf::Infinity,
                                    float deltaTime = Application::deltaTime());

    static inline float SmoothStep(float from, float to, float value){
        float t = Mathf::Clamp01(value);
        t = -2.0f * t * t * t + 3.0f * t * t;
        return to * t + from * (1.0f - t);
    }

    static inline float Sqrt(float f){
        return glm::sqrt(f);
    }

    static inline float Tan(float f){
        return glm::tan(f);
    }
};
*/

}