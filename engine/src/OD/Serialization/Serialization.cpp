#include "Serialization.h"

namespace YAML{
using namespace OD;

Node convert<Vector2>::encode(const Vector2& v){
    Node node;
    node.push_back(v.x);
    node.push_back(v.y);
    return node;
}

bool convert<Vector2>::decode(const Node& node, Vector2& v){
    if(!node.IsSequence() || node.size() !=2) return false;

    v.x = node[0].as<float>();
    v.y = node[1].as<float>();
    return true;
}

Node convert<Vector3>::encode(const Vector3& v){
    Node node;
    node.push_back(v.x);
    node.push_back(v.y);
    node.push_back(v.z);
    return node;
}

bool convert<Vector3>::decode(const Node& node, Vector3& v){
    if(!node.IsSequence() || node.size() != 3) return false;

    v.x = node[0].as<float>();
    v.y = node[1].as<float>();
    v.z = node[2].as<float>();
    return true;
}

Node convert<Vector4>::encode(const Vector4& v){
    Node node;
    node.push_back(v.x);
    node.push_back(v.y);
    node.push_back(v.z);
    node.push_back(v.w);
    return node;
}

bool convert<Vector4>::decode(const Node& node, Vector4& v){
    if(!node.IsSequence() || node.size() != 4) return false;

    v.x = node[0].as<float>();
    v.y = node[1].as<float>();
    v.z = node[2].as<float>();
    v.w = node[3].as<float>();
    return true;
}

Node convert<Quaternion>::encode(const Quaternion& v){
    Node node;
    node.push_back(v.x);
    node.push_back(v.y);
    node.push_back(v.z);
    node.push_back(v.w);
    return node;
}

bool convert<Quaternion>::decode(const Node& node, Quaternion& v){
    if(!node.IsSequence() || node.size() != 4) return false;

    v.x = node[0].as<float>();
    v.y = node[1].as<float>();
    v.z = node[2].as<float>();
    v.w = node[3].as<float>();
    return true;
}

}


namespace OD{

YAML::Emitter& operator<<(YAML::Emitter& out, const Vector2& v){
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const Vector3& v){
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const Vector4& v){
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const Quaternion& v){
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

}