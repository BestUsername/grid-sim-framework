#ifndef LIBMAP_MAP_TILE_HPP_INCLUDED
#define LIBMAP_MAP_TILE_HPP_INCLUDED

#include <string>
#include <unordered_map>

namespace grid::libmap {

/// Surface classification for a map cell.
enum class SurfaceType {
    Unknown,
    Water,       ///< Open water: sea, lake, river
    Land,        ///< Generic flat land / grass
    Mountain,    ///< High terrain / rock
    Forest,      ///< Forested / woodland area
    Road,        ///< Paved road or track
    Building,    ///< Built structure (impassable to ground units)
    Sand,        ///< Beach or desert
    Swamp,       ///< Wetlands
};

/**
 * @brief A single map cell carrying surface type, elevation, and key/value tags.
 *
 * Tags follow the OpenStreetMap convention (e.g. "name" → "River Thames",
 * "highway" → "primary") and are populated when loading from OSM data.
 */
struct TerrainTile {
    SurfaceType surface   = SurfaceType::Land;
    float       elevation = 0.0f;  ///< metres above sea level
    std::unordered_map<std::string, std::string> tags;

    /// True when the tile represents open water.
    bool isWater() const { return surface == SurfaceType::Water; }

    /// Movement speed multiplier for ground units (0.0 = impassable).
    float speedFactor() const;
};

inline float TerrainTile::speedFactor() const
{
    switch (surface) {
    case SurfaceType::Water:    return 0.3f;
    case SurfaceType::Land:     return 1.0f;
    case SurfaceType::Mountain: return 0.5f;
    case SurfaceType::Forest:   return 0.6f;
    case SurfaceType::Road:     return 1.3f;
    case SurfaceType::Building: return 0.0f;
    case SurfaceType::Sand:     return 0.7f;
    case SurfaceType::Swamp:    return 0.4f;
    default:                    return 1.0f;
    }
}

} // namespace grid::libmap

#endif // LIBMAP_MAP_TILE_HPP_INCLUDED
