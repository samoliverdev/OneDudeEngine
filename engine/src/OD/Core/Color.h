#pragma once
#include "OD/Defines.h"
#include "Math.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

struct OD_API Color{
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;
    //bool hdr = false;

    /*static inline const Color Black = {0, 0, 0, 1};
    static inline const Color Blue = {0, 0, 1, 1};
    static inline const Color Clear = {0, 0, 0, 0};
    static inline const Color Cyan = {0, 1, 1, 1};
    static inline const Color Gray = {0.5f, 0.5f, 0.5f, 1};
    static inline const Color Green = {0, 1, 0, 1};
    static inline const Color Grey = {0.5, 0.5, 0.5, 1};
    static inline const Color Magenta = {1, 0, 1, 1};
    static inline const Color Red = {1, 0, 0, 1};
    static inline const Color White = {1, 1, 1, 1};
    static inline const Color Yellow = {1, 0.92, 0.016, 1};*/

    inline operator Vector4(){
        return Vector4(r, g, b, a);
    }

    inline operator Vector3(){
        return Vector3(r, g, b);
    }

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(r));
        ArchiveDump(ar, CEREAL_NVP(g));
        ArchiveDump(ar, CEREAL_NVP(b));
        ArchiveDump(ar, CEREAL_NVP(a));
    }
};

constexpr Color operator*(Color& a, Color& b){
    return Color{a.r*b.r, a.g*b.g, a.b*b.b, a.a*b.a};
}

constexpr Color operator*(Color& a, float& b){
    return Color{a.r*b, a.g*b, a.b*b, a.a*b};
}

}