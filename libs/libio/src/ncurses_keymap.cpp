#include "libio/ncurses_keymap.hpp"

#include <ncurses.h>

namespace io::ncurses {

Key mapKey(int ch) {
    return mapKeyWithModifiers(ch).first;
}

std::pair<Key, Modifier> mapKeyWithModifiers(int ch) {
    switch (ch) {
        // Whitespace / editing
        case ' ':              return {Key::Space, Modifier::None};
        case '\t':             return {Key::Tab, Modifier::None};
        case '\n':             return {Key::Enter, Modifier::None};
        case KEY_ENTER:        return {Key::Enter, Modifier::None};
        case KEY_BACKSPACE:    return {Key::Backspace, Modifier::None};
        case 127:              return {Key::Backspace, Modifier::None};
        case 27:               return {Key::Escape, Modifier::None};
    }

    // Ctrl+letter: terminal sends codes 1–26 for Ctrl+A through Ctrl+Z
    if (ch >= 1 && ch <= 26)
        return {static_cast<Key>(static_cast<int>(Key::A) + (ch - 1)), Modifier::Ctrl};

    // Uppercase letters: Shift is held
    if (ch >= 'A' && ch <= 'Z')
        return {static_cast<Key>(static_cast<int>(Key::A) + (ch - 'A')), Modifier::Shift};

    // Lowercase letters
    if (ch >= 'a' && ch <= 'z')
        return {static_cast<Key>(static_cast<int>(Key::A) + (ch - 'a')), Modifier::None};

    // Digits
    if (ch >= '0' && ch <= '9')
        return {static_cast<Key>(static_cast<int>(Key::Num0) + (ch - '0')), Modifier::None};

    // Function keys
    if (ch >= KEY_F(1) && ch <= KEY_F(12))
        return {static_cast<Key>(static_cast<int>(Key::F1) + (ch - KEY_F(1))), Modifier::None};

    switch (ch) {
        // Navigation
        case KEY_UP:    return {Key::Up, Modifier::None};
        case KEY_DOWN:  return {Key::Down, Modifier::None};
        case KEY_LEFT:  return {Key::Left, Modifier::None};
        case KEY_RIGHT: return {Key::Right, Modifier::None};
        case KEY_HOME:  return {Key::Home, Modifier::None};
        case KEY_END:   return {Key::End, Modifier::None};
        case KEY_PPAGE: return {Key::PageUp, Modifier::None};
        case KEY_NPAGE: return {Key::PageDown, Modifier::None};
        case KEY_IC:    return {Key::Insert, Modifier::None};
        case KEY_DC:    return {Key::Delete, Modifier::None};

        // Punctuation / symbols
        case ',':  return {Key::Comma, Modifier::None};
        case '.':  return {Key::Period, Modifier::None};
        case '/':  return {Key::Slash, Modifier::None};
        case '\\': return {Key::Backslash, Modifier::None};
        case ';':  return {Key::Semicolon, Modifier::None};
        case '\'': return {Key::Apostrophe, Modifier::None};
        case '[':  return {Key::LeftBracket, Modifier::None};
        case ']':  return {Key::RightBracket, Modifier::None};
        case '-':  return {Key::Minus, Modifier::None};
        case '=':  return {Key::Equals, Modifier::None};
        case '`':  return {Key::Grave, Modifier::None};

        default:   return {Key::Unknown, Modifier::None};
    }
}

} // namespace io::ncurses
