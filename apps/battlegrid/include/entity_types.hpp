#ifndef BATTLEGRID_ENTITY_TYPES_HPP_INCLUDED
#define BATTLEGRID_ENTITY_TYPES_HPP_INCLUDED

#include <ostream>
#include <string>

namespace battlegrid {

enum class EntityType {
    Soldier,
    Civilian,
    LandVehicle,
    SeaVehicle,
    AirVehicle,
};

enum class Faction {
    Blue,
    Red,
    Neutral,
};

inline std::string entityTypeName(EntityType t)
{
    switch (t) {
    case EntityType::Soldier:      return "Soldier";
    case EntityType::Civilian:     return "Civilian";
    case EntityType::LandVehicle:  return "LandVehicle";
    case EntityType::SeaVehicle:   return "SeaVehicle";
    case EntityType::AirVehicle:   return "AirVehicle";
    }
    return "Unknown";
}

inline std::string factionName(Faction f)
{
    switch (f) {
    case Faction::Blue:    return "Blue";
    case Faction::Red:     return "Red";
    case Faction::Neutral: return "Neutral";
    }
    return "Unknown";
}

inline std::ostream& operator<<(std::ostream& os, EntityType t)
{
    return os << entityTypeName(t);
}

inline std::ostream& operator<<(std::ostream& os, Faction f)
{
    return os << factionName(f);
}

} // namespace battlegrid

#endif // BATTLEGRID_ENTITY_TYPES_HPP_INCLUDED
