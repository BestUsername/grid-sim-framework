#include "libio/sdl_gamepad.hpp"
#include "libio/sdl_keymap.hpp"

#include <algorithm>

namespace io {

std::optional<InputEvent> SDLGamepad::poll() {
    if (mQueue.empty()) {
        return std::nullopt;
    }
    auto ev = mQueue.front();
    mQueue.pop();
    return ev;
}

void SDLGamepad::feedButton(int gamepadIndex, int sdlButton, bool pressed) {
    GamepadButtonEvent be;
    be.gamepadIndex = gamepadIndex;
    be.button       = sdl::mapGamepadButton(sdlButton);
    be.action       = pressed ? Action::Press : Action::Release;
    mQueue.push(InputEvent{be});
}

void SDLGamepad::feedAxis(int gamepadIndex, int sdlAxis, int value) {
    GamepadAxisEvent ae;
    ae.gamepadIndex = gamepadIndex;
    ae.axis         = sdl::mapGamepadAxis(sdlAxis);
    // Normalize: SDL axis range is −32768..32767 → −1.0..1.0
    ae.value = std::clamp(static_cast<float>(value) / 32767.0f, -1.0f, 1.0f);
    mQueue.push(InputEvent{ae});
}

} // namespace io
