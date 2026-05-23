#ifndef LIBIO_INPUT_EVENT_HPP
#define LIBIO_INPUT_EVENT_HPP

#include "libio/keycodes.hpp"

#include <variant>
#include <string>

namespace io {

/**
 * @brief The type of action that triggered an input event.
 */
enum class Action {
    Press,
    Release,
    Repeat
};

/**
 * @brief Identifies the category of input device.
 */
enum class DeviceType {
    Keyboard,
    Mouse,
    Gamepad,
    Window
};

// ── Concrete event payloads ─────────────────────────────────────────────

/**
 * @brief A key press / release / repeat event.
 */
struct KeyEvent {
    Key       key       = Key::Unknown;
    Action    action    = Action::Press;
    Modifier  modifiers = Modifier::None;
};

/**
 * @brief A mouse motion, button, or scroll event.
 *
 * For motion events, `button` is MouseButton::Unknown and
 * `action` is Action::Press (arbitrary — check dx/dy instead).
 */
struct MouseMoveEvent {
    int x  = 0;   ///< Absolute X position (if available)
    int y  = 0;   ///< Absolute Y position (if available)
    int dx = 0;   ///< Relative X motion
    int dy = 0;   ///< Relative Y motion
};

struct MouseButtonEvent {
    MouseButton button = MouseButton::Unknown;
    Action      action = Action::Press;
    int         x      = 0;   ///< X position at time of click
    int         y      = 0;   ///< Y position at time of click
};

struct MouseScrollEvent {
    int scrollX = 0;  ///< Horizontal scroll amount
    int scrollY = 0;  ///< Vertical scroll amount
};

/**
 * @brief A gamepad button event.
 */
struct GamepadButtonEvent {
    int           gamepadIndex = 0;
    GamepadButton button       = GamepadButton::Unknown;
    Action        action       = Action::Press;
};

/**
 * @brief A gamepad axis motion event.
 */
struct GamepadAxisEvent {
    int         gamepadIndex = 0;
    GamepadAxis axis         = GamepadAxis::Unknown;
    float       value        = 0.0f;  ///< Normalized to [-1, 1] (triggers: [0, 1])
};

/**
 * @brief A terminal / window resize event.
 *
 * Emitted when the hosting terminal or window changes size.
 * For ncurses this corresponds to KEY_RESIZE (SIGWINCH),
 * for SDL it maps to SDL_WINDOWEVENT_RESIZED.
 */
struct ResizeEvent {
    int width  = 0;   ///< New width  (columns / pixels)
    int height = 0;   ///< New height (rows    / pixels)
};

// ── Unified event type ──────────────────────────────────────────────────

/**
 * @brief A device-agnostic input event.
 *
 * Wraps one of the concrete event payloads in a std::variant so that
 * consumers can handle any input uniformly via std::visit / std::get_if.
 */
using InputEvent = std::variant<
    KeyEvent,
    MouseMoveEvent,
    MouseButtonEvent,
    MouseScrollEvent,
    GamepadButtonEvent,
    GamepadAxisEvent,
    ResizeEvent
>;

/// Helper: return the DeviceType for a given InputEvent.
inline DeviceType deviceType(const InputEvent& e) {
    return std::visit([](auto&& arg) -> DeviceType {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, KeyEvent>)
            return DeviceType::Keyboard;
        else if constexpr (std::is_same_v<T, MouseMoveEvent>
                        || std::is_same_v<T, MouseButtonEvent>
                        || std::is_same_v<T, MouseScrollEvent>)
            return DeviceType::Mouse;
        else if constexpr (std::is_same_v<T, ResizeEvent>)
            return DeviceType::Window;
        else
            return DeviceType::Gamepad;
    }, e);
}

} // namespace io

#endif // LIBIO_INPUT_EVENT_HPP
