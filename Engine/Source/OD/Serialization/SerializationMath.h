#pragma once
#include <cereal/cereal.hpp>
#include "OD/Core/Math.h"

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
