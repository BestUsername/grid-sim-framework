#include "libio/sdl_keymap.hpp"

#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_gamecontroller.h>

namespace io::sdl {

Key mapScancode(int sc) {
    switch (sc) {
        // Letters
        case SDL_SCANCODE_A: return Key::A;
        case SDL_SCANCODE_B: return Key::B;
        case SDL_SCANCODE_C: return Key::C;
        case SDL_SCANCODE_D: return Key::D;
        case SDL_SCANCODE_E: return Key::E;
        case SDL_SCANCODE_F: return Key::F;
        case SDL_SCANCODE_G: return Key::G;
        case SDL_SCANCODE_H: return Key::H;
        case SDL_SCANCODE_I: return Key::I;
        case SDL_SCANCODE_J: return Key::J;
        case SDL_SCANCODE_K: return Key::K;
        case SDL_SCANCODE_L: return Key::L;
        case SDL_SCANCODE_M: return Key::M;
        case SDL_SCANCODE_N: return Key::N;
        case SDL_SCANCODE_O: return Key::O;
        case SDL_SCANCODE_P: return Key::P;
        case SDL_SCANCODE_Q: return Key::Q;
        case SDL_SCANCODE_R: return Key::R;
        case SDL_SCANCODE_S: return Key::S;
        case SDL_SCANCODE_T: return Key::T;
        case SDL_SCANCODE_U: return Key::U;
        case SDL_SCANCODE_V: return Key::V;
        case SDL_SCANCODE_W: return Key::W;
        case SDL_SCANCODE_X: return Key::X;
        case SDL_SCANCODE_Y: return Key::Y;
        case SDL_SCANCODE_Z: return Key::Z;

        // Numbers
        case SDL_SCANCODE_0: return Key::Num0;
        case SDL_SCANCODE_1: return Key::Num1;
        case SDL_SCANCODE_2: return Key::Num2;
        case SDL_SCANCODE_3: return Key::Num3;
        case SDL_SCANCODE_4: return Key::Num4;
        case SDL_SCANCODE_5: return Key::Num5;
        case SDL_SCANCODE_6: return Key::Num6;
        case SDL_SCANCODE_7: return Key::Num7;
        case SDL_SCANCODE_8: return Key::Num8;
        case SDL_SCANCODE_9: return Key::Num9;

        // Function keys
        case SDL_SCANCODE_F1:  return Key::F1;
        case SDL_SCANCODE_F2:  return Key::F2;
        case SDL_SCANCODE_F3:  return Key::F3;
        case SDL_SCANCODE_F4:  return Key::F4;
        case SDL_SCANCODE_F5:  return Key::F5;
        case SDL_SCANCODE_F6:  return Key::F6;
        case SDL_SCANCODE_F7:  return Key::F7;
        case SDL_SCANCODE_F8:  return Key::F8;
        case SDL_SCANCODE_F9:  return Key::F9;
        case SDL_SCANCODE_F10: return Key::F10;
        case SDL_SCANCODE_F11: return Key::F11;
        case SDL_SCANCODE_F12: return Key::F12;

        // Navigation
        case SDL_SCANCODE_UP:       return Key::Up;
        case SDL_SCANCODE_DOWN:     return Key::Down;
        case SDL_SCANCODE_LEFT:     return Key::Left;
        case SDL_SCANCODE_RIGHT:    return Key::Right;
        case SDL_SCANCODE_HOME:     return Key::Home;
        case SDL_SCANCODE_END:      return Key::End;
        case SDL_SCANCODE_PAGEUP:   return Key::PageUp;
        case SDL_SCANCODE_PAGEDOWN: return Key::PageDown;
        case SDL_SCANCODE_INSERT:   return Key::Insert;
        case SDL_SCANCODE_DELETE:   return Key::Delete;

        // Whitespace / editing
        case SDL_SCANCODE_SPACE:     return Key::Space;
        case SDL_SCANCODE_TAB:       return Key::Tab;
        case SDL_SCANCODE_RETURN:    return Key::Enter;
        case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
        case SDL_SCANCODE_ESCAPE:    return Key::Escape;

        // Modifiers
        case SDL_SCANCODE_LSHIFT: return Key::LeftShift;
        case SDL_SCANCODE_RSHIFT: return Key::RightShift;
        case SDL_SCANCODE_LCTRL:  return Key::LeftCtrl;
        case SDL_SCANCODE_RCTRL:  return Key::RightCtrl;
        case SDL_SCANCODE_LALT:   return Key::LeftAlt;
        case SDL_SCANCODE_RALT:   return Key::RightAlt;
        case SDL_SCANCODE_LGUI:   return Key::LeftSuper;
        case SDL_SCANCODE_RGUI:   return Key::RightSuper;

        // Punctuation / symbols
        case SDL_SCANCODE_COMMA:        return Key::Comma;
        case SDL_SCANCODE_PERIOD:       return Key::Period;
        case SDL_SCANCODE_SLASH:        return Key::Slash;
        case SDL_SCANCODE_BACKSLASH:    return Key::Backslash;
        case SDL_SCANCODE_SEMICOLON:    return Key::Semicolon;
        case SDL_SCANCODE_APOSTROPHE:   return Key::Apostrophe;
        case SDL_SCANCODE_LEFTBRACKET:  return Key::LeftBracket;
        case SDL_SCANCODE_RIGHTBRACKET: return Key::RightBracket;
        case SDL_SCANCODE_MINUS:        return Key::Minus;
        case SDL_SCANCODE_EQUALS:       return Key::Equals;
        case SDL_SCANCODE_GRAVE:        return Key::Grave;

        // Lock keys
        case SDL_SCANCODE_CAPSLOCK:   return Key::CapsLock;
        case SDL_SCANCODE_NUMLOCKCLEAR: return Key::NumLock;
        case SDL_SCANCODE_SCROLLLOCK: return Key::ScrollLock;

        // Numpad
        case SDL_SCANCODE_KP_0:        return Key::KP0;
        case SDL_SCANCODE_KP_1:        return Key::KP1;
        case SDL_SCANCODE_KP_2:        return Key::KP2;
        case SDL_SCANCODE_KP_3:        return Key::KP3;
        case SDL_SCANCODE_KP_4:        return Key::KP4;
        case SDL_SCANCODE_KP_5:        return Key::KP5;
        case SDL_SCANCODE_KP_6:        return Key::KP6;
        case SDL_SCANCODE_KP_7:        return Key::KP7;
        case SDL_SCANCODE_KP_8:        return Key::KP8;
        case SDL_SCANCODE_KP_9:        return Key::KP9;
        case SDL_SCANCODE_KP_PERIOD:   return Key::KPDecimal;
        case SDL_SCANCODE_KP_DIVIDE:   return Key::KPDivide;
        case SDL_SCANCODE_KP_MULTIPLY: return Key::KPMultiply;
        case SDL_SCANCODE_KP_MINUS:    return Key::KPSubtract;
        case SDL_SCANCODE_KP_PLUS:     return Key::KPAdd;
        case SDL_SCANCODE_KP_ENTER:    return Key::KPEnter;

        // Misc
        case SDL_SCANCODE_PRINTSCREEN: return Key::PrintScreen;
        case SDL_SCANCODE_PAUSE:       return Key::Pause;
        case SDL_SCANCODE_MENU:        return Key::Menu;

        default: return Key::Unknown;
    }
}

Modifier mapModifiers(unsigned short mod) {
    Modifier result = Modifier::None;
    if (mod & KMOD_SHIFT) result = result | Modifier::Shift;
    if (mod & KMOD_CTRL)  result = result | Modifier::Ctrl;
    if (mod & KMOD_ALT)   result = result | Modifier::Alt;
    if (mod & KMOD_GUI)   result = result | Modifier::Super;
    if (mod & KMOD_CAPS)  result = result | Modifier::Caps;
    if (mod & KMOD_NUM)   result = result | Modifier::Num;
    return result;
}

MouseButton mapMouseButton(unsigned char btn) {
    switch (btn) {
        case SDL_BUTTON_LEFT:   return MouseButton::Left;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_RIGHT:  return MouseButton::Right;
        case SDL_BUTTON_X1:     return MouseButton::X1;
        case SDL_BUTTON_X2:     return MouseButton::X2;
        default:                return MouseButton::Unknown;
    }
}

GamepadButton mapGamepadButton(int btn) {
    switch (btn) {
        case SDL_CONTROLLER_BUTTON_A:             return GamepadButton::A;
        case SDL_CONTROLLER_BUTTON_B:             return GamepadButton::B;
        case SDL_CONTROLLER_BUTTON_X:             return GamepadButton::X;
        case SDL_CONTROLLER_BUTTON_Y:             return GamepadButton::Y;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return GamepadButton::LeftBumper;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return GamepadButton::RightBumper;
        case SDL_CONTROLLER_BUTTON_BACK:          return GamepadButton::Back;
        case SDL_CONTROLLER_BUTTON_START:         return GamepadButton::Start;
        case SDL_CONTROLLER_BUTTON_GUIDE:         return GamepadButton::Guide;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return GamepadButton::LeftStick;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:     return GamepadButton::RightStick;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:       return GamepadButton::DPadUp;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return GamepadButton::DPadDown;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return GamepadButton::DPadLeft;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return GamepadButton::DPadRight;
        default:                                   return GamepadButton::Unknown;
    }
}

GamepadAxis mapGamepadAxis(int axis) {
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:        return GamepadAxis::LeftX;
        case SDL_CONTROLLER_AXIS_LEFTY:        return GamepadAxis::LeftY;
        case SDL_CONTROLLER_AXIS_RIGHTX:       return GamepadAxis::RightX;
        case SDL_CONTROLLER_AXIS_RIGHTY:       return GamepadAxis::RightY;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:  return GamepadAxis::LeftTrigger;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return GamepadAxis::RightTrigger;
        default:                                return GamepadAxis::Unknown;
    }
}

} // namespace io::sdl
