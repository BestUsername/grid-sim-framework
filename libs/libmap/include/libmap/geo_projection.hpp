#ifndef LIBMAP_GEO_PROJECTION_HPP_INCLUDED
#define LIBMAP_GEO_PROJECTION_HPP_INCLUDED

#include <cmath>
#include <utility>

namespace grid::libmap {

/**
 * @brief Geographic origin that ties a MapWorld's (0, 0) cell to WGS-84 coordinates.
 *
 * @c metersPerCell specifies the real-world size of one grid cell, which allows
 * the projection to convert between cell coordinates and geographic coordinates.
 */
struct GeoOrigin {
    double lat;           ///< Latitude  of cell (0, 0) in decimal degrees.
    double lon;           ///< Longitude of cell (0, 0) in decimal degrees.
    double metersPerCell; ///< Real-world metres represented by one grid cell.
};

/**
 * @brief Equirectangular projection between WGS-84 lat/lon and local grid (x, z).
 *
 * The grid X axis points East; the grid Z axis points South.
 *
 * Accuracy is sufficient for areas up to ~200 km across.  For larger
 * extents a proper UTM or Mercator projection should be used instead.
 */
class GeoProjection {
public:
    /**
     * @brief Construct a projection centred on @p origin.
     * @param origin Geographic origin and scale of the grid.
     */
    explicit GeoProjection(GeoOrigin origin);

    /**
     * @brief Convert a geographic coordinate to a local grid cell position.
     * @param lat Latitude  in decimal degrees.
     * @param lon Longitude in decimal degrees.
     * @return {x, z} grid cell coordinates (may be fractional / out-of-bounds).
     */
    std::pair<double, double> toLocal(double lat, double lon) const;

    /**
     * @brief Convert a local grid cell position to geographic coordinates.
     * @param x Grid cell X (East).
     * @param z Grid cell Z (South).
     * @return {lat, lon} in decimal degrees.
     */
    std::pair<double, double> toGeo(double x, double z) const;

    const GeoOrigin& origin() const { return m_origin; }

private:
    GeoOrigin m_origin;
    double    m_latScale;  ///< metres per degree latitude
    double    m_lonScale;  ///< metres per degree longitude at the origin latitude
};

// ── Inline implementation ────────────────────────────────────────────────────

inline GeoProjection::GeoProjection(GeoOrigin origin)
    : m_origin(origin)
{
    static constexpr double kEarthRadius = 6'371'000.0;
    static constexpr double kDegToRad   = M_PI / 180.0;
    m_latScale = kEarthRadius * kDegToRad;
    m_lonScale = kEarthRadius * kDegToRad * std::cos(origin.lat * kDegToRad);
}

inline std::pair<double, double> GeoProjection::toLocal(double lat, double lon) const
{
    double dLon = lon - m_origin.lon;
    double dLat = lat - m_origin.lat;
    double xMetres =  dLon * m_lonScale;
    double zMetres = -dLat * m_latScale;  // Z points South → invert latitude delta
    return { xMetres / m_origin.metersPerCell, zMetres / m_origin.metersPerCell };
}

inline std::pair<double, double> GeoProjection::toGeo(double x, double z) const
{
    double xMetres = x * m_origin.metersPerCell;
    double zMetres = z * m_origin.metersPerCell;
    double lat = m_origin.lat + (-zMetres) / m_latScale;
    double lon = m_origin.lon +   xMetres  / m_lonScale;
    return { lat, lon };
}

} // namespace grid::libmap

#endif // LIBMAP_GEO_PROJECTION_HPP_INCLUDED
