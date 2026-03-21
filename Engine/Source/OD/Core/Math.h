#pragma once

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

}