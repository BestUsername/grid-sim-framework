#ifndef BATTLEGRID_INPUT_MAP_HPP_INCLUDED
#define BATTLEGRID_INPUT_MAP_HPP_INCLUDED

#include "libio/input_event.hpp"
#include "libio/keycodes.hpp"

#include <array>
#include <optional>
#include <unordered_map>

namespace battlegrid {

/// Logical game actions that can be bound to physical inputs.
enum class GameAction : int {
    // Axis actions (continuous values).
    // State-based (held keys / stick positions): queried via axis().
    // Delta-based  (mouse motion / scroll):     queried via delta().
    MoveX,          ///< Strafe: -1 left, +1 right
    MoveZ,          ///< Forward/back: -1 forward, +1 backward
    LookX,          ///< Yaw
    LookY,          ///< Pitch
    Zoom,           ///< Camera distance change
    Sprint,         ///< Sprint factor: 0 walk, 1 full sprint (analog-capable)

    // Button actions (edge-triggered): queried via pressed().
    Jump,           ///< Jump (when grounded)
    Interact,       ///< Mount / dismount
    Shout,          ///< Shout
    ToggleCamera,   ///< Switch FPS / third-person
    ToggleMap,      ///< Toggle overhead map overlay

    Count
};

/**
 * @brief Configurable mapping from physical inputs to game actions.
 *
 * Supports binding keys, gamepad buttons, gamepad axes, mouse motion,
 * and mouse scroll to any GameAction.  Multiple physical inputs may
 * contribute to the same action; their values are summed.
 *
 * There are two query modes for continuous actions:
 *  - axis()  — persistent state from held keys and gamepad axes.
 *  - delta() — per-frame accumulation from mouse motion / scroll.
 *
 * Digital button sources (keys, gamepad buttons) contribute their
 * configured scale when held.  Analog sources (gamepad axes) contribute
 * value × scale.  Mouse / scroll contribute instantaneous delta × scale.
 *
 * Call endFrame() once per frame to reset deltas and edge flags.
 */
class InputMap {
public:
    // ── Binding API ──────────────────────────────────────────────

    void bindKey(io::Key key, GameAction action, float scale = 1.0f);
    void bindButton(io::GamepadButton button, GameAction action, float scale = 1.0f);
    void bindAxis(io::GamepadAxis axis, GameAction action, float scale = 1.0f);
    void bindMouseX(GameAction action, float scale = 1.0f);
    void bindMouseY(GameAction action, float scale = 1.0f);
    void bindScrollY(GameAction action, float scale = 1.0f);

    // ── Event processing ─────────────────────────────────────────

    /// Feed an input event.  Updates held state and frame deltas.
    void processEvent(const io::InputEvent& event);

    // ── Query API ────────────────────────────────────────────────

    /// Persistent axis value from held keys and gamepad axes.
    float axis(GameAction action) const;

    /// Per-frame delta from mouse motion and scroll events.
    float delta(GameAction action) const;

    /// True if the action was activated this frame (rising edge).
    bool pressed(GameAction action) const;

    /// Reset per-frame deltas and edge-triggered flags.
    void endFrame();

    /// Clear held and pending input when the display loses focus.
    void clear();

private:
    struct Binding { GameAction action; float scale; };
    struct MouseBinding { GameAction action; float scale; };

    // Physical input → binding maps
    std::unordered_map<int, Binding> m_keyBindings;
    std::unordered_map<int, Binding> m_gpButtonBindings;
    std::unordered_map<int, Binding> m_gpAxisBindings;
    std::optional<MouseBinding> m_mouseDx;
    std::optional<MouseBinding> m_mouseDy;
    std::optional<MouseBinding> m_scrollY;

    // Tracked held state
    std::unordered_map<int, bool> m_keysHeld;
    std::unordered_map<int, bool> m_gpButtonsHeld;
    std::unordered_map<int, float> m_gpAxisValues;

    // Per-frame accumulators
    static constexpr int kActionCount = static_cast<int>(GameAction::Count);
    std::array<float, kActionCount> m_frameDelta{};
    std::array<bool, kActionCount> m_justPressed{};
};

} // namespace battlegrid

#endif // BATTLEGRID_INPUT_MAP_HPP_INCLUDED
