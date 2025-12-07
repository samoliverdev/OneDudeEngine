#pragma once

#include <toml++/toml.h>

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

#include <bitsery/bitsery.h>
#include <bitsery/brief_syntax.h>
#include <bitsery/adapter/stream.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/core/std_defaults.h>

#include <vector>
#include <string>
#include <type_traits>

namespace Static{

class TomlOutputArchive{
public:
    explicit TomlOutputArchive(toml::table& tbl):m_current(&tbl){}

    // PRIMITIVES
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    value(const char* key, const T& v) { m_current->insert_or_assign(key, v); }

    void value(const char* key, const std::string& v) { m_current->insert_or_assign(key, v); }
    void value(const char* key, const char* v) { m_current->insert_or_assign(key, std::string(v)); }

    // OBJECTS
    template<typename T>
    typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_same<T, std::string>::value, void>::type
    object(const char* key, T& obj){
        toml::table subtable;
        TomlOutputArchive child(subtable);
        obj.serialize(child);

        m_current->insert_or_assign(key, std::move(subtable));
    }

    // CONTAINERS
    template<typename T>
    void container(const char* key, std::vector<T>& vec){
        toml::array arr;
        for(auto& v : vec){
            if constexpr (std::is_arithmetic<T>::value || std::is_same<T, std::string>::value){
                arr.push_back(v);
            } else {
                toml::table subtable;
                TomlOutputArchive child(subtable);
                v.serialize(child);
                arr.push_back(std::move(subtable));
            }
        }
        m_current->insert_or_assign(key, std::move(arr));
    }

private:
    toml::table* m_current;
};

class CerealOutputArchive{
public:
    explicit CerealOutputArchive(std::ostream& os):m_os(os), m_cerealArchive(os){}

    // PRIMITIVES
    template<typename T>
    void value(const char* key, const T& v) { m_cerealArchive(cereal::make_nvp(key, v)); }
    void value(const char* key, const std::string& v) { m_cerealArchive(cereal::make_nvp(key, v)); }
    void value(const char* key, const char* v) { std::string s(v); m_cerealArchive(cereal::make_nvp(key, s)); }

    // OBJECTS
    template<typename T>
    void object(const char* key, T& obj){
        m_cerealArchive.setNextName(key);
        m_cerealArchive.startNode();

        obj.serialize(*this);

        m_cerealArchive.finishNode();
    }

    // CONTAINERS (std::vector)
    template<typename T>
    void container(const char* key, std::vector<T>& vec){
        m_cerealArchive.setNextName(key);
        m_cerealArchive.startNode();

        int idx = 0;
        for(auto& v : vec){
            // Name each element as value0, value1, ...
            std::string elemName = "value" + std::to_string(idx++);
            if constexpr (std::is_arithmetic<T>::value || std::is_same<T, std::string>::value){
                m_cerealArchive(cereal::make_nvp(elemName.c_str(), v));
            } else {
                object(elemName.c_str(), v);
            }
        }

        m_cerealArchive.finishNode();
    }

private:
    //bool isRoot = true;
    std::ostream& m_os;
    cereal::JSONOutputArchive m_cerealArchive;
};

class BitseryOutputArchive {
public:
    BitseryOutputArchive(std::ostream& os){
        m_serializer = new bitsery::Serializer<bitsery::OutputBufferedStreamAdapter>{os};
    }

    ~BitseryOutputArchive(){
        delete m_serializer;
        //delete m_adapter;
    }

    /*template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    value(const char* key, const T& v) {
        m_serializer->value(v);
    }*/

    void value(const char* key, int& v){
        m_serializer->value4b(v);
    }

    void value(const char* key, float& v){
        m_serializer->value4b(v);
    }

    void value(const char* key, const std::string& v){
        m_serializer->text1b(v, v.size());
    }

    void value(const char* key, const char* v){
        std::string s(v);
        m_serializer->text1b(s, s.size());
    }

    template<typename T>
    typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_same<T, std::string>::value, void>::type
    object(const char* key, T& obj){
        obj.serialize(*this);
    }

    // ---------------------------------------------------------
    // CONTAINERS (std::vector)
    // ---------------------------------------------------------

    template<typename T>
    void container(const char* key, std::vector<T>& vec){
        // Write the vector size
        m_serializer->value8b(vec.size());

        // Serialize each element
        for(auto& v : vec){
            if constexpr (std::is_arithmetic<T>::value){
                m_serializer->value(v);
            } else if constexpr (std::is_same<T, std::string>::value){
                m_serializer->text1b(v, v.size());
            } else {
                object(nullptr, v);
            }
        }
    }

    // ---------------------------------------------------------
    // FINALIZE
    // ---------------------------------------------------------

    void flush(){
        m_serializer->adapter().flush();
    }

private:
    bitsery::Serializer<bitsery::OutputBufferedStreamAdapter>* m_serializer;
};

/*class BitseryInputArchive {
public:
    BitseryInputArchive(std::istream& os){
        m_serializer = new bitsery::Serializer<bitsery::InputStreamAdapter>{os};
    }

    ~BitseryInputArchive(){
        delete m_serializer;
        //delete m_adapter;
    }

    void value(const char* key, int& v){
        m_serializer->value4b(v);
    }

    void value(const char* key, float& v){
        m_serializer->value4b(v);
    }

    void value(const char* key, const std::string& v){
        m_serializer->text1b(v, 128);
    }

    void value(const char* key, const char* v){
        std::string s(v);
        m_serializer->text1b(s, 128);
    }

    template<typename T>
    typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_same<T, std::string>::value, void>::type
    object(const char* key, T& obj){
        obj.serialize(*this);
    }

    template<typename T>
    void container(const char* key, std::vector<T>& vec){
        // Write the vector size
        size_t size;
        m_serializer->value8b(size);
        vec.resize(0);

        // Serialize each element
        for(auto& v : vec){
            if constexpr (std::is_arithmetic<T>::value) {
                m_serializer->value(v);
            } else if constexpr (std::is_same<T, std::string>::value){
                m_serializer->text1b(v, 128);
            }
            else {
                object(nullptr, v);
            }
        }
    }

private:
    bitsery::Serializer<bitsery::InputStreamAdapter>* m_serializer;
};*/

/////////////////////////////////


}