#include "Color.h"
#include "Lua.h"

namespace OD{

void Color::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Color>(
        "Color",
        sol::call_constructor,
        sol::factories(
            [](){ return Color(); },
            [](float r, float g, float b){ return Color{r, g, b}; },
            [](float r, float g, float b, float a){ return Color{r, g, b, a}; }
        ),
        "r", &Color::r,
        "g", &Color::g,
        "b", &Color::b,
        "a", &Color::a,
        sol::meta_function::multiplication, 
        sol::overload(
            [](const Color& a, const Color& b){ return a * b; },
            [](const Color& a, float b){ return a * b; }
        )
    );
}

}