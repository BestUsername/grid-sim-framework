#ifndef LIBIO_SDL_KEYBOARD_HPP
#define LIBIO_SDL_KEYBOARD_HPP

#include "libio/input_device.hpp"

#include <queue>

namespace io {

/**
 * @brief SDL2-backed keyboard input device.
 *
 * Call feedSDLEvent() with each SDL_KEYDOWN / SDL_KEYUP event obtained
 * from SDL_PollEvent, then drain via poll().
 *
 * This class does NOT call SDL_PollEvent itself — that belongs to the
 * application's event loop, which may also need to handle window and
 * quit events.  See SDLMouse / SDLGamepad for the same pattern.
 */
class SDLKeyboard : public InputDevice {
public:
    SDLKeyboard() = default;

    std::optional<InputEvent> poll() override;
    std::string name() const override { return "SDL Keyboard"; }
    DeviceType  type() const override { return DeviceType::Keyboard; }

    /**
     * @brief Translate and enqueue an SDL keyboard event.
     *
     * @param sdlScancode  SDL_Scancode value
     * @param sdlMod       SDL modifier bitmask (SDL_Keymod)
     * @param pressed      true for key-down, false for key-up
     * @param repeat       true if this is an auto-repeat
     */
    void feedSDLEvent(int sdlScancode, unsigned short sdlMod,
                      bool pressed, bool repeat);

private:
    std::queue<InputEvent> mQueue;
};

} // namespace io

#endif // LIBIO_SDL_KEYBOARD_HPP
