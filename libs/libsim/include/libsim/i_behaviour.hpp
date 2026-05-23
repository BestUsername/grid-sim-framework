#ifndef I_BEHAVIOUR_HPP_INCLUDED
#define I_BEHAVIOUR_HPP_INCLUDED

#include "libsim/types.hpp"
#include <chrono>

namespace grid::libsim {

/**
 * @brief Interface for defining agent behaviors.
 * 
 * The IBehaviour class provides an interface for defining agent behaviors.
 * Any class that implements this interface must provide an implementation
 * for the execute() function.
 */
// LCOV_EXCL_START
class IBehaviour {
public:
    virtual ~IBehaviour() = default;
    /**
     * @brief Executes the behavior for the given agent.
     * 
     * This pure virtual function must be implemented by any class that
     * inherits from IBehaviour. It defines the behavior that will be
     * executed for the given agent.
     */
    virtual void execute(DeltaType delta) = 0;
};
// LCOV_EXCL_STOP
}// namespace grid::libsim
#endif//I_BEHAVIOUR_HPP_INCLUDED
