#include "Input.h"
#include <vector>

namespace OD {

struct InputState{
    bool pressed;
    bool lastPressed;
};

InputState keysStates[KeyCodeMaxKeys];

std::vector<KeyCode> allKeys = {
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
};

InputState mouseButtonStates[MaxMouseButtons];
std::vector<MouseButton> allMouseButtons = {
    MouseButton::Left,
    MouseButton::Right,
    MouseButton::Middle
};

bool Input::IsKeyDown(KeyCode key){
    return keysStates[(int)key].lastPressed == false && keysStates[(int)key].pressed == true;
}

bool Input::IsKeyUp(KeyCode key){
    return keysStates[(int)key].lastPressed == true && keysStates[(int)key].pressed == false;
}

void Input::Update(){
    for(auto i: allKeys){
        keysStates[(int)i].lastPressed = keysStates[(int)i].pressed;
        keysStates[(int)i].pressed = IsKey(i);
    }

    for(auto i: allMouseButtons){
        mouseButtonStates[(int)i].lastPressed = mouseButtonStates[(int)i].pressed;
        mouseButtonStates[(int)i].pressed = IsMouseButton(i);
    }
}

bool Input::IsMouseButtonDown(MouseButton button){
    return mouseButtonStates[(int)button].lastPressed == false && mouseButtonStates[(int)button].pressed == true;
}

bool Input::IsMouseButtonUp(MouseButton button){
    return mouseButtonStates[(int)button].lastPressed == false && mouseButtonStates[(int)button].pressed == true;
}

}