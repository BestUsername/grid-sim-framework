#include "libsim/types.hpp"

namespace grid::libsim {

    std::ostream& operator<<(std::ostream& os, const Need& need) {
        switch (need) {
            case Need::SURVIVAL: os << "Survival"; break;
            case Need::SAFETY: os << "Safety"; break;
            case Need::SOCIAL: os << "Social"; break;
            case Need::ESTEEM: os << "Esteem"; break;
            case Need::SELF_ACTUALIZATION: os << "Self Actualization"; break;
            default: os.setstate(std::ios_base::failbit);
        }
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Senses& sense) {
        switch (sense) {
            case Senses::Sight: os << "Sight"; break;
            case Senses::Hearing: os << "Hearing"; break;
            case Senses::Smell: os << "Smell"; break;
            case Senses::Taste: os << "Taste"; break;
            case Senses::Touch: os << "Touch"; break;
            default: os.setstate(std::ios_base::failbit);
        }
        return os;
    }

} // namespace grid::libsim
