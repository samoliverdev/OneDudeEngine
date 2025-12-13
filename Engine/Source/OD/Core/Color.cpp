#include "OD/pch.h"
#include "Color.h"
#include "Lua.h"

namespace OD{

const Color Color::Black = {0, 0, 0, 1};
const Color Color::Blue = {0, 0, 1, 1};
const Color Color::Clear = {0, 0, 0, 0};
const Color Color::Cyan = {0, 1, 1, 1};
const Color Color::Gray = {0.5f, 0.5f, 0.5f, 1};
const Color Color::Green = {0, 1, 0, 1};
const Color Color::Grey = {0.5, 0.5, 0.5, 1};
const Color Color::Magenta = {1, 0, 1, 1};
const Color Color::Red = {1, 0, 0, 1};
const Color Color::White = {1, 1, 1, 1};
const Color Color::Yellow = {1, 0.92, 0.016, 1};

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