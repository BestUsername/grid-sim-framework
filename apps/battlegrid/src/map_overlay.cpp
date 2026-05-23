#include "map_overlay.hpp"
#include "civilian.hpp"
#include "vehicle.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <cstddef>
#include <map>
#include <vector>

namespace battlegrid {

// Unit quad in XY plane: (0,0) → (1,1), two triangles.
static const float kQuadVerts[] = {
    0.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
};

// ── Construction / Destruction ────────────────────────────────────────────────

MapOverlay::MapOverlay(const grid::libmap::MapWorld& world)
    : m_world(world)
{
}

MapOverlay::~MapOverlay()
{
    destroy();
}

// ── Public interface ──────────────────────────────────────────────────────────

void MapOverlay::init(uint32_t shaderProgram, int mvpLoc, int colorLoc, int alphaLoc)
{
    if (m_initialized) return;

    m_shaderProgram = shaderProgram;
    m_mvpLoc        = mvpLoc;
    m_colorLoc      = colorLoc;
    m_alphaLoc      = alphaLoc;

    // ── Unit quad VAO ──
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVerts), kQuadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Compute NDC layout ──
    // The map occupies an 85% × 85% centred square (letterboxed for aspect).
    // Tiles map 1:1 in NDC unless the map is very large, in which case we
    // simply scale to fit.
    constexpr float kMargin = 0.10f;  // fraction of NDC half-extent left as margin
    m_mapLeft = -1.0f + kMargin;
    m_mapTop  =  1.0f - kMargin;
    m_mapW    =  2.0f - 2.0f * kMargin;
    m_mapH    =  2.0f - 2.0f * kMargin;

    auto mapCols = static_cast<float>(std::max(m_world.width(),  size_t(1)));
    auto mapRows = static_cast<float>(std::max(m_world.height(), size_t(1)));
    m_tileW = m_mapW / mapCols;
    m_tileH = m_mapH / mapRows;

    buildTerrainBatches(m_mapLeft, m_mapTop, m_tileW, m_tileH);
    m_initialized = true;
}

void MapOverlay::destroy()
{
    for (auto& b : m_batches) {
        if (b.vao) glDeleteVertexArrays(1, &b.vao);
        if (b.vbo) glDeleteBuffers(1, &b.vbo);
    }
    m_batches.clear();

    if (m_quadVAO) { glDeleteVertexArrays(1, &m_quadVAO); m_quadVAO = 0; }
    if (m_quadVBO) { glDeleteBuffers(1, &m_quadVBO);      m_quadVBO = 0; }
    m_initialized = false;
}

void MapOverlay::render(const std::vector<std::shared_ptr<I_AGENT>>& agents,
                         const PositionSnapshot& positions,
                         int /*windowW*/, int /*windowH*/) const
{
    if (!m_initialized) return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── Semi-transparent background ──
    drawQuadNDC(m_mapLeft - 0.02f, m_mapTop - m_mapH - 0.02f,
                m_mapW + 0.04f,    m_mapH   + 0.04f,
                0.05f, 0.05f, 0.05f, 0.80f);

    // ── Terrain batches ──
    // Identity MVP: vertices are already in NDC.
    float ident[16] = {
        1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1
    };
    glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, ident);

    for (const auto& batch : m_batches) {
        glUniform3f(m_colorLoc, batch.r, batch.g, batch.b);
        glUniform1f(m_alphaLoc, 0.90f);
        glBindVertexArray(batch.vao);
        glDrawArrays(GL_TRIANGLES, 0, batch.vertCount);
    }

    // ── Agent dots ──
    auto mapCols = static_cast<double>(std::max(m_world.width(),  size_t(1)));
    auto mapRows = static_cast<double>(std::max(m_world.height(), size_t(1)));

    constexpr float kDotW = 0.018f;
    constexpr float kDotH = 0.018f;

    for (const auto& agent : agents) {
        auto posIt = positions.find(agent->name());
        if (posIt == positions.end()) continue;

        double wx = posIt->second[0];
        double wz = posIt->second[2];

        // Map world-space XZ → NDC position (top-left origin for Z axis).
        float nx = m_mapLeft + static_cast<float>(wx / mapCols) * m_mapW;
        float ny = m_mapTop  - static_cast<float>(wz / mapRows) * m_mapH;

        // Faction / entity colour.
        float r = 0.8f, g = 0.8f, b = 0.2f;  // default: yellow
        auto* soldier  = dynamic_cast<Soldier*>(agent.get());
        auto* civilian = dynamic_cast<Civilian*>(agent.get());
        auto* vehicle  = dynamic_cast<Vehicle*>(agent.get());

        if (soldier) {
            if (soldier->isDead()) { r = 0.4f; g = 0.4f; b = 0.4f; }
            else if (soldier->faction() == Faction::Blue) { r = 0.2f; g = 0.5f; b = 1.0f; }
            else if (soldier->faction() == Faction::Red)  { r = 1.0f; g = 0.2f; b = 0.2f; }
            else                                           { r = 0.8f; g = 0.8f; b = 0.2f; }

            if (soldier->isPlayerControlled()) {
                // Draw player as a slightly larger white dot.
                drawQuadNDC(nx - kDotW, ny - kDotH, kDotW * 2.0f, kDotH * 2.0f,
                            1.0f, 1.0f, 1.0f, 1.0f);
                continue;
            }
        } else if (civilian) {
            r = 0.6f; g = 0.9f; b = 0.6f;
        } else if (vehicle) {
            if (vehicle->faction() == Faction::Blue)      { r = 0.4f; g = 0.7f; b = 1.0f; }
            else if (vehicle->faction() == Faction::Red)  { r = 1.0f; g = 0.4f; b = 0.4f; }
            else                                           { r = 0.9f; g = 0.9f; b = 0.5f; }
        }

        drawQuadNDC(nx - kDotW * 0.5f, ny - kDotH * 0.5f,
                    kDotW, kDotH, r, g, b, 1.0f);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void MapOverlay::surfaceColor(grid::libmap::SurfaceType s,
                               float& r, float& g, float& b)
{
    using grid::libmap::SurfaceType;
    switch (s) {
    case SurfaceType::Water:    r = 0.15f; g = 0.40f; b = 0.75f; return;
    case SurfaceType::Mountain: r = 0.55f; g = 0.50f; b = 0.45f; return;
    case SurfaceType::Forest:   r = 0.13f; g = 0.45f; b = 0.13f; return;
    case SurfaceType::Road:     r = 0.65f; g = 0.60f; b = 0.55f; return;
    case SurfaceType::Building: r = 0.50f; g = 0.45f; b = 0.40f; return;
    case SurfaceType::Sand:     r = 0.85f; g = 0.80f; b = 0.50f; return;
    case SurfaceType::Swamp:    r = 0.35f; g = 0.50f; b = 0.30f; return;
    default: /* Land */         r = 0.30f; g = 0.65f; b = 0.20f; return;
    }
}

void MapOverlay::buildTerrainBatches(float mapLeft, float mapTop,
                                      float tileW,   float tileH)
{
    using grid::libmap::SurfaceType;

    // Group tile quads by surface type to minimise draw calls.
    std::map<SurfaceType, std::vector<float>> groups;

    for (size_t z = 0; z < m_world.height(); ++z) {
        for (size_t x = 0; x < m_world.width(); ++x) {
            SurfaceType s = m_world.terrain().get(x, z).surface;
            float nx = mapLeft + static_cast<float>(x) * tileW;
            float ny = mapTop  - static_cast<float>(z) * tileH;

            // Two triangles, Z=0 in NDC (overlay plane).
            auto& v = groups[s];
            // Triangle 1
            v.push_back(nx);        v.push_back(ny - tileH); v.push_back(0.0f);
            v.push_back(nx + tileW);v.push_back(ny - tileH); v.push_back(0.0f);
            v.push_back(nx + tileW);v.push_back(ny);          v.push_back(0.0f);
            // Triangle 2
            v.push_back(nx);        v.push_back(ny - tileH); v.push_back(0.0f);
            v.push_back(nx + tileW);v.push_back(ny);          v.push_back(0.0f);
            v.push_back(nx);        v.push_back(ny);          v.push_back(0.0f);
        }
    }

    for (auto& [surface, verts] : groups) {
        SurfaceBatch batch;
        surfaceColor(surface, batch.r, batch.g, batch.b);
        batch.vertCount = static_cast<int>(verts.size() / 3);

        glGenVertexArrays(1, &batch.vao);
        glGenBuffers(1, &batch.vbo);
        glBindVertexArray(batch.vao);
        glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                     verts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        m_batches.push_back(batch);
    }
}

void MapOverlay::drawQuadNDC(float x, float y, float w, float h,
                               float r, float g, float b, float alpha) const
{
    // Scale the unit quad (0,0)→(1,1) to (x,y)→(x+w,y+h) in NDC.
    float mvp[16] = {
        w,    0,    0,    0,
        0,    h,    0,    0,
        0,    0,    1,    0,
        x,    y,    0,    1,
    };
    glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, mvp);
    glUniform3f(m_colorLoc, r, g, b);
    glUniform1f(m_alphaLoc, alpha);
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

} // namespace battlegrid
