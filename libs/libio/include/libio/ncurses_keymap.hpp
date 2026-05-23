#ifndef LIBIO_NCURSES_KEYMAP_HPP
#define LIBIO_NCURSES_KEYMAP_HPP

#include "libio/keycodes.hpp"

#include <utility>

namespace io::ncurses {

/**
 * @brief Map an ncurses key code (from getch()) to a unified io::Key.
 */
Key mapKey(int ncursesKey);

/**
 * @brief Map an ncurses key code to a Key + Modifier pair.
 *
 * Detects:
 *  - Ctrl+letter  (codes 1–26 → Key::A–Z with Modifier::Ctrl)
 *  - Shift+letter (uppercase 'A'–'Z' → Key::A–Z with Modifier::Shift)
 *
 * For all other keys, the modifier is Modifier::None.
 */
std::pair<Key, Modifier> mapKeyWithModifiers(int ncursesKey);

} // namespace io::ncurses

#endif // LIBIO_NCURSES_KEYMAP_HPP
