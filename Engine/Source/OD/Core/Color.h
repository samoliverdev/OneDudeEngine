#pragma once
#include "OD/Defines.h"
#include "Math.h"
#include "OD/Serialization/Serialization.h"
//#include "OD/Core/Lua.h"

namespace sol{ class state; }

namespace OD{

struct OD_API Color{
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;
    //bool hdr = false;

    Color() = default;
    constexpr Color(float r, float g, float b):r(r),g(g),b(b),a(1.0f){}
    constexpr Color(float r, float g, float b, float a):r(r),g(g),b(b),a(a){}
    constexpr Color(Vector3 v):r(v.r),g(v.r),b(v.b),a(1.0f){}
    constexpr Color(Vector4 v):r(v.r),g(v.r),b(v.b),a(v.a){}

    static const Color Black;
    static const Color Blue;
    static const Color Clear;
    static const Color Cyan;
    static const Color Gray;
    static const Color Green;
    static const Color Grey;
    static const Color Magenta;
    static const Color Red;
    static const Color White;
    static const Color Yellow;

    inline operator Vector4(){ return Vector4(r, g, b, a); }
    inline operator Vector3(){ return Vector3(r, g, b); }

    inline Color Linear(){ return (Color)math::pow((Vector4)*this, Vector4(2.2f, 2.2f, 2.2f, 1)); }
    inline static Color Lerp(const Color& a, const Color& b, float t){
        return Color(
            math::mix(a.r, b.r, t),
            math::mix(a.g, b.g, t),
            math::mix(a.b, b.b, t),
            math::mix(a.a, b.a, t)
        );
    }
    
    static void CreateLuaBind(sol::state& lua);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(r));
        ArchiveDump(ar, CEREAL_NVP(g));
        ArchiveDump(ar, CEREAL_NVP(b));
        ArchiveDump(ar, CEREAL_NVP(a));
    }
};

constexpr Color operator*(const Color& a, const Color& b){
    return {a.r*b.r, a.g*b.g, a.b*b.b, a.a*b.a};
}

constexpr Color operator*(const Color& a, float& b){
    return {a.r*b, a.g*b, a.b*b, a.a*b};
}

}