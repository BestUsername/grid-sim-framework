#ifndef BATTLEGRID_GL_DISPLAY_HPP_INCLUDED
#define BATTLEGRID_GL_DISPLAY_HPP_INCLUDED

#include "defines.hpp"
#include "terrain.hpp"
#include "entity_types.hpp"
#include "sense_indicator.hpp"
#include "player_controller.hpp"
#include "map_overlay.hpp"

#include "libmap/map_world.hpp"
#include "libio/sdl_keyboard.hpp"
#include "libio/sdl_mouse.hpp"
#include "libio/sdl_gamepad.hpp"
#include "libio/input_event.hpp"
#include "libsim/base_engine.hpp"
#include "libsim/game_log.hpp"

#include <cstdint>
#include <optional>
#include <queue>
#include <string>
#include <vector>

struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;

struct SDL_Window;

namespace battlegrid {

/**
 * @brief OpenGL 3.3 3D display for BattleGrid.
 *
 * Renders terrain as colored height-mapped tiles, entities as 3D
 * shapes, and supports player-following camera in FPS / 3rd-person.
 * Press M (ToggleMap action) to show / hide the overhead map overlay.
 */
class GLDisplay {
public:
    GLDisplay(const TerrainMap& terrain,
              const grid::libmap::MapWorld& mapWorld,
              int windowW = 1280, int windowH = 720);
    ~GLDisplay();

    /// Poll for user input events (keyboard, mouse, quit).
    std::optional<io::InputEvent> pollEvent();

    /// Render the full scene.
    void renderFrame(const std::vector<std::shared_ptr<I_AGENT>>& agents,
                     const PositionSnapshot& positions,
                     grid::libsim::State engineState,
                     const grid::libsim::GameLog& log,
                     const PlayerController& ctrl,
                     const SenseIndicatorManager& indicators,
                     bool showMap = false);

private:
    // ── Init helpers ──
    void initGL();
    void initShaders();
    void initGeometry();
    void buildTerrainMesh();
    void updateWindowTitle(grid::libsim::State state, int agentCount,
                           const grid::libsim::GameLog& log);

    // ── Math (inline, no deps) ──
    struct Mat4 { float m[16]; };
    static Mat4 identity();
    static Mat4 perspective(float fovY, float aspect, float nearP, float farP);
    static Mat4 lookAt(float ex, float ey, float ez,
                       float ax, float ay, float az,
                       float ux, float uy, float uz);
    static Mat4 multiply(const Mat4& a, const Mat4& b);
    static Mat4 translate(float x, float y, float z);
    static Mat4 rotateY(float radians);
    static Mat4 scale(float sx, float sy, float sz);

    void drawCube(const Mat4& vp, float x, float y, float z,
                  float sx, float sy, float sz,
                  float r, float g, float b);
    void drawPyramid(const Mat4& vp, float x, float y, float z,
                     float s, float r, float g, float b);
    void drawDiamond(const Mat4& vp, float x, float y, float z,
                     float s, float r, float g, float b, float alpha);
    void drawHudPings(const SenseIndicatorManager& indicators);
    void drawHudHealthBar(const std::vector<std::shared_ptr<I_AGENT>>& agents);

    // ── SDL / GL state ──
    SDL_Window* m_window = nullptr;
    void*       m_glCtx  = nullptr;
    int m_windowW, m_windowH;

    const TerrainMap& m_terrain;

    // ── GL objects ──
    uint32_t m_shaderProgram = 0;
    int      m_mvpLoc        = -1;
    int      m_colorLoc      = -1;
    int      m_alphaLoc      = -1;

    // Geometry
    uint32_t m_cubeVAO = 0, m_cubeVBO = 0;
    uint32_t m_terrainVAO = 0, m_terrainVBO = 0;
    int      m_terrainVertCount = 0;
    uint32_t m_pyramidVAO = 0, m_pyramidVBO = 0;
    uint32_t m_diamondVAO = 0, m_diamondVBO = 0;
    uint32_t m_hudTriVAO  = 0, m_hudTriVBO  = 0;
    uint32_t m_quadVAO     = 0, m_quadVBO    = 0;

    // ── Input ──
    io::SDLKeyboard m_keyboard;
    io::SDLMouse    m_mouse;
    io::SDLGamepad  m_gamepad;
    std::queue<io::InputEvent> m_pendingEvents;
    std::vector<SDL_GameController*> m_controllers;

    // ── Map overlay ──
    MapOverlay m_mapOverlay;
};

} // namespace battlegrid

#endif // BATTLEGRID_GL_DISPLAY_HPP_INCLUDED
