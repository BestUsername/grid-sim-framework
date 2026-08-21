#include "libmap/ascii_format.hpp"
#include "libmap/map_world.hpp"
#include "libmap/map_tile.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace grid::libmap {

static TerrainTile charToTile(char c)
{
    TerrainTile tile;
    switch (c) {
    case '~':
        tile.surface = SurfaceType::Water;
        break;
    case '^':
        tile.surface = SurfaceType::Mountain;
        break;
    case ',':
        tile.elevation = 0.25f;
        break;
    case 'N':
    case 'S':
    case 'E':
    case 'W':
        tile.elevation = 0.25f;
        tile.tags.emplace("battlegrid:slope", std::string(1, c));
        break;
    case 'H':
        tile.elevation = 0.5f;
        break;
    default:
        break;
    }
    return tile;
}

static char tileToChar(const TerrainTile& tile)
{
    switch (tile.surface) {
    case SurfaceType::Water:    return '~';
    case SurfaceType::Mountain: return '^';
    default:
        if (const auto slope = tile.tags.find("battlegrid:slope");
            slope != tile.tags.end() && slope->second.size() == 1) {
            const char direction = slope->second.front();
            if (direction == 'N' || direction == 'S' || direction == 'E' || direction == 'W') {
                return direction;
            }
        }
        return tile.elevation >= 0.5f ? 'H' : tile.elevation > 0.0f ? ',' : '.';
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
            layer.set(col, row, charToTile(c));
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
