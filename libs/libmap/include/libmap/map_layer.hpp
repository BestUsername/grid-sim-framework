#ifndef LIBMAP_MAP_LAYER_HPP_INCLUDED
#define LIBMAP_MAP_LAYER_HPP_INCLUDED

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace grid::libmap {

/**
 * @brief A 2D grid of cells stored row-major: cell(x, z) at index z*width+x.
 *
 * The X axis points East and the Z axis points South, matching the BattleGrid
 * world-space convention.
 *
 * @tparam T Cell type (e.g. TerrainTile, float, int).
 */
template <typename T>
class MapLayer {
public:
    MapLayer() = default;

    /**
     * @brief Construct a layer with all cells initialised to @p fill.
     * @param width  Number of cells along X.
     * @param height Number of cells along Z.
     * @param fill   Initial cell value.
     */
    MapLayer(size_t width, size_t height, T fill = T{});

    size_t width()  const { return m_width; }
    size_t height() const { return m_height; }
    bool   empty()  const { return m_cells.empty(); }

    /// @return true if (x, z) is inside the layer bounds.
    bool inBounds(size_t x, size_t z) const;

    /**
     * @brief Access a cell by reference.
     * @throws std::out_of_range if (x, z) is out of bounds.
     */
    T&       at(size_t x, size_t z);
    const T& at(size_t x, size_t z) const;

    /**
     * @brief Read a cell value without throwing.
     * @return The cell value, or a default-constructed T if out of bounds.
     */
    T get(size_t x, size_t z) const;

    /// Set a cell value.  Out-of-bounds writes are silently ignored.
    void set(size_t x, size_t z, T value);

    /// Direct access to the underlying flat cell array (row-major).
    const std::vector<T>& cells() const { return m_cells; }
    std::vector<T>&       cells()       { return m_cells; }

private:
    size_t         m_width  = 0;
    size_t         m_height = 0;
    std::vector<T> m_cells;
};

// ── Template implementation ──────────────────────────────────────────────────

template <typename T>
MapLayer<T>::MapLayer(size_t width, size_t height, T fill)
    : m_width(width), m_height(height), m_cells(width * height, fill)
{
}

template <typename T>
bool MapLayer<T>::inBounds(size_t x, size_t z) const
{
    return x < m_width && z < m_height;
}

template <typename T>
T& MapLayer<T>::at(size_t x, size_t z)
{
    if (!inBounds(x, z))
        throw std::out_of_range("MapLayer::at out of bounds");
    return m_cells[z * m_width + x];
}

template <typename T>
const T& MapLayer<T>::at(size_t x, size_t z) const
{
    if (!inBounds(x, z))
        throw std::out_of_range("MapLayer::at out of bounds");
    return m_cells[z * m_width + x];
}

template <typename T>
T MapLayer<T>::get(size_t x, size_t z) const
{
    if (!inBounds(x, z)) return T{};
    return m_cells[z * m_width + x];
}

template <typename T>
void MapLayer<T>::set(size_t x, size_t z, T value)
{
    if (inBounds(x, z))
        m_cells[z * m_width + x] = std::move(value);
}

} // namespace grid::libmap

#endif // LIBMAP_MAP_LAYER_HPP_INCLUDED
