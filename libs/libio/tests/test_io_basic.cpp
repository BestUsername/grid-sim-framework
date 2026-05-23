#include "gtest/gtest.h"
#include "libio/input_event.hpp"
#include "libio/keycodes.hpp"

// ── InputEvent variant helpers ──────────────────────────────────────────

TEST(InputEventTest, KeyEventHoldsCorrectType) {
    io::KeyEvent ke{io::Key::A, io::Action::Press, io::Modifier::None};
    io::InputEvent ev{ke};

    EXPECT_EQ(io::deviceType(ev), io::DeviceType::Keyboard);
    auto* p = std::get_if<io::KeyEvent>(&ev);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->key, io::Key::A);
    EXPECT_EQ(p->action, io::Action::Press);
}

TEST(InputEventTest, MouseMoveEventHoldsCorrectType) {
    io::MouseMoveEvent me{10, 20, 1, -1};
    io::InputEvent ev{me};

    EXPECT_EQ(io::deviceType(ev), io::DeviceType::Mouse);
    auto* p = std::get_if<io::MouseMoveEvent>(&ev);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->x, 10);
    EXPECT_EQ(p->dy, -1);
}

TEST(InputEventTest, MouseButtonEventHoldsCorrectType) {
    io::MouseButtonEvent be{io::MouseButton::Left, io::Action::Press, 5, 6};
    io::InputEvent ev{be};

    EXPECT_EQ(io::deviceType(ev), io::DeviceType::Mouse);
    auto* p = std::get_if<io::MouseButtonEvent>(&ev);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->button, io::MouseButton::Left);
}

TEST(InputEventTest, MouseScrollEventHoldsCorrectType) {
    io::MouseScrollEvent se{0, 3};
    io::InputEvent ev{se};

    EXPECT_EQ(io::deviceType(ev), io::DeviceType::Mouse);
    auto* p = std::get_if<io::MouseScrollEvent>(&ev);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->scrollY, 3);
}

TEST(InputEventTest, GamepadButtonEventHoldsCorrectType) {
    io::GamepadButtonEvent gbe{0, io::GamepadButton::A, io::Action::Press};
    io::InputEvent ev{gbe};

    EXPECT_EQ(io::deviceType(ev), io::DeviceType::Gamepad);
    auto* p = std::get_if<io::GamepadButtonEvent>(&ev);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->button, io::GamepadButton::A);
}

TEST(InputEventTest, GamepadAxisEventHoldsCorrectType) {
    io::GamepadAxisEvent gae{1, io::GamepadAxis::LeftX, 0.5f};
    io::InputEvent ev{gae};

    EXPECT_EQ(io::deviceType(ev), io::DeviceType::Gamepad);
    auto* p = std::get_if<io::GamepadAxisEvent>(&ev);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->gamepadIndex, 1);
    EXPECT_FLOAT_EQ(p->value, 0.5f);
}

TEST(InputEventTest, ResizeEventHoldsCorrectType) {
    io::ResizeEvent re{120, 40};
    io::InputEvent ev{re};

    EXPECT_EQ(io::deviceType(ev), io::DeviceType::Window);
    auto* p = std::get_if<io::ResizeEvent>(&ev);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->width, 120);
    EXPECT_EQ(p->height, 40);
}

// ── Modifier bitmask helpers ────────────────────────────────────────────

TEST(ModifierTest, CombineAndTest) {
    auto mods = io::Modifier::Shift | io::Modifier::Ctrl;
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Shift));
    EXPECT_TRUE(io::hasModifier(mods, io::Modifier::Ctrl));
    EXPECT_FALSE(io::hasModifier(mods, io::Modifier::Alt));
}

TEST(ModifierTest, NoneIsEmpty) {
    EXPECT_FALSE(io::hasModifier(io::Modifier::None, io::Modifier::Shift));
}

// ── Keycodes enum sanity ────────────────────────────────────────────────

TEST(KeycodesTest, LetterRange) {
    // A..Z should be 26 consecutive values
    int a = static_cast<int>(io::Key::A);
    int z = static_cast<int>(io::Key::Z);
    EXPECT_EQ(z - a, 25);
}

TEST(KeycodesTest, MouseButtonRange) {
    EXPECT_NE(io::MouseButton::Left, io::MouseButton::Right);
    EXPECT_NE(io::MouseButton::Unknown, io::MouseButton::Left);
}

TEST(KeycodesTest, GamepadButtonRange) {
    EXPECT_NE(io::GamepadButton::A, io::GamepadButton::B);
    EXPECT_NE(io::GamepadButton::Unknown, io::GamepadButton::A);
}

