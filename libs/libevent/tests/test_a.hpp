#ifndef TEST_A_HPP
#define TEST_A_HPP

#include <cmath>
#include "libevent/thread_event_component.hpp"

using namespace grid::libevent;

class A : public ThreadEventComponent
{
public:
    A() : ThreadEventComponent()
    {
    }

    void Init() override
    {
        Subscribe("a stop");
        Subscribe("a");
    }

    void MainLoop() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void ProcessEvent(Event *incoming) override
    {
        if (incoming->GetKey() == "a")
        {
            auto e = event_cast<std::vector<int>>(incoming);

            m_data = e->GetObject();
            for (unsigned int i = 0; i < m_data.size(); i++)
            {
                m_data[i] = std::sqrt(m_data[i]);
            }

            SendEvent(TEvent<std::vector<int>>("a done", m_data));
        }
        else if (incoming->GetKey() == "a stop")
        {
            Stop();
        }
    }

private:
    std::vector<int> m_data;
};

#endif // TEST_A_HPP
