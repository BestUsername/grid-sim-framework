#ifndef LIBIO_KEYCODES_HPP
#define LIBIO_KEYCODES_HPP

namespace io {

/**
 * @brief Device-agnostic key codes.
 *
 * A unified set of key identifiers that abstracts over backend-specific
 * scancodes (SDL, ncurses, Linux input.h). Backend implementations map
 * their native codes to these values.
 */
enum class Key {
    Unknown = 0,

    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers (top row)
    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,

    // Function keys
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,

    // Navigation
    Up, Down, Left, Right,
    Home, End, PageUp, PageDown,
    Insert, Delete,

    // Whitespace / editing
    Space, Tab, Enter, Backspace, Escape,

    // Modifiers
    LeftShift, RightShift,
    LeftCtrl, RightCtrl,
    LeftAlt, RightAlt,
    LeftSuper, RightSuper,

    // Punctuation / symbols
    Comma, Period, Slash, Backslash,
    Semicolon, Apostrophe,
    LeftBracket, RightBracket,
    Minus, Equals, Grave,

    // Lock keys
    CapsLock, NumLock, ScrollLock,

    // Numpad
    KP0, KP1, KP2, KP3, KP4,
    KP5, KP6, KP7, KP8, KP9,
    KPDecimal, KPDivide, KPMultiply,
    KPSubtract, KPAdd, KPEnter,

    // Misc
    PrintScreen, Pause, Menu,

    Count  // number of entries — keep last
};

/**
 * @brief Modifier flags (bitmask).
 */
enum class Modifier : unsigned {
    None  = 0,
    Shift = 1 << 0,
    Ctrl  = 1 << 1,
    Alt   = 1 << 2,
    Super = 1 << 3,
    Caps  = 1 << 4,
    Num   = 1 << 5,
};

inline Modifier operator|(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}
inline Modifier operator&(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<unsigned>(a) & static_cast<unsigned>(b));
}
inline bool hasModifier(Modifier set, Modifier flag) {
    return (static_cast<unsigned>(set) & static_cast<unsigned>(flag)) != 0;
}

/**
 * @brief Mouse button identifiers.
 */
enum class MouseButton {
    Unknown = 0,
    Left,
    Middle,
    Right,
    X1,
    X2,
    Count
};

/**
 * @brief Gamepad button identifiers (modeled after a standard dual-stick controller).
 */
enum class GamepadButton {
    Unknown = 0,
    A, B, X, Y,               // Face buttons (ABXY / Cross, Circle, Square, Triangle)
    LeftBumper, RightBumper,   // Shoulder buttons
    Back, Start, Guide,        // Center cluster
    LeftStick, RightStick,     // Stick clicks
    DPadUp, DPadDown,
    DPadLeft, DPadRight,
    Count
};

/**
 * @brief Gamepad axis identifiers.
 */
enum class GamepadAxis {
    Unknown = 0,
    LeftX, LeftY,
    RightX, RightY,
    LeftTrigger, RightTrigger,
    Count
};

} // namespace io

#endif // LIBIO_KEYCODES_HPP
