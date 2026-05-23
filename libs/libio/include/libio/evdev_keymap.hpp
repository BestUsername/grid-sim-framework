#ifndef LIBIO_EVDEV_KEYMAP_HPP
#define LIBIO_EVDEV_KEYMAP_HPP

#include "libio/keycodes.hpp"

namespace io::evdev {

/**
 * @brief Map a Linux input.h KEY_* code to a unified io::Key.
 */
Key mapKey(int linuxKeyCode);

} // namespace io::evdev

#endif // LIBIO_EVDEV_KEYMAP_HPP
