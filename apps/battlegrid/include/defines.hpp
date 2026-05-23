#ifndef BATTLEGRID_DEFINES_HPP_INCLUDED
#define BATTLEGRID_DEFINES_HPP_INCLUDED

#include "libsim/base_engine.hpp"
#include "libsim/base_agent.hpp"
#include "libsim/types.hpp"
#include "libsim/i_environment.hpp"
#include "libsim/i_agent.hpp"

#include <string>
#include <unordered_map>

using NUMBER_TYPE = double;
constexpr int NUM_DIMENSIONS = 3;

using COORD = grid::libsim::VectX<NUMBER_TYPE, NUM_DIMENSIONS>;
using BASE_ENGINE = grid::libsim::BaseEngine<NUMBER_TYPE, NUM_DIMENSIONS>;
using BASE_AGENT = grid::libsim::BaseAgent<NUMBER_TYPE, NUM_DIMENSIONS>;
using I_ENVIRONMENT = grid::libsim::IEnvironment<NUMBER_TYPE, NUM_DIMENSIONS>;
using I_AGENT = grid::libsim::IAgent<NUMBER_TYPE, NUM_DIMENSIONS>;

using RANGE = grid::libsim::RangeXD<NUMBER_TYPE, NUM_DIMENSIONS>;

using PositionSnapshot = std::unordered_map<std::string, COORD>;

#endif // BATTLEGRID_DEFINES_HPP_INCLUDED
