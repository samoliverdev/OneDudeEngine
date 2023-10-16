#pragma once

#include "OD/Core/Math.h"
#include <string>
#include <vector>
#include <functional>
#include "Serialization.h"

namespace OD{

struct ArchiveListFunctions{
    std::function<void(void)> clean;
    std::function<void(void)> push;
    std::function<void(void)> pop;
    std::function<int(void)> size;
};

struct ArchiveNode{
    enum class Type{Object, Float, Int, String, Vector3, Vector4, Quaternion, List};

    ArchiveNode(){}
    ArchiveNode(Type _type, std::string _name, void* _value, bool _isReadMode):
        type(_type), name(_name), value(_value), isReadMode(_isReadMode){}

    Type type;
    std::string name;
    void* value;
    bool isReadMode;

    std::string stringValue;
    union{
        float floatValue;
        int intValue;
        float vectorValue[4];
    };
    ArchiveListFunctions listFunctions;

    std::map<std::string, ArchiveNode> values;

    inline void Add(float* value, std::string name){
        /*if(isReadMode){
            *value = values[name].floatValue;
        } else {*/
            ArchiveNode node(Type::Float, name, value, isReadMode);
            node.floatValue = *value;
            values[name] = node;
        //}
    }

    inline void Add(int* value, std::string name){
        /*if(isReadMode){
            *value = values[name].intValue;
        } else {*/
            ArchiveNode node(Type::Int, name, value, isReadMode);
            node.intValue = *value;
            values[name] = node;
        //}
    }

    inline void Add(Vector3* value, std::string name){
        /*if(isReadMode){
            *value = Vector3(
                values[name].vectorValue[0],
                values[name].vectorValue[1],
                values[name].vectorValue[2]
            );
        } else {*/
            ArchiveNode node(Type::Vector3, name, value, isReadMode);
            node.vectorValue[0] = value->x;
            node.vectorValue[1] = value->y;
            node.vectorValue[2] = value->z;
            values[name] = node;
        //}
    }

    inline void Add(Vector4* value, std::string name){
        /*if(isReadMode){
            *value = Vector4(
                values[name].vectorValue[0],
                values[name].vectorValue[1],
                values[name].vectorValue[2],
                values[name].vectorValue[3]
            );
        } else {*/
            ArchiveNode node(Type::Vector4, name, value, isReadMode);
            node.vectorValue[0] = value->x;
            node.vectorValue[1] = value->y;
            node.vectorValue[2] = value->z;
            node.vectorValue[3] = value->w;
            values[name] = node;
        //}
    }

    inline void Add(Quaternion* value, std::string name){
        /*if(isReadMode){
            *value = Quaternion(
                values[name].vectorValue[0],
                values[name].vectorValue[1],
                values[name].vectorValue[2],
                values[name].vectorValue[3]
            );
        } else {*/
            ArchiveNode node(Type::Quaternion, name, value, isReadMode);
            node.vectorValue[0] = value->x;
            node.vectorValue[1] = value->y;
            node.vectorValue[2] = value->z;
            node.vectorValue[3] = value->w;
            values[name] = node;
        //}
    }

    inline void Add(std::string* value, std::string name){
        /*if(isReadMode){
            *value = values[name].stringValue;
        } else {*/
            ArchiveNode node(Type::String, name, value, isReadMode);
            node.stringValue = *value;
            values[name] = node;
        //}
    }

    template<typename T>
    void Add(T& value, std::string name){
        /*if(isReadMode){
            value.Serialize(values[name]);
        } else {*/
            ArchiveNode node(Type::Object, name, nullptr, isReadMode);
            value.Serialize(node);
            values[name] = node;
        //}
    }

    template<typename T>
    void Add(std::vector<T>& value, std::string name){
        /*if(isReadMode){
            value.clear();
            for(auto i: values[name].values){
                value.push_back(T());
                value[value.size()-1].Serialize(i.second);
            }   
        } else {*/
            ArchiveNode nodeList(Type::List, name, nullptr, isReadMode);
            
            nodeList.listFunctions.clean = [&]{value.clear();};
            nodeList.listFunctions.push = [&]{value.push_back(T());};

            for(int i = 0; i < value.size(); i++){
                ArchiveNode _node(Type::Object, name, nullptr, isReadMode);
                value[i].Serialize(_node);
                nodeList.values[std::to_string(i)] = _node;
            }

            values[name] = nodeList;
        //}


    }

    inline void Clean(){
        values.clear();
    }

    inline Vector3 AsVector3(){ return Vector3(vectorValue[0], vectorValue[1], vectorValue[2]); }
    inline Vector4 AsVector4(){ return Vector4(vectorValue[0], vectorValue[1], vectorValue[2], vectorValue[3]); }
    inline Quaternion AsQuaternion(){ return Quaternion(vectorValue[0], vectorValue[1], vectorValue[2], vectorValue[3]); }
    
    static void SaveSerializer(ArchiveNode& s, std::string name, YAML::Emitter& out);
    static void LoadSerializer(ArchiveNode& s, YAML::Node& node);
    static void DrawArchive(ArchiveNode& ar);
};


}