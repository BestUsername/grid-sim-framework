#ifndef GRID_BASE_AGENT_HPP_INCLUDED
#define GRID_BASE_AGENT_HPP_INCLUDED

#include "libsim/i_agent.hpp"
#include "libsim/i_entity.hpp"
#include "libsim/i_behaviour.hpp"
#include "libsim/i_environment.hpp"
#include "libsim/sense_event.hpp"
#include "libsim/types.hpp"
#include "libsim/shapes.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace grid::libsim {

    // IEnvironment is included via i_environment.hpp above.

    /**
     * @brief A base class to represent an Agent on the Grid.
     * 
     */
    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    class BaseAgent : public IAgent<NUMBER_TYPE, NUM_DIMENSIONS> {
    public:
        BaseAgent(IEnvironment<NUMBER_TYPE, NUM_DIMENSIONS>& env, 
                  const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& location, 
                  const std::string& name = "John Doe") 
            : m_environment(env), m_location(location), m_name(name)
        {

        }

        // ----- Behaviour management -----

        void addBehaviour(std::unique_ptr<IBehaviour> behaviour) {
            m_behaviours.push_back(std::move(behaviour));
        }

        void removeBehaviour(IBehaviour* behaviour) {
            auto it = std::find_if(m_behaviours.begin(), m_behaviours.end(),
                [behaviour](const auto& b) { return b.get() == behaviour; });
            if (it != m_behaviours.end()) {
                m_behaviours.erase(it);
            }
        }

        template<typename T>
        T* getBehaviour() const {
            for (const auto& b : m_behaviours) {
                auto* casted = dynamic_cast<T*>(b.get());
                if (casted) return casted;
            }
            return nullptr;
        }

        // ----- Lifecycle -----

        virtual void start() override
        {
            communicate(Senses::Hearing, "I live!");
        }

        virtual void update(DeltaType delta) override 
        {
            for (auto& b : m_behaviours) {
                b->execute(delta);
            }
        }

        virtual void on_destroy() override {
            communicate(Senses::Hearing, "Aaaarrrrgh!");
        }

        // Optional lifecycle hooks for future implementation:
        // virtual void fixed_update() {};
        // virtual void on_became_visible() {};
        // virtual void on_became_invisible() {};
        // virtual void on_collision_enter() {};
        // virtual void on_trigger_enter() {};
        
        virtual void communicate(const Senses& sense, const std::string& message) override
        {
            if (sense == Senses::Hearing) {
                // Emit a spatially-attenuated audio event.
                // Default loudness 1.0, range 50 — subclasses can
                // override communicate() for custom values.
                AudioEvent<NUMBER_TYPE, NUM_DIMENSIONS> audio(
                    m_name, m_location, /*loudness=*/1.0f,
                    /*range=*/50.0f, message);
                m_environment.emitSenseEvent(audio);
            } else {
                // Other senses: just log for now (no spatial dispatch yet).
                std::ostringstream oss;
                oss << m_location;
                m_environment.getGameLog().log(
                    m_name, oss.str(), sense, message);
            }
        }

        friend std::ostream& operator<< (std::ostream &out, const BaseAgent<NUMBER_TYPE, NUM_DIMENSIONS>& agent) {
            out << agent.name() << agent.location();
            return out;
        }


        const std::string& name() const override
        {
            return this->m_name;
        }

        const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& location() const override
        {
            return this->m_location;
        }

        void set_location(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& location) override
        {
            this->m_location = location;
        }

    protected:
        IEnvironment<NUMBER_TYPE, NUM_DIMENSIONS>& m_environment;
        VectX<NUMBER_TYPE, NUM_DIMENSIONS> m_location;
        std::string m_name;
        std::vector<std::unique_ptr<IBehaviour>> m_behaviours;
    };

} // namespace grid::libsim

#endif//GRID_BASE_AGENT_HPP_INCLUDED
