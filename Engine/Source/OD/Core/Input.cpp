#include "Input.h"
#include "Math.h"
#include "Lua.h"
#include "OD/Base.h"
#include "OD/Core/Instrumentor.h"

namespace OD {

struct InputState{
    bool pressed;
    bool lastPressed;
};

InputState keysStates[KeyCodeMaxKeys];

/*static const std::vector<KeyCode> allKeys {
    KeyCode::Backspace,
    KeyCode::Enter,
    KeyCode::Tab,
    KeyCode::Shift,
    KeyCode::Control,
    KeyCode::Alt,
    KeyCode::Escape,
    KeyCode::Space,
    KeyCode::Left,
    KeyCode::Up,
    KeyCode::Right,
    KeyCode::Down,
    KeyCode::Alpha0,
    KeyCode::Alpha1,
    KeyCode::Alpha2,
    KeyCode::Alpha3,
    KeyCode::Alpha4,
    KeyCode::Alpha5,
    KeyCode::Alpha6,
    KeyCode::Alpha7,
    KeyCode::Alpha8,
    KeyCode::Alpha9,
    KeyCode::A,
    KeyCode::B,
    KeyCode::C,
    KeyCode::D,
    KeyCode::E,
    KeyCode::F,
    KeyCode::G,
    KeyCode::H,
    KeyCode::I,
    KeyCode::J,
    KeyCode::K,
    KeyCode::L,
    KeyCode::M,
    KeyCode::N,
    KeyCode::O,
    KeyCode::P,
    KeyCode::Q,
    KeyCode::R,
    KeyCode::S,
    KeyCode::T,
    KeyCode::U,
    KeyCode::V,
    KeyCode::W,
    KeyCode::X,
    KeyCode::Y,
    KeyCode::Z,
    KeyCode::F1,
    KeyCode::F2,
    KeyCode::F3,
    KeyCode::F4,
    KeyCode::F5,
    KeyCode::F6,
    KeyCode::F7,
    KeyCode::F8,
    KeyCode::F9,
    KeyCode::F10,
    KeyCode::F11,
    KeyCode::F12,
    KeyCode::LShift,
    KeyCode::RShift,
    KeyCode::LControl,
    KeyCode::RControl,
    KeyCode::LAlt,
    KeyCode::RAlt
};*/

InputState mouseButtonStates[MaxMouseButtons];

/*static const std::vector<MouseButton> allMouseButtons {
    MouseButton::Left,
    MouseButton::Right,
    MouseButton::Middle
};*/

static double lastMouseX = 0.0;
static double lastMouseY = 0.0;
static double mouseDeltaX = 0.0;
static double mouseDeltaY = 0.0;

bool Input::IsKeyDown(KeyCode key){
    return keysStates[(int)key].lastPressed == false && keysStates[(int)key].pressed == true;
}

bool Input::IsKeyUp(KeyCode key){
    return keysStates[(int)key].lastPressed == true && keysStates[(int)key].pressed == false;
}

Vector2 Input::GetMouseDelta() {
    return Vector2((float)mouseDeltaX, (float)mouseDeltaY);
}

void Input::Update(){
    //return;
    OD_PROFILE_SCOPE("Platform::Update");

    const std::vector<KeyCode> allKeys {
        KeyCode::Backspace,
        KeyCode::Enter,
        KeyCode::Tab,
        KeyCode::Shift,
        KeyCode::Control,
        KeyCode::Alt,
        KeyCode::Escape,
        KeyCode::Space,
        KeyCode::Delete,
        KeyCode::Left,
        KeyCode::Up,
        KeyCode::Right,
        KeyCode::Down,
        KeyCode::Alpha0,
        KeyCode::Alpha1,
        KeyCode::Alpha2,
        KeyCode::Alpha3,
        KeyCode::Alpha4,
        KeyCode::Alpha5,
        KeyCode::Alpha6,
        KeyCode::Alpha7,
        KeyCode::Alpha8,
        KeyCode::Alpha9,
        KeyCode::A,
        KeyCode::B,
        KeyCode::C,
        KeyCode::D,
        KeyCode::E,
        KeyCode::F,
        KeyCode::G,
        KeyCode::H,
        KeyCode::I,
        KeyCode::J,
        KeyCode::K,
        KeyCode::L,
        KeyCode::M,
        KeyCode::N,
        KeyCode::O,
        KeyCode::P,
        KeyCode::Q,
        KeyCode::R,
        KeyCode::S,
        KeyCode::T,
        KeyCode::U,
        KeyCode::V,
        KeyCode::W,
        KeyCode::X,
        KeyCode::Y,
        KeyCode::Z,
        KeyCode::F1,
        KeyCode::F2,
        KeyCode::F3,
        KeyCode::F4,
        KeyCode::F5,
        KeyCode::F6,
        KeyCode::F7,
        KeyCode::F8,
        KeyCode::F9,
        KeyCode::F10,
        KeyCode::F11,
        KeyCode::F12,
        KeyCode::LShift,
        KeyCode::RShift,
        KeyCode::LControl,
        KeyCode::RControl,
        KeyCode::LAlt,
        KeyCode::RAlt
    };
    
    const std::vector<MouseButton> allMouseButtons {
        MouseButton::Left,
        MouseButton::Right,
        MouseButton::Middle
    };

    //allKeys.resize(KeyCodeMaxKeys);

    //LogInfo("llKeys.size: %zd", allKeys.size() );

    Assert(allKeys.size() <= KeyCodeMaxKeys);
    Assert(allMouseButtons.size() <= MaxMouseButtons);
    
    for(auto& i: allKeys){
        keysStates[(int)i].lastPressed = keysStates[(int)i].pressed;
        keysStates[(int)i].pressed = IsKey(i);
    }

    for(auto& i: allMouseButtons){
        mouseButtonStates[(int)i].lastPressed = mouseButtonStates[(int)i].pressed;
        mouseButtonStates[(int)i].pressed = IsMouseButton(i);
    }

    double currentX, currentY;
    GetMousePosition(&currentX, &currentY);

    mouseDeltaX = currentX - lastMouseX;
    mouseDeltaY = currentY - lastMouseY;

    lastMouseX = currentX;
    lastMouseY = currentY;
}

bool Input::IsMouseButtonDown(MouseButton button){
    return mouseButtonStates[(int)button].lastPressed == false && mouseButtonStates[(int)button].pressed == true;
}

bool Input::IsMouseButtonUp(MouseButton button){
    return mouseButtonStates[(int)button].lastPressed == false && mouseButtonStates[(int)button].pressed == true;
}

void Input::CreateLuaBind(sol::state& lua){
    lua.new_enum(
        "MouseButton",
        "Left", MouseButton::Left,
        "Right", MouseButton::Right,
        "Middle", MouseButton::Middle
    );

    lua.new_enum(
        "KeyCode",
        "Backspace", KeyCode::Backspace,
        "Enter", KeyCode::Enter,
        "Tab", KeyCode::Tab,
        "Shift", KeyCode::Shift,
        "Control", KeyCode::Control,
        "Alt", KeyCode::Alt,
        "Escape", KeyCode::Escape,
        "Space", KeyCode::Space,
        "Left", KeyCode::Left,
        "Up", KeyCode::Up,
        "Right", KeyCode::Right,
        "Down", KeyCode::Down,
        "Alpha0", KeyCode::Alpha0,
        "Alpha1", KeyCode::Alpha1,
        "Alpha2", KeyCode::Alpha2,
        "Alpha3", KeyCode::Alpha3,
        "Alpha4", KeyCode::Alpha4,
        "Alpha5", KeyCode::Alpha5,
        "Alpha6", KeyCode::Alpha6,
        "Alpha7", KeyCode::Alpha7,
        "Alpha8", KeyCode::Alpha8,
        "Alpha9", KeyCode::Alpha9,
        "A", KeyCode::A,
        "B", KeyCode::B,
        "C", KeyCode::C,
        "D", KeyCode::D,
        "E", KeyCode::E,
        "F", KeyCode::F,
        "G", KeyCode::G,
        "H", KeyCode::H,
        "I", KeyCode::I,
        "J", KeyCode::J,
        "K", KeyCode::K,
        "L", KeyCode::L,
        "M", KeyCode::M,
        "N", KeyCode::N,
        "O", KeyCode::O,
        "P", KeyCode::P,
        "Q", KeyCode::Q,
        "R", KeyCode::R,
        "S", KeyCode::S,
        "T", KeyCode::T,
        "U", KeyCode::U,
        "V", KeyCode::V,
        "W", KeyCode::W,
        "X", KeyCode::X,
        "Y", KeyCode::Y,
        "Z", KeyCode::Z,
        "F1", KeyCode::F1,
        "F2", KeyCode::F2,
        "F3", KeyCode::F3,
        "F4", KeyCode::F4,
        "F5", KeyCode::F5,
        "F6", KeyCode::F6,
        "F7", KeyCode::F7,
        "F8", KeyCode::F8,
        "F9", KeyCode::F9,
        "F10", KeyCode::F10,
        "F11", KeyCode::F11,
        "F12", KeyCode::F12,
        "LShift", KeyCode::LShift,
        "RShift", KeyCode::RShift,
        "LControl", KeyCode::LControl,
        "RControl", KeyCode::RControl,
        "LAlt", KeyCode::LAlt,
        "RAlt", KeyCode::RAlt
    );

    lua.new_usertype<Input>(
        "Input",
        "IsKey", Input::IsKey,
        "IsKeyDown", Input::IsKeyDown,
        "IsKeyUp", Input::IsKeyUp,
        "IsMouseButton", Input::IsMouseButton,
        "IsMouseButtonDown", Input::IsMouseButtonDown,
        "IsMouseButtonUp", Input::IsMouseButtonUp,
        "GetMousePosition", [](){
            double x, y;
            Input::GetMousePosition(&x, &y);
            return Vector2(x, y);
        }
    );
}

}