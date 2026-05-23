#ifndef GRID_ENGINE_HPP_INCLUDED
#define GRID_ENGINE_HPP_INCLUDED

#include "libsim/types.hpp"
#include "libsim/i_environment.hpp"
#include "libsim/i_agent.hpp"
#include "libsim/game_log.hpp"
#include "libsim/sense_event.hpp"
#include "libevent/event_component.hpp"
#include "libevent/event_bus.hpp"

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace grid::libsim {

    enum State { RUNNING, STOPPED, PAUSED};

    template <typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    class BaseEngine : public IEnvironment<NUMBER_TYPE, NUM_DIMENSIONS> {

        /// Private event proxy that inherits EventComponent and
        /// delegates ProcessEvent back to the owning engine.
        class EventProxy : public grid::libevent::EventComponent {
        public:
            explicit EventProxy(BaseEngine& owner, grid::libevent::EventBus& bus)
                : EventComponent(bus), m_owner(owner) {}

            void ProcessEvent(grid::libevent::Event* incoming) override {
                m_owner.dispatchEventToAgents(incoming);
            }
        private:
            BaseEngine& m_owner;
        };

        std::atomic<State> _state {State::STOPPED};
        std::vector<std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>>> _agents;
        mutable std::recursive_mutex _agents_mutex;
        int _target_fps;
        GameLog m_game_log;
        grid::libevent::EventBus m_eventBus;
        EventProxy m_eventProxy;
    public:
        explicit BaseEngine(int target_fps = 60)
            : _target_fps(target_fps), m_eventProxy(*this, m_eventBus)
        {
        }

        // ── Event system forwarding ─────────────────────────────────
        void Subscribe(grid::libevent::EventKey key) { m_eventProxy.Subscribe(key); }
        void Unsubscribe(grid::libevent::EventKey key) { m_eventProxy.Unsubscribe(key); }
        bool IsSubscribed(grid::libevent::EventKey key) { return m_eventProxy.IsSubscribed(key); }
        void SendEvent(const grid::libevent::Event& e) { m_eventProxy.SendEvent(e); }
        size_t PollEvents() { return m_eventProxy.PollEvents(); }

        void broadcastEvent(const grid::libevent::Event& event) override
        {
            SendEvent(event);
        }

    private:
        void dispatchEventToAgents(grid::libevent::Event* incoming)
        {
            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            for (const auto& agent : _agents) {
                agent->on_event(incoming);
            }
        }

    public:

        GameLog& getGameLog() override
        {
            return m_game_log;
        }

        /// Emit a sense event with spatial attenuation.
        /// Each agent inside the emission region receives a copy
        /// with perceivedIntensity computed from distance.
        void emitSenseEvent(const SenseEvent<NUMBER_TYPE, NUM_DIMENSIONS>& event) override
        {
            // Log to the game log (once, at source intensity)
            std::ostringstream locStr;
            locStr << event.origin();
            m_game_log.log(event.source(), locStr.str(),
                           event.sense(), event.message());

            // Deliver to agents within the emission region
            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            for (const auto& agent : _agents) {
                if (!event.contains(agent->location())) continue;

                float dist = event.distanceTo(agent->location());
                float perceived = event.attenuate(dist);

                // Clone so each receiver gets an independent copy
                auto cloned = event.clone();
                auto* se = dynamic_cast<SenseEvent<NUMBER_TYPE, NUM_DIMENSIONS>*>(cloned.get());
                if (se) {
                    se->setPerceivedIntensity(perceived);
                }
                agent->on_event(cloned.get());
            }
        }

        void run_step(DeltaType delta)
        {
            // Process any pending events and dispatch to agents
            PollEvents();

            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            std::for_each(_agents.begin(), _agents.end(), [delta](const auto& a) { a->update(delta); });
        }

        void stop() {
            _state = State::STOPPED;
        }
        
        void setState(const State& state)
        {
            _state = state;
        }

        State getState()
        {
            return _state;
        }

        void addAgent(std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>> agent) override
        {
            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            _agents.push_back(agent);
        }

        void removeAgent(const std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>>& agent) override
        {
            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            auto result = std::find(_agents.begin(), _agents.end(), agent);
            if (result == _agents.end()) {
                std::cerr << "Cannot find Agent to remove" << std::endl;
            } else {
                _agents.erase(result);
            }
        }

        std::vector<std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>>> getAllAgents() const override
        {
            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            return _agents;
        }

        std::vector<std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>>> getAgentsInRange(const RangeXD<NUMBER_TYPE, NUM_DIMENSIONS>& range) const override
        {
            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            std::vector<std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>>> agentsInRange;
            for (const auto& agent : _agents) {
                if (range.contains(agent->location())) {
                    agentsInRange.push_back(agent);
                }
            }
            return agentsInRange;
        }

        /// Thread-safe snapshot of every agent's name and position.
        /// Call once per frame on the render thread to avoid data races.
        using PositionSnapshot = std::unordered_map<std::string, VectX<NUMBER_TYPE, NUM_DIMENSIONS>>;
        PositionSnapshot snapshotAgentPositions() const
        {
            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            PositionSnapshot snap;
            snap.reserve(_agents.size());
            for (const auto& agent : _agents) {
                snap.emplace(agent->name(), agent->location());
            }
            return snap;
        }

        /// Access the engine's event bus (e.g. for creating components
        /// that need to communicate through the same bus).
        grid::libevent::EventBus& eventBus() { return m_eventBus; }

        /// Run a callable while holding the agents mutex.
        /// Use from the render thread to serialise writes to agent state
        /// (e.g. player position) with the engine's run_step().
        template <typename F>
        auto withAgentsLock(F&& fn) -> decltype(fn())
        {
            std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
            return fn();
        }
        
        std::thread run()
        {
            this->setState(State::RUNNING);
            std::thread engineThread([this]() {
                {
                    std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
                    std::for_each(_agents.begin(), _agents.end(), [](const auto& a) { a->start(); });
                }

                const std::chrono::duration<double> timePerFrame(1.0 / _target_fps);
                auto lastTime = std::chrono::high_resolution_clock::now();

                while (_state != State::STOPPED) {
                    if (_state == State::PAUSED) {
                        std::this_thread::sleep_for(timePerFrame);
                        lastTime = std::chrono::high_resolution_clock::now();
                        continue;
                    }

                    auto currentTime = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> delta = currentTime - lastTime;

                    if (delta >= timePerFrame) {
                        run_step(delta);
                        lastTime = currentTime;
                    } else {
                        std::this_thread::sleep_for(timePerFrame - delta);
                    }
                }

                {
                    std::lock_guard<std::recursive_mutex> lock(_agents_mutex);
                    std::for_each(_agents.begin(), _agents.end(), [](const auto& a) { a->on_destroy(); });
                }
            });

            return engineThread;
        }
    }; // class BaseEngine

} // namespace grid::libsim
#endif//GRID_ENGINE_HPP_INCLUDED

