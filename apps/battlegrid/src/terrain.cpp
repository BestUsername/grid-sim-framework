#include "terrain.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace battlegrid {

TerrainMap::TerrainMap(size_t width, size_t height, TerrainType fill)
    : m_width(width)
    , m_height(height)
    , m_grid(height, std::vector<TerrainType>(width, fill))
{
}

bool TerrainMap::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "TerrainMap: cannot open '" << path << "'\n";
        return false;
    }

    size_t w = 0, h = 0;
    if (!(file >> w >> h) || w == 0 || h == 0) {
        std::cerr << "TerrainMap: invalid header in '" << path << "'\n";
        return false;
    }

    std::vector<std::vector<TerrainType>> grid;
    grid.reserve(h);

    std::string line;
    std::getline(file, line); // consume rest of header line
    for (size_t row = 0; row < h; ++row) {
        if (!std::getline(file, line)) {
            std::cerr << "TerrainMap: unexpected end of file at row " << row << "\n";
            return false;
        }

        std::vector<TerrainType> rowData;
        rowData.reserve(w);
        for (size_t col = 0; col < w && col < line.size(); ++col) {
            char c = line[col];
            switch (c) {
            case '~': rowData.push_back(TerrainType::Water);    break;
            case '^': rowData.push_back(TerrainType::Mountain); break;
            default:  rowData.push_back(TerrainType::Land);     break;
            }
        }
        // Pad short rows with land
        while (rowData.size() < w) {
            rowData.push_back(TerrainType::Land);
        }
        grid.push_back(std::move(rowData));
    }

    m_width  = w;
    m_height = h;
    m_grid   = std::move(grid);
    return true;
}

bool TerrainMap::saveToFile(const std::string& path) const
{
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << m_width << " " << m_height << "\n";
    for (const auto& row : m_grid) {
        for (auto t : row) {
            file << static_cast<char>(t);
        }
        file << "\n";
    }
    return true;
}

TerrainType TerrainMap::at(size_t x, size_t z) const
{
    if (!inBounds(x, z)) return TerrainType::Water;
    return m_grid[z][x];
}

void TerrainMap::set(size_t x, size_t z, TerrainType t)
{
    if (inBounds(x, z)) {
        m_grid[z][x] = t;
    }
}

double TerrainMap::heightAt(double x, double z) const
{
    auto ix = static_cast<size_t>(std::max(0.0, x));
    auto iz = static_cast<size_t>(std::max(0.0, z));
    return terrainHeight(at(ix, iz));
}

double TerrainMap::maxHeightInRadius(double x, double z, double radius) const
{
    int minX = static_cast<int>(std::floor(x - radius));
    int maxX = static_cast<int>(std::floor(x + radius));
    int minZ = static_cast<int>(std::floor(z - radius));
    int maxZ = static_cast<int>(std::floor(z + radius));

    double maxH = heightAt(x, z);
    for (int ix = minX; ix <= maxX; ++ix) {
        for (int iz = minZ; iz <= maxZ; ++iz) {
            double h = terrainHeight(at(
                static_cast<size_t>(std::max(0, ix)),
                static_cast<size_t>(std::max(0, iz))));
            if (h > maxH) maxH = h;
        }
    }
    return maxH;
}

TerrainMap::TerrainSample TerrainMap::maxTerrainInRadius(double x, double z,
                                                         double radius) const
{
    int minX = static_cast<int>(std::floor(x - radius));
    int maxX = static_cast<int>(std::floor(x + radius));
    int minZ = static_cast<int>(std::floor(z - radius));
    int maxZ = static_cast<int>(std::floor(z + radius));

    auto cx = static_cast<size_t>(std::max(0.0, x));
    auto cz = static_cast<size_t>(std::max(0.0, z));
    TerrainType bestType = at(cx, cz);
    double bestH = terrainHeight(bestType);

    for (int ix = minX; ix <= maxX; ++ix) {
        for (int iz = minZ; iz <= maxZ; ++iz) {
            TerrainType t = at(static_cast<size_t>(std::max(0, ix)),
                               static_cast<size_t>(std::max(0, iz)));
            double h = terrainHeight(t);
            if (h > bestH) {
                bestH = h;
                bestType = t;
            }
        }
    }
    return {bestH, bestType};
}

bool TerrainMap::inBounds(size_t x, size_t z) const
{
    return x < m_width && z < m_height;
}

void TerrainMap::print(std::ostream& os) const
{
    for (const auto& row : m_grid) {
        for (auto t : row) {
            os << static_cast<char>(t);
        }
        os << "\n";
    }
}

} // namespace battlegrid
