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
void LoadArchive(const char* path, Archive& data){
    std::ifstream os(path);
    cereal::JSONOutputArchive ar(os);
    ArchiveDumpNVP(ar, data);
}

template<class Archive>
void LoadOrCreateArchive(const char* path, Archive& data){
    std::ifstream stream(path);
    if(stream.fail()){
        std::ofstream os(path);
        cereal::JSONOutputArchive ar(os);
        ArchiveDumpNVP(ar, data);
    } else {
        cereal::JSONInputArchive ar{stream};
        ArchiveDumpNVP(ar, data);
    }
}


template<class Archive>
void SaveArchive(const char* path, Archive& data){
    std::ofstream os(path);
    cereal::JSONOutputArchive ar(os);
    ArchiveDumpNVP(ar, data);
}

}