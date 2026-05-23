#ifndef I_ENVIRONMENT_HPP
#define I_ENVIRONMENT_HPP

#include <vector>
#include <memory>
#include "libsim/types.hpp"
#include "libsim/game_log.hpp"
#include "libevent/event.hpp"

// Forward-declare SenseEvent so emitSenseEvent can appear in the interface.
namespace grid::libsim {
    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    class SenseEvent;
}

namespace grid::libsim {
    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    class IAgent; // Forward declaration

    template <typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    // LCOV_EXCL_START
    class IEnvironment {
    public:
        virtual ~IEnvironment() = default;

        virtual void addAgent(std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>> agent) = 0;
        virtual void removeAgent(const std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>>& agent) = 0;
        virtual std::vector<std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>>> getAllAgents() const = 0;
        virtual std::vector<std::shared_ptr<IAgent<NUMBER_TYPE, NUM_DIMENSIONS>>> getAgentsInRange(const RangeXD<NUMBER_TYPE, NUM_DIMENSIONS>& range) const = 0;

        virtual void broadcastEvent(const grid::libevent::Event& event) = 0;

        /// Emit a spatially-attenuated sense event.  The engine delivers
        /// a per-receiver copy (with perceivedIntensity set) to every
        /// agent inside the emission region, and logs it to the GameLog.
        virtual void emitSenseEvent(const SenseEvent<NUMBER_TYPE, NUM_DIMENSIONS>& event) = 0;

        virtual GameLog& getGameLog() = 0;
    };
    // LCOV_EXCL_STOP
} // namespace grid::libsim

#endif // I_ENVIRONMENT_HPP
