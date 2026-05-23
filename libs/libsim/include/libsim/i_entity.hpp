#ifndef GRID_I_ENTITY_HPP_INCLUDED
#define GRID_I_ENTITY_HPP_INCLUDED

#include "libsim/types.hpp"  // Coord
#include "libsim/shapes.hpp" // Shape

#include <string>            // std::string
#include <vector>            // std::vector
#include <chrono>            // std::chrono::duration

namespace grid::libsim {

    /**
     * @brief An interface to represent a physical entity on the Grid.
     * 
     */
    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    // LCOV_EXCL_START
    class IEntity {
    public:
        virtual ~IEntity() = default;
        
        virtual const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& location() const = 0;
        virtual void set_location(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& location) = 0;

        virtual void start() = 0;
        virtual void update(DeltaType delta) = 0;
        virtual void on_destroy() = 0;

        // Optional lifecycle hooks for future implementation:
        // virtual void fixed_update() = 0;
        // virtual void on_became_visible() = 0;
        // virtual void on_became_invisible() = 0;
        // virtual void on_collision_enter() = 0;
        // virtual void on_trigger_enter() = 0;
    };
    // LCOV_EXCL_STOP

} // namespace grid::libsim

#endif//GRID_I_ENTITY_HPP_INCLUDED
