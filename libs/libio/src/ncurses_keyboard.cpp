#include "libio/ncurses_keyboard.hpp"
#include "libio/ncurses_keymap.hpp"

#include <ncurses.h>

namespace io {

std::optional<InputEvent> ncurses::translatePolledKey(int ch, int rows, int cols) {
    if (ch == ERR) {
        return std::nullopt;
    }

    if (ch == KEY_RESIZE) {
        ResizeEvent re;
        re.width = cols;
        re.height = rows;
        return InputEvent{re};
    }

    auto [key, mods] = ncurses::mapKeyWithModifiers(ch);

    KeyEvent ke;
    ke.key = key;
    ke.action = Action::Press;
    ke.modifiers = mods;
    return InputEvent{ke};
}

// LCOV_EXCL_START
std::optional<InputEvent> NCursesKeyboard::poll() {
    int ch = getch();
    if (ch == KEY_RESIZE) {
        int rows = 0, cols = 0;
        getmaxyx(stdscr, rows, cols);
        return ncurses::translatePolledKey(ch, rows, cols);
    }
    return ncurses::translatePolledKey(ch);
}
// LCOV_EXCL_STOP

} // namespace io
