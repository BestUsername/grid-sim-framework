#include "libio/sdl_mouse.hpp"
#include "libio/sdl_keymap.hpp"

namespace io {

std::optional<InputEvent> SDLMouse::poll() {
    if (mQueue.empty()) {
        return std::nullopt;
    }
    auto ev = mQueue.front();
    mQueue.pop();
    return ev;
}

void SDLMouse::feedMotion(int x, int y, int dx, int dy) {
    MouseMoveEvent me;
    me.x  = x;
    me.y  = y;
    me.dx = dx;
    me.dy = dy;
    mQueue.push(InputEvent{me});
}

void SDLMouse::feedButton(unsigned char sdlButton, bool pressed, int x, int y) {
    MouseButtonEvent be;
    be.button = sdl::mapMouseButton(sdlButton);
    be.action = pressed ? Action::Press : Action::Release;
    be.x      = x;
    be.y      = y;
    mQueue.push(InputEvent{be});
}

void SDLMouse::feedScroll(int scrollX, int scrollY) {
    MouseScrollEvent se;
    se.scrollX = scrollX;
    se.scrollY = scrollY;
    mQueue.push(InputEvent{se});
}

} // namespace io
