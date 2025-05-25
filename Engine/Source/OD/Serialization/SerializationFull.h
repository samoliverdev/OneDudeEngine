#pragma once
#include "Serialization.h"
#include <fstream>
#include <cereal/details/helpers.hpp>
#include <cereal/access.hpp>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/json.hpp>

#define ODOutputArchive cereal::JSONOutputArchive
#define ODInputArchive cereal::JSONInputArchive

namespace OD{

template<class Archive>
void LoadArchive(const std::string& path, Archive& data, const std::string& name = ""){
    std::ofstream os(path);
    cereal::JSONOutputArchive ar(os);

    if(name.empty()){
        ArchiveDumpNVP(ar, data);
    } else {
        ArchiveDumpNamed(ar, name, data);
    }
}

template<class Archive>
void LoadOrCreateArchive(const std::string& path, Archive& data, const std::string& name = ""){
    /*std::ifstream stream(path);
    if(stream.fail()){
        std::ofstream os(path);
        cereal::JSONOutputArchive ar(os);
        
        if(name.empty()){
            ArchiveDumpNVP(ar, data);
        } else { 
            ArchiveDumpNamed(ar, name, data);
        }
    } else {
        cereal::JSONInputArchive ar{stream};
        
        if(name.empty()){
            ArchiveDumpNVP(ar, data);
        } else { 
            ArchiveDumpNamed(ar, name, data);
        }
    }*/

    bool success = false;

    try{
        std::ifstream stream(path);
        if(stream.is_open()){
            cereal::JSONInputArchive ar(stream);
            if(name.empty()){
                ArchiveDumpNVP(ar, data);
            } else {
                ArchiveDumpNamed(ar, name, data);
            }
            success = true;
        }
    } catch(const std::exception& e){
        LogError("Failed to load archive: %s", e.what());
    }

    if(!success){
        std::ofstream os(path);
        if(!os.is_open()){
            LogError("Failed to open file for writing: %s", path.c_str());
            return;
        }

        cereal::JSONOutputArchive ar(os);
        if(name.empty()){
            ArchiveDumpNVP(ar, data);
        } else {
            ArchiveDumpNamed(ar, name, data);
        }
    }
}

template<class Archive>
void SaveArchive(const std::string& path, Archive& data, const std::string& name = ""){
    std::ofstream os(path);
    cereal::JSONOutputArchive ar(os);

    if(name.empty()){
        ArchiveDumpNVP(ar, data);
    } else {
        ArchiveDumpNamed(ar, name, data);
    }
}

}