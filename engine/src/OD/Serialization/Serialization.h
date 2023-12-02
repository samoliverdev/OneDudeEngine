#pragma once

#include <cereal/access.hpp>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/memory.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/json.hpp>

#include <yaml-cpp/yaml.h>
#include "OD/Core/Math.h"

namespace glm{

template<class Archive>
void serialize(Archive &archive, glm::vec2 &v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y)
    );
}

template<class Archive>
void serialize(Archive &archive, glm::vec3 &v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y), 
        CEREAL_NVP(v.z)
    );
}

template<class Archive>
void serialize(Archive &archive, glm::vec4 &v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y), 
        CEREAL_NVP(v.z), 
        CEREAL_NVP(v.w)
    );
}

template<class Archive>
void serialize(Archive &archive, glm::quat &q){
    archive(
        CEREAL_NVP(q.x), 
        CEREAL_NVP(q.y), 
        CEREAL_NVP(q.z), 
        CEREAL_NVP(q.w)
    );
}

}

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