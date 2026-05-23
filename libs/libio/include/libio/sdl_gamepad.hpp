#ifndef LIBIO_SDL_GAMEPAD_HPP
#define LIBIO_SDL_GAMEPAD_HPP

#include "libio/input_device.hpp"

#include <queue>

namespace io {

/**
 * @brief SDL2-backed gamepad input device.
 *
 * Feed SDL game-controller events (button, axis) via the appropriate
 * feed methods, then drain via poll().
 */
class SDLGamepad : public InputDevice {
public:
    SDLGamepad() = default;

    std::optional<InputEvent> poll() override;
    std::string name() const override { return "SDL Gamepad"; }
    DeviceType  type() const override { return DeviceType::Gamepad; }

    /// Enqueue a gamepad button event.
    void feedButton(int gamepadIndex, int sdlButton, bool pressed);

    /// Enqueue a gamepad axis event.  `value` is the raw SDL axis value (−32768..32767).
    void feedAxis(int gamepadIndex, int sdlAxis, int value);

private:
    std::queue<InputEvent> mQueue;
};

} // namespace io

#endif // LIBIO_SDL_GAMEPAD_HPP
