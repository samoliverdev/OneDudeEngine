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

    template <class T>
    void operator()(T& value, const char* label){
        data(value, label);
    }

    // PRIMITIVES
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    data(const T& v, const char* key) { m_current->insert_or_assign(key, v); }

    void data(const std::string& v, const char* key) { m_current->insert_or_assign(key, v); }
    void data(const char* v, const char* key) { m_current->insert_or_assign(key, std::string(v)); }

    // OBJECTS
    template<typename T>
    typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_same<T, std::string>::value, void>::type
    data(T& obj, const char* key){
        toml::table subtable;
        TomlOutputArchive child(subtable);
        obj.serialize(child);

        m_current->insert_or_assign(key, std::move(subtable));
    }

    // CONTAINERS
    template<typename T>
    void _data(std::vector<T>& vec, const char* key){
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

    template <class T>
    void operator()(T& value, const char* label){
        //m_cerealArchive(cereal::make_nvp(label, value));
        data(value, label);
    }

    // PRIMITIVES
    //template<typename T> void data(const T& v, const char* label) { m_cerealArchive(cereal::make_nvp(label, v)); }
    void data(float& v, const char* label) { m_cerealArchive(cereal::make_nvp(label, v)); }
    void data(int& v, const char* label) { m_cerealArchive(cereal::make_nvp(label, v)); }
    void data(std::string& v, const char* label) { m_cerealArchive(cereal::make_nvp(label, v)); }
    void data(char* v, const char* label) { std::string s(v); m_cerealArchive(cereal::make_nvp(label, s)); }

    // OBJECTS
    template<typename T>
    void data(T& obj, const char* label){
        //m_cerealArchive(cereal::make_nvp(label, obj));

        m_cerealArchive.setNextName(label);
        m_cerealArchive.startNode();
        obj.serialize(*this);
        m_cerealArchive.finishNode();
    }

    // CONTAINERS (std::vector)
    template<typename T>
    void data(std::vector<T>& vec, const char* label){
        //m_cerealArchive(cereal::make_nvp(label, vec));
        
        m_cerealArchive.setNextName(label);
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

    template <class T>
    void operator()(T& value, const char* label){
        data(value, label);
    }

    /*template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    value(const char* key, const T& v) {
        m_serializer->value(v);
    }*/

    void data(int& v, const char* label){
        m_serializer->value4b(v);
    }

    void data(float& v, const char* label){
        m_serializer->value4b(v);
    }

    void data(const std::string& v, const char* label){
        m_serializer->text1b(v, v.size());
    }

    void data(const char* v, const char* label){
        std::string s(v);
        m_serializer->text1b(s, s.size());
    }

    template<typename T>
    typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_same<T, std::string>::value, void>::type
    data(T& obj, const char* label){
        obj.serialize(*this);
    }

    // ---------------------------------------------------------
    // CONTAINERS (std::vector)
    // ---------------------------------------------------------

    template<typename T>
    void data(std::vector<T>& vec, const char* label){
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

namespace Dynamic{

#include <iostream>
#include <vector>
#include <string>

// =====================================================
// Base Archive
// =====================================================
class IArchive{
public:
    virtual ~IArchive() {}

    // Primitive types handled by virtual overloads
    virtual bool serializeValue(int& v, const char* name, const char* label) = 0;
    virtual bool serializeValue(float& v, const char* name, const char* label) = 0;
    virtual bool serializeValue(std::string& v, const char* name, const char* label) = 0;

    // Blocks
    virtual bool openBlock(const char* name, const char* label) = 0;
    virtual void closeBlock() = 0;

    ///////////////////////

    template<typename T>
    void operator()(T& value, const char* name, const char* label){
        serialize(*this, value, name, label);
    }
};

template<typename T>
bool serialize(IArchive& ar, T& value, const char* name, const char* label);

// =====================================================
// Generic Template Dispatcher
// =====================================================


// Primitive passthrough
inline bool serialize(IArchive& ar, int& v, const char* name, const char* label){
    return ar.serializeValue(v, name, label);
}

inline bool serialize(IArchive& ar, float& v, const char* name, const char* label){
    return ar.serializeValue(v, name, label);
}

inline bool serialize(IArchive& ar, std::string& v, const char* name, const char* label){
    return ar.serializeValue(v, name, label);
}

// =====================================================
// std::vector<T>
// =====================================================
template<typename T>
bool serialize(IArchive& ar, std::vector<T>& arr, const char* name, const char* label){
    if(!ar.openBlock(name, label))
        return false;

    for(size_t i = 0; i < arr.size(); ++i){
        std::string n = "item" + std::to_string(i);
        serialize(ar, arr[i], n.c_str(), n.c_str());
    }

    ar.closeBlock();
    return true;
}

// =====================================================
// Struct support
// =====================================================
template<typename T>
auto serializeStruct(IArchive& ar, T& v, const char* name, const char* label, int) -> decltype(v.Serialize(ar), bool()){
    if(!ar.openBlock(name, label)) return false;

    v.Serialize(ar);

    ar.closeBlock();
    return true;
}

template<typename T>
bool serialize(IArchive& ar, T& v, const char* name, const char* label){
    return serializeStruct(ar, v, name, label, 0);
}

// =====================================================
// PrintArchive – prints JSON-like output
// =====================================================
class PrintArchive : public IArchive
{
    int indent = 0;

    void pad()
    {
        for (int i = 0; i < indent; ++i) std::cout << "  ";
    }

public:
    bool serializeValue(int& v, const char* name, const char* label) override
    {
        pad();
        std::cout << name << " = " << v << "\n";
        return true;
    }

    bool serializeValue(float& v, const char* name, const char* label) override
    {
        pad();
        std::cout << name << " = " << v << "\n";
        return true;
    }

    bool serializeValue(std::string& v, const char* name, const char* label) override
    {
        pad();
        std::cout << name << " = \"" << v << "\"\n";
        return true;
    }

    bool openBlock(const char* name, const char* label) override
    {
        pad();
        std::cout << name << " {\n";
        indent++;
        return true;
    }

    void closeBlock() override
    {
        indent--;
        pad();
        std::cout << "}\n";
    }
};




}