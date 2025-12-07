// TomlCerealArchive.h
#pragma once

// Requires:
//  - cereal (https://github.com/USCiLab/cereal)
//  - toml++ (https://github.com/marzer/tomlplusplus)
//
// Build: C++17, MSVC 2019/2022 tested.

#include <cereal/cereal.hpp>
#include <cereal/details/traits.hpp>
#include <cereal/types/array.hpp>
#include <toml++/toml.h>

#include <type_traits>
#include <string>
#include <utility>

namespace cereal {
    
class TomlOutputArchive : public cereal::OutputArchive<TomlOutputArchive>{
public:
    explicit TomlOutputArchive(toml::table& root) :
        cereal::OutputArchive<TomlOutputArchive>(this),
        m_current(&root)
    {}

    // routing + helpers (declared here; defined below out-of-line)
    template <class T>
    void saveNode(const char* key, const T& value){
        _save(key, value);
    }

private:

    //template<class T, typename = std::enable_if_t<!std::is_enum_v<T>>>
    template<class T, typename = cereal::traits::is_output_serializable<T, TomlOutputArchive>>
    void _save(const char* key, const T& v){
        toml::table nested;
        {
            TomlOutputArchive child(nested);
            child(v);
        }
        m_current->insert_or_assign(key, std::move(nested));
    }

    /*template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, void>
    _save(const char* key, const T& v){
        //m_current->insert_or_assign(key, v);
    }*/

    void _save(const char* key, const int& v){
        m_current->insert_or_assign(key, v);
    }

    void _save(const char* key, const float& v){
        m_current->insert_or_assign(key, v);
    }

    void _save(const char* key, const std::string& v){
        m_current->insert_or_assign(key, v);
    }

    void _save(const char* key, const char* v){
        m_current->insert_or_assign(key, std::string(v));
    }

    // pointer to current table where we write values (we don't mutate it while
    // serializing nested objects; instead we build a nested table first and then insert it)
    toml::table* m_current;
};

template <class T> inline
void CEREAL_SAVE_FUNCTION_NAME(TomlOutputArchive& ar, const NameValuePair<T>& t){
    ar.saveNode<T>(t.name, t.value);
}

template <class T, traits::EnableIf<std::is_arithmetic<T>::value> = traits::sfinae> inline
void CEREAL_SAVE_FUNCTION_NAME(TomlOutputArchive& ar, const T& t){
    ar.saveNode<T>("value", t);
}

class TomlOutputArchive2{
public:
    TomlOutputArchive2(toml::table& root): m_current(&root){}

    // routing + helpers (declared here; defined below out-of-line)
    template <class T>
    void operator()(const NameValuePair<T>& t){
        _save(t.name, t.value);
    }

    template <class T>
    void operator()(const T& value){
        _save("value", value);
    }

private:

    //template<class T, typename = std::enable_if_t<!std::is_enum_v<T>>>
    template<class T, typename = cereal::traits::is_output_serializable<T, TomlOutputArchive>>
    void _save(const char* key, const T& v){
        toml::table nested;
        {
            TomlOutputArchive child(nested);
            child(v);
        }
        m_current->insert_or_assign(key, std::move(nested));
    }

    /*template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, void>
    _save(const char* key, const T& v){
        //m_current->insert_or_assign(key, v);
    }*/

    void _save(const char* key, const int& v){
        m_current->insert_or_assign(key, v);
    }

    void _save(const char* key, const float& v){
        m_current->insert_or_assign(key, v);
    }

    void _save(const char* key, const std::string& v){
        m_current->insert_or_assign(key, v);
    }

    void _save(const char* key, const char* v){
        m_current->insert_or_assign(key, std::string(v));
    }

    // pointer to current table where we write values (we don't mutate it while
    // serializing nested objects; instead we build a nested table first and then insert it)
    toml::table* m_current;
};

/*template <class T>
void TomlOutputArchive::saveNode(const char* key, const T& value)
{
    
    if constexpr (std::is_arithmetic<T>::value)
    {
        savePrimitive(key, value);
    }
    else if constexpr (std::is_same<T, std::string>::value)
    {
        savePrimitive(key, value);
    }
    else if constexpr (cereal::traits::is_sequence_container<T>::value)
    {
        saveArray(key, value);
    }
    else if constexpr (cereal::traits::is_output_serializable<T, TomlOutputArchive>::value)
    {
        saveTable(key, value);
    }
    else
    {
        static_assert(cereal::traits::is_output_serializable<T, TomlOutputArchive>::value,
            "TomlOutputArchive: type is not serializable (no cereal serialize/save found)");
    }
}

// primitives
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value, void>
TomlOutputArchive::savePrimitive(const char* key, const T& v)
{
    // use the API that takes a built value (no brace-init inside templates)
    m_current->insert_or_assign(key, v);
}

inline void TomlOutputArchive::savePrimitive(const char* key, const std::string& v)
{
    m_current->insert_or_assign(key, v);
}

inline void TomlOutputArchive::savePrimitive(const char* key, const char* v)
{
    m_current->insert_or_assign(key, std::string(v));
}

// router: MSVC-safe because defined after the class

// arrays / sequence containers
template <class Container>
void TomlOutputArchive::saveArray(const char* key, const Container& container)
{
    toml::array arr;

    using Elem = typename Container::value_type;

    for (auto const& e : container)
    {
        if constexpr (std::is_arithmetic<Elem>::value)
        {
            arr.push_back(e);
        }
        else if constexpr (std::is_same<Elem, std::string>::value)
        {
            arr.push_back(e);
        }
        else
        {
            // build nested table for complex element
            toml::table nested;
            TomlOutputArchive child(nested);
            // serialize element into nested table
            child(e);
            arr.push_back(std::move(nested));
        }
    }

    m_current->insert_or_assign(key, std::move(arr));
}

// user-defined object -> nested table
template <class Obj>
void TomlOutputArchive::saveTable(const char* key, const Obj& obj)
{
    // build nested table first (MSVC-safe)
    toml::table nested;
    {
        TomlOutputArchive child(nested);
        child(obj); // fills nested table
    }

    // now insert fully-formed nested table into parent
    m_current->insert_or_assign(key, std::move(nested));
}*/

}