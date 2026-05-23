#ifndef LIBMAP_OSM_FORMAT_HPP_INCLUDED
#define LIBMAP_OSM_FORMAT_HPP_INCLUDED

#include "libmap/map_format.hpp"

namespace grid::libmap {

/**
 * @brief OpenStreetMap XML reader.
 *
 * Parses OSM XML (as produced by the Overpass API, JOSM, or planet extracts)
 * and rasterises the vector feature data into a @c MapLayer<TerrainTile> grid.
 *
 * **Supported features:**
 * | OSM tags                                  | SurfaceType         |
 * |-------------------------------------------|---------------------|
 * | `building=*`                              | Building            |
 * | `highway=*`, `railway=*`                  | Road                |
 * | `natural=water`, `waterway=*`             | Water               |
 * | `natural=wood`, `landuse=forest`          | Forest              |
 * | `natural=sand`, `natural=beach`           | Sand                |
 * | `natural=wetland`, `natural=marsh`        | Swamp               |
 * | `natural=peak`, `natural=cliff`           | Mountain            |
 *
 * Closed ways that carry area-type tags (building, landuse, water polygon, …)
 * are filled as solid polygons.  Open ways (roads, rivers, railways) are
 * drawn as lines with a width proportional to road class.
 *
 * When multiple features overlap a cell the higher-priority surface wins
 * (Building > Road > Water > Forest > Swamp > Sand > Mountain > Land).
 *
 * The resulting @c MapWorld has a @c GeoOrigin set to the north-west corner
 * of the bounding box derived from all parsed nodes; @c cellSize is taken
 * from @c Options::cellSize.
 */
class OsmFormat : public IMapReader {
public:
    /** @brief Rasterisation parameters. */
    struct Options {
        double cellSize = 10.0; ///< Real-world metres represented by one grid cell.
    };

    /// Construct with default options (cellSize = 10 m).
    OsmFormat() = default;
    /// Construct with explicit options.
    explicit OsmFormat(Options opts);

    /**
     * @brief Parse an OSM XML file into a MapWorld.
     * @param path  Filesystem path to the @c .osm file.
     * @return Populated MapWorld on success; @c std::nullopt on any error.
     */
    std::optional<MapWorld> readFromFile(const std::string& path) const override;

    /**
     * @brief Parse an OSM XML string into a MapWorld.
     * @param xml  Raw OSM XML content.
     * @return Populated MapWorld on success; @c std::nullopt on any error.
     */
    std::optional<MapWorld> readFromString(const std::string& xml) const override;

private:
    Options m_opts;
};

} // namespace grid::libmap

#endif // LIBMAP_OSM_FORMAT_HPP_INCLUDED
