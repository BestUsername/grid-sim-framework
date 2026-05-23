#ifndef LIBIO_EVDEV_KEYBOARD_HPP
#define LIBIO_EVDEV_KEYBOARD_HPP

#include "libio/input_device.hpp"
#include "libio/keycodes.hpp"

#include <string>

namespace io {

/**
 * @brief Linux evdev keyboard input device.
 *
 * Reads directly from a /dev/input/eventN device file using the
 * Linux input subsystem. Reports individual key presses, releases,
 * and repeats — including modifier keys (Shift, Ctrl, Alt, Super).
 *
 * Tracks modifier state internally so every KeyEvent includes the
 * current modifier bitmask.
 *
 * Requirements:
 *  - Linux only
 *  - Read permission on the device (typically requires root or
 *    membership in the "input" group)
 *  - The device file must be opened in non-blocking mode (O_NONBLOCK)
 *    for poll() to return std::nullopt when no events are pending.
 *
 * Usage:
 * @code
 *   io::EvdevKeyboard kb("/dev/input/event4");
 *   while (auto ev = kb.poll()) { ... }
 * @endcode
 */
class EvdevKeyboard : public InputDevice {
public:
    /**
     * @brief Open an evdev device for reading.
     * @param devicePath  Path to the input device (e.g. "/dev/input/event4").
     * @throws std::runtime_error if the device cannot be opened.
     */
    explicit EvdevKeyboard(const std::string& devicePath);

    ~EvdevKeyboard() override;

    // Non-copyable
    EvdevKeyboard(const EvdevKeyboard&) = delete;
    EvdevKeyboard& operator=(const EvdevKeyboard&) = delete;

    // Movable
    EvdevKeyboard(EvdevKeyboard&& other) noexcept;
    EvdevKeyboard& operator=(EvdevKeyboard&& other) noexcept;

    std::optional<InputEvent> poll() override;
    std::string name() const override { return "evdev Keyboard (" + mPath + ")"; }
    DeviceType  type() const override { return DeviceType::Keyboard; }

private:
    void updateModifierState(Key key, bool pressed);

    int         mFd   = -1;
    std::string mPath;
    Modifier    mModifiers = Modifier::None;
};

} // namespace io

#endif // LIBIO_EVDEV_KEYBOARD_HPP
