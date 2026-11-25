#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
//#include "Lua.h"

namespace sol{ class state; }

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
    Alt = 342,
    Escape = 256,
    Space = 32,
    //8

    Delete = 261,

    Left = 263,
    Up = 265,
    Right = 262,
    Down = 264,
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
#define KeyCodeMaxKeys 348

enum class GamepadButton{
    A, B, X, Y,
    LeftBumper, RightBumper, Back, Start, Guide,
    LeftStick, RightStick, DpadUp, DpadRight, DpadDown, DpadLeft,
    LeftTrigger, RightTrigger, Count
};

class Application;

class OD_API Input{
    friend class Application;
public:
    // keyboard input
    static bool IsKey(KeyCode key);
    static bool IsKeyDown(KeyCode key);
    static bool IsKeyUp(KeyCode key);

    static bool IsMouseButton(MouseButton button);
    static bool IsMouseButtonDown(MouseButton button);
    static bool IsMouseButtonUp(MouseButton button);

    static void GetMousePosition(double* x, double* y);
    static Vector2 GetMouseWheelMove();

    static Vector2 GetMouseDelta();

    // Controller slots are zero-based. By default, queries use the primary gamepad.
    static constexpr int MaxGamepads = 16;
    static constexpr int PrimaryGamepad = -1;

    static int GetPrimaryGamepad(); // Returns -1 when no gamepad is connected.
    static bool HasGamepad(int device = PrimaryGamepad);
    static bool IsGamepadButton(GamepadButton button, int device = PrimaryGamepad);
    static bool IsGamepadButtonDown(GamepadButton button, int device = PrimaryGamepad);
    static bool IsGamepadButtonUp(GamepadButton button, int device = PrimaryGamepad);
    // Stick Y is positive upwards.
    static Vector2 GetGamepadLeftStick(int device = PrimaryGamepad);
    static Vector2 GetGamepadRightStick(int device = PrimaryGamepad);

    static void CreateLuaBind(sol::state& lua);

private:
    static void Update();
    static void PostUpdate();

    struct GamepadState{
        bool connected = false;
        bool buttons[static_cast<int>(GamepadButton::Count)]{};
        Vector2 leftStick{0, 0};
        Vector2 rightStick{0, 0};
    };

    static GamepadState ReadGamepad(int device);
    static const GamepadState& GetGamepadState(int device, bool previous = false);
    static GamepadState gamepads[MaxGamepads];
    static GamepadState previousGamepads[MaxGamepads];
    static int primaryGamepad;
    static int previousPrimaryGamepad;
};

}