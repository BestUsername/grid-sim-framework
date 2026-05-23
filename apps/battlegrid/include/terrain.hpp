#ifndef BATTLEGRID_TERRAIN_HPP_INCLUDED
#define BATTLEGRID_TERRAIN_HPP_INCLUDED

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace battlegrid {

enum class TerrainType : char {
    Water    = '~',
    Land     = '.',
    Mountain = '^',
};

inline std::ostream& operator<<(std::ostream& os, TerrainType t)
{
    return os << static_cast<char>(t);
}

/// Height constants for terrain types (Y-axis in 3D space).
inline double terrainHeight(TerrainType t)
{
    switch (t) {
    case TerrainType::Water:    return -2.0;
    case TerrainType::Land:     return  0.0;
    case TerrainType::Mountain: return  5.0;
    }
    return 0.0;
}

/// Movement speed multiplier on this terrain (1.0 = normal).
inline double terrainSpeedFactor(TerrainType t)
{
    switch (t) {
    case TerrainType::Water:    return 0.3;
    case TerrainType::Land:     return 1.0;
    case TerrainType::Mountain: return 0.5;
    }
    return 1.0;
}

/// How hard this terrain resists a vehicle that tries to drive through
/// a height barrier it creates.  Zero means no resistance (water).
inline double terrainObstacleStrength(TerrainType t)
{
    switch (t) {
    case TerrainType::Water:    return     0.0;
    case TerrainType::Land:     return  5000.0;  // dirt / rubble — breakable
    case TerrainType::Mountain: return 50000.0;  // solid rock
    }
    return 10000.0;
}

/// Whether a ground unit can traverse this terrain.
inline bool isTraversableByLand(TerrainType t) { return t != TerrainType::Water; }
inline bool isTraversableBySea(TerrainType t)  { return t == TerrainType::Water; }
inline bool isTraversableByAir(TerrainType /*t*/) { return true; }

/**
 * @brief A 2D grid-based terrain map that can be loaded from a file.
 *
 * Map file format:
 *   - First line: <width> <height>
 *   - Subsequent lines: rows of characters where:
 *       '~' = Water, '.' = Land, '^' = Mountain
 *
 * The map is stored row-major. grid[z][x] gives the terrain at (x, z).
 * The Y coordinate is derived from terrainHeight().
 */
class TerrainMap {
public:
    TerrainMap() = default;
    TerrainMap(size_t width, size_t height, TerrainType fill = TerrainType::Land);

    /// Load a map from a file. Returns false on failure.
    bool loadFromFile(const std::string& path);

    /// Save current map to a file.
    bool saveToFile(const std::string& path) const;

    size_t width()  const { return m_width; }
    size_t height() const { return m_height; }

    /// Get terrain at grid position (x, z). Out-of-bounds returns Water.
    TerrainType at(size_t x, size_t z) const;

    /// Set terrain at (x, z).
    void set(size_t x, size_t z, TerrainType t);

    /// Get the 3D Y-height for a world position (x, z).
    double heightAt(double x, double z) const;

    /// Get the maximum terrain height across all grid cells within a
    /// circular footprint of the given radius centred at (x, z).
    double maxHeightInRadius(double x, double z, double radius) const;

    /// Height + terrain type of the tallest cell within a footprint.
    struct TerrainSample {
        double      height;
        TerrainType type;
    };
    TerrainSample maxTerrainInRadius(double x, double z, double radius) const;

    /// Check if a position is within map bounds.
    bool inBounds(size_t x, size_t z) const;

    /// Print the map to a stream (for debugging).
    void print(std::ostream& os) const;

private:
    size_t m_width  = 0;
    size_t m_height = 0;
    std::vector<std::vector<TerrainType>> m_grid;
};

} // namespace battlegrid

#endif // BATTLEGRID_TERRAIN_HPP_INCLUDED
