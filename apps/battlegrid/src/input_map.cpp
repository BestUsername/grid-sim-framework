#include "input_map.hpp"

#include <algorithm>

namespace battlegrid {

// ── Binding API ──────────────────────────────────────────────────────

void InputMap::bindKey(io::Key key, GameAction action, float scale) {
    m_keyBindings[static_cast<int>(key)] = {action, scale};
}

void InputMap::bindButton(io::GamepadButton button, GameAction action, float scale) {
    m_gpButtonBindings[static_cast<int>(button)] = {action, scale};
}

void InputMap::bindAxis(io::GamepadAxis axis, GameAction action, float scale) {
    m_gpAxisBindings[static_cast<int>(axis)] = {action, scale};
}

void InputMap::bindMouseX(GameAction action, float scale) {
    m_mouseDx = MouseBinding{action, scale};
}

void InputMap::bindMouseY(GameAction action, float scale) {
    m_mouseDy = MouseBinding{action, scale};
}

void InputMap::bindScrollY(GameAction action, float scale) {
    m_scrollY = MouseBinding{action, scale};
}

// ── Event processing ─────────────────────────────────────────────────

void InputMap::processEvent(const io::InputEvent& event) {
    if (auto* ke = std::get_if<io::KeyEvent>(&event)) {
        int k = static_cast<int>(ke->key);
        auto it = m_keyBindings.find(k);
        if (it == m_keyBindings.end()) return;

        bool wasHeld = m_keysHeld[k];
        bool isHeld = (ke->action != io::Action::Release);
        m_keysHeld[k] = isHeld;

        if (isHeld && !wasHeld)
            m_justPressed[static_cast<int>(it->second.action)] = true;
    }
    else if (auto* mm = std::get_if<io::MouseMoveEvent>(&event)) {
        if (m_mouseDx)
            m_frameDelta[static_cast<int>(m_mouseDx->action)] += mm->dx * m_mouseDx->scale;
        if (m_mouseDy)
            m_frameDelta[static_cast<int>(m_mouseDy->action)] += mm->dy * m_mouseDy->scale;
    }
    else if (auto* ms = std::get_if<io::MouseScrollEvent>(&event)) {
        if (m_scrollY)
            m_frameDelta[static_cast<int>(m_scrollY->action)] += ms->scrollY * m_scrollY->scale;
    }
    else if (auto* gb = std::get_if<io::GamepadButtonEvent>(&event)) {
        int b = static_cast<int>(gb->button);
        auto it = m_gpButtonBindings.find(b);
        if (it == m_gpButtonBindings.end()) return;

        bool wasHeld = m_gpButtonsHeld[b];
        bool isHeld = (gb->action == io::Action::Press);
        m_gpButtonsHeld[b] = isHeld;

        if (isHeld && !wasHeld)
            m_justPressed[static_cast<int>(it->second.action)] = true;
    }
    else if (auto* ga = std::get_if<io::GamepadAxisEvent>(&event)) {
        m_gpAxisValues[static_cast<int>(ga->axis)] = ga->value;
    }
}

// ── Query API ────────────────────────────────────────────────────────

float InputMap::axis(GameAction action) const {
    float value = 0.0f;

    // Digital key contributions
    for (const auto& [key, binding] : m_keyBindings) {
        if (binding.action != action) continue;
        auto it = m_keysHeld.find(key);
        if (it != m_keysHeld.end() && it->second)
            value += binding.scale;
    }

    // Digital gamepad button contributions
    for (const auto& [btn, binding] : m_gpButtonBindings) {
        if (binding.action != action) continue;
        auto it = m_gpButtonsHeld.find(btn);
        if (it != m_gpButtonsHeld.end() && it->second)
            value += binding.scale;
    }

    // Analog gamepad axis contributions
    for (const auto& [ax, binding] : m_gpAxisBindings) {
        if (binding.action != action) continue;
        auto it = m_gpAxisValues.find(ax);
        if (it != m_gpAxisValues.end())
            value += it->second * binding.scale;
    }

    return std::clamp(value, -1.0f, 1.0f);
}

float InputMap::delta(GameAction action) const {
    return m_frameDelta[static_cast<int>(action)];
}

bool InputMap::pressed(GameAction action) const {
    return m_justPressed[static_cast<int>(action)];
}

void InputMap::endFrame() {
    m_frameDelta.fill(0.0f);
    m_justPressed.fill(false);
}

void InputMap::clear()
{
    m_keysHeld.clear();
    m_gpButtonsHeld.clear();
    m_gpAxisValues.clear();
    endFrame();
}

} // namespace battlegrid
