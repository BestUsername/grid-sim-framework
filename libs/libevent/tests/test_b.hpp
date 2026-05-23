#ifndef TEST_B_HPP
#define TEST_B_HPP

#include <cstdlib>
#include "libevent/thread_event_component.hpp"

using namespace grid::libevent;

static int compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

class B : public ThreadEventComponent
{
public:
    B() : ThreadEventComponent()
    {
    }

    void Init() override
    {
        Subscribe("b stop");
        Subscribe("b");
    }

    void MainLoop() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void ProcessEvent(Event *incoming) override
    {
        if (incoming->GetKey() == "b")
        {
            auto e = event_cast<std::vector<int>>(incoming);

            m_data = e->GetObject();
            qsort(&m_data[0], m_data.size(), sizeof(int), compare);

            SendEvent(TEvent<std::vector<int>>("b done", m_data));
        }
        else if (incoming->GetKey() == "b stop")
        {
            Stop();
        }
    }

private:
    std::vector<int> m_data;
};

#endif // TEST_B_HPP
