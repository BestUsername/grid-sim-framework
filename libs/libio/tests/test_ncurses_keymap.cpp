#include "libio/input_event.hpp"
#include "libio/ncurses_keymap.hpp"
#include "libio/ncurses_keyboard.hpp"

#include <gtest/gtest.h>
#include <ncurses.h>

#include <array>

TEST(NCursesKeymapTest, MapCtrlLettersWithCtrlModifier)
{
    const auto [key, modifier] = io::ncurses::mapKeyWithModifiers(3);

    EXPECT_EQ(key, io::Key::C);
    EXPECT_EQ(modifier, io::Modifier::Ctrl);
}

TEST(NCursesKeymapTest, MapUppercaseLettersWithShiftModifier)
{
    const auto [key, modifier] = io::ncurses::mapKeyWithModifiers('Q');

    EXPECT_EQ(key, io::Key::Q);
    EXPECT_EQ(modifier, io::Modifier::Shift);
}

TEST(NCursesKeymapTest, MapLowercaseLettersWithoutModifier)
{
    const auto [key, modifier] = io::ncurses::mapKeyWithModifiers('m');

    EXPECT_EQ(key, io::Key::M);
    EXPECT_EQ(modifier, io::Modifier::None);
}

TEST(NCursesKeymapTest, MapDigitsAndFunctionKeys)
{
    EXPECT_EQ(io::ncurses::mapKey('7'), io::Key::Num7);
    EXPECT_EQ(io::ncurses::mapKey(KEY_F(5)), io::Key::F5);
}

TEST(NCursesKeymapTest, MapAllLettersAndDigits)
{
    for (int i = 0; i < 26; ++i) {
        EXPECT_EQ(io::ncurses::mapKey('a' + i),
                  static_cast<io::Key>(static_cast<int>(io::Key::A) + i));
        EXPECT_EQ(io::ncurses::mapKey('A' + i),
                  static_cast<io::Key>(static_cast<int>(io::Key::A) + i));
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(io::ncurses::mapKey('0' + i),
                  static_cast<io::Key>(static_cast<int>(io::Key::Num0) + i));
    }
}

TEST(NCursesKeymapTest, MapAllSupportedFunctionKeys)
{
    for (int i = 0; i < 12; ++i) {
        EXPECT_EQ(io::ncurses::mapKey(KEY_F(i + 1)),
                  static_cast<io::Key>(static_cast<int>(io::Key::F1) + i));
    }
}

TEST(NCursesKeymapTest, MapNavigationAndEditingKeys)
{
    EXPECT_EQ(io::ncurses::mapKey(KEY_UP), io::Key::Up);
    EXPECT_EQ(io::ncurses::mapKey(KEY_DOWN), io::Key::Down);
    EXPECT_EQ(io::ncurses::mapKey(KEY_LEFT), io::Key::Left);
    EXPECT_EQ(io::ncurses::mapKey(KEY_RIGHT), io::Key::Right);
    EXPECT_EQ(io::ncurses::mapKey(KEY_HOME), io::Key::Home);
    EXPECT_EQ(io::ncurses::mapKey(KEY_END), io::Key::End);
    EXPECT_EQ(io::ncurses::mapKey(KEY_PPAGE), io::Key::PageUp);
    EXPECT_EQ(io::ncurses::mapKey(KEY_NPAGE), io::Key::PageDown);
    EXPECT_EQ(io::ncurses::mapKey(KEY_IC), io::Key::Insert);
    EXPECT_EQ(io::ncurses::mapKey(KEY_DC), io::Key::Delete);
    EXPECT_EQ(io::ncurses::mapKey(' '), io::Key::Space);
    EXPECT_EQ(io::ncurses::mapKey('\t'), io::Key::Tab);
    EXPECT_EQ(io::ncurses::mapKey(KEY_ENTER), io::Key::Enter);
    EXPECT_EQ(io::ncurses::mapKey(KEY_BACKSPACE), io::Key::Backspace);
    EXPECT_EQ(io::ncurses::mapKey(127), io::Key::Backspace);
    EXPECT_EQ(io::ncurses::mapKey(27), io::Key::Escape);
}

TEST(NCursesKeymapTest, MapPunctuationKeys)
{
    EXPECT_EQ(io::ncurses::mapKey(','), io::Key::Comma);
    EXPECT_EQ(io::ncurses::mapKey('.'), io::Key::Period);
    EXPECT_EQ(io::ncurses::mapKey('/'), io::Key::Slash);
    EXPECT_EQ(io::ncurses::mapKey('\\'), io::Key::Backslash);
    EXPECT_EQ(io::ncurses::mapKey(';'), io::Key::Semicolon);
    EXPECT_EQ(io::ncurses::mapKey('\''), io::Key::Apostrophe);
    EXPECT_EQ(io::ncurses::mapKey('['), io::Key::LeftBracket);
    EXPECT_EQ(io::ncurses::mapKey(']'), io::Key::RightBracket);
    EXPECT_EQ(io::ncurses::mapKey('-'), io::Key::Minus);
    EXPECT_EQ(io::ncurses::mapKey('='), io::Key::Equals);
    EXPECT_EQ(io::ncurses::mapKey('`'), io::Key::Grave);
}

TEST(NCursesKeymapTest, UnknownCodesMapToUnknownWithoutModifier)
{
    const auto [key, modifier] = io::ncurses::mapKeyWithModifiers(KEY_RESIZE);

    EXPECT_EQ(key, io::Key::Unknown);
    EXPECT_EQ(modifier, io::Modifier::None);
}

TEST(NCursesKeyboardTest, TranslatePolledKeyReturnsNulloptForErr)
{
    EXPECT_FALSE(io::ncurses::translatePolledKey(ERR).has_value());
}

TEST(NCursesKeyboardTest, TranslatePolledKeyCreatesResizeEvent)
{
    auto event = io::ncurses::translatePolledKey(KEY_RESIZE, 24, 80);
    ASSERT_TRUE(event.has_value());

    auto* resize = std::get_if<io::ResizeEvent>(&*event);
    ASSERT_NE(resize, nullptr);
    EXPECT_EQ(resize->width, 80);
    EXPECT_EQ(resize->height, 24);
}

TEST(NCursesKeyboardTest, TranslatePolledKeyCreatesKeyPress)
{
    auto event = io::ncurses::translatePolledKey('A');
    ASSERT_TRUE(event.has_value());

    auto* key = std::get_if<io::KeyEvent>(&*event);
    ASSERT_NE(key, nullptr);
    EXPECT_EQ(key->key, io::Key::A);
    EXPECT_EQ(key->action, io::Action::Press);
    EXPECT_EQ(key->modifiers, io::Modifier::Shift);
}

TEST(NCursesKeyboardTest, NameAndType)
{
    io::NCursesKeyboard keyboard;
    EXPECT_EQ(keyboard.name(), "NCurses Keyboard");
    EXPECT_EQ(keyboard.type(), io::DeviceType::Keyboard);
}
