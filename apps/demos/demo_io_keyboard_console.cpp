/**
 * @brief Console-only keyboard demo using the libio InputDevice abstraction.
 *
 * Supports two backends selected via command-line argument:
 *
 *   ./demo_io_keyboard_console ncurses       (default)
 *   ./demo_io_keyboard_console evdev /dev/input/event4
 *
 * NCurses mode: uses ncurses terminal input. Detects Ctrl+letter and
 *               Shift (uppercase). Press ESC to quit.
 *
 * Evdev mode:   reads /dev/input/eventN directly. Reports full modifier
 *               state including individual Shift/Ctrl/Alt/Super presses
 *               and releases. Requires read access to the device.
 *               Press ESC to quit.
 */

#include "libio/ncurses_keyboard.hpp"
#include "libio/evdev_keyboard.hpp"
#include "libio/input_event.hpp"
#include "libio/keycodes.hpp"

#include <ncurses.h>
#include <iostream>
#include <cstring>
#include <memory>
#include <chrono>
#include <thread>

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

static const char* actionStr(io::Action a) {
    switch (a) {
        case io::Action::Press:   return "Press";
        case io::Action::Release: return "Release";
        case io::Action::Repeat:  return "Repeat";
    }
    return "?";
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

// ── NCurses mode ────────────────────────────────────────────────────────

static int runNCurses() {
    initscr();
    raw();              // pass ALL keys through (disables flow-control & signals)
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    scrollok(stdscr, TRUE);

    printw("libio keyboard demo (ncurses backend) -- press ESC to quit\n\n");
    refresh();

    io::NCursesKeyboard keyboard;
    bool running = true;

    while (running) {
        while (auto ev = keyboard.poll()) {
            auto* ke = std::get_if<io::KeyEvent>(&*ev);
            if (!ke) continue;

            if (ke->key == io::Key::Escape ||
                (ke->key == io::Key::C && io::hasModifier(ke->modifiers, io::Modifier::Ctrl))) {
                running = false;
                break;
            }

            printw("[Keyboard] %-7s  %-12s%s\n",
                   actionStr(ke->action), keyStr(ke->key),
                   modifierStr(ke->modifiers).c_str());
            refresh();
        }
        napms(16);
    }

    endwin();
    return 0;
}

// ── Evdev mode ──────────────────────────────────────────────────────────

static int runEvdev(const char* devicePath) {
    std::unique_ptr<io::EvdevKeyboard> keyboard;
    try {
        keyboard = std::make_unique<io::EvdevKeyboard>(devicePath);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::cout << "libio keyboard demo (evdev backend: " << devicePath << ")\n"
              << "Press ESC to quit.\n\n";

    bool running = true;
    while (running) {
        while (auto ev = keyboard->poll()) {
            auto* ke = std::get_if<io::KeyEvent>(&*ev);
            if (!ke) continue;

            if (ke->key == io::Key::Escape && ke->action == io::Action::Press) {
                running = false;
                break;
            }

            std::cout << "[Keyboard] " << actionStr(ke->action)
                      << "  " << keyStr(ke->key)
                      << modifierStr(ke->modifiers) << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "\nDone.\n";
    return 0;
}

// ── Main ────────────────────────────────────────────────────────────────

static void printUsage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " ncurses                     (default)\n"
              << "  " << prog << " evdev /dev/input/eventN\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2 || std::strcmp(argv[1], "ncurses") == 0) {
        return runNCurses();
    }
    if (std::strcmp(argv[1], "evdev") == 0) {
        if (argc < 3) {
            std::cerr << "evdev mode requires a device path.\n";
            printUsage(argv[0]);
            return 1;
        }
        return runEvdev(argv[2]);
    }
    printUsage(argv[0]);
    return 1;
}
