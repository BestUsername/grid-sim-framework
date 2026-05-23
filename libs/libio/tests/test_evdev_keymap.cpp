#include "gtest/gtest.h"
#include "libio/evdev_keymap.hpp"
#include "libio/keycodes.hpp"

#include <linux/input-event-codes.h>

#include <array>

// ── Letter keys ─────────────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapLetterKeys) {
    EXPECT_EQ(io::evdev::mapKey(KEY_A), io::Key::A);
    EXPECT_EQ(io::evdev::mapKey(KEY_Z), io::Key::Z);
    EXPECT_EQ(io::evdev::mapKey(KEY_M), io::Key::M);
}

// ── Number keys ─────────────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapNumberKeys) {
    EXPECT_EQ(io::evdev::mapKey(KEY_0), io::Key::Num0);
    EXPECT_EQ(io::evdev::mapKey(KEY_9), io::Key::Num9);
    EXPECT_EQ(io::evdev::mapKey(KEY_5), io::Key::Num5);
}

// ── Function keys ───────────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapFunctionKeys) {
    EXPECT_EQ(io::evdev::mapKey(KEY_F1), io::Key::F1);
    EXPECT_EQ(io::evdev::mapKey(KEY_F12), io::Key::F12);
}

// ── Navigation ──────────────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapNavigation) {
    EXPECT_EQ(io::evdev::mapKey(KEY_UP), io::Key::Up);
    EXPECT_EQ(io::evdev::mapKey(KEY_DOWN), io::Key::Down);
    EXPECT_EQ(io::evdev::mapKey(KEY_LEFT), io::Key::Left);
    EXPECT_EQ(io::evdev::mapKey(KEY_RIGHT), io::Key::Right);
    EXPECT_EQ(io::evdev::mapKey(KEY_HOME), io::Key::Home);
    EXPECT_EQ(io::evdev::mapKey(KEY_END), io::Key::End);
    EXPECT_EQ(io::evdev::mapKey(KEY_PAGEUP), io::Key::PageUp);
    EXPECT_EQ(io::evdev::mapKey(KEY_PAGEDOWN), io::Key::PageDown);
}

// ── Modifiers ───────────────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapModifierKeys) {
    EXPECT_EQ(io::evdev::mapKey(KEY_LEFTSHIFT), io::Key::LeftShift);
    EXPECT_EQ(io::evdev::mapKey(KEY_RIGHTSHIFT), io::Key::RightShift);
    EXPECT_EQ(io::evdev::mapKey(KEY_LEFTCTRL), io::Key::LeftCtrl);
    EXPECT_EQ(io::evdev::mapKey(KEY_RIGHTCTRL), io::Key::RightCtrl);
    EXPECT_EQ(io::evdev::mapKey(KEY_LEFTALT), io::Key::LeftAlt);
    EXPECT_EQ(io::evdev::mapKey(KEY_RIGHTALT), io::Key::RightAlt);
    EXPECT_EQ(io::evdev::mapKey(KEY_LEFTMETA), io::Key::LeftSuper);
    EXPECT_EQ(io::evdev::mapKey(KEY_RIGHTMETA), io::Key::RightSuper);
}

// ── Whitespace / editing ────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapWhitespaceKeys) {
    EXPECT_EQ(io::evdev::mapKey(KEY_SPACE), io::Key::Space);
    EXPECT_EQ(io::evdev::mapKey(KEY_TAB), io::Key::Tab);
    EXPECT_EQ(io::evdev::mapKey(KEY_ENTER), io::Key::Enter);
    EXPECT_EQ(io::evdev::mapKey(KEY_BACKSPACE), io::Key::Backspace);
    EXPECT_EQ(io::evdev::mapKey(KEY_ESC), io::Key::Escape);
}

// ── Punctuation ─────────────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapPunctuation) {
    EXPECT_EQ(io::evdev::mapKey(KEY_COMMA), io::Key::Comma);
    EXPECT_EQ(io::evdev::mapKey(KEY_DOT), io::Key::Period);
    EXPECT_EQ(io::evdev::mapKey(KEY_SLASH), io::Key::Slash);
    EXPECT_EQ(io::evdev::mapKey(KEY_MINUS), io::Key::Minus);
    EXPECT_EQ(io::evdev::mapKey(KEY_EQUAL), io::Key::Equals);
    EXPECT_EQ(io::evdev::mapKey(KEY_GRAVE), io::Key::Grave);
}

// ── Numpad ──────────────────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapNumpad) {
    EXPECT_EQ(io::evdev::mapKey(KEY_KP0), io::Key::KP0);
    EXPECT_EQ(io::evdev::mapKey(KEY_KP9), io::Key::KP9);
    EXPECT_EQ(io::evdev::mapKey(KEY_KPENTER), io::Key::KPEnter);
    EXPECT_EQ(io::evdev::mapKey(KEY_KPPLUS), io::Key::KPAdd);
    EXPECT_EQ(io::evdev::mapKey(KEY_KPMINUS), io::Key::KPSubtract);
}

// ── Lock keys ───────────────────────────────────────────────────────────

TEST(EvdevKeymapTest, MapLockKeys) {
    EXPECT_EQ(io::evdev::mapKey(KEY_CAPSLOCK), io::Key::CapsLock);
    EXPECT_EQ(io::evdev::mapKey(KEY_NUMLOCK), io::Key::NumLock);
    EXPECT_EQ(io::evdev::mapKey(KEY_SCROLLLOCK), io::Key::ScrollLock);
}

// ── Unknown ─────────────────────────────────────────────────────────────

TEST(EvdevKeymapTest, UnknownKeyCode) {
    EXPECT_EQ(io::evdev::mapKey(9999), io::Key::Unknown);
}

TEST(EvdevKeymapTest, MapFullLetterNumberAndFunctionRanges) {
    const std::array<int, 26> letter_codes = {
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
        KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    };
    for (size_t i = 0; i < letter_codes.size(); ++i) {
        EXPECT_EQ(io::evdev::mapKey(letter_codes[i]),
                  static_cast<io::Key>(static_cast<int>(io::Key::A) + static_cast<int>(i)));
    }

    const std::array<int, 10> number_codes = {
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    };
    for (size_t i = 0; i < number_codes.size(); ++i) {
        EXPECT_EQ(io::evdev::mapKey(number_codes[i]),
                  static_cast<io::Key>(static_cast<int>(io::Key::Num0) + static_cast<int>(i)));
    }

    const std::array<int, 12> function_codes = {
        KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
        KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    };
    for (size_t i = 0; i < function_codes.size(); ++i) {
        EXPECT_EQ(io::evdev::mapKey(function_codes[i]),
                  static_cast<io::Key>(static_cast<int>(io::Key::F1) + static_cast<int>(i)));
    }
}

TEST(EvdevKeymapTest, MapAllRemainingSupportedKeys) {
    const std::array<std::pair<int, io::Key>, 28> mappings = {{
        {KEY_INSERT, io::Key::Insert},
        {KEY_DELETE, io::Key::Delete},
        {KEY_BACKSLASH, io::Key::Backslash},
        {KEY_SEMICOLON, io::Key::Semicolon},
        {KEY_APOSTROPHE, io::Key::Apostrophe},
        {KEY_LEFTBRACE, io::Key::LeftBracket},
        {KEY_RIGHTBRACE, io::Key::RightBracket},
        {KEY_KP1, io::Key::KP1},
        {KEY_KP2, io::Key::KP2},
        {KEY_KP3, io::Key::KP3},
        {KEY_KP4, io::Key::KP4},
        {KEY_KP5, io::Key::KP5},
        {KEY_KP6, io::Key::KP6},
        {KEY_KP7, io::Key::KP7},
        {KEY_KP8, io::Key::KP8},
        {KEY_KPDOT, io::Key::KPDecimal},
        {KEY_KPSLASH, io::Key::KPDivide},
        {KEY_KPASTERISK, io::Key::KPMultiply},
        {KEY_SYSRQ, io::Key::PrintScreen},
        {KEY_PAUSE, io::Key::Pause},
        {KEY_COMPOSE, io::Key::Menu},
        {KEY_LEFTSHIFT, io::Key::LeftShift},
        {KEY_RIGHTSHIFT, io::Key::RightShift},
        {KEY_LEFTCTRL, io::Key::LeftCtrl},
        {KEY_RIGHTCTRL, io::Key::RightCtrl},
        {KEY_LEFTALT, io::Key::LeftAlt},
        {KEY_RIGHTALT, io::Key::RightAlt},
        {KEY_RIGHTMETA, io::Key::RightSuper},
    }};

    for (const auto& [code, key] : mappings) {
        EXPECT_EQ(io::evdev::mapKey(code), key);
    }
}
