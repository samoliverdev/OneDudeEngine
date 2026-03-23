#if 0
#pragma once
#include <magic_enum/magic_enum.hpp>
#include <cereal/cereal.hpp>

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

//TODO: Review this i think this is not working
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
#endif