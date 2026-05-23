#ifndef LIBMAP_MAP_WORLD_HPP_INCLUDED
#define LIBMAP_MAP_WORLD_HPP_INCLUDED

#include "libmap/map_layer.hpp"
#include "libmap/map_tile.hpp"
#include "libmap/geo_projection.hpp"

#include <optional>
#include <stdexcept>
#include <string>

namespace grid::libmap {

/**
 * @brief Root map object that holds terrain tile data and optional geographic metadata.
 *
 * A MapWorld owns:
 * - A terrain layer (MapLayer<TerrainTile>) holding one tile per grid cell.
 * - An optional GeoOrigin that ties grid coordinates to WGS-84 lat/lon.
 * - A human-readable name.
 * - A cell size in metres (how large each grid cell is in the real world).
 *
 * Additional named layers (elevation grids, road networks, etc.) can be added
 * in future without breaking existing code.
 */
class MapWorld {
public:
    MapWorld() = default;

    /**
     * @brief Construct an empty world of the given dimensions.
     * @param width    Number of cells along X (East).
     * @param height   Number of cells along Z (South).
     * @param cellSize Real-world metres per cell (default 1 m).
     */
    MapWorld(size_t width, size_t height, double cellSize = 1.0);

    // ── Geometry ──────────────────────────────────────────────────────────────

    size_t width()    const { return m_terrain.width(); }
    size_t height()   const { return m_terrain.height(); }

    /// Real-world size of one grid cell in metres.
    double cellSize() const { return m_cellSize; }
    void   setCellSize(double s) { m_cellSize = s; }

    // ── Terrain layer ─────────────────────────────────────────────────────────

    MapLayer<TerrainTile>&       terrain()       { return m_terrain; }
    const MapLayer<TerrainTile>& terrain() const { return m_terrain; }

    // ── Geographic origin ──────────────────────────────────────────────────────

    bool hasGeoOrigin() const { return m_geoOrigin.has_value(); }

    /// Set a geographic origin to enable lat/lon ↔ grid projection.
    void setGeoOrigin(GeoOrigin origin) { m_geoOrigin = origin; }

    /**
     * @brief Access the raw geographic origin.
     * @pre hasGeoOrigin() must be true.
     */
    const GeoOrigin& geoOrigin() const { return *m_geoOrigin; }

    /**
     * @brief Obtain a projection object for coordinate conversion.
     * @throws std::runtime_error if no geographic origin has been set.
     */
    GeoProjection projection() const
    {
        if (!m_geoOrigin)
            throw std::runtime_error("MapWorld: no geographic origin set");
        return GeoProjection(*m_geoOrigin);
    }

    // ── Metadata ───────────────────────────────────────────────────────────────

    const std::string& name() const { return m_name; }
    void setName(std::string n)     { m_name = std::move(n); }

private:
    MapLayer<TerrainTile>  m_terrain;
    std::optional<GeoOrigin> m_geoOrigin;
    double      m_cellSize = 1.0;
    std::string m_name;
};

} // namespace grid::libmap

#endif // LIBMAP_MAP_WORLD_HPP_INCLUDED
