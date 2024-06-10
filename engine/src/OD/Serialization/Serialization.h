#pragma once

#include <cereal/cereal.hpp>
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

//#include <entt/entt.hpp>

#include "OD/Core/Math.h"

#define ODOutputArchive cereal::JSONOutputArchive
#define ODInputArchive cereal::JSONInputArchive

#define TYPE_TO_STRING(T) #T

#define ArchiveDump(archive, data) try{ archive(data); }catch(...){ LogWarning("ErrorOnTrySerialize"); }
#define ArchiveDumpNVP(archive, data) try{ archive(CEREAL_NVP(data)); }catch(...){ LogWarning("ErrorOnTrySerialize"); }

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
