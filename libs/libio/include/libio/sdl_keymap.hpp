#ifndef LIBIO_SDL_KEYMAP_HPP
#define LIBIO_SDL_KEYMAP_HPP

#include "libio/keycodes.hpp"

struct SDL_Keysym;

namespace io::sdl {

/**
 * @brief Map an SDL scancode to a unified io::Key.
 */
Key mapScancode(int sdlScancode);

/**
 * @brief Map SDL modifier bitmask to io::Modifier flags.
 */
Modifier mapModifiers(unsigned short sdlMod);

/**
 * @brief Map an SDL mouse button id to io::MouseButton.
 */
MouseButton mapMouseButton(unsigned char sdlButton);

/**
 * @brief Map an SDL game-controller button to io::GamepadButton.
 */
GamepadButton mapGamepadButton(int sdlButton);

/**
 * @brief Map an SDL game-controller axis to io::GamepadAxis.
 */
GamepadAxis mapGamepadAxis(int sdlAxis);

} // namespace io::sdl

#endif // LIBIO_SDL_KEYMAP_HPP
