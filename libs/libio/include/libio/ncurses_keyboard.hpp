#ifndef LIBIO_NCURSES_KEYBOARD_HPP
#define LIBIO_NCURSES_KEYBOARD_HPP

#include "libio/input_device.hpp"

namespace io {

namespace ncurses {

/**
 * @brief Convert a single ncurses key code into a normalized InputEvent.
 *
 * @param ch The code returned by getch().
 * @param rows Current terminal row count, used only for KEY_RESIZE.
 * @param cols Current terminal column count, used only for KEY_RESIZE.
 * @return std::nullopt when no key is available (ERR), otherwise the
 *         translated keyboard or resize event.
 */
std::optional<InputEvent> translatePolledKey(int ch, int rows = 0, int cols = 0);

} // namespace ncurses

/**
 * @brief NCurses-based keyboard input device.
 *
 * Calls getch() in poll() and maps the result to a unified KeyEvent.
 * NCurses must be initialized (initscr, cbreak, noecho, nodelay)
 * before calling poll().
 *
 * This class is a pure InputDevice — it no longer inherits from
 * ThreadEventComponent.  If you need threaded polling, wrap it
 * in a ThreadEventComponent or use your own threading.
 */
class NCursesKeyboard : public InputDevice {
public:
    NCursesKeyboard() = default;

    std::optional<InputEvent> poll() override;
    std::string name() const override { return "NCurses Keyboard"; }
    DeviceType  type() const override { return DeviceType::Keyboard; }
};

} // namespace io

#endif // LIBIO_NCURSES_KEYBOARD_HPP
