#include "libio/evdev_keyboard.hpp"
#include "libio/evdev_keymap.hpp"

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace io {

EvdevKeyboard::EvdevKeyboard(const std::string& devicePath)
    : mPath(devicePath)
{
    mFd = ::open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
    if (mFd < 0) {
        throw std::runtime_error(
            "EvdevKeyboard: cannot open " + devicePath + ": " + std::strerror(errno));
    }
}

EvdevKeyboard::~EvdevKeyboard() {
    if (mFd >= 0) {
        ::close(mFd);
    }
}

EvdevKeyboard::EvdevKeyboard(EvdevKeyboard&& other) noexcept
    : mFd(other.mFd)
    , mPath(std::move(other.mPath))
    , mModifiers(other.mModifiers)
{
    other.mFd = -1;
}

EvdevKeyboard& EvdevKeyboard::operator=(EvdevKeyboard&& other) noexcept {
    if (this != &other) {
        if (mFd >= 0) ::close(mFd);
        mFd        = other.mFd;
        mPath      = std::move(other.mPath);
        mModifiers = other.mModifiers;
        other.mFd  = -1;
    }
    return *this;
}

std::optional<InputEvent> EvdevKeyboard::poll() {
    struct input_event ev{};

    while (true) {
        ssize_t n = ::read(mFd, &ev, sizeof(ev));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return std::nullopt; // no events pending
            }
            if (errno == EINTR) {
                continue; // interrupted, retry
            }
            return std::nullopt; // other error — treat as empty
        }
        if (n != sizeof(ev)) {
            continue; // partial read — skip
        }

        // We only care about EV_KEY events with value 0 (release), 1 (press), 2 (repeat)
        if (ev.type != EV_KEY || ev.value < 0 || ev.value > 2) {
            continue;
        }

        Key key = evdev::mapKey(ev.code);
        bool pressed = (ev.value != 0);

        // Update tracked modifier state
        updateModifierState(key, pressed);

        KeyEvent ke;
        ke.key       = key;
        ke.modifiers = mModifiers;

        switch (ev.value) {
            case 0: ke.action = Action::Release; break;
            case 1: ke.action = Action::Press;   break;
            case 2: ke.action = Action::Repeat;  break;
        }

        return InputEvent{ke};
    }
}

void EvdevKeyboard::updateModifierState(Key key, bool pressed) {
    Modifier flag = Modifier::None;

    switch (key) {
        case Key::LeftShift:
        case Key::RightShift:
            flag = Modifier::Shift;
            break;
        case Key::LeftCtrl:
        case Key::RightCtrl:
            flag = Modifier::Ctrl;
            break;
        case Key::LeftAlt:
        case Key::RightAlt:
            flag = Modifier::Alt;
            break;
        case Key::LeftSuper:
        case Key::RightSuper:
            flag = Modifier::Super;
            break;
        case Key::CapsLock:
            // Toggle on press only
            if (pressed) {
                mModifiers = static_cast<Modifier>(
                    static_cast<unsigned>(mModifiers) ^ static_cast<unsigned>(Modifier::Caps));
            }
            return;
        case Key::NumLock:
            if (pressed) {
                mModifiers = static_cast<Modifier>(
                    static_cast<unsigned>(mModifiers) ^ static_cast<unsigned>(Modifier::Num));
            }
            return;
        default:
            return;
    }

    if (pressed) {
        mModifiers = mModifiers | flag;
    } else {
        mModifiers = static_cast<Modifier>(
            static_cast<unsigned>(mModifiers) & ~static_cast<unsigned>(flag));
    }
}

} // namespace io
