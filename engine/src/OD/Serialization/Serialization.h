#pragma once

#include <yaml-cpp/yaml.h>
#include "OD/Core/Math.h"

namespace YAML{
using namespace OD;

template<>
struct convert<Vector2>{
    static Node encode(const Vector2& v);
    static bool decode(const Node& node, Vector2& v);
};

template<>
struct convert<Vector3>{
    static Node encode(const Vector3& v);
    static bool decode(const Node& node, Vector3& v);
};

template<>
struct convert<Vector4>{
    static Node encode(const Vector4& v);
    static bool decode(const Node& node, Vector4& v);
};

template<>
struct convert<Quaternion>{
    static Node encode(const Quaternion& v);
    static bool decode(const Node& node, Quaternion& v);
};

}

namespace OD{

YAML::Emitter& operator<<(YAML::Emitter& out, const Vector2& v);
YAML::Emitter& operator<<(YAML::Emitter& out, const Vector3& v);
YAML::Emitter& operator<<(YAML::Emitter& out, const Vector4& v);
YAML::Emitter& operator<<(YAML::Emitter& out, const Quaternion& v);

}