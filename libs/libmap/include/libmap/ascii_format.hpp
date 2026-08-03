#ifndef LIBMAP_ASCII_FORMAT_HPP_INCLUDED
#define LIBMAP_ASCII_FORMAT_HPP_INCLUDED

#include "libmap/map_format.hpp"

#include <iosfwd>

namespace grid::libmap {

/**
 * @brief Reader/writer for the legacy ASCII .map format.
 *
 * File format:
 * @code
 *   <width> <height>
 *   <row 0 of width chars>
 *   <row 1 of width chars>
 *   ...
 * @endcode
 *
 * Character mapping:
 *   - '~'  → SurfaceType::Water
 *   - '.'  → SurfaceType::Land
 *   - ','  → SurfaceType::Land at 0.25m elevation (low navigable bump)
 *   - '^'  → SurfaceType::Mountain
 *   - other → SurfaceType::Land (default)
 *
 * On write, SurfaceType values not in the legacy set are emitted as '.'.
 * Positive-elevation land tiles are emitted as ','; other elevations are
 * not represented by this compact legacy format.
 */
class AsciiMapFormat : public IMapReader, public IMapWriter {
public:
    std::optional<MapWorld> readFromFile(const std::string& path)     const override;
    std::optional<MapWorld> readFromString(const std::string& data)   const override;
    bool        writeToFile(const MapWorld& world, const std::string& path) const override;
    std::string writeToString(const MapWorld& world)                  const override;

private:
    std::optional<MapWorld> parse(std::istream& stream) const;
    void                    serialize(const MapWorld& world, std::ostream& out) const;
};

} // namespace grid::libmap

#endif // LIBMAP_ASCII_FORMAT_HPP_INCLUDED
