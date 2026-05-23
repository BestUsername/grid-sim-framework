#ifndef GRID_I_AGENT_HPP_INCLUDED
#define GRID_I_AGENT_HPP_INCLUDED

#include "libsim/i_entity.hpp" // IEntity
#include "libsim/types.hpp"    // Coord
#include "libsim/shapes.hpp"   // Shape
#include "libevent/event.hpp"  // Event

#include <string>              // std::string
#include <vector>              // std::vector


namespace grid::libsim {

    /**
     * @brief An interface to represent an Agent on the Grid.
     * 
     */
    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    // LCOV_EXCL_START
    class IAgent : public IEntity<NUMBER_TYPE, NUM_DIMENSIONS> {
    public:
        virtual ~IAgent() = default;

        virtual const std::string& name() const = 0;

        virtual void communicate(const Senses& sense, const std::string& message) = 0;

        /**
         * @brief Called by the engine when an event is dispatched.
         *
         * Default implementation is a no-op. Override in derived classes
         * to react to events from the libevent system.
         */
        virtual void on_event(grid::libevent::Event* /*event*/) {}
    };
    // LCOV_EXCL_STOP

} // namespace grid::libsim

#endif//GRID_I_AGENT_HPP_INCLUDED
