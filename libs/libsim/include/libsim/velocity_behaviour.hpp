#ifndef LIBSIM_BASE_BEHAVIOUR_HPP
#define LIBSIM_BASE_BEHAVIOUR_HPP

#include "libsim/i_behaviour.hpp"
#include "libsim/i_agent.hpp"
#include "libsim/types.hpp"

namespace grid::libsim {
    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    class VelocityBehaviour : public IBehaviour {
    public:
        VelocityBehaviour(IAgent<NUMBER_TYPE, NUM_DIMENSIONS>& agent, const VectX<NUMBER_TYPE, NUM_DIMENSIONS> & velocity) 
            : m_agent(agent), m_velocity(velocity) {
        }

        void execute(DeltaType delta) override {
            m_agent.set_location(m_agent.location() + m_velocity * delta.count());
        }

        void set_velocity(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& velocity) {
            m_velocity = velocity;
        }

        const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& velocity() const {
            return m_velocity;
        }
        
    protected:
        VectX<NUMBER_TYPE, NUM_DIMENSIONS> m_velocity;
        IAgent<NUMBER_TYPE, NUM_DIMENSIONS>& m_agent;
    };
}

#endif//LIBSIM_BASE_BEHAVIOUR_HPP
