#ifndef BATTLEGRID_MAP_OVERLAY_HPP_INCLUDED
#define BATTLEGRID_MAP_OVERLAY_HPP_INCLUDED

#include "defines.hpp"
#include "entity_types.hpp"
#include "soldier.hpp"

#include "libmap/map_world.hpp"
#include "libmap/map_tile.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace battlegrid {

/**
 * @brief Fullscreen 2D overhead map overlay rendered as a HUD layer.
 *
 * The overlay draws a top-down view of the MapWorld terrain and plots
 * colored dots for each agent.  It uses the same OpenGL shader as the
 * rest of the display (MVP + uColor uniform) so no additional shaders
 * are required.
 *
 * Usage:
 *   1. Construct after the GL context exists.
 *   2. Call init() once (passes shader uniforms from the parent GLDisplay).
 *   3. Call render() each frame when the overlay is visible.
 *   4. Clean up by calling destroy() before the GL context is torn down,
 *      or by relying on the destructor.
 */
class MapOverlay {
public:
    explicit MapOverlay(const grid::libmap::MapWorld& world);
    ~MapOverlay();

    MapOverlay(const MapOverlay&) = delete;
    MapOverlay& operator=(const MapOverlay&) = delete;

    /**
     * @brief Allocate GPU resources.
     *
     * Must be called once after the GL context is current and the parent
     * shader program has been compiled.
     *
     * @param shaderProgram The parent GLDisplay shader program handle.
     * @param mvpLoc        Location of the uMVP uniform.
     * @param colorLoc      Location of the uColor uniform.
     * @param alphaLoc      Location of the uAlpha uniform.
     */
    void init(uint32_t shaderProgram, int mvpLoc, int colorLoc, int alphaLoc);

    /**
     * @brief Render the overlay.
     *
     * @param agents    All active agents (used to determine type / faction).
     * @param positions Latest position snapshot (world-space coordinates).
     * @param windowW   Current window width in pixels (for aspect correction).
     * @param windowH   Current window height in pixels.
     */
    void render(const std::vector<std::shared_ptr<I_AGENT>>& agents,
                const PositionSnapshot& positions,
                int windowW, int windowH) const;

    /// Free GPU resources.  Safe to call multiple times.
    void destroy();

private:
    // ── GPU resource helpers ──────────────────────────────────────────────
    struct SurfaceBatch {
        uint32_t vao      = 0;
        uint32_t vbo      = 0;
        int      vertCount = 0;
        float    r = 0, g = 0, b = 0;
    };

    void buildTerrainBatches(float mapLeft, float mapTop,
                             float tileW,  float tileH);
    void drawQuadNDC(float x, float y, float w, float h,
                     float r, float g, float b, float alpha) const;

    static void surfaceColor(grid::libmap::SurfaceType s,
                             float& r, float& g, float& b);

    // ── Data ─────────────────────────────────────────────────────────────
    const grid::libmap::MapWorld& m_world;

    // Shader handles (owned by GLDisplay, not by this class)
    uint32_t m_shaderProgram = 0;
    int      m_mvpLoc        = -1;
    int      m_colorLoc      = -1;
    int      m_alphaLoc      = -1;

    // Per-surface-type terrain batches (built once, static geometry)
    std::vector<SurfaceBatch> m_batches;

    // Single unit quad VAO shared for all dynamic elements
    uint32_t m_quadVAO = 0;
    uint32_t m_quadVBO = 0;

    // Cached NDC layout (computed in init())
    float m_mapLeft = 0, m_mapTop = 0;
    float m_mapW    = 0, m_mapH   = 0;
    float m_tileW   = 0, m_tileH  = 0;

    bool m_initialized = false;
};

} // namespace battlegrid

#endif // BATTLEGRID_MAP_OVERLAY_HPP_INCLUDED
