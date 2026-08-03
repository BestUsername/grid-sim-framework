#include "libmap/ascii_format.hpp"
#include "libmap/map_world.hpp"
#include "libmap/map_tile.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace grid::libmap {

static SurfaceType charToSurface(char c)
{
    switch (c) {
    case '~': return SurfaceType::Water;
    case '^': return SurfaceType::Mountain;
    default:  return SurfaceType::Land;
    }
}

static char tileToChar(const TerrainTile& tile)
{
    switch (tile.surface) {
    case SurfaceType::Water:    return '~';
    case SurfaceType::Mountain: return '^';
    default:                    return tile.elevation > 0.0f ? ',' : '.';
    }
}

std::optional<MapWorld> AsciiMapFormat::parse(std::istream& stream) const
{
    size_t w = 0, h = 0;
    if (!(stream >> w >> h) || w == 0 || h == 0) {
        std::cerr << "AsciiMapFormat: invalid or missing header\n";
        return std::nullopt;
    }

    MapWorld world(w, h);
    auto& layer = world.terrain();

    std::string line;
    std::getline(stream, line); // consume remainder of the header line
    for (size_t row = 0; row < h; ++row) {
        if (!std::getline(stream, line)) {
            std::cerr << "AsciiMapFormat: unexpected end of file at row " << row << "\n";
            return std::nullopt;
        }
        for (size_t col = 0; col < w; ++col) {
            char c = col < line.size() ? line[col] : '.';
            TerrainTile tile;
            tile.surface   = charToSurface(c);
            tile.elevation = c == ',' ? 0.25f : 0.0f;
            layer.set(col, row, tile);
        }
    }
    return world;
}

std::optional<MapWorld> AsciiMapFormat::readFromFile(const std::string& path) const
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "AsciiMapFormat: cannot open '" << path << "'\n";
        return std::nullopt;
    }
    return parse(file);
}

std::optional<MapWorld> AsciiMapFormat::readFromString(const std::string& data) const
{
    std::istringstream ss(data);
    return parse(ss);
}

void AsciiMapFormat::serialize(const MapWorld& world, std::ostream& out) const
{
    out << world.width() << ' ' << world.height() << '\n';
    const auto& layer = world.terrain();
    for (size_t z = 0; z < world.height(); ++z) {
        for (size_t x = 0; x < world.width(); ++x)
            out << tileToChar(layer.get(x, z));
        out << '\n';
    }
}

bool AsciiMapFormat::writeToFile(const MapWorld& world, const std::string& path) const
{
    std::ofstream file(path);
    if (!file.is_open()) return false;
    serialize(world, file);
    return true;
}

std::string AsciiMapFormat::writeToString(const MapWorld& world) const
{
    std::ostringstream ss;
    serialize(world, ss);
    return ss.str();
}

} // namespace grid::libmap
