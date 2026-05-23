#ifndef LIBMAP_MAP_FORMAT_HPP_INCLUDED
#define LIBMAP_MAP_FORMAT_HPP_INCLUDED

#include "libmap/map_world.hpp"

#include <optional>
#include <string>

namespace grid::libmap {

/**
 * @brief Interface for reading a MapWorld from a file or string.
 *
 * Implementations (e.g. AsciiMapFormat, OsmFormat) provide format-specific
 * parsing without coupling the caller to a particular file type.
 */
// LCOV_EXCL_START
class IMapReader {
public:
    virtual ~IMapReader() = default;

    /// Load a MapWorld from the file at @p path.
    virtual std::optional<MapWorld> readFromFile(const std::string& path) const = 0;

    /// Parse a MapWorld from the string @p data.
    virtual std::optional<MapWorld> readFromString(const std::string& data) const = 0;
};

/**
 * @brief Interface for writing a MapWorld to a file or string.
 */
class IMapWriter {
public:
    virtual ~IMapWriter() = default;

    /// Serialise @p world and write it to the file at @p path.
    virtual bool writeToFile(const MapWorld& world, const std::string& path) const = 0;

    /// Serialise @p world to a string and return it.
    virtual std::string writeToString(const MapWorld& world) const = 0;
};
// LCOV_EXCL_STOP

} // namespace grid::libmap

#endif // LIBMAP_MAP_FORMAT_HPP_INCLUDED
