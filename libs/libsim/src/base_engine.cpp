#include "libsim/base_engine.hpp"

namespace grid::libsim {
    template class BaseEngine<int, 2>;
    template class BaseEngine<double, 2>;
    template class BaseEngine<int, 3>;
    template class BaseEngine<double, 3>;
} // namespace grid::libsim
