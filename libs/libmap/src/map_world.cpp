#include "libmap/map_world.hpp"

namespace grid::libmap {

MapWorld::MapWorld(size_t width, size_t height, double cellSize)
    : m_terrain(width, height)
    , m_cellSize(cellSize)
{
}

} // namespace grid::libmap
