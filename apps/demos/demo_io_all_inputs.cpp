/**
 * @brief Demo: prints unified InputEvents from keyboard, mouse, and gamepad.
 *
 * Opens an SDL window and feeds all input through the libio abstraction.
 * Every event is printed to stdout in a human-readable format.
 *
 * Press ESC or close the window to quit.
 */

#include "libio/sdl_keyboard.hpp"
#include "libio/sdl_mouse.hpp"
#include "libio/sdl_gamepad.hpp"
#include "libio/input_event.hpp"
#include "libio/keycodes.hpp"

#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <cstdlib>

// ── Pretty-print helpers ────────────────────────────────────────────────

static const char* actionStr(io::Action a) {
    switch (a) {
        case io::Action::Press:   return "Press";
        case io::Action::Release: return "Release";
        case io::Action::Repeat:  return "Repeat";
    }
    return "?";
}

static const char* keyStr(io::Key k) {
    switch (k) {
        case io::Key::Unknown: return "Unknown";
        case io::Key::A: return "A"; case io::Key::B: return "B";
        case io::Key::C: return "C"; case io::Key::D: return "D";
        case io::Key::E: return "E"; case io::Key::F: return "F";
        case io::Key::G: return "G"; case io::Key::H: return "H";
        case io::Key::I: return "I"; case io::Key::J: return "J";
        case io::Key::K: return "K"; case io::Key::L: return "L";
        case io::Key::M: return "M"; case io::Key::N: return "N";
        case io::Key::O: return "O"; case io::Key::P: return "P";
        case io::Key::Q: return "Q"; case io::Key::R: return "R";
        case io::Key::S: return "S"; case io::Key::T: return "T";
        case io::Key::U: return "U"; case io::Key::V: return "V";
        case io::Key::W: return "W"; case io::Key::X: return "X";
        case io::Key::Y: return "Y"; case io::Key::Z: return "Z";
        case io::Key::Num0: return "0"; case io::Key::Num1: return "1";
        case io::Key::Num2: return "2"; case io::Key::Num3: return "3";
        case io::Key::Num4: return "4"; case io::Key::Num5: return "5";
        case io::Key::Num6: return "6"; case io::Key::Num7: return "7";
        case io::Key::Num8: return "8"; case io::Key::Num9: return "9";
        case io::Key::F1: return "F1"; case io::Key::F2: return "F2";
        case io::Key::F3: return "F3"; case io::Key::F4: return "F4";
        case io::Key::F5: return "F5"; case io::Key::F6: return "F6";
        case io::Key::F7: return "F7"; case io::Key::F8: return "F8";
        case io::Key::F9: return "F9"; case io::Key::F10: return "F10";
        case io::Key::F11: return "F11"; case io::Key::F12: return "F12";
        case io::Key::Up: return "Up"; case io::Key::Down: return "Down";
        case io::Key::Left: return "Left"; case io::Key::Right: return "Right";
        case io::Key::Home: return "Home"; case io::Key::End: return "End";
        case io::Key::PageUp: return "PageUp"; case io::Key::PageDown: return "PageDown";
        case io::Key::Insert: return "Insert"; case io::Key::Delete: return "Delete";
        case io::Key::Space: return "Space"; case io::Key::Tab: return "Tab";
        case io::Key::Enter: return "Enter"; case io::Key::Backspace: return "Backspace";
        case io::Key::Escape: return "Escape";
        case io::Key::LeftShift: return "LShift"; case io::Key::RightShift: return "RShift";
        case io::Key::LeftCtrl: return "LCtrl"; case io::Key::RightCtrl: return "RCtrl";
        case io::Key::LeftAlt: return "LAlt"; case io::Key::RightAlt: return "RAlt";
        case io::Key::LeftSuper: return "LSuper"; case io::Key::RightSuper: return "RSuper";
        case io::Key::Comma: return ","; case io::Key::Period: return ".";
        case io::Key::Slash: return "/"; case io::Key::Backslash: return "\\";
        case io::Key::Semicolon: return ";"; case io::Key::Apostrophe: return "'";
        case io::Key::LeftBracket: return "["; case io::Key::RightBracket: return "]";
        case io::Key::Minus: return "-"; case io::Key::Equals: return "=";
        case io::Key::Grave: return "`";
        case io::Key::CapsLock: return "CapsLock"; case io::Key::NumLock: return "NumLock";
        case io::Key::ScrollLock: return "ScrollLock";
        case io::Key::PrintScreen: return "PrintScreen";
        case io::Key::Pause: return "Pause"; case io::Key::Menu: return "Menu";
        default: return "?";
    }
}

static const char* mouseButtonStr(io::MouseButton b) {
    switch (b) {
        case io::MouseButton::Left:    return "Left";
        case io::MouseButton::Middle:  return "Middle";
        case io::MouseButton::Right:   return "Right";
        case io::MouseButton::X1:      return "X1";
        case io::MouseButton::X2:      return "X2";
        default:                       return "?";
    }
}

static const char* gamepadButtonStr(io::GamepadButton b) {
    switch (b) {
        case io::GamepadButton::A: return "A"; case io::GamepadButton::B: return "B";
        case io::GamepadButton::X: return "X"; case io::GamepadButton::Y: return "Y";
        case io::GamepadButton::LeftBumper: return "LB";
        case io::GamepadButton::RightBumper: return "RB";
        case io::GamepadButton::Back: return "Back";
        case io::GamepadButton::Start: return "Start";
        case io::GamepadButton::Guide: return "Guide";
        case io::GamepadButton::LeftStick: return "LS";
        case io::GamepadButton::RightStick: return "RS";
        case io::GamepadButton::DPadUp: return "DUp";
        case io::GamepadButton::DPadDown: return "DDown";
        case io::GamepadButton::DPadLeft: return "DLeft";
        case io::GamepadButton::DPadRight: return "DRight";
        default: return "?";
    }
}

static const char* gamepadAxisStr(io::GamepadAxis a) {
    switch (a) {
        case io::GamepadAxis::LeftX: return "LeftX";
        case io::GamepadAxis::LeftY: return "LeftY";
        case io::GamepadAxis::RightX: return "RightX";
        case io::GamepadAxis::RightY: return "RightY";
        case io::GamepadAxis::LeftTrigger: return "LTrigger";
        case io::GamepadAxis::RightTrigger: return "RTrigger";
        default: return "?";
    }
}

static std::string modifierStr(io::Modifier m) {
    if (m == io::Modifier::None) return "";
    std::string s;
    if (io::hasModifier(m, io::Modifier::Shift)) s += "Shift+";
    if (io::hasModifier(m, io::Modifier::Ctrl))  s += "Ctrl+";
    if (io::hasModifier(m, io::Modifier::Alt))   s += "Alt+";
    if (io::hasModifier(m, io::Modifier::Super)) s += "Super+";
    if (io::hasModifier(m, io::Modifier::Caps))  s += "Caps+";
    if (io::hasModifier(m, io::Modifier::Num))   s += "Num+";
    if (!s.empty()) s.pop_back(); // remove trailing '+'
    return " [" + s + "]";
}

// ── Event printer ───────────────────────────────────────────────────────

static void printEvent(const io::InputEvent& ev) {
    std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, io::KeyEvent>) {
            std::cout << "[Keyboard] " << actionStr(arg.action)
                      << "  " << keyStr(arg.key)
                      << modifierStr(arg.modifiers) << "\n";
        } else if constexpr (std::is_same_v<T, io::MouseMoveEvent>) {
            std::cout << "[Mouse]    Move  pos(" << arg.x << "," << arg.y
                      << ")  delta(" << arg.dx << "," << arg.dy << ")\n";
        } else if constexpr (std::is_same_v<T, io::MouseButtonEvent>) {
            std::cout << "[Mouse]    " << actionStr(arg.action)
                      << "  " << mouseButtonStr(arg.button)
                      << "  at(" << arg.x << "," << arg.y << ")\n";
        } else if constexpr (std::is_same_v<T, io::MouseScrollEvent>) {
            std::cout << "[Mouse]    Scroll(" << arg.scrollX << "," << arg.scrollY << ")\n";
        } else if constexpr (std::is_same_v<T, io::GamepadButtonEvent>) {
            std::cout << "[Gamepad " << arg.gamepadIndex << "] "
                      << actionStr(arg.action) << "  "
                      << gamepadButtonStr(arg.button) << "\n";
        } else if constexpr (std::is_same_v<T, io::GamepadAxisEvent>) {
            std::cout << "[Gamepad " << arg.gamepadIndex << "] Axis "
                      << gamepadAxisStr(arg.axis) << " = " << arg.value << "\n";
        }
    }, ev);
}

// ── Main ────────────────────────────────────────────────────────────────

int main(int /*argc*/, char** /*argv*/) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow(
        "libio input demo — press ESC to quit",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Open any game controllers that are already connected
    std::vector<SDL_GameController*> controllers;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            auto* gc = SDL_GameControllerOpen(i);
            if (gc) {
                std::cout << "Opened controller " << i << ": "
                          << SDL_GameControllerName(gc) << "\n";
                controllers.push_back(gc);
            }
        }
    }

    io::SDLKeyboard keyboard;
    io::SDLMouse    mouse;
    io::SDLGamepad  gamepad;

    std::cout << "Ready — interact with the window. Press ESC to quit.\n\n";

    bool running = true;
    while (running) {
        // Feed SDL events into the devices
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    keyboard.feedSDLEvent(
                        ev.key.keysym.scancode, ev.key.keysym.mod,
                        ev.type == SDL_KEYDOWN, ev.key.repeat != 0);
                    break;

                case SDL_MOUSEMOTION:
                    mouse.feedMotion(ev.motion.x, ev.motion.y,
                                    ev.motion.xrel, ev.motion.yrel);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    mouse.feedButton(ev.button.button,
                                     ev.type == SDL_MOUSEBUTTONDOWN,
                                     ev.button.x, ev.button.y);
                    break;
                case SDL_MOUSEWHEEL:
                    mouse.feedScroll(ev.wheel.x, ev.wheel.y);
                    break;

                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP:
                    gamepad.feedButton(ev.cbutton.which, ev.cbutton.button,
                                      ev.type == SDL_CONTROLLERBUTTONDOWN);
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    gamepad.feedAxis(ev.caxis.which, ev.caxis.axis, ev.caxis.value);
                    break;

                case SDL_CONTROLLERDEVICEADDED: {
                    auto* gc = SDL_GameControllerOpen(ev.cdevice.which);
                    if (gc) {
                        std::cout << "** Controller connected: "
                                  << SDL_GameControllerName(gc) << "\n";
                        controllers.push_back(gc);
                    }
                    break;
                }
                case SDL_CONTROLLERDEVICEREMOVED:
                    std::cout << "** Controller disconnected\n";
                    break;

                default:
                    break;
            }
        }

        // Drain and print all unified events
        while (auto e = keyboard.poll()) {
            printEvent(*e);
            // Quit on ESC
            if (auto* ke = std::get_if<io::KeyEvent>(&*e)) {
                if (ke->key == io::Key::Escape && ke->action == io::Action::Press)
                    running = false;
            }
        }
        while (auto e = mouse.poll())    { printEvent(*e); }
        while (auto e = gamepad.poll())  { printEvent(*e); }

        SDL_Delay(1); // don't burn CPU
    }

    // Cleanup
    for (auto* gc : controllers) SDL_GameControllerClose(gc);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "\nDone.\n";
    return EXIT_SUCCESS;
}
