#pragma once

#include "OD/Core/Math.h"
#include <string>
#include <vector>

namespace OD{

struct Archive;

struct ArchiveValue{
    enum class Type{Float, Int, String, Vector3, Vector4, Quaternion, T, TList};

    Type type;
    std::string name;
    
    union{
        float* floatValue;
        int* intValue;
        std::string* stringValue;
        Vector3* vector3Value;
        Vector4* vector4Value;
        Quaternion* quaternionValue;
    };

    std::vector<Archive> children;
};

struct Archive{
public:
    inline void Clean(){
        _values.clear();
    }

    inline void Add(float* value, std::string name){
        ArchiveValue sv;
        sv.type = ArchiveValue::Type::Float;
        sv.floatValue = value;
        sv.name = name;
        _values.push_back(sv);
    }

    inline void Add(int* value, std::string name){
        ArchiveValue sv;
        sv.type = ArchiveValue::Type::Int;
        sv.intValue = value;
        sv.name = name;
        _values.push_back(sv);
    }

    inline void Add(std::string* value, std::string name){
        ArchiveValue sv;
        sv.type = ArchiveValue::Type::String;
        sv.stringValue = value;
        sv.name = name;
        _values.push_back(sv);
    }

    inline void Add(Vector3* value, std::string name){
        ArchiveValue sv;
        sv.type = ArchiveValue::Type::String;
        sv.vector3Value = value;
        sv.name = name;
        _values.push_back(sv);
    }

    inline void Add(Vector4* value, std::string name){
        ArchiveValue sv;
        sv.type = ArchiveValue::Type::String;
        sv.vector4Value = value;
        sv.name = name;
        _values.push_back(sv);
    }

    inline void Add(Quaternion* value, std::string name){
        ArchiveValue sv;
        sv.type = ArchiveValue::Type::String;
        sv.quaternionValue = value;
        sv.name = name;
        _values.push_back(sv);
    }

    template<typename T>
    void Add(std::vector<T>& list, std::string name){
        ArchiveValue sv;
        sv.type = ArchiveValue::Type::TList;
        sv.name = name;

        for(int i = 0; i < list.size(); i++){
            Archive ar;
            list[i].Serialize(ar);
            sv.children.push_back(ar);
        }

        _values.push_back(sv);
    }

    template<typename T>
    void Add(T& list, std::string name){
        ArchiveValue sv;
        sv.type = ArchiveValue::Type::T;
        sv.name = name;

        Archive ar;
        list.Serialize(ar);
        sv.children.push_back(ar);

        _values.push_back(sv);
    }

    inline void Show(int level = 0){
        for(int x = 0; x < level; x++){
            printf("  ");
        }

        printf("Archive: %s\n", _name.c_str());

        for(auto& i: _values){
            printf("  ");
            for(int x = 0; x < level; x++){
                printf("  ");
            }

            if(i.type == ArchiveValue::Type::Float) printf("%s: %f\n", i.name.c_str(), *i.floatValue);
            if(i.type == ArchiveValue::Type::Int) printf("%s: %d\n", i.name.c_str(), *i.intValue);
            //if(i.type == ArchiveValue::Type::String){
            //    printf("Value: %s %s\n", i.name.c_str(), (*i.stringValue)->c_str());
            //}

            if(i.type == ArchiveValue::Type::TList){
                printf("%s:\n", i.name.c_str());
                level += 1;
                for(auto& j: i.children){
                    j.Show(level+1);
                }
            }

            if(i.type == ArchiveValue::Type::T){
                printf("%s:\n", i.name.c_str());
                level += 1;
                i.children[0].Show(level+1);
            }
        }
    }

    inline std::string& name(){ return _name; }
    inline void name(std::string name){ _name = name; }

    inline std::vector<ArchiveValue>& values(){ return _values; }

private:
    std::vector<ArchiveValue> _values;
    std::string _name;
};

}