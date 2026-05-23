#include "libio/sdl_keyboard.hpp"
#include "libio/sdl_keymap.hpp"

namespace io {

std::optional<InputEvent> SDLKeyboard::poll() {
    if (mQueue.empty()) {
        return std::nullopt;
    }
    auto ev = mQueue.front();
    mQueue.pop();
    return ev;
}

void SDLKeyboard::feedSDLEvent(int sdlScancode, unsigned short sdlMod,
                               bool pressed, bool repeat) {
    KeyEvent ke;
    ke.key       = sdl::mapScancode(sdlScancode);
    ke.action    = repeat ? Action::Repeat : (pressed ? Action::Press : Action::Release);
    ke.modifiers = sdl::mapModifiers(sdlMod);

    mQueue.push(InputEvent{ke});
}

} // namespace io