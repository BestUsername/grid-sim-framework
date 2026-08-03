#include "gl_display.hpp"
#include "map_overlay.hpp"
#include "sense_indicator.hpp"
#include "soldier.hpp"
#include "civilian.hpp"
#include "vehicle.hpp"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "libio/sdl_keymap.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace battlegrid {

// ─────────────────────────────────────────────────────────
//  Shaders
// ─────────────────────────────────────────────────────────
static const char* kVertSrc = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)glsl";

static const char* kFragSrc = R"glsl(
#version 330 core
uniform vec3 uColor;
uniform float uAlpha;
out vec4 FragColor;
void main() {
    FragColor = vec4(uColor, uAlpha);
}
)glsl";

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), nullptr, buf);
        throw std::runtime_error(std::string("Shader compile error: ") + buf);
    }
    return s;
}

// ─────────────────────────────────────────────────────────
//  Unit cube (36 verts, 12 tris)
// ─────────────────────────────────────────────────────────
static const float kCubeVerts[] = {
    // front
    -0.5f,-0.5f, 0.5f,   0.5f,-0.5f, 0.5f,   0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,   0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
    // back
    -0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,   0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,  -0.5f, 0.5f,-0.5f,   0.5f, 0.5f,-0.5f,
    // left
    -0.5f,-0.5f,-0.5f,  -0.5f,-0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,  -0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,-0.5f,
    // right
     0.5f,-0.5f,-0.5f,   0.5f, 0.5f, 0.5f,   0.5f,-0.5f, 0.5f,
     0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,   0.5f, 0.5f, 0.5f,
    // top
    -0.5f, 0.5f, 0.5f,   0.5f, 0.5f, 0.5f,   0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,   0.5f, 0.5f,-0.5f,  -0.5f, 0.5f,-0.5f,
    // bottom
    -0.5f,-0.5f, 0.5f,   0.5f,-0.5f,-0.5f,   0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,  -0.5f,-0.5f,-0.5f,   0.5f,-0.5f,-0.5f,
};

// ─────────────────────────────────────────────────────────
//  Pyramid (6 tris — 4 sides + 2 base)
// ─────────────────────────────────────────────────────────
static const float kPyramidVerts[] = {
    // base (two tris)
    -0.5f, 0.0f,-0.5f,   0.5f, 0.0f,-0.5f,   0.5f, 0.0f, 0.5f,
    -0.5f, 0.0f,-0.5f,   0.5f, 0.0f, 0.5f,  -0.5f, 0.0f, 0.5f,
    // front
    -0.5f, 0.0f, 0.5f,   0.5f, 0.0f, 0.5f,   0.0f, 1.0f, 0.0f,
    // right
     0.5f, 0.0f, 0.5f,   0.5f, 0.0f,-0.5f,   0.0f, 1.0f, 0.0f,
    // back
     0.5f, 0.0f,-0.5f,  -0.5f, 0.0f,-0.5f,   0.0f, 1.0f, 0.0f,
    // left
    -0.5f, 0.0f,-0.5f,  -0.5f, 0.0f, 0.5f,   0.0f, 1.0f, 0.0f,
};

// ─────────────────────────────────────────────────────────
//  Diamond (8 tris — top pyramid + bottom inverted pyramid, no base)
// ─────────────────────────────────────────────────────────
static const float kDiamondVerts[] = {
    // Top 4 faces (apex at Y=+0.5)
    -0.3f, 0.0f, 0.3f,   0.3f, 0.0f, 0.3f,   0.0f, 0.5f, 0.0f,
     0.3f, 0.0f, 0.3f,   0.3f, 0.0f,-0.3f,   0.0f, 0.5f, 0.0f,
     0.3f, 0.0f,-0.3f,  -0.3f, 0.0f,-0.3f,   0.0f, 0.5f, 0.0f,
    -0.3f, 0.0f,-0.3f,  -0.3f, 0.0f, 0.3f,   0.0f, 0.5f, 0.0f,
    // Bottom 4 faces (nadir at Y=-0.5)
     0.3f, 0.0f, 0.3f,  -0.3f, 0.0f, 0.3f,   0.0f,-0.5f, 0.0f,
     0.3f, 0.0f,-0.3f,   0.3f, 0.0f, 0.3f,   0.0f,-0.5f, 0.0f,
    -0.3f, 0.0f,-0.3f,   0.3f, 0.0f,-0.3f,   0.0f,-0.5f, 0.0f,
    -0.3f, 0.0f, 0.3f,  -0.3f, 0.0f,-0.3f,   0.0f,-0.5f, 0.0f,
};

// ─────────────────────────────────────────────────────────
//  HUD triangle (pointing up, used for directional pings)
// ─────────────────────────────────────────────────────────
static const float kHudTriVerts[] = {
    0.0f,  0.04f, 0.0f,
   -0.015f, -0.01f, 0.0f,
    0.015f, -0.01f, 0.0f,
};

// Unit quad in XY plane (two triangles, (0,0) to (1,1)).
static const float kQuadVerts[] = {
    0.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
};

// ─────────────────────────────────────────────────────────
//  Construction / Destruction
// ─────────────────────────────────────────────────────────
GLDisplay::GLDisplay(const TerrainMap& terrain,
                     const grid::libmap::MapWorld& mapWorld,
                     int windowW, int windowH)
    : m_windowW(windowW), m_windowH(windowH), m_terrain(terrain),
      m_mapOverlay(mapWorld)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());

    // Open any connected game controllers
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            if (auto* gc = SDL_GameControllerOpen(i))
                m_controllers.push_back(gc);
        }
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    m_window = SDL_CreateWindow(
        "BattleGrid",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        m_windowW, m_windowH,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

    if (!m_window)
        throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());

    m_glCtx = SDL_GL_CreateContext(m_window);
    if (!m_glCtx)
        throw std::runtime_error(std::string("SDL_GL_CreateContext: ") + SDL_GetError());

    // Capture mouse for FPS-style look
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_GL_SetSwapInterval(1);

    initGL();
    initShaders();
    initGeometry();
    buildTerrainMesh();
    m_mapOverlay.init(m_shaderProgram, m_mvpLoc, m_colorLoc, m_alphaLoc);
}

GLDisplay::~GLDisplay()
{
    for (auto* gc : m_controllers)
        SDL_GameControllerClose(gc);
    m_controllers.clear();

    if (m_cubeVAO)    glDeleteVertexArrays(1, &m_cubeVAO);
    if (m_cubeVBO)    glDeleteBuffers(1, &m_cubeVBO);
    if (m_pyramidVAO) glDeleteVertexArrays(1, &m_pyramidVAO);
    if (m_pyramidVBO) glDeleteBuffers(1, &m_pyramidVBO);
    if (m_diamondVAO) glDeleteVertexArrays(1, &m_diamondVAO);
    if (m_diamondVBO) glDeleteBuffers(1, &m_diamondVBO);
    if (m_hudTriVAO)  glDeleteVertexArrays(1, &m_hudTriVAO);
    if (m_hudTriVBO)  glDeleteBuffers(1, &m_hudTriVBO);
    if (m_terrainVAO) glDeleteVertexArrays(1, &m_terrainVAO);
    if (m_terrainVBO) glDeleteBuffers(1, &m_terrainVBO);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_glCtx)  SDL_GL_DeleteContext(m_glCtx);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

// ─────────────────────────────────────────────────────────
//  GL init
// ─────────────────────────────────────────────────────────
void GLDisplay::initGL()
{
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
        throw std::runtime_error(std::string("glewInit: ") +
            reinterpret_cast<const char*>(glewGetErrorString(err)));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.52f, 0.73f, 0.87f, 1.0f);  // sky blue
}

void GLDisplay::initShaders()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER,   kVertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragSrc);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vs);
    glAttachShader(m_shaderProgram, fs);
    glLinkProgram(m_shaderProgram);

    GLint ok = 0;
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(m_shaderProgram, sizeof(buf), nullptr, buf);
        throw std::runtime_error(std::string("Shader link: ") + buf);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    m_mvpLoc   = glGetUniformLocation(m_shaderProgram, "uMVP");
    m_colorLoc = glGetUniformLocation(m_shaderProgram, "uColor");
    m_alphaLoc = glGetUniformLocation(m_shaderProgram, "uAlpha");
}

void GLDisplay::initGeometry()
{
    // ── Cube VAO ──
    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO);
    glBindVertexArray(m_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Pyramid VAO ──
    glGenVertexArrays(1, &m_pyramidVAO);
    glGenBuffers(1, &m_pyramidVBO);
    glBindVertexArray(m_pyramidVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kPyramidVerts), kPyramidVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Diamond VAO ──
    glGenVertexArrays(1, &m_diamondVAO);
    glGenBuffers(1, &m_diamondVBO);
    glBindVertexArray(m_diamondVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_diamondVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kDiamondVerts), kDiamondVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── HUD triangle VAO ──
    glGenVertexArrays(1, &m_hudTriVAO);
    glGenBuffers(1, &m_hudTriVBO);
    glBindVertexArray(m_hudTriVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_hudTriVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kHudTriVerts), kHudTriVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Quad VAO (for HUD bars) ──
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVerts), kQuadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────
//  Terrain mesh: one quad (2 tris) per tile, Y = height
// ─────────────────────────────────────────────────────────
void GLDisplay::buildTerrainMesh()
{
    std::vector<float> verts;
    size_t W = m_terrain.width();
    size_t H = m_terrain.height();
    verts.reserve(W * H * 6 * 3); // 2 tris * 3 verts * 3 floats

    for (size_t z = 0; z < H; ++z) {
        for (size_t x = 0; x < W; ++x) {
            float fx = static_cast<float>(x);
            float fz = static_cast<float>(z);
            float fy = static_cast<float>(m_terrain.heightAt(fx + 0.5, fz + 0.5));

            // Two triangles forming a quad for this tile
            // Triangle 1
            verts.push_back(fx);       verts.push_back(fy); verts.push_back(fz);
            verts.push_back(fx + 1.0f);verts.push_back(fy); verts.push_back(fz);
            verts.push_back(fx + 1.0f);verts.push_back(fy); verts.push_back(fz + 1.0f);
            // Triangle 2
            verts.push_back(fx);       verts.push_back(fy); verts.push_back(fz);
            verts.push_back(fx + 1.0f);verts.push_back(fy); verts.push_back(fz + 1.0f);
            verts.push_back(fx);       verts.push_back(fy); verts.push_back(fz + 1.0f);
        }
    }

    m_terrainVertCount = static_cast<int>(verts.size() / 3);

    glGenVertexArrays(1, &m_terrainVAO);
    glGenBuffers(1, &m_terrainVBO);
    glBindVertexArray(m_terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_terrainVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────────────────
std::optional<io::InputEvent> GLDisplay::pollEvent()
{
    if (!m_pendingEvents.empty()) {
        auto ev = m_pendingEvents.front();
        m_pendingEvents.pop();
        return ev;
    }

    SDL_Event sdlEv;
    while (SDL_PollEvent(&sdlEv)) {
        switch (sdlEv.type) {
        case SDL_QUIT: {
            io::KeyEvent ke;
            ke.key    = io::Key::Q;
            ke.action = io::Action::Press;
            ke.modifiers = io::Modifier::None;
            return io::InputEvent{ke};
        }

        case SDL_WINDOWEVENT:
            if (sdlEv.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                m_windowW = sdlEv.window.data1;
                m_windowH = sdlEv.window.data2;
                glViewport(0, 0, m_windowW, m_windowH);
            }
            break;

        case SDL_MOUSEMOTION:
            m_mouse.feedMotion(sdlEv.motion.x, sdlEv.motion.y,
                               sdlEv.motion.xrel, sdlEv.motion.yrel);
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            m_mouse.feedButton(sdlEv.button.button,
                               sdlEv.type == SDL_MOUSEBUTTONDOWN,
                               sdlEv.button.x, sdlEv.button.y);
            break;

        case SDL_MOUSEWHEEL:
            m_mouse.feedScroll(sdlEv.wheel.x, sdlEv.wheel.y);
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            m_keyboard.feedSDLEvent(
                sdlEv.key.keysym.scancode,
                sdlEv.key.keysym.mod,
                sdlEv.type == SDL_KEYDOWN,
                sdlEv.key.repeat != 0);
            break;

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            m_gamepad.feedButton(
                sdlEv.cbutton.which,
                sdlEv.cbutton.button,
                sdlEv.type == SDL_CONTROLLERBUTTONDOWN);
            break;

        case SDL_CONTROLLERAXISMOTION:
            m_gamepad.feedAxis(
                sdlEv.caxis.which,
                sdlEv.caxis.axis,
                sdlEv.caxis.value);
            break;

        case SDL_CONTROLLERDEVICEADDED:
            if (SDL_IsGameController(sdlEv.cdevice.which)) {
                if (auto* gc = SDL_GameControllerOpen(sdlEv.cdevice.which))
                    m_controllers.push_back(gc);
            }
            break;

        default:
            break;
        }
    }

    // Drain mouse first (for look), then keyboard, then gamepad
    if (auto mev = m_mouse.poll())
        return mev;
    if (auto kev = m_keyboard.poll())
        return kev;
    if (auto gev = m_gamepad.poll())
        return gev;

    return std::nullopt;
}

// ─────────────────────────────────────────────────────────
//  Matrix math
// ─────────────────────────────────────────────────────────
GLDisplay::Mat4 GLDisplay::identity()
{
    Mat4 m{};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

GLDisplay::Mat4 GLDisplay::perspective(float fovY, float aspect, float nearP, float farP)
{
    Mat4 m{};
    float tanHalf = std::tan(fovY * 0.5f);
    m.m[0]  = 1.0f / (aspect * tanHalf);
    m.m[5]  = 1.0f / tanHalf;
    m.m[10] = -(farP + nearP) / (farP - nearP);
    m.m[11] = -1.0f;
    m.m[14] = -(2.0f * farP * nearP) / (farP - nearP);
    return m;
}

GLDisplay::Mat4 GLDisplay::lookAt(float ex, float ey, float ez,
                                   float ax, float ay, float az,
                                   float ux, float uy, float uz)
{
    float fx = ax - ex, fy = ay - ey, fz = az - ez;
    float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
    if (fl < 1e-6f) return identity();
    fx /= fl; fy /= fl; fz /= fl;

    float rx = fy*uz - fz*uy;
    float ry = fz*ux - fx*uz;
    float rz = fx*uy - fy*ux;
    float rl = std::sqrt(rx*rx + ry*ry + rz*rz);
    if (rl < 1e-6f) return identity();
    rx /= rl; ry /= rl; rz /= rl;

    float tux = ry*fz - rz*fy;
    float tuy = rz*fx - rx*fz;
    float tuz = rx*fy - ry*fx;

    Mat4 m{};
    m.m[0]  = rx;   m.m[4]  = ry;   m.m[8]  = rz;
    m.m[1]  = tux;  m.m[5]  = tuy;  m.m[9]  = tuz;
    m.m[2]  = -fx;  m.m[6]  = -fy;  m.m[10] = -fz;
    m.m[12] = -(rx*ex + ry*ey + rz*ez);
    m.m[13] = -(tux*ex + tuy*ey + tuz*ez);
    m.m[14] =  (fx*ex + fy*ey + fz*ez);
    m.m[15] = 1.0f;
    return m;
}

GLDisplay::Mat4 GLDisplay::multiply(const Mat4& a, const Mat4& b)
{
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
                sum += a.m[k * 4 + row] * b.m[c * 4 + k];
            r.m[c * 4 + row] = sum;
        }
    return r;
}

GLDisplay::Mat4 GLDisplay::translate(float x, float y, float z)
{
    Mat4 m = identity();
    m.m[12] = x; m.m[13] = y; m.m[14] = z;
    return m;
}

GLDisplay::Mat4 GLDisplay::rotateY(float radians)
{
    Mat4 m = identity();
    float c = std::cos(radians);
    float s = std::sin(radians);
    m.m[0]  =  c;
    m.m[2]  = -s;
    m.m[8]  =  s;
    m.m[10] =  c;
    return m;
}

GLDisplay::Mat4 GLDisplay::scale(float sx, float sy, float sz)
{
    Mat4 m{};
    m.m[0] = sx; m.m[5] = sy; m.m[10] = sz; m.m[15] = 1.0f;
    return m;
}

// ─────────────────────────────────────────────────────────
//  Drawing helpers
// ─────────────────────────────────────────────────────────
void GLDisplay::drawCube(const Mat4& vp, float x, float y, float z,
                          float sx, float sy, float sz,
                          float r, float g, float b)
{
    Mat4 model = multiply(translate(x, y, z), scale(sx, sy, sz));
    Mat4 mvp = multiply(vp, model);
    glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, mvp.m);
    glUniform3f(m_colorLoc, r, g, b);
    glUniform1f(m_alphaLoc, 1.0f);
    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void GLDisplay::drawPyramid(const Mat4& vp, float x, float y, float z,
                              float s, float r, float g, float b)
{
    Mat4 model = multiply(translate(x, y, z), scale(s, s, s));
    Mat4 mvp = multiply(vp, model);
    glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, mvp.m);
    glUniform3f(m_colorLoc, r, g, b);
    glUniform1f(m_alphaLoc, 1.0f);
    glBindVertexArray(m_pyramidVAO);
    glDrawArrays(GL_TRIANGLES, 0, 18);
}

void GLDisplay::drawDiamond(const Mat4& vp, float x, float y, float z,
                             float s, float r, float g, float b, float alpha)
{
    Mat4 model = multiply(translate(x, y, z), scale(s, s, s));
    Mat4 mvp = multiply(vp, model);
    glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, mvp.m);
    glUniform3f(m_colorLoc, r, g, b);
    glUniform1f(m_alphaLoc, alpha);
    glBindVertexArray(m_diamondVAO);
    glDrawArrays(GL_TRIANGLES, 0, 24); // 8 triangles
}

void GLDisplay::drawHudPings(const SenseIndicatorManager& indicators)
{
    // Draw 2D directional triangles around screen edge in NDC space.
    // Disable depth testing for HUD overlay.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Use identity MVP — positions are in NDC (-1..1)
    Mat4 ident = identity();

    for (const auto& ping : indicators.hudPings()) {
        float r, g, b;
        senseColor(ping.sense, r, g, b);
        float alpha = ping.alpha();

        // Place triangle on an ellipse at screen edge, rotated to point inward
        float angle = ping.bearing; // radians, 0 = right on screen
        float edgeX = std::cos(angle) * 0.85f;
        float edgeY = std::sin(angle) * 0.85f;

        // Rotation: triangle points inward (toward center)
        // Rotate the triangle by (angle + pi) so it points toward center
        float rot = angle + 3.14159265f;
        float cosR = std::cos(rot);
        float sinR = std::sin(rot);

        // Build a model matrix: translate to edge pos, rotate around Z, scale up
        Mat4 model{};
        float sc = 1.5f; // scale up the small triangle
        model.m[0]  =  cosR * sc;  model.m[1]  = sinR * sc;
        model.m[4]  = -sinR * sc;  model.m[5]  = cosR * sc;
        model.m[10] = 1.0f;
        model.m[12] = edgeX;       model.m[13] = edgeY;
        model.m[15] = 1.0f;

        glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, model.m);
        glUniform3f(m_colorLoc, r, g, b);
        glUniform1f(m_alphaLoc, alpha);
        glBindVertexArray(m_hudTriVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void GLDisplay::drawHudHealthBar(
    const std::vector<std::shared_ptr<I_AGENT>>& agents)
{
    // Find the player soldier.
    Soldier* player = nullptr;
    for (const auto& agent : agents) {
        auto* s = dynamic_cast<Soldier*>(agent.get());
        if (s && s->isPlayerControlled()) { player = s; break; }
    }
    if (!player) return;

    float healthFrac = static_cast<float>(player->health() / player->maxHealth());
    if (healthFrac < 0.0f) healthFrac = 0.0f;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Health bar position in NDC: bottom-left corner.
    // Background bar: dark grey, full width.
    constexpr float barX  = -0.92f;
    constexpr float barY  = -0.92f;
    constexpr float barW  =  0.30f;
    constexpr float barH  =  0.04f;

    auto drawQuad = [&](float x, float y, float w, float h,
                        float r, float g, float b, float a) {
        // Map unit quad (0,0)-(1,1) to (x,y)-(x+w,y+h) in NDC.
        Mat4 mvp = multiply(translate(x, y, 0.0f), scale(w, h, 1.0f));
        glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, mvp.m);
        glUniform3f(m_colorLoc, r, g, b);
        glUniform1f(m_alphaLoc, a);
        glBindVertexArray(m_quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    };

    // Background
    drawQuad(barX, barY, barW, barH, 0.2f, 0.2f, 0.2f, 0.6f);

    // Foreground: green → yellow → red as health drops.
    float r = (healthFrac < 0.5f) ? 1.0f : 1.0f - (healthFrac - 0.5f) * 2.0f;
    float g = (healthFrac > 0.5f) ? 1.0f : healthFrac * 2.0f;
    drawQuad(barX, barY, barW * healthFrac, barH, r, g, 0.1f, 0.85f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

// ─────────────────────────────────────────────────────────
//  Window title
// ─────────────────────────────────────────────────────────
void GLDisplay::updateWindowTitle(grid::libsim::State state, int agentCount,
                                   const grid::libsim::GameLog& log)
{
    const char* stStr = "PAUSED";
    if (state == grid::libsim::State::RUNNING) stStr = "RUNNING";
    else if (state == grid::libsim::State::STOPPED) stStr = "STOPPED";

    std::ostringstream oss;
    oss << "BattleGrid  |  " << stStr
        << "  |  Entities: " << agentCount;

    auto lastOpt = log.getLastEntry();
    if (lastOpt)
        oss << "  |  " << lastOpt->source << ": " << lastOpt->message;

    SDL_SetWindowTitle(m_window, oss.str().c_str());
}

// ─────────────────────────────────────────────────────────
//  Render frame
// ─────────────────────────────────────────────────────────
void GLDisplay::renderFrame(
    const std::vector<std::shared_ptr<I_AGENT>>& agents,
    const PositionSnapshot& positions,
    grid::libsim::State engineState,
    const grid::libsim::GameLog& log,
    const PlayerController& ctrl,
    const SenseIndicatorManager& indicators,
    bool showMap)
{
    glViewport(0, 0, m_windowW, m_windowH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ── Camera from player controller ──
    COORD camPos = ctrl.cameraPosition();
    COORD camTarget = ctrl.cameraTarget();

    float aspect = static_cast<float>(m_windowW) /
                   static_cast<float>(std::max(1, m_windowH));
    constexpr float pi = 3.14159265358979f;
    Mat4 proj = perspective(60.0f * pi / 180.0f, aspect, 0.1f, 500.0f);
    Mat4 view = lookAt(
        static_cast<float>(camPos[0]),    static_cast<float>(camPos[1]),    static_cast<float>(camPos[2]),
        static_cast<float>(camTarget[0]), static_cast<float>(camTarget[1]), static_cast<float>(camTarget[2]),
        0.0f, 1.0f, 0.0f);
    Mat4 vp = multiply(proj, view);

    glUseProgram(m_shaderProgram);

    // ── Draw terrain ──
    // We draw the terrain mesh once per terrain type with different colors.
    // Since we built one big mesh, we'll color it tile-by-tile.
    // For efficiency, draw the whole mesh in one pass with a neutral color,
    // then overlay per-tile coloring via individual quads.
    // Actually, let's draw terrain tile by tile using cubes for the
    // first generation — simple and effective.
    {
        size_t W = m_terrain.width();
        size_t H = m_terrain.height();
        for (size_t z = 0; z < H; ++z) {
            for (size_t x = 0; x < W; ++x) {
                TerrainType t = m_terrain.at(x, z);
                float fy = static_cast<float>(terrainHeight(t));

                float r, g, b;
                switch (t) {
                case TerrainType::Water:
                    r = 0.15f; g = 0.40f; b = 0.75f; break;
                case TerrainType::Mountain:
                    r = 0.55f; g = 0.50f; b = 0.45f; break;
                case TerrainType::Bump:
                    r = 0.42f; g = 0.58f; b = 0.18f; break;
                default: // Land
                    r = 0.30f; g = 0.65f; b = 0.20f; break;
                }

                // Tile as a flat box: 1 wide, some height, 1 deep
                float tileH = (t == TerrainType::Mountain) ? 5.0f :
                              (t == TerrainType::Water)    ? 1.0f :
                              (t == TerrainType::Bump)     ? 0.75f : 0.5f;
                float cx = static_cast<float>(x) + 0.5f;
                float cy = fy - tileH * 0.5f;
                float cz = static_cast<float>(z) + 0.5f;

                drawCube(vp, cx, cy, cz, 1.0f, tileH, 1.0f, r, g, b);
            }
        }
    }

    // ── Draw entities ──
    for (const auto& agent : agents) {
        auto posIt = positions.find(agent->name());
        if (posIt == positions.end()) continue;
        const auto& pos = posIt->second;
        float ax = static_cast<float>(pos[0]);
        float ay = static_cast<float>(pos[1]);
        float az = static_cast<float>(pos[2]);

        // Determine entity type by trying dynamic_cast
        auto* soldier  = dynamic_cast<Soldier*>(agent.get());
        auto* civilian = dynamic_cast<Civilian*>(agent.get());
        auto* airV     = dynamic_cast<AirVehicle*>(agent.get());
        auto* seaV     = dynamic_cast<SeaVehicle*>(agent.get());
        auto* landV    = dynamic_cast<LandVehicle*>(agent.get());

        // Get yaw for rotation
        float yaw = 0.0f;
        if      (soldier)  yaw = static_cast<float>(soldier->yaw());
        else if (civilian) yaw = static_cast<float>(civilian->yaw());
        else if (airV)     yaw = static_cast<float>(airV->yaw());
        else if (seaV)     yaw = static_cast<float>(seaV->yaw());
        else if (landV)    yaw = static_cast<float>(landV->yaw());

        // Base transform: translate to entity position, then rotate by yaw.
        // Model forward is +Z; camera yaw 0 faces +X, so offset by π/2.
        // Negate yaw to match the world-space coordinate handedness.
        constexpr float kHalfPi = 1.5707963267948966f;
        Mat4 base = multiply(translate(ax, ay, az), rotateY(-yaw + kHalfPi));

        auto drawPart = [&](float ox, float oy, float oz,
                            float sx, float sy, float sz,
                            float r, float g, float b) {
            Mat4 model = multiply(base, multiply(translate(ox, oy, oz),
                                                 scale(sx, sy, sz)));
            Mat4 mvp = multiply(vp, model);
            glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, mvp.m);
            glUniform3f(m_colorLoc, r, g, b);
            glUniform1f(m_alphaLoc, 1.0f);
            glBindVertexArray(m_cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        };

        if (soldier) {
            // In first-person mode, skip rendering the player soldier entirely
            if (soldier->isPlayerControlled()
                && ctrl.cameraMode() == CameraMode::FirstPerson) {
                continue;
            }

            if (soldier->isDead()) {
                // Dead: squashed + grey
                drawPart(0.0f, 0.1f, 0.0f,  0.5f, 0.2f, 0.5f,
                         0.4f, 0.4f, 0.4f);
            } else {
                float r, g, b;
                if (soldier->faction() == Faction::Blue)      { r = 0.2f; g = 0.4f; b = 0.9f; }
                else if (soldier->faction() == Faction::Red)   { r = 0.9f; g = 0.2f; b = 0.2f; }
                else                                           { r = 0.7f; g = 0.7f; b = 0.7f; }

                drawPart(0.0f, 0.75f, 0.0f,  0.4f, 1.5f, 0.3f, r, g, b);        // body
                drawPart(0.0f, 1.7f,  0.0f,  0.25f, 0.25f, 0.25f,
                         0.9f, 0.75f, 0.6f);                                      // head
            }
        } else if (civilian) {
            if (civilian->isDead()) {
                drawPart(0.0f, 0.1f, 0.0f,  0.45f, 0.2f, 0.45f,
                         0.4f, 0.4f, 0.4f);
            } else {
                drawPart(0.0f, 0.65f, 0.0f,  0.35f, 1.3f, 0.3f,
                         0.85f, 0.75f, 0.3f);                                     // body
                drawPart(0.0f, 1.5f,  0.0f,  0.22f, 0.22f, 0.22f,
                         0.9f, 0.75f, 0.6f);                                      // head
            }
        } else if (airV) {
            if (airV->isDead()) {
                drawPart(0.0f, 0.0f, 0.0f,  1.2f, 0.15f, 3.0f,
                         0.4f, 0.4f, 0.4f);
            } else {
                drawPart(0.0f, 0.0f, 0.0f,   1.2f, 0.4f, 3.0f,
                         0.4f, 0.4f, 0.45f);                                      // fuselage
                drawPart(0.0f, 0.1f, 0.0f,   4.0f, 0.1f, 0.8f,
                         0.45f, 0.45f, 0.5f);                                     // wings
            }
        } else if (seaV) {
            if (seaV->isDead()) {
                drawPart(0.0f, 0.1f, 0.0f,  0.8f, 0.2f, 2.0f,
                         0.4f, 0.4f, 0.4f);
            } else {
                drawPart(0.0f, 0.3f,  0.0f,  0.8f, 0.6f, 2.0f,
                         0.2f, 0.25f, 0.5f);                                      // hull
                drawPart(0.0f, 0.8f, -0.2f,  0.5f, 0.4f, 0.6f,
                         0.6f, 0.6f, 0.65f);                                      // cabin
            }
        } else if (landV) {
            if (landV->isDead()) {
                drawPart(0.0f, 0.2f, 0.0f,  1.2f, 0.3f, 2.2f,
                         0.4f, 0.4f, 0.4f);
            } else {
                drawPart(0.0f, 0.5f, 0.0f,   1.2f, 0.8f, 2.2f,
                         0.4f, 0.45f, 0.25f);                                     // body
                drawPart(0.0f, 1.1f, 0.1f,   0.5f, 0.3f, 0.5f,
                         0.35f, 0.38f, 0.22f);                                    // turret
            }
        } else {
            drawPart(0.0f, 0.5f, 0.0f,   0.5f, 0.5f, 0.5f,
                     1.0f, 1.0f, 1.0f);                                       // unknown
        }
    }

    // ── Draw mountains as pyramids on top of terrain ──
    {
        size_t W = m_terrain.width();
        size_t H = m_terrain.height();
        for (size_t z = 0; z < H; ++z) {
            for (size_t x = 0; x < W; ++x) {
                if (m_terrain.at(x, z) == TerrainType::Mountain) {
                    float fx = static_cast<float>(x) + 0.5f;
                    float fz = static_cast<float>(z) + 0.5f;
                    float fy = static_cast<float>(terrainHeight(TerrainType::Mountain));
                    drawPyramid(vp, fx, fy, fz, 0.8f, 0.65f, 0.6f, 0.55f);
                }
            }
        }
    }

    // ── Draw sense indicators (floating diamonds above entities) ──
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (const auto& ind : indicators.indicators()) {
            float r, g, b;
            senseColor(ind.sense, r, g, b);
            float alpha = ind.alpha();
            float fx = static_cast<float>(ind.position[0]);
            float fy = static_cast<float>(ind.position[1]) + 2.5f; // float above head
            float fz = static_cast<float>(ind.position[2]);
            // Bob up and down slightly based on remaining lifetime
            fy += 0.15f * std::sin(ind.lifetime * 4.0f);
            drawDiamond(vp, fx, fy, fz, 0.35f, r, g, b, alpha);
        }

        // ── Draw HUD directional pings ──
        drawHudPings(indicators);

        // ── Draw player health bar ──
        drawHudHealthBar(agents);

        glDisable(GL_BLEND);
    }

    // ── Overhead map overlay (toggled by M key) ──
    if (showMap)
        m_mapOverlay.render(agents, positions, m_windowW, m_windowH);

    updateWindowTitle(engineState, static_cast<int>(agents.size()), log);
    SDL_GL_SwapWindow(m_window);
}

} // namespace battlegrid
