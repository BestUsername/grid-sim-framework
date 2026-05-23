#ifndef THREAD_EVENT_COMPONENT_HPP
#define THREAD_EVENT_COMPONENT_HPP

#include <thread>
#include <atomic>

#include "libevent/event_component.hpp"

namespace grid::libevent {

/// Thread-owning event component.  Inherits the event queue and pub/sub
/// from EventComponent and adds a dedicated thread that blocks on the
/// condition variable, processing events as they arrive.
///
/// Subclasses must implement Init(), MainLoop(), and ProcessEvent().
class ThreadEventComponent : public EventComponent
{
public:
    /// Construct with an explicit bus.
    explicit ThreadEventComponent(EventBus& bus);

    /// Construct without a bus (must call SetBus() before Subscribe/Send).
    ThreadEventComponent();

    ~ThreadEventComponent() override;

    void Start(bool detached = false);
    void Stop();
    void ForceStop();
    void WaitToFinish();

    virtual void Init() = 0;
    virtual void MainLoop() = 0;

protected:
    std::thread m_thread;
    std::atomic<bool> m_shouldExit;

    void threadLoop();
};

} // namespace grid::libevent

#endif // THREAD_EVENT_COMPONENT_HPP
