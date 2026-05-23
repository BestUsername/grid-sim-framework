#ifndef LIBIO_SDL_MOUSE_HPP
#define LIBIO_SDL_MOUSE_HPP

#include "libio/input_device.hpp"

#include <queue>

namespace io {

/**
 * @brief SDL2-backed mouse input device.
 *
 * Feed SDL mouse events (motion, button, wheel) via the appropriate
 * feed methods, then drain via poll().
 */
class SDLMouse : public InputDevice {
public:
    SDLMouse() = default;

    std::optional<InputEvent> poll() override;
    std::string name() const override { return "SDL Mouse"; }
    DeviceType  type() const override { return DeviceType::Mouse; }

    /// Enqueue a mouse motion event.
    void feedMotion(int x, int y, int dx, int dy);

    /// Enqueue a mouse button event.
    void feedButton(unsigned char sdlButton, bool pressed, int x, int y);

    /// Enqueue a mouse scroll event.
    void feedScroll(int scrollX, int scrollY);

private:
    std::queue<InputEvent> mQueue;
};

} // namespace io

#endif // LIBIO_SDL_MOUSE_HPP
