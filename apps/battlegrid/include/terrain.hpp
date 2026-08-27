#ifndef BATTLEGRID_TERRAIN_HPP_INCLUDED
#define BATTLEGRID_TERRAIN_HPP_INCLUDED

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace battlegrid {

inline constexpr double kBumpHeight = 0.10;

enum class TerrainType : char {
    Water    = '~',
    Land     = '.',
    Bump     = ',',
    SlopeNorth = 'N',
    SlopeSouth = 'S',
    SlopeEast  = 'E',
    SlopeWest  = 'W',
    Hill       = 'H',
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
    case TerrainType::Bump:     return  kBumpHeight;
    case TerrainType::SlopeNorth:
    case TerrainType::SlopeSouth:
    case TerrainType::SlopeEast:
    case TerrainType::SlopeWest: return 0.25;
    case TerrainType::Hill:     return  0.5;
    case TerrainType::Mountain: return  5.0;
    }
    return 0.0;
}

/**
 * @brief A 2D grid-based terrain map that can be loaded from a file.
 *
 * Map file format:
 *   - First line: <width> <height>
 *   - Subsequent lines: rows of characters where:
 *       '~' = Water, '.' = Land, ',' = low navigable bump, N/S/E/W = directional
 *       ramps, H = low hill plateau, '^' = Mountain
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
