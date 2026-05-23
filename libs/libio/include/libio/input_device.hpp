#ifndef LIBIO_INPUT_DEVICE_HPP
#define LIBIO_INPUT_DEVICE_HPP

#include "libio/input_event.hpp"

#include <optional>
#include <string>

namespace io {

/**
 * @brief Abstract interface for any input device.
 *
 * All input sources (keyboards, mice, gamepads, …) implement this
 * interface. The consumer calls poll() in a loop to drain pending
 * events; each call returns the next event or std::nullopt when the
 * queue is empty.
 *
 * Implementations are responsible for translating backend-specific
 * data (SDL, ncurses, Linux evdev, …) into device-agnostic
 * InputEvent values.
 *
 * Threading policy: InputDevice itself is *not* thread-aware.
 * If you need to poll from a dedicated thread, compose it with
 * ThreadEventComponent or your own threading wrapper.
 */
// LCOV_EXCL_START
class InputDevice {
public:
    virtual ~InputDevice() = default;

    /**
     * @brief Poll for the next available input event.
     * @return The next InputEvent, or std::nullopt if no events are pending.
     */
    virtual std::optional<InputEvent> poll() = 0;

    /// Human-readable name of this device (e.g. "SDL Keyboard").
    virtual std::string name() const = 0;

    /// The category of input this device provides.
    virtual DeviceType type() const = 0;

    // Non-copyable, movable
    InputDevice() = default;
    InputDevice(const InputDevice&) = delete;
    InputDevice& operator=(const InputDevice&) = delete;
    InputDevice(InputDevice&&) = default;
    InputDevice& operator=(InputDevice&&) = default;
};
// LCOV_EXCL_STOP

} // namespace io

#endif // LIBIO_INPUT_DEVICE_HPP
