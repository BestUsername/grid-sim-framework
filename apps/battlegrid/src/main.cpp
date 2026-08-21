#include "defines.hpp"
#include "battlegrid_world.hpp"
#include "gl_display.hpp"
#include "input_map.hpp"
#include "sense_indicator.hpp"

#include "libsim/game_log.hpp"
#include "libio/input_event.hpp"
#include "libio/keycodes.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <variant>

using namespace grid::libsim;

namespace {

void bindDefaultInputs(battlegrid::InputMap& inputMap)
{
    inputMap.bindKey(io::Key::W,          battlegrid::GameAction::MoveZ,  -1.0f);
    inputMap.bindKey(io::Key::S,          battlegrid::GameAction::MoveZ,   1.0f);
    inputMap.bindKey(io::Key::A,          battlegrid::GameAction::MoveX,  -1.0f);
    inputMap.bindKey(io::Key::D,          battlegrid::GameAction::MoveX,   1.0f);
    inputMap.bindKey(io::Key::LeftShift,  battlegrid::GameAction::Sprint,  1.0f);
    inputMap.bindKey(io::Key::RightShift, battlegrid::GameAction::Sprint,  1.0f);

    inputMap.bindKey(io::Key::Space, battlegrid::GameAction::Jump);
    inputMap.bindKey(io::Key::E,     battlegrid::GameAction::Interact);
    inputMap.bindKey(io::Key::T,     battlegrid::GameAction::Shout);
    inputMap.bindKey(io::Key::Tab,   battlegrid::GameAction::ToggleCamera);
    inputMap.bindKey(io::Key::M,     battlegrid::GameAction::ToggleMap);

    inputMap.bindMouseX(battlegrid::GameAction::LookX, 1.0f);
    inputMap.bindMouseY(battlegrid::GameAction::LookY, 1.0f);
    inputMap.bindScrollY(battlegrid::GameAction::Zoom, 0.5f);

    inputMap.bindAxis(io::GamepadAxis::LeftX, battlegrid::GameAction::MoveX, 1.0f);
    inputMap.bindAxis(io::GamepadAxis::LeftY, battlegrid::GameAction::MoveZ, 1.0f);
    inputMap.bindAxis(io::GamepadAxis::RightX, battlegrid::GameAction::LookX, 1.0f);
    inputMap.bindAxis(io::GamepadAxis::RightY, battlegrid::GameAction::LookY, 1.0f);
    inputMap.bindAxis(io::GamepadAxis::LeftTrigger, battlegrid::GameAction::Sprint, 1.0f);

    inputMap.bindButton(io::GamepadButton::B,          battlegrid::GameAction::Jump);
    inputMap.bindButton(io::GamepadButton::A,          battlegrid::GameAction::Interact);
    inputMap.bindButton(io::GamepadButton::Y,          battlegrid::GameAction::Shout);
    inputMap.bindButton(io::GamepadButton::LeftBumper, battlegrid::GameAction::ToggleCamera);
    inputMap.bindButton(io::GamepadButton::DPadUp,     battlegrid::GameAction::MoveZ, -1.0f);
    inputMap.bindButton(io::GamepadButton::DPadDown,   battlegrid::GameAction::MoveZ,  1.0f);
    inputMap.bindButton(io::GamepadButton::DPadLeft,   battlegrid::GameAction::MoveX, -1.0f);
    inputMap.bindButton(io::GamepadButton::DPadRight,  battlegrid::GameAction::MoveX,  1.0f);
}

battlegrid::TerrainMap makeDefaultMap()
{
    battlegrid::TerrainMap map(64, 64, battlegrid::TerrainType::Land);

    for (size_t z = 0; z < 64; ++z) {
        for (size_t x = 0; x < 64; ++x) {
            if (x < 8 || z < 5) {
                map.set(x, z, battlegrid::TerrainType::Water);
            }
            if (x > 45 && z > 40 && (x + z) > 95) {
                map.set(x, z, battlegrid::TerrainType::Mountain);
            }

            double cx = static_cast<double>(x) - 32.0;
            double cz = static_cast<double>(z) - 25.0;
            if (cx * cx + cz * cz < 36.0) {
                map.set(x, z, battlegrid::TerrainType::Water);
            }
        }
    }

    // Keep the no-argument demo map comparable to maps/default.map: a
    // half-metre plateau approached from each side by physical ramp tiles.
    map.set(20, 19, battlegrid::TerrainType::SlopeSouth);
    map.set(19, 20, battlegrid::TerrainType::SlopeEast);
    map.set(20, 20, battlegrid::TerrainType::Hill);
    map.set(21, 20, battlegrid::TerrainType::SlopeWest);
    map.set(20, 21, battlegrid::TerrainType::SlopeNorth);

    return map;
}

void printUsage(const char* argv0)
{
    std::cout << "Usage: " << argv0 << " [--map path/to/map.map]\n";
}

} // namespace

int main(int argc, char** argv)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::string mapPath;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--map") == 0 && i + 1 < argc) {
            mapPath = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            std::cerr << "Unknown argument: " << argv[i] << "\n";
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    battlegrid::BattleGridWorld world;
    if (!mapPath.empty()) {
        if (!world.loadMap(mapPath)) {
            return EXIT_FAILURE;
        }
    } else {
        std::cout << "No map specified (use --map <path>). Using built-in default map.\n";
        world.loadMap(makeDefaultMap());
    }

    battlegrid::InputMap inputMap;
    bindDefaultInputs(inputMap);
    world.populate(inputMap);

    auto display = std::make_unique<battlegrid::GLDisplay>(
        world.terrainMap(), world.mapWorld(), 1280, 720);

    std::thread engineThread = world.engine().run();
    auto lastFrame = std::chrono::steady_clock::now();
    size_t lastLogIndex = 0;
    battlegrid::SenseIndicatorManager senseIndicators;

    static std::atomic<bool> g_quit{false};
    std::signal(SIGINT, [](int) { g_quit.store(true); });

    bool running = true;
    bool showMap = false;
    while (running && !g_quit.load()) {
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - lastFrame).count();
        lastFrame = now;

        auto positions = world.engine().snapshotAgentPositions();

        while (auto evt = display->pollEvent()) {
            if (auto* ke = std::get_if<io::KeyEvent>(&*evt)) {
                if (ke->key == io::Key::Q && ke->action == io::Action::Press) {
                    running = false;
                    break;
                }
                if (ke->key == io::Key::Escape && ke->action == io::Action::Press) {
                    running = false;
                    break;
                }
                if (ke->key == io::Key::P && ke->action == io::Action::Press) {
                    world.engine().setState(
                        world.engine().getState() == State::RUNNING
                            ? State::PAUSED
                            : State::RUNNING);
                    continue;
                }
            }

            inputMap.processEvent(*evt);
        }

        if (inputMap.pressed(battlegrid::GameAction::ToggleMap)) {
            showMap = !showMap;
        }

        auto& playerCtrl = world.playerController();
        world.engine().withAgentsLock([&] {
            playerCtrl.update(dt, positions);

            world.stepCollisions(dt, positions);

            std::string cameraEntity = playerCtrl.inVehicle()
                ? playerCtrl.currentVehicle()->name()
                : std::string("Player");
            auto it = positions.find(cameraEntity);
            if (it != positions.end()) {
                playerCtrl.setSnapshotPosition(it->second);
            }
        });

        senseIndicators.update(static_cast<float>(dt));

        auto agents = world.engine().getAllAgents();
        display->renderFrame(
            agents,
            positions,
            world.engine().getState(),
            world.engine().getGameLog(),
            playerCtrl,
            senseIndicators,
            showMap);

        auto& log = world.engine().getGameLog();
        auto entries = log.getEntries();
        if (entries.size() > lastLogIndex) {
            COORD playerPos = positions.contains("Player")
                ? positions.at("Player")
                : COORD{0.0, 0.0, 0.0};
            double camYaw = playerCtrl.yaw();

            for (size_t i = lastLogIndex; i < entries.size(); ++i) {
                std::cout << entries[i].format() << "\n";

                bool isReaction = entries[i].message.rfind("Heard:", 0) == 0;
                auto it = positions.find(entries[i].source);
                if (it != positions.end() && !isReaction) {
                    senseIndicators.addIndicator(it->second, entries[i].sense);

                    double dx = it->second[0] - playerPos[0];
                    double dz = it->second[2] - playerPos[2];
                    double distSq = dx * dx + dz * dz;
                    if (distSq > 1.0 && entries[i].source != "Player") {
                        double worldAngle = std::atan2(dz, dx);
                        float bearing = static_cast<float>(worldAngle - camYaw);
                        senseIndicators.addHudPing(bearing, entries[i].sense);
                    }
                }
            }

            lastLogIndex = entries.size();
        }

        inputMap.endFrame();
    }

    world.engine().stop();
    engineThread.join();

    std::cout << "BattleGrid simulation ended.\n";
    return EXIT_SUCCESS;
}
