#pragma once

#include "OD/Core/Math.h"
#include <string>
#include <vector>
#include <functional>
#include "Serialization.h"

namespace OD{

struct ArchiveNode;

struct ArchiveListFunctions{
    std::function<void(ArchiveNode&)> clean;
    std::function<void(ArchiveNode&)> push;
    std::function<void(ArchiveNode&)> pop;
    std::function<int(ArchiveNode&)> size;
};

struct ArchiveNode{
    enum class Type{None, Object, Float, Int, String, Vector3, Vector4, Quaternion, List};

    ArchiveNode():_type(Type::None), _name(""), value(nullptr){}
    ArchiveNode(Type _type, std::string _name, void* _value):
        _type(_type), _name(_name), value(_value){}

        
private:
    Type _type;
    std::string _name;

public:
    void* value;
    ArchiveListFunctions listFunctions;
    std::map<std::string, ArchiveNode> values;

    inline const Type& type(){ return _type; }
    inline const std::string name(){ return _name; }

    inline void Add(float* value, std::string name){ values[name] = ArchiveNode(Type::Float, name, value); }
    inline void Add(int* value, std::string name){ values[name] = ArchiveNode(Type::Int, name, value); }
    inline void Add(Vector3* value, std::string name){ values[name] = ArchiveNode(Type::Vector3, name, value); }
    inline void Add(Vector4* value, std::string name){ values[name] = ArchiveNode(Type::Vector4, name, value); }
    inline void Add(Quaternion* value, std::string name){ values[name] = ArchiveNode(Type::Quaternion, name, value); }
    inline void Add(std::string* value, std::string name){ values[name] = ArchiveNode(Type::String, name, value); }

    template<typename T>
    void Add(T& value, std::string name){
        ArchiveNode node(Type::Object, name, nullptr);
        value.Serialize(node);
        values[name] = node;
    }

    template<typename T>
    void Add(std::vector<T>* value, std::string name){
        values[name] = ArchiveNode(Type::List, name, value);
        
        values[name].listFunctions.clean = [&](ArchiveNode& node){
            std::vector<T>* list = static_cast<std::vector<T>*>(node.value);
            list->clear();
            node.values.clear();
        };

        values[name].listFunctions.push = [&](ArchiveNode& node){
            std::vector<T>* list = static_cast<std::vector<T>*>(node.value);
            list->push_back(T());

            std::string nameIndex = std::to_string(list->size()-1);
            
            ArchiveNode _node(Type::Object, nameIndex, nullptr);
            (*list)[list->size()-1].Serialize(_node);
            node.values[nameIndex] = _node;
        };

        for(int i = 0; i < value->size(); i++){
            std::string nameIndex = std::to_string(i);

            ArchiveNode _node(Type::Object, nameIndex, nullptr);
            (*value)[i].Serialize(_node);
            values[name].values[nameIndex] = _node;
        }
    }

    inline void Clean(){
        values.clear();
    }

    static void SaveSerializer(ArchiveNode& s, std::string name, YAML::Emitter& out);
    static void LoadSerializer(ArchiveNode& s, YAML::Node& node);
    static void DrawArchive(ArchiveNode& ar);
};


}