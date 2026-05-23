#include <iostream>
#include <random>

#include <cmath>
#include "libevent/thread_event_component.hpp"
#include "libevent/event_bus.hpp"

using namespace grid::libevent;

static int compare(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

class A : public ThreadEventComponent {
public:
    explicit A(EventBus& bus) : ThreadEventComponent(bus)
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

class B : public ThreadEventComponent {
public:
    explicit B(EventBus& bus) : ThreadEventComponent(bus)
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


class Master : public ThreadEventComponent {
public:
    explicit Master(EventBus& bus) : ThreadEventComponent(bus)
    {
    }

    void Init() override
    {
        Subscribe("a done");
        Subscribe("b done");
    }

    void MainLoop() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void ProcessEvent(Event *incoming) override
    {
        if (incoming->GetKey() == "a done")
        {
            auto *e = event_cast<std::vector<int>>(incoming);
            std::cout << "A finished" << std::endl;
            m_dataSetA = e->GetObject();

            for (unsigned int i = 0; i < m_dataSetA.size(); i++)
            {
                std::cout << m_dataSetA[i] << " ";
            }
            std::cout << std::endl
                      << std::endl;
        }
        else if (incoming->GetKey() == "b done")
        {
            auto *e = event_cast<std::vector<int>>(incoming);
            std::cout << "B finished" << std::endl;
            m_dataSetB = e->GetObject();

            for (unsigned int i = 0; i < m_dataSetB.size(); i++)
            {
                std::cout << m_dataSetB[i] << " ";
            }
            std::cout << std::endl
                      << std::endl;
        }
    }

private:
    std::vector<int> m_dataSetA;
    std::vector<int> m_dataSetB;
};

int main()
{
    srand(time(0));

    EventBus bus;

    A a(bus);
    B b(bus);
    std::cout << "Starting A" << std::endl;
    a.Start();
    std::cout << "Starting B" << std::endl;
    b.Start();

    std::cout << "Generating Data" << std::endl;
    std::vector<int> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(rand() % 100);
    }

    Master master(bus);
    std::cout << "Starting Master" << std::endl;
    master.Start();

    std::cout << "a1" << std::endl;
    master.SendEvent(TEvent<std::vector<int>>("a", data));
    std::cout << "b1" << std::endl;
    master.SendEvent(TEvent<std::vector<int>>("b", data));
    std::cout << "a2" << std::endl;
    master.SendEvent(TEvent<std::vector<int>>("a", data));
    std::cout << "b2" << std::endl;
    master.SendEvent(TEvent<std::vector<int>>("b", data));
    std::cout << "a stop" << std::endl;
    master.SendEvent(Event("a stop"));
    std::cout << "b stop" << std::endl;
    master.SendEvent(Event("b stop"));

    std::cout << "a wait" << std::endl;
    a.WaitToFinish();
    std::cout << "b wait" << std::endl;
    b.WaitToFinish();

    // cin.get();
    
    std::cout << "master stop" << std::endl;
    master.Stop();

    std::cout << "master wait" << std::endl;
    master.WaitToFinish();
    std::cout << "exit success" << std::endl;
    return EXIT_SUCCESS;
}
