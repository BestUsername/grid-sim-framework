#include "libevent/thread_event_component.hpp"

namespace grid::libevent {

ThreadEventComponent::ThreadEventComponent(EventBus& bus)
    : EventComponent(bus)
{
    m_shouldExit = false;
}

ThreadEventComponent::ThreadEventComponent()
{
    m_shouldExit = false;
}

ThreadEventComponent::~ThreadEventComponent()
{
    Stop();
    WaitToFinish();
}

void ThreadEventComponent::Start(bool detached)
{
    m_shouldExit = false;
    m_thread = std::thread(&ThreadEventComponent::threadLoop, this);
    if (detached)
        m_thread.detach();
}

void ThreadEventComponent::Stop()
{
    m_shouldExit = true;
    m_queueCV.notify_one();
}

void ThreadEventComponent::ForceStop()
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_eventQueue.empty())
        {
            m_eventQueue.pop();
        }
    }
    m_shouldExit = true;
    m_queueCV.notify_one();
}

void ThreadEventComponent::WaitToFinish()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ThreadEventComponent::threadLoop()
{
    Init();
    while (true)
    {
        std::unique_ptr<Event> event;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this] {
                return !m_eventQueue.empty() || m_shouldExit.load();
            });

            if (!m_eventQueue.empty())
            {
                event = std::move(m_eventQueue.front());
                m_eventQueue.pop();
            }
            else if (m_shouldExit)
            {
                break;
            }
        }

        if (event)
        {
            ProcessEvent(event.get());
        }

        MainLoop();
    }
}

} // namespace grid::libevent