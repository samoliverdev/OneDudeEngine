#pragma once
#include "OD/Defines.h"
#include <array>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace OD{

enum class Format { Json, Binary };
enum class Mode { Read, Write };

class Serializer;

// Optional adapters are specialized in Glm.hpp and Enum.hpp.
template<class T> struct Adapter;

// ADL discovers free functions in the namespace associated with T.
// No fallback serialize/save_minimal/load_minimal is provided intentionally.
namespace detail {
template<class...> struct always_false : std::false_type {};

template<class T, class = void> struct has_free_serialize : std::false_type {};
template<class T> struct has_free_serialize<T, std::void_t<decltype(serialize(std::declval<Serializer&>(), std::declval<T&>()))>> : std::true_type {};

template<class T, class = void> struct has_member_serialize : std::false_type {};
template<class T> struct has_member_serialize<T, std::void_t<decltype(std::declval<T&>().Serialize(std::declval<Serializer&>()))>> : std::true_type {};

template<class T, class = void> struct has_adapter : std::false_type {};
template<class T> struct has_adapter<T, std::void_t<decltype(Adapter<T>::Apply(std::declval<Serializer&>(),std::declval<const char*>(), std::declval<T&>()))>> : std::true_type {};

template<class T, class = void> struct has_save_minimal : std::false_type {};
template<class T> struct has_save_minimal<T, std::void_t<decltype(save_minimal(std::declval<const Serializer&>(), std::declval<const T&>()))>> : std::bool_constant<std::is_convertible_v<decltype(save_minimal(std::declval<const Serializer&>(), std::declval<const T&>())),std::string>> {};

template<class T, class = void> struct has_load_minimal : std::false_type {};
template<class T> struct has_load_minimal<T, std::void_t<decltype(load_minimal(std::declval<const Serializer&>(), std::declval<T&>(), std::declval<const std::string&>()))>> : std::true_type {};
} // namespace detail

class OD_API Serializer{
public:
    explicit Serializer(Mode mode);
    ~Serializer();
    Serializer(Serializer&&) noexcept;
    Serializer& operator=(Serializer&&) noexcept;
    Serializer(const Serializer&) = delete;
    Serializer& operator=(const Serializer&) = delete;

    // On write: Field(...) then Save(...). On read: Load(...) then Field(...).
    void Save(std::ostream& stream, Format format) const;
    void Load(std::istream& stream, Format format);

    void Field(const char* name, bool& value);
    void Field(const char* name, std::string& value);
    void Field(const char* name, float& value);
    void Field(const char* name, double& value);
    void Field(const char* name, std::int8_t& value);
    void Field(const char* name, std::uint8_t& value);
    void Field(const char* name, std::int16_t& value);
    void Field(const char* name, std::uint16_t& value);
    void Field(const char* name, std::int32_t& value);
    void Field(const char* name, std::uint32_t& value);
    void Field(const char* name, std::int64_t& value);
    void Field(const char* name, std::uint64_t& value);

    // Priority: free minimal pair > adapter > member Serialize > free serialize.
    // Templates here only dispatch; archive code stays in Serializer.cpp.
    template<class T>
    void Field(const char* name, T& value) {
        if constexpr(detail::has_save_minimal<T>::value && detail::has_load_minimal<T>::value) {
            std::string text;
            if(IsWriting()) text = save_minimal(static_cast<const Serializer&>(*this), static_cast<const T&>(value));
            Field(name, text);
            if(IsReading()) load_minimal(static_cast<const Serializer&>(*this), value, text);
        } else if constexpr (detail::has_adapter<T>::value) {
            Adapter<T>::Apply(*this, name, value);
        } else if constexpr (detail::has_member_serialize<T>::value || detail::has_free_serialize<T>::value) {
            BeginObject(name);
            try{
                if constexpr (detail::has_member_serialize<T>::value)
                    value.Serialize(*this);
                else
                    serialize(*this, value); // found via ADL
            } catch(...){ End(); throw; }
            End();
        } else {
            static_assert(
                detail::always_false<T>::value,
                "No serialization available: define Serialize(Serializer&), "
                "free serialize(Serializer&, T&), an Adapter<T>, or "
                "free save_minimal/load_minimal pair"
            );
        }
    }

    template<class T>
    void Field(const char* name, std::vector<T>& values) {
        static_assert(!std::is_same_v<T, bool>, "std::vector<bool> uses proxy references; not supported");
        const std::size_t count = BeginArray(name, values.size());
        try{
            if(IsReading()) values.resize(count);
            for(auto& item : values) Field("", item);
        } catch(...){ End(); throw; }
        End();
    }

    template<class T, std::size_t N>
    void Field(const char* name, std::array<T, N>& values) {
        const std::size_t count = BeginArray(name, N);
        try{
            if(count != N) throw std::runtime_error("Fixed array length mismatch");
            for(auto& item : values) Field("", item);
        } catch(...){ End(); throw; }
        End();
    }

    bool IsReading() const noexcept;
    bool IsWriting() const noexcept { return !IsReading(); }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void BeginObject(const char* name);
    std::size_t BeginArray(const char* name, std::size_t count);
    void End() noexcept;
};

template<typename T>
bool TrySerialize(Serializer& s, const char* name, T& value){
    try{
        s.Field(name, value);
        return true;
    } catch(const std::exception& e){
        LogWarning("Serialization error '{}': {}", name, e.what());
        return false;
    }
}

#define ArchiveDump2(s, data) OD::TrySerialize(s, #data, data)
#define ArchiveDumpNVP2(s, data) OD::TrySerialize(s, #data, data)
#define ArchiveDumpNamed2(s, name, data) OD::TrySerialize(s, name, data)

} // namespace compactser
