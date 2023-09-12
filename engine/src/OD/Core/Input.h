#pragma once

#include "OD/Defines.h"

namespace OD {
    
enum class MouseButton {
    Left,
    Right,
    Middle
};
#define MaxMouseButtons 3

enum class KeyCode {
    Backspace = 259,
    Enter = 257,
    Tab = 258,
    Shift = 340,
    Control = 341,
    Alt = 0x11,
    Escape = 256,
    Space = 342,
    //8

    Left = 0x25,
    Up = 0x26,
    Right = 0x27,
    Down = 0x28,
    //4

    Alpha0 = 48,
    Alpha1 = 49,
    Alpha2 = 50,
    Alpha3 = 51,
    Alpha4 = 52,
    Alpha5 = 53,
    Alpha6 = 54,
    Alpha7 = 55,
    Alpha8 = 56,
    Alpha9 = 57,
    //10

    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,
    //26

    F1 = 290,
    F2 = 291,
    F3 = 292,
    F4 = 293,
    F5 = 294,
    F6 = 295,
    F7 = 296,
    F8 = 297,
    F9 = 298,
    F10 = 299,
    F11 = 300,
    F12 = 301,
    //12

    LShift = 340,
    RShift = 344,
    LControl = 341,
    RControl = 345,
    LAlt = 342,
    RAlt = 346,
    //6
};
#define KeyCodeMaxKeys 66

class Input {
public:
    // keyboard input
    static bool IsKey(KeyCode key);
    static bool IsMouseButton(MouseButton button);
    static void GetMousePosition(double* x, double* y);
};

}