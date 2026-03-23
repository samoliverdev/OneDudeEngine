//#pragma once
//#include "SerializationCore.h"
//#include "SerializationMath.h"
//#include "SerializationEnum.h"

#pragma once
#include "OD/Base.h"
#include "OD/Core/Log.h"
#include "OD/Core/Math.h"
#include <magic_enum/magic_enum.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
/*#include <cereal/details/helpers.hpp>
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
#include <cereal/archives/json.hpp>*/

//#define ODOutputArchive cereal::JSONOutputArchive
//#define ODInputArchive cereal::JSONInputArchive

#define TYPE_TO_STRING(T) #T
#define ArchiveDump(archive, data) try{ archive(data); }catch(const cereal::Exception& e){ /*LogWarning("ErrorOnTrySerialize: {}", std::string(e.what()));*/ }
#define ArchiveDumpNVP(archive, data) try{ archive(CEREAL_NVP(data)); }catch(const cereal::Exception& e){ /*LogWarning("ErrorOnTrySerialize: {}", std::string(e.what()));*/ }
#define ArchiveDumpNamed(archive, name, data) try{ archive(cereal::make_nvp(name, data)); }catch(const cereal::Exception& e){ /*LogWarning("ErrorOnTrySerialize: {}", std::string(e.what()));*/ }

/*
namespace cereal {
template <class Archive, cereal::traits::EnableIf<cereal::traits::is_text_archive<Archive>::value>
= cereal::traits::sfinae, class T>
std::enable_if_t<std::is_enum_v<T>, std::string> save_minimal(Archive&, const T& h)
{
    return std::string(magic_enum::enum_name(h));
}

template <class Archive, cereal::traits::EnableIf<cereal::traits::is_text_archive<Archive>::value>
        = cereal::traits::sfinae, class T> std::enable_if_t<std::is_enum_v<T>, void> load_minimal(Archive const&, T& enumType, std::string const& str)
{
    enumType = magic_enum::enum_cast<T>(str).value();
}

}
*/

#define RegisterEnumNameSerialize(EnumType)                                                                                                     \
namespace cereal {                                                                                                                              \
    template <class Archive> inline std::string save_minimal(Archive&, const EnumType& h){ return std::string(magic_enum::enum_name(h)); }      \
    template <class Archive> inline void load_minimal(const Archive&, EnumType& enumType, const std::string& str){                              \
        enumType = magic_enum::enum_cast<EnumType>(str).value();                                                                                \
    }                                                                                                                                           \
}    

/*enum class TestEnum{ A, B, C };                                                                              
namespace cereal {                                                                                                                               
    template <class Archive> inline std::string save_minimal(Archive&, const TestEnum& h){ return std::string(magic_enum::enum_name(h)); }    
    template <class Archive> inline void load_minimal(Archive const&, TestEnum& enumType, std::string const& str){                     
        enumType = magic_enum::enum_cast<TestEnum>(str).value();                                                                
    }                                                                                                                           
}*/ 

namespace glm{

template<class Archive>
void serialize(Archive& archive, glm::vec2& v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y)
    );
}

template<class Archive>
void serialize(Archive& archive, glm::vec3& v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y), 
        CEREAL_NVP(v.z)
    );
}

template<class Archive>
void serialize(Archive& archive, glm::vec4& v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y), 
        CEREAL_NVP(v.z), 
        CEREAL_NVP(v.w)
    );
}

template<class Archive>
void serialize(Archive& archive, glm::quat& q){
    archive(
        CEREAL_NVP(q.x), 
        CEREAL_NVP(q.y), 
        CEREAL_NVP(q.z), 
        CEREAL_NVP(q.w)
    );
}

template<class Archive>
void serialize(Archive& archive, glm::ivec2& v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y)
    );
}

template<class Archive>
void serialize(Archive& archive, glm::ivec3& v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y), 
        CEREAL_NVP(v.z)
    );
}

template<class Archive>
void serialize(Archive& archive, glm::ivec4& v){
    archive(
        CEREAL_NVP(v.x), 
        CEREAL_NVP(v.y), 
        CEREAL_NVP(v.z), 
        CEREAL_NVP(v.w)
    );
}

template<class Archive>
void serialize(Archive& archive, glm::mat4& m){
    glm::vec4 col0 = m[0];
    glm::vec4 col1 = m[1];
    glm::vec4 col2 = m[2];
    glm::vec4 col3 = m[3];

    archive( 
        CEREAL_NVP(col0),
        CEREAL_NVP(col1),
        CEREAL_NVP(col2),
        CEREAL_NVP(col3) 
    );

    if constexpr (Archive::is_loading()){
        m[0] = col0;
        m[1] = col1;
        m[2] = col2;
        m[3] = col3;
    }
}

}

