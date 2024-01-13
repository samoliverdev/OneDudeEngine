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

#include <entt/entt.hpp>

#include "OD/Core/Math.h"

#define ODOutputArchive cereal::JSONOutputArchive
#define ODInputArchive cereal::JSONInputArchive

#define ODSnapshot entt::snapshot
//#define ODSnapshot entt::Snapshot<entt::registry>
#define ODSnapshotLoader entt::snapshot_loader

#define TYPE_TO_STRING(T) #T

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

namespace entt{

template<typename Registry>
class Snapshot {
    static_assert(!std::is_const_v<Registry>, "Non-const registry type required");
    using traits_type = typename Registry::traits_type;

public:
    using registry_type = Registry;
    using entity_type = typename registry_type::entity_type;

    Snapshot(const registry_type &source) noexcept
        : reg{&source} {}

    Snapshot(Snapshot &&) noexcept = default;
    Snapshot &operator=(Snapshot &&) noexcept = default;

    template<typename Type, typename Archive>
    const Snapshot &get(Archive &archive, const id_type id = type_hash<Type>::value()) const {
        if(const auto *storage = reg->template storage<Type>(id); storage) {
            archive(CEREAL_NVP( static_cast<typename traits_type::entity_type>(storage->size()) ));

            if constexpr(std::is_same_v<Type, entity_type>) {
                archive(CEREAL_NVP( static_cast<typename traits_type::entity_type>(storage->in_use()) ));

                for(auto first = storage->data(), last = first + storage->size(); first != last; ++first) {
                    archive(CEREAL_NVP( *first ));
                }
            } else {
                for(auto elem: storage->reach()) {
                    std::apply([&archive](auto &&...args) { (archive(cereal::make_nvp(TYPE_TO_STRING(Type), std::forward<decltype(args)>(args) )), ...); }, elem);
                }
            }
        } else {
            archive(CEREAL_NVP( typename traits_type::entity_type{} ));
        }

        return *this;
    }

    template<typename Type, typename Archive, typename It>
    const Snapshot &get(Archive &archive, It first, It last, const id_type id = type_hash<Type>::value()) const {
        static_assert(!std::is_same_v<Type, entity_type>, "Entity types not supported");

        if(const auto *storage = reg->template storage<Type>(id); storage && !storage->empty()) {
            archive(CEREAL_NVP( static_cast<typename traits_type::entity_type>(std::distance(first, last)) ));

            for(; first != last; ++first) {
                if(const auto entt = *first; storage->contains(entt)) {
                    archive(entt);
                    std::apply([&archive](auto &&...args) { (archive(CEREAL_NVP( std::forward<decltype(args)>(args) )), ...); }, storage->get_as_tuple(entt));
                } else {
                    archive(CEREAL_NVP( static_cast<entity_type>(null) ));
                }
            }
        } else {
            archive(typename traits_type::entity_type{});
        }

        return *this;
    }

private:
    const registry_type *reg;
};

}
