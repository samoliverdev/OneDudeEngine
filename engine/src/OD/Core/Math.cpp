#include "Math.h"

#include <string>

#include "glm/geometric.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtx/orthonormalize.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/matrix_query.hpp"
#include "glm/gtx/vector_angle.hpp"

namespace OD{

Vector2::Vector2(glm::vec2 v): glm::vec2(v){}
Vector2::Vector2(float x, float y): glm::vec2(x, y){}

const Vector2 Vector2::down(0.0f, -1.0f);
const Vector2 Vector2::left(-1.0f, 0.0f);
const Vector2 Vector2::one(1.0f, 1.0f);
const Vector2 Vector2::right(1.0f, 0.0f);
const Vector2 Vector2::up(0.0f, 1.0f);
const Vector2 Vector2::zero(0.0f, 0.0f);

float Vector2::magnitude() const{
    float result = glm::length(static_cast<glm::vec2>(*this));
    return result;
}

Vector2 Vector2::normalized() const{
    glm::vec2 result = *this;
    glm::normalize(result);
    return static_cast<Vector2>(result);
}

float Vector2::sqrMagnitude() const{
    float result = glm::length2(static_cast<glm::vec2>(*this));
    return result;
}

void Vector2::Normalize(){
    glm::normalize(static_cast<glm::vec2>(*this));
}

void Vector2::Set(float new_x, float new_y){
    x = new_x;
    y = new_y;
}

float Vector2::Angle(Vector2 from, Vector2 to){
    glm::vec2 a = glm::normalize(static_cast<glm::vec2>(from));
    glm::vec2 b = glm::normalize(static_cast<glm::vec2>(to));
    float result = glm::angle(a, b);
    return result;
}

Vector2 Vector2::ClampMagnitude(Vector2 vector, float maxLength){
    float length = glm::length(static_cast<glm::vec2>(vector));
    glm::vec2 result = vector;
    result *= maxLength / length;
    return static_cast<Vector2>(result);
}

float Vector2::Distance(Vector2 a, Vector2 b){
    float result = glm::distance(static_cast<glm::vec2>(a),
                                    static_cast<glm::vec2>(b));
    return result;
}

float Vector2::Dot(Vector2 lhs, Vector2 rhs){
    float result = glm::dot(static_cast<glm::vec2>(lhs),
                            static_cast<glm::vec2>(rhs));
    return result;
}

Vector2 Vector2::Lerp(Vector2 a, Vector2 b, float t){
    t = glm::clamp(t, 0.0f, 1.0f);
    return LerpUnclamped(a, b, t);
}

Vector2 Vector2::LerpUnclamped(Vector2 a, Vector2 b, float t){
    glm::vec2 result = a*(1.0f-t) + b*t;
    return static_cast<Vector2>(result);
}

Vector2 Vector2::Max(Vector2 lhs, Vector2 rhs){
    glm::vec2 result = glm::max(static_cast<glm::vec2>(lhs),
                                static_cast<glm::vec2>(rhs));
    return static_cast<Vector2>(result);
}

Vector2 Vector2::Min(Vector2 lhs, Vector2 rhs){
    glm::vec2 result = glm::min(static_cast<glm::vec2>(lhs),
                                static_cast<glm::vec2>(rhs));
    return static_cast<Vector2>(result);
}

Vector2 Vector2::MoveTowards(Vector2 current, Vector2 target, float maxDistanceDelta){
    glm::vec2 delta = target - current;
    glm::vec2 result = (maxDistanceDelta >= glm::length(delta)) ? target : current + maxDistanceDelta;
    return static_cast<Vector2>(result);
}

Vector2 Vector2::Reflect(Vector2 inDirection, Vector2 inNormal){
    glm::vec2 result = glm::reflect(static_cast<glm::vec2>(inDirection),
                                    static_cast<glm::vec2>(inNormal));
    return static_cast<Vector2>(result);
}

Vector2 Vector2::Scale(Vector2 a, Vector2 b){
    glm::vec2 result = a * b;
    return static_cast<Vector2>(result);
}

///////////////////////////////////////////////////

const Vector3 Vector3::back(0.0f, 0.0f, -1.0f);
const Vector3 Vector3::down(0.0f, -1.0f, 0.0f);
const Vector3 Vector3::forward(0.0f, 0.0f, 1.0f);
const Vector3 Vector3::left(-1.0f, 0.0f, 0.0f);
const Vector3 Vector3::one(1.0f, 1.0f, 1.0f);
const Vector3 Vector3::right(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::up(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::zero(0.0f, 0.0f, 0.0f);

float Vector3::magnitude() const{
    return glm::length(static_cast<glm::vec3>(*this));
}

Vector3 Vector3::normalized() const{
    glm::vec3 result = *this;
    glm::normalize(result);
    return Vector3(result);
}

float Vector3::sqrMagnitude() const{
    float result = glm::length2(static_cast<glm::vec3>(*this));
    return result;
}

void Vector3::Normalize(){
    glm::normalize(static_cast<glm::vec3>(*this));
}

void Vector3::Set(float new_x, float new_y, float new_z){
    x = new_x;
    y = new_y;
    z = new_z;
}

float Vector3::Angle(Vector3 from, Vector3 to){
    glm::vec3 a = glm::normalize(static_cast<glm::vec3>(from));
    glm::vec3 b = glm::normalize(static_cast<glm::vec3>(to));
    float result = glm::angle(a, b);
    return result;
}

Vector3 Vector3::ClampMagnitude(Vector3 vector, float maxLength){
    float length = glm::length(static_cast<glm::vec3>(vector));
    glm::vec3 result = vector;
    result *= maxLength / length;
    return static_cast<Vector3>(result);
}

float Vector3::Distance(Vector3 a, Vector3 b){
    float result = glm::distance(static_cast<glm::vec3>(a),
                                    static_cast<glm::vec3>(b));
    return result;
}

float Vector3::Dot(Vector3 lhs, Vector3 rhs){
    float result = glm::dot(static_cast<glm::vec3>(lhs),
                            static_cast<glm::vec3>(rhs));
    return result;
}

Vector3 Vector3::Lerp(Vector3 a, Vector3 b, float t){
    t = glm::clamp(t, 0.0f, 1.0f);
    return LerpUnclamped(a, b, t);
}

Vector3 Vector3::LerpUnclamped(Vector3 a, Vector3 b, float t){
    glm::vec3 result = a*(1.0f-t) + b*t;
    return static_cast<Vector3>(result);
}

Vector3 Vector3::Max(Vector3 lhs, Vector3 rhs){
    glm::vec3 result = glm::max(static_cast<glm::vec3>(lhs),
                                static_cast<glm::vec3>(rhs));
    return static_cast<Vector3>(result);
}

Vector3 Vector3::Min(Vector3 lhs, Vector3 rhs){
    glm::vec3 result = glm::min(static_cast<glm::vec3>(lhs),
                                static_cast<glm::vec3>(rhs));
    return static_cast<Vector3>(result);
}

Vector3 Vector3::MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta){
    glm::vec3 delta = target - current;
    glm::vec3 result = (maxDistanceDelta >= glm::length(delta)) ? target : current + maxDistanceDelta;
    return static_cast<Vector3>(result);
}

void Vector3::OrthoNormalize(Vector3 *normal, Vector3 *tangent){
    glm::orthonormalize(*normal, *tangent);
}

Vector3 Vector3::Project(Vector3 vector, Vector3 onNormal){
    /*
    float num = Vector3::Dot(onNormal, onNormal);
    if (num < Mathf::Epsilon)
    {
        return Vector3::zero;
    }
    return onNormal * Vector3::Dot(vector, onNormal) / num;
    */

    return Vector3::zero;
}

Vector3 Vector3::ProjectOnPlane(Vector3 vector, Vector3 planeNormal){
    return vector - Vector3::Project(vector, planeNormal);
}

Vector3 Vector3::Reflect(Vector3 inDirection, Vector3 inNormal){
    glm::vec3 result = glm::reflect(static_cast<glm::vec3>(inDirection),
                                    static_cast<glm::vec3>(inNormal));
    return static_cast<Vector3>(result);
}

Vector3 Vector3::Scale(Vector3 a, Vector3 b){
    glm::vec3 result = a * b;
    return static_cast<Vector3>(result);
}

Vector3 Vector3::Slerp(Vector3 a, Vector3 b, float t){
    t = glm::clamp(t, 0.0f, 1.0f);
    return SlerpUnclamped(a, b, t);
}

Vector3 Vector3::SlerpUnclamped(Vector3 a, Vector3 b, float t){
    glm::vec3 result = glm::slerp(static_cast<glm::vec3>(a),
                                    static_cast<glm::vec3>(b),
                                    t);
    return static_cast<Vector3>(result);
}

///////////////////////////////////////////////////

Vector4::Vector4(glm::vec4 v): glm::vec4(v){}
Vector4::Vector4(float x, float y, float z, float w): glm::vec4(x, y, z, w){}
Vector4::Vector4(Vector3 v): glm::vec4(v.x, v.y, v.z, 0.0f){}

const Vector4 Vector4::one(0.0f, 0.0f, 0.0f, 0.0f);
const Vector4 Vector4::zero(0.0f, 0.0f, 0.0f, 0.0f);

float Vector4::magnitude() const{
    float result = glm::length(static_cast<glm::vec4>(*this));
    return result;
}

Vector4 Vector4::normalized() const{
    glm::vec4 result = *this;
    glm::normalize(result);
    return static_cast<Vector4>(result);
}

float Vector4::sqrMagnitude() const{
    float result = glm::length2(static_cast<glm::vec4>(*this));
    return result;
}

void Vector4::Normalize(){
    glm::normalize(static_cast<glm::vec4>(*this));
}

void Vector4::Set(float new_x, float new_y, float new_z, float new_w){
    x = new_x;
    y = new_y;
    z = new_z;
    w = new_w;
}

float Vector4::Distance(Vector4 a, Vector4 b){
    float result = glm::distance(static_cast<glm::vec4>(a), static_cast<glm::vec4>(b));
    return result;
}

float Vector4::Dot(Vector4 lhs, Vector4 rhs){
    float result = glm::dot(static_cast<glm::vec4>(lhs), static_cast<glm::vec4>(rhs));
    return result;
}

Vector4 Vector4::Lerp(Vector4 a, Vector4 b, float t){
    //t = bx::fclamp(t, 0.0f, 1.0f);
    return LerpUnclamped(a, b, t);
}

Vector4 Vector4::LerpUnclamped(Vector4 a, Vector4 b, float t){
    glm::vec4 result = a*(1.0f-t) + b*t;
    return static_cast<Vector4>(result);
}

Vector4 Vector4::Max(Vector4 lhs, Vector4 rhs){
    glm::vec4 result = glm::max(static_cast<glm::vec4>(lhs), static_cast<glm::vec4>(rhs));
    return static_cast<Vector4>(result);
}

Vector4 Vector4::Min(Vector4 lhs, Vector4 rhs){
    glm::vec4 result = glm::min(static_cast<glm::vec4>(lhs), static_cast<glm::vec4>(rhs));
    return static_cast<Vector4>(result);
}

Vector4 Vector4::MoveTowards(Vector4 current, Vector4 target, float maxDistanceDelta){
    glm::vec4 delta = target - current;
    glm::vec4 result = (maxDistanceDelta >= glm::length(delta)) ? target : current + maxDistanceDelta;
    return static_cast<Vector4>(result);
}

Vector4 Vector4::Scale(Vector4 a, Vector4 b){
    glm::vec4 result = a * b;
    return static_cast<Vector4>(result);
}

///////////////////////////////////////////////////////

Quaternion::Quaternion(glm::quat q): glm::quat(q){}
Quaternion::Quaternion(float x, float y, float z, float w): glm::quat(w, x, y, z){}

const Quaternion Quaternion::identity = Quaternion(0, 0, 0, 1);

Vector3 Quaternion::eulerAngles() const{
    glm::vec3 result = glm::eulerAngles(*this);
    return static_cast<Vector3>(result);
}

void Quaternion::eulerAngles(Vector3 euler){
    glm::quat result = Euler(euler);
    x = result.x;
    y = result.y;
    z = result.z;
    w = result.w;
}

Quaternion Quaternion::normalized(){
    return glm::normalize(*this);
}

void Quaternion::Set(float new_x, float new_y, float new_z, float new_w){
    x = new_x;
    y = new_y;
    z = new_z;
    w = new_w;
}

void Quaternion::SetFromToRotation(Vector3 fromDirection, Vector3 toDirection){
    glm::quat result = glm::rotation(fromDirection, toDirection);
    x = result.x;
    y = result.y;
    z = result.z;
    w = result.w;
}

void Quaternion::SetLookRotation(Vector3 view, Vector3 up){
    glm::quat result = glm::quatLookAt(view, up);
    x = result.x;
    y = result.y;
    z = result.z;
    w = result.w;
}

void Quaternion::ToAngleAxis(float *angle, Vector3 *axis){
    assert(false);
}

float Quaternion::Angle(Quaternion a, Quaternion b){
    assert(false);
    return 0.0f;
}

Quaternion Quaternion::AngleAxis(float angle, Vector3 axis){
    glm::quat result = glm::angleAxis(Mathf::Deg2Rad(angle), static_cast<glm::vec3>(axis));
    return static_cast<Quaternion>(result);
}

float Quaternion::Dot(Quaternion a, Quaternion b){
    float result = glm::dot(static_cast<glm::quat>(a), static_cast<glm::quat>(b));
    return result;
}

Quaternion Quaternion::Euler(Vector3 euler){
    //glm::quat result = glm::quat(Vector3(Mathf::Deg2Rad(euler.x), Mathf::Deg2Rad(euler.y), Mathf::Deg2Rad(euler.z)));
    glm::quat result = glm::quat(Vector3(euler.x, euler.y, euler.z));
    return static_cast<Quaternion>(result);
}

Quaternion Quaternion::FromToRotation(Vector3 fromDirection, Vector3 toDirection){
    glm::quat result = glm::rotation(fromDirection, toDirection);
    return static_cast<Quaternion>(result);
}

Quaternion Quaternion::Inverse(Quaternion rotation){
    glm::quat result = glm::inverse(rotation);
    return static_cast<Quaternion>(result);
}

Quaternion Quaternion::Lerp(Quaternion a, Quaternion b, float t){
    t = glm::clamp(t, 0.0f, 1.0f);
    return LerpUnclamped(a, b, t);
}

Quaternion Quaternion::LerpUnclamped(Quaternion a, Quaternion b, float t){
    glm::quat result = glm::lerp(a, b, t);
    return static_cast<Quaternion>(result);
}

Quaternion Quaternion::LookRotation(Vector3 forward, Vector3 upwards){
    return static_cast<Quaternion>(glm::quatLookAt(forward, upwards));
}

Quaternion Quaternion::RotateTowards(Quaternion from, Quaternion to, float maxDegreesDelta){
    assert(false);
    return identity;
}

Quaternion Quaternion::Slerp(Quaternion a, Quaternion b, float t){
    t = glm::clamp(t, 0.0f, 1.0f);
    return SlerpUnclamped(a, b, t);
}

Quaternion Quaternion::SlerpUnclamped(Quaternion a, Quaternion b, float t){
    glm::quat result = glm::mix(static_cast<glm::quat>(a), static_cast<glm::quat>(b), t);
    return static_cast<Quaternion>(result);
}

////////////////////////////////////////////////

Matrix4::Matrix4(glm::mat4 m): glm::mat4(m){}
Matrix4::Matrix4(float f): glm::mat4(f){}

const Matrix4 Matrix4::identity(1.0f);
const Matrix4 Matrix4::zero(0.0f);

float Matrix4::determinant() const{
    float result = glm::determinant(*this);
    return result;
}

Matrix4 Matrix4::inverse() const{
    glm::mat4 result = glm::inverse(*this);
    return static_cast<Matrix4>(result);
}

Matrix4 Matrix4::transpose() const{
    glm::mat4x4 result = glm::transpose(*this);
    return static_cast<Matrix4>(result);
}

Vector4 Matrix4::GetColumn(int i) const{
    Vector4 result = static_cast<Vector4>(glm::column(*this, i));
    return result;
}

Vector4 Matrix4::GetRow(int i) const{
    Vector4 result = static_cast<Vector4>(glm::row(*this, i));
    return result;
}

Vector3 Matrix4::MultiplyPoint(Vector3 v) const{
    Vector3 result;
    result.x = (*this)[0][0] * v.x + (*this)[0][1] * v.y + (*this)[0][2] * v.z + (*this)[0][3];
    result.y = (*this)[1][0] * v.x + (*this)[1][1] * v.y + (*this)[1][2] * v.z + (*this)[1][3];
    result.z = (*this)[2][0] * v.x + (*this)[2][1] * v.y + (*this)[2][2] * v.z + (*this)[2][3];
    float num = (*this)[3][0] * v.x + (*this)[3][1] * v.y + (*this)[3][2] * v.z + (*this)[3][3];
    num = 1.0f / num;
    result.x *= num;
    result.y *= num;
    result.z *= num;
    return result;
}

Vector3 Matrix4::MultiplyPoint3x4(Vector3 v) const{
    Vector3 result;
    result.x = (*this)[0][0] * v.x + (*this)[0][1] * v.y + (*this)[0][2] * v.z + (*this)[0][3];
    result.y = (*this)[1][0] * v.x + (*this)[1][1] * v.y + (*this)[1][2] * v.z + (*this)[1][3];
    result.z = (*this)[2][0] * v.x + (*this)[2][1] * v.y + (*this)[2][2] * v.z + (*this)[2][3];
    return result;
}

Vector3 Matrix4::MultiplyVector(Vector3 v) const{
    Vector3 result;
    result.x = (*this)[0][0] * v.x + (*this)[0][1] * v.y + (*this)[0][2] * v.z;
    result.y = (*this)[1][0] * v.x + (*this)[1][1] * v.y + (*this)[1][2] * v.z;
    result.z = (*this)[2][0] * v.x + (*this)[2][1] * v.y + (*this)[2][2] * v.z;
    return result;
}

void Matrix4::SetColumn(int i, Vector4 v){
    glm::column(*this, i, v);
}

void Matrix4::SetRow(int i, Vector4 v){
    glm::row(*this, i, v);
}

void Matrix4::SetTRS(Vector3 pos, Quaternion q, Vector3 s){
    glm::mat4 translate = glm::translate(pos);
    glm::mat4 rotate = glm::mat4_cast(q);
    glm::mat4 scale = glm::scale(s);

    *this = static_cast<Matrix4>(translate * rotate * scale);
}

Matrix4 Matrix4::Ortho(float left, float right, float bottom, float top, float zNear, float zFar){
    glm::mat4 result = glm::ortho(left, right, bottom, top, zNear, zFar);
    return static_cast<Matrix4>(result);
}

Matrix4 Matrix4::Perspective(float fov, float aspect, float zNear, float zFar){
    glm::mat4 result = glm::perspective(fov, aspect, zNear, zFar);
    return static_cast<Matrix4>(result);
}

Matrix4 Matrix4::TRS(Vector3 pos, Quaternion q, Vector3 s){
    glm::mat4 translate = glm::translate(pos);
    glm::mat4 rotate = glm::mat4_cast(q);
    glm::mat4 scale = glm::scale(s);

    glm::mat4 result = translate * rotate * scale;
    return static_cast<Matrix4>(result);
}

Matrix4 Matrix4::LookAt(Vector3 eye, Vector3 center, Vector3 up){
    return static_cast<Matrix4>(glm::lookAt(eye, center, up));
}

Matrix4 Matrix4::Rotate(float euler, Vector3 axis){ 
    return static_cast<Matrix4>(glm::rotate(glm::mat4(1.0f), euler, axis)); 
}

Matrix4 Matrix4::Translate(Vector3 position){ 
    return static_cast<Matrix4>(glm::translate(glm::mat4(1.0f), position)); 
}

Matrix4 Matrix4::Scale(Vector3 scale){ 
    return static_cast<Matrix4>(glm::scale(glm::mat4(1.0f), scale)); 
}

Matrix4 Matrix4::ToMatrix(Quaternion quaternion){
    return static_cast<Matrix4>(glm::toMat4(quaternion));
}

///////////////////////////////////////////////

//const float Mathf::Deg2Rad = (glm::pi<float>() * 2.0f) / 360.0f;
const float Mathf::Epsilon = glm::epsilon<float>();
const float Mathf::Infinity = 0.0f;
const float Mathf::NegativeInfinity = 0.0f;
const float Mathf::PI = glm::pi<float>();
//const float Mathf::Rad2Deg = 360.0f / (glm::pi<float>() * 2.0f);

float Mathf::Max(const std::vector<float>& values){
    float result = 0.0f;
    if (values.size() > 0){
        result = values[0];

        if (values.size() > 1){
            for (size_t i = 1; i < values.size(); ++i){
                result = Max(result, values[i]);
            }
        }
    }
    return result;
}

float Mathf::Min(const std::vector<float>& values){
    float result = 0.0f;
    if (values.size() > 0){
        result = values[0];

        if (values.size() > 1){
            for (size_t i = 1; i < values.size(); ++i){
                result = Min(result, values[i]);
            }
        }
    }
    return result;
}

float Mathf::PerlinNoise(float x, float y){
    Assert(false);
    //float result = stb_perlin_noise3(x, y, 0.0f);
    return 0;
}

float Mathf::SmoothDamp(float current,
                        float target,
                        float *currentVelocity,
                        float smoothTime,
                        float maxSpeed,
                        float deltaTime)
{
    // https://graemepottsfolio.wordpress.com/tag/damped-spring/

    smoothTime = Max(0.0001f, smoothTime);
    float omega = 2.0f / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f*x*x + 0.235f*x*x*x);
    float deltaX = target - current;
    float maxDelta = maxSpeed * smoothTime;

    // ensure we do not exceed our max speed
    deltaX = Mathf::Clamp(deltaX, -maxDelta, maxDelta);
    float temp = (*currentVelocity + omega * deltaX) * deltaTime;
    float result = current - deltaX + (deltaX + temp) * exp;
    *currentVelocity = (*currentVelocity - omega * temp) * exp;

    // ensure that we do not overshoot our target
    if (target - current > 0.0f == result > target){
        result = target;
        *currentVelocity = 0.0f;
    }
    return result;
}

float Mathf::SmoothDampAngle(float current,
                                float target,
                                float *currentVelocity,
                                float smoothTime,
                                float maxSpeed,
                                float deltaTime)
{
    smoothTime = Max(0.0001f, smoothTime);
    float omega = 2.0f / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f*x*x + 0.235f*x*x*x);
    float deltaX = target - current;
    float maxDelta = maxSpeed * smoothTime;

    // ensure we do not exceed our max speed
    deltaX = Mathf::Clamp(deltaX, -maxDelta, maxDelta);
    float temp = (*currentVelocity + omega * deltaX) * deltaTime;
    float result = current - deltaX + (deltaX + temp) * exp;
    *currentVelocity = (*currentVelocity - omega * temp) * exp;

    // ensure that we do not overshoot our target
    if (target - current > 0.0f == result > target){
        result = target;
        *currentVelocity = 0.0f;
    }
    return result;
}

}