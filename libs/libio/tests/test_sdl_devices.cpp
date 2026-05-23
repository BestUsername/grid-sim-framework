#include "gtest/gtest.h"
#include "libio/sdl_keyboard.hpp"
#include "libio/sdl_mouse.hpp"
#include "libio/sdl_gamepad.hpp"
#include "libio/sdl_keymap.hpp"

#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_gamecontroller.h>

#include <array>

// ── SDLKeyboard ─────────────────────────────────────────────────────────

TEST(SDLKeyboardTest, PollReturnsNulloptWhenEmpty) {
    io::SDLKeyboard kb;
    EXPECT_FALSE(kb.poll().has_value());
}

TEST(SDLKeyboardTest, FeedAndPollKeyPress) {
    io::SDLKeyboard kb;
    kb.feedSDLEvent(SDL_SCANCODE_A, KMOD_NONE, true, false);

    auto ev = kb.poll();
    ASSERT_TRUE(ev.has_value());

    auto* ke = std::get_if<io::KeyEvent>(&*ev);
    ASSERT_NE(ke, nullptr);
    EXPECT_EQ(ke->key, io::Key::A);
    EXPECT_EQ(ke->action, io::Action::Press);
    EXPECT_EQ(ke->modifiers, io::Modifier::None);

    // Queue should now be empty
    EXPECT_FALSE(kb.poll().has_value());
}

TEST(SDLKeyboardTest, FeedKeyRelease) {
    io::SDLKeyboard kb;
    kb.feedSDLEvent(SDL_SCANCODE_SPACE, KMOD_NONE, false, false);

    auto ev = kb.poll();
    ASSERT_TRUE(ev.has_value());
    auto* ke = std::get_if<io::KeyEvent>(&*ev);
    ASSERT_NE(ke, nullptr);
    EXPECT_EQ(ke->key, io::Key::Space);
    EXPECT_EQ(ke->action, io::Action::Release);
}

TEST(SDLKeyboardTest, FeedKeyRepeat) {
    io::SDLKeyboard kb;
    kb.feedSDLEvent(SDL_SCANCODE_B, KMOD_NONE, true, true);

    auto ev = kb.poll();
    ASSERT_TRUE(ev.has_value());
    auto* ke = std::get_if<io::KeyEvent>(&*ev);
    ASSERT_NE(ke, nullptr);
    EXPECT_EQ(ke->action, io::Action::Repeat);
}

TEST(SDLKeyboardTest, FeedWithModifiers) {
    io::SDLKeyboard kb;
    kb.feedSDLEvent(SDL_SCANCODE_C, KMOD_LSHIFT | KMOD_LCTRL, true, false);

    auto ev = kb.poll();
    ASSERT_TRUE(ev.has_value());
    auto* ke = std::get_if<io::KeyEvent>(&*ev);
    ASSERT_NE(ke, nullptr);
    EXPECT_TRUE(io::hasModifier(ke->modifiers, io::Modifier::Shift));
    EXPECT_TRUE(io::hasModifier(ke->modifiers, io::Modifier::Ctrl));
    EXPECT_FALSE(io::hasModifier(ke->modifiers, io::Modifier::Alt));
}

TEST(SDLKeyboardTest, MultipleEventsFIFO) {
    io::SDLKeyboard kb;
    kb.feedSDLEvent(SDL_SCANCODE_X, KMOD_NONE, true, false);
    kb.feedSDLEvent(SDL_SCANCODE_Y, KMOD_NONE, true, false);

    auto ev1 = kb.poll();
    auto ev2 = kb.poll();
    ASSERT_TRUE(ev1.has_value());
    ASSERT_TRUE(ev2.has_value());
    EXPECT_EQ(std::get<io::KeyEvent>(*ev1).key, io::Key::X);
    EXPECT_EQ(std::get<io::KeyEvent>(*ev2).key, io::Key::Y);
    EXPECT_FALSE(kb.poll().has_value());
}

TEST(SDLKeyboardTest, NameAndType) {
    io::SDLKeyboard kb;
    EXPECT_EQ(kb.name(), "SDL Keyboard");
    EXPECT_EQ(kb.type(), io::DeviceType::Keyboard);
}

// ── SDLMouse ────────────────────────────────────────────────────────────

TEST(SDLMouseTest, PollReturnsNulloptWhenEmpty) {
    io::SDLMouse mouse;
    EXPECT_FALSE(mouse.poll().has_value());
}

TEST(SDLMouseTest, FeedMotion) {
    io::SDLMouse mouse;
    mouse.feedMotion(100, 200, 5, -3);

    auto ev = mouse.poll();
    ASSERT_TRUE(ev.has_value());
    auto* me = std::get_if<io::MouseMoveEvent>(&*ev);
    ASSERT_NE(me, nullptr);
    EXPECT_EQ(me->x, 100);
    EXPECT_EQ(me->y, 200);
    EXPECT_EQ(me->dx, 5);
    EXPECT_EQ(me->dy, -3);
}

TEST(SDLMouseTest, FeedButtonPress) {
    io::SDLMouse mouse;
    mouse.feedButton(1, true, 50, 60); // SDL_BUTTON_LEFT = 1

    auto ev = mouse.poll();
    ASSERT_TRUE(ev.has_value());
    auto* be = std::get_if<io::MouseButtonEvent>(&*ev);
    ASSERT_NE(be, nullptr);
    EXPECT_EQ(be->button, io::MouseButton::Left);
    EXPECT_EQ(be->action, io::Action::Press);
    EXPECT_EQ(be->x, 50);
}

TEST(SDLMouseTest, FeedScroll) {
    io::SDLMouse mouse;
    mouse.feedScroll(0, 3);

    auto ev = mouse.poll();
    ASSERT_TRUE(ev.has_value());
    auto* se = std::get_if<io::MouseScrollEvent>(&*ev);
    ASSERT_NE(se, nullptr);
    EXPECT_EQ(se->scrollY, 3);
}

TEST(SDLMouseTest, NameAndType) {
    io::SDLMouse mouse;
    EXPECT_EQ(mouse.name(), "SDL Mouse");
    EXPECT_EQ(mouse.type(), io::DeviceType::Mouse);
}

// ── SDLGamepad ──────────────────────────────────────────────────────────

TEST(SDLGamepadTest, PollReturnsNulloptWhenEmpty) {
    io::SDLGamepad gp;
    EXPECT_FALSE(gp.poll().has_value());
}

TEST(SDLGamepadTest, FeedButton) {
    io::SDLGamepad gp;
    gp.feedButton(0, 0, true); // SDL_CONTROLLER_BUTTON_A = 0

    auto ev = gp.poll();
    ASSERT_TRUE(ev.has_value());
    auto* be = std::get_if<io::GamepadButtonEvent>(&*ev);
    ASSERT_NE(be, nullptr);
    EXPECT_EQ(be->gamepadIndex, 0);
    EXPECT_EQ(be->button, io::GamepadButton::A);
    EXPECT_EQ(be->action, io::Action::Press);
}

TEST(SDLGamepadTest, FeedAxis) {
    io::SDLGamepad gp;
    gp.feedAxis(0, 0, 16383); // SDL_CONTROLLER_AXIS_LEFTX = 0, ~half right

    auto ev = gp.poll();
    ASSERT_TRUE(ev.has_value());
    auto* ae = std::get_if<io::GamepadAxisEvent>(&*ev);
    ASSERT_NE(ae, nullptr);
    EXPECT_EQ(ae->axis, io::GamepadAxis::LeftX);
    EXPECT_NEAR(ae->value, 0.5f, 0.01f);
}

TEST(SDLGamepadTest, AxisNormalizationFullRange) {
    io::SDLGamepad gp;
    gp.feedAxis(0, 0, -32768);  // full left
    gp.feedAxis(0, 0, 32767);   // full right

    auto ev1 = gp.poll();
    auto ev2 = gp.poll();
    EXPECT_FLOAT_EQ(std::get<io::GamepadAxisEvent>(*ev1).value, -1.0f);
    EXPECT_FLOAT_EQ(std::get<io::GamepadAxisEvent>(*ev2).value, 1.0f);
}

TEST(SDLGamepadTest, NameAndType) {
    io::SDLGamepad gp;
    EXPECT_EQ(gp.name(), "SDL Gamepad");
    EXPECT_EQ(gp.type(), io::DeviceType::Gamepad);
}

// ── SDL keymap functions ────────────────────────────────────────────────

TEST(SDLKeymapTest, MapLetterScancodes) {
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_A), io::Key::A);
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_Z), io::Key::Z);
}

TEST(SDLKeymapTest, MapNumberScancodes) {
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_0), io::Key::Num0);
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_9), io::Key::Num9);
}

TEST(SDLKeymapTest, MapFunctionKeys) {
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_F1), io::Key::F1);
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_F12), io::Key::F12);
}

TEST(SDLKeymapTest, MapNavigation) {
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_UP), io::Key::Up);
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_ESCAPE), io::Key::Escape);
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_RETURN), io::Key::Enter);
}

TEST(SDLKeymapTest, UnknownScancode) {
    EXPECT_EQ(io::sdl::mapScancode(9999), io::Key::Unknown);
}

TEST(SDLKeymapTest, MapModifiers) {
    auto mods = io::sdl::mapModifiers(KMOD_LSHIFT | KMOD_LALT);
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Shift));
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Alt));
    EXPECT_FALSE(io::hasModifier(mods, io::Modifier::Ctrl));
}

TEST(SDLKeymapTest, MapModifiersNone) {
    auto mods = io::sdl::mapModifiers(KMOD_NONE);
    EXPECT_EQ(mods, io::Modifier::None);
}

TEST(SDLKeymapTest, MapFullScancodeCoverage) {
    const std::array<std::pair<int, io::Key>, 75> mappings = {{
        {SDL_SCANCODE_A, io::Key::A}, {SDL_SCANCODE_B, io::Key::B}, {SDL_SCANCODE_C, io::Key::C},
        {SDL_SCANCODE_D, io::Key::D}, {SDL_SCANCODE_E, io::Key::E}, {SDL_SCANCODE_F, io::Key::F},
        {SDL_SCANCODE_G, io::Key::G}, {SDL_SCANCODE_H, io::Key::H}, {SDL_SCANCODE_I, io::Key::I},
        {SDL_SCANCODE_J, io::Key::J}, {SDL_SCANCODE_K, io::Key::K}, {SDL_SCANCODE_L, io::Key::L},
        {SDL_SCANCODE_M, io::Key::M}, {SDL_SCANCODE_N, io::Key::N}, {SDL_SCANCODE_O, io::Key::O},
        {SDL_SCANCODE_P, io::Key::P}, {SDL_SCANCODE_Q, io::Key::Q}, {SDL_SCANCODE_R, io::Key::R},
        {SDL_SCANCODE_S, io::Key::S}, {SDL_SCANCODE_T, io::Key::T}, {SDL_SCANCODE_U, io::Key::U},
        {SDL_SCANCODE_V, io::Key::V}, {SDL_SCANCODE_W, io::Key::W}, {SDL_SCANCODE_X, io::Key::X},
        {SDL_SCANCODE_Y, io::Key::Y}, {SDL_SCANCODE_Z, io::Key::Z},
        {SDL_SCANCODE_0, io::Key::Num0}, {SDL_SCANCODE_1, io::Key::Num1}, {SDL_SCANCODE_2, io::Key::Num2},
        {SDL_SCANCODE_3, io::Key::Num3}, {SDL_SCANCODE_4, io::Key::Num4}, {SDL_SCANCODE_5, io::Key::Num5},
        {SDL_SCANCODE_6, io::Key::Num6}, {SDL_SCANCODE_7, io::Key::Num7}, {SDL_SCANCODE_8, io::Key::Num8},
        {SDL_SCANCODE_9, io::Key::Num9},
        {SDL_SCANCODE_F1, io::Key::F1}, {SDL_SCANCODE_F2, io::Key::F2}, {SDL_SCANCODE_F3, io::Key::F3},
        {SDL_SCANCODE_F4, io::Key::F4}, {SDL_SCANCODE_F5, io::Key::F5}, {SDL_SCANCODE_F6, io::Key::F6},
        {SDL_SCANCODE_F7, io::Key::F7}, {SDL_SCANCODE_F8, io::Key::F8}, {SDL_SCANCODE_F9, io::Key::F9},
        {SDL_SCANCODE_F10, io::Key::F10}, {SDL_SCANCODE_F11, io::Key::F11}, {SDL_SCANCODE_F12, io::Key::F12},
        {SDL_SCANCODE_UP, io::Key::Up}, {SDL_SCANCODE_DOWN, io::Key::Down}, {SDL_SCANCODE_LEFT, io::Key::Left},
        {SDL_SCANCODE_RIGHT, io::Key::Right}, {SDL_SCANCODE_HOME, io::Key::Home}, {SDL_SCANCODE_END, io::Key::End},
        {SDL_SCANCODE_PAGEUP, io::Key::PageUp}, {SDL_SCANCODE_PAGEDOWN, io::Key::PageDown},
        {SDL_SCANCODE_INSERT, io::Key::Insert}, {SDL_SCANCODE_DELETE, io::Key::Delete},
        {SDL_SCANCODE_SPACE, io::Key::Space}, {SDL_SCANCODE_TAB, io::Key::Tab},
        {SDL_SCANCODE_RETURN, io::Key::Enter}, {SDL_SCANCODE_BACKSPACE, io::Key::Backspace},
        {SDL_SCANCODE_ESCAPE, io::Key::Escape},
        {SDL_SCANCODE_LSHIFT, io::Key::LeftShift}, {SDL_SCANCODE_RSHIFT, io::Key::RightShift},
        {SDL_SCANCODE_LCTRL, io::Key::LeftCtrl}, {SDL_SCANCODE_RCTRL, io::Key::RightCtrl},
        {SDL_SCANCODE_LALT, io::Key::LeftAlt}, {SDL_SCANCODE_RALT, io::Key::RightAlt},
        {SDL_SCANCODE_LGUI, io::Key::LeftSuper}, {SDL_SCANCODE_RGUI, io::Key::RightSuper},
        {SDL_SCANCODE_CAPSLOCK, io::Key::CapsLock}, {SDL_SCANCODE_NUMLOCKCLEAR, io::Key::NumLock},
        {SDL_SCANCODE_SCROLLLOCK, io::Key::ScrollLock},
    }};

    for (const auto& [scancode, key] : mappings) {
        EXPECT_EQ(io::sdl::mapScancode(scancode), key);
    }
}

TEST(SDLKeymapTest, MapRemainingPunctuationNumpadAndMiscScancodes) {
    const std::array<std::pair<int, io::Key>, 26> mappings = {{
        {SDL_SCANCODE_COMMA, io::Key::Comma},
        {SDL_SCANCODE_PERIOD, io::Key::Period},
        {SDL_SCANCODE_SLASH, io::Key::Slash},
        {SDL_SCANCODE_BACKSLASH, io::Key::Backslash},
        {SDL_SCANCODE_SEMICOLON, io::Key::Semicolon},
        {SDL_SCANCODE_APOSTROPHE, io::Key::Apostrophe},
        {SDL_SCANCODE_LEFTBRACKET, io::Key::LeftBracket},
        {SDL_SCANCODE_RIGHTBRACKET, io::Key::RightBracket},
        {SDL_SCANCODE_MINUS, io::Key::Minus},
        {SDL_SCANCODE_EQUALS, io::Key::Equals},
        {SDL_SCANCODE_GRAVE, io::Key::Grave},
        {SDL_SCANCODE_KP_0, io::Key::KP0},
        {SDL_SCANCODE_KP_1, io::Key::KP1},
        {SDL_SCANCODE_KP_2, io::Key::KP2},
        {SDL_SCANCODE_KP_3, io::Key::KP3},
        {SDL_SCANCODE_KP_4, io::Key::KP4},
        {SDL_SCANCODE_KP_5, io::Key::KP5},
        {SDL_SCANCODE_KP_6, io::Key::KP6},
        {SDL_SCANCODE_KP_7, io::Key::KP7},
        {SDL_SCANCODE_KP_8, io::Key::KP8},
        {SDL_SCANCODE_KP_9, io::Key::KP9},
        {SDL_SCANCODE_KP_PERIOD, io::Key::KPDecimal},
        {SDL_SCANCODE_KP_DIVIDE, io::Key::KPDivide},
        {SDL_SCANCODE_KP_MULTIPLY, io::Key::KPMultiply},
        {SDL_SCANCODE_KP_MINUS, io::Key::KPSubtract},
        {SDL_SCANCODE_KP_PLUS, io::Key::KPAdd},
    }};

    for (const auto& [scancode, key] : mappings) {
        EXPECT_EQ(io::sdl::mapScancode(scancode), key);
    }
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_KP_ENTER), io::Key::KPEnter);
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_PRINTSCREEN), io::Key::PrintScreen);
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_PAUSE), io::Key::Pause);
    EXPECT_EQ(io::sdl::mapScancode(SDL_SCANCODE_MENU), io::Key::Menu);
}

TEST(SDLKeymapTest, MapAllModifierFlagsMouseButtonsAndGamepadEnums) {
    auto mods = io::sdl::mapModifiers(KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_GUI | KMOD_CAPS | KMOD_NUM);
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Shift));
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Ctrl));
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Alt));
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Super));
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Caps));
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Num));

    EXPECT_EQ(io::sdl::mapMouseButton(SDL_BUTTON_LEFT), io::MouseButton::Left);
    EXPECT_EQ(io::sdl::mapMouseButton(SDL_BUTTON_MIDDLE), io::MouseButton::Middle);
    EXPECT_EQ(io::sdl::mapMouseButton(SDL_BUTTON_RIGHT), io::MouseButton::Right);
    EXPECT_EQ(io::sdl::mapMouseButton(SDL_BUTTON_X1), io::MouseButton::X1);
    EXPECT_EQ(io::sdl::mapMouseButton(SDL_BUTTON_X2), io::MouseButton::X2);
    EXPECT_EQ(io::sdl::mapMouseButton(0), io::MouseButton::Unknown);

    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_A), io::GamepadButton::A);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_B), io::GamepadButton::B);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_X), io::GamepadButton::X);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_Y), io::GamepadButton::Y);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER), io::GamepadButton::LeftBumper);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), io::GamepadButton::RightBumper);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_BACK), io::GamepadButton::Back);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_START), io::GamepadButton::Start);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_GUIDE), io::GamepadButton::Guide);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_LEFTSTICK), io::GamepadButton::LeftStick);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_RIGHTSTICK), io::GamepadButton::RightStick);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_DPAD_UP), io::GamepadButton::DPadUp);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN), io::GamepadButton::DPadDown);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT), io::GamepadButton::DPadLeft);
    EXPECT_EQ(io::sdl::mapGamepadButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT), io::GamepadButton::DPadRight);
    EXPECT_EQ(io::sdl::mapGamepadButton(-1), io::GamepadButton::Unknown);

    EXPECT_EQ(io::sdl::mapGamepadAxis(SDL_CONTROLLER_AXIS_LEFTX), io::GamepadAxis::LeftX);
    EXPECT_EQ(io::sdl::mapGamepadAxis(SDL_CONTROLLER_AXIS_LEFTY), io::GamepadAxis::LeftY);
    EXPECT_EQ(io::sdl::mapGamepadAxis(SDL_CONTROLLER_AXIS_RIGHTX), io::GamepadAxis::RightX);
    EXPECT_EQ(io::sdl::mapGamepadAxis(SDL_CONTROLLER_AXIS_RIGHTY), io::GamepadAxis::RightY);
    EXPECT_EQ(io::sdl::mapGamepadAxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT), io::GamepadAxis::LeftTrigger);
    EXPECT_EQ(io::sdl::mapGamepadAxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT), io::GamepadAxis::RightTrigger);
    EXPECT_EQ(io::sdl::mapGamepadAxis(-1), io::GamepadAxis::Unknown);
}

// ── Polymorphism via InputDevice* ───────────────────────────────────────

TEST(InputDeviceTest, PolymorphicPoll) {
    io::SDLKeyboard kb;
    io::InputDevice* dev = &kb;

    EXPECT_EQ(dev->name(), "SDL Keyboard");
    EXPECT_EQ(dev->type(), io::DeviceType::Keyboard);
    EXPECT_FALSE(dev->poll().has_value());

    kb.feedSDLEvent(SDL_SCANCODE_RETURN, KMOD_NONE, true, false);
    auto ev = dev->poll();
    ASSERT_TRUE(ev.has_value());
    auto* ke = std::get_if<io::KeyEvent>(&*ev);
    ASSERT_NE(ke, nullptr);
    EXPECT_EQ(ke->key, io::Key::Enter);
}
