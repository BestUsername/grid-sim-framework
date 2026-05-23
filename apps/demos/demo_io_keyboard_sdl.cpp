#include "libio/sdl_keyboard.hpp"
#include "libio/sdl_mouse.hpp"
#include "libio/sdl_gamepad.hpp"
#include "libio/input_event.hpp"

#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <stdlib.h> //EXIT_SUCCESS

enum GameState {PLAY, PAUSE, EXIT};
class Game {
public:
    Game() {}
    ~Game() {}
    void run() {
        init();
        gameLoop();
        uninit();
    }

private:
    void init() {
        SDL_Init(SDL_INIT_EVERYTHING);
        _window = SDL_CreateWindow("The Grid", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _win_width, _win_height, SDL_WINDOW_OPENGL);
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            _joysticks.push_back(SDL_JoystickOpen(i));
        }
    }
    void gameLoop() {
        while (_gamestate != GameState::EXIT) {
            processInput();
            drainDevices();
        }
    }
    void uninit() {
        for (auto joystick : _joysticks) {
            SDL_JoystickClose(joystick);
        }
        _joysticks.clear();
        SDL_DestroyWindow(_window);
        _window = nullptr;
        SDL_Quit();
    }

    void processInput() {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch (event.type) {
                // Application events
                case SDL_QUIT:
                    std::cout << "Quitting..." << std::endl;
                    _gamestate = GameState::EXIT;
                    break;
                // Keyboard events
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    _keyboard.feedSDLEvent(
                        event.key.keysym.scancode,
                        event.key.keysym.mod,
                        event.type == SDL_KEYDOWN,
                        event.key.repeat != 0);
                    break;
                // Mouse events
                case SDL_MOUSEMOTION:
                    _mouse.feedMotion(event.motion.x, event.motion.y,
                                      event.motion.xrel, event.motion.yrel);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    _mouse.feedButton(event.button.button,
                                       event.type == SDL_MOUSEBUTTONDOWN,
                                       event.button.x, event.button.y);
                    break;
                case SDL_MOUSEWHEEL:
                    _mouse.feedScroll(event.wheel.x, event.wheel.y);
                    break;
                // Game controller events
                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP:
                    _gamepad.feedButton(event.cbutton.which,
                                         event.cbutton.button,
                                         event.type == SDL_CONTROLLERBUTTONDOWN);
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    _gamepad.feedAxis(event.caxis.which,
                                       event.caxis.axis,
                                       event.caxis.value);
                    break;
                default:
                    break;
            }
        }
    }

    /// Drain all queued events from each input device and print them.
    void drainDevices() {
        auto printEvent = [](const io::InputEvent& ev) {
            std::visit([](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, io::KeyEvent>) {
                    const char* action = arg.action == io::Action::Press   ? "Press"
                                       : arg.action == io::Action::Release ? "Release"
                                       :                                      "Repeat";
                    std::cout << "Key " << action
                              << " [code " << static_cast<int>(arg.key) << "]" << std::endl;
                } else if constexpr (std::is_same_v<T, io::MouseMoveEvent>) {
                    std::cout << "Mouse Move (" << arg.x << "," << arg.y
                              << ") delta(" << arg.dx << "," << arg.dy << ")" << std::endl;
                } else if constexpr (std::is_same_v<T, io::MouseButtonEvent>) {
                    const char* action = arg.action == io::Action::Press ? "Press" : "Release";
                    std::cout << "Mouse Button " << action
                              << " [btn " << static_cast<int>(arg.button) << "]" << std::endl;
                } else if constexpr (std::is_same_v<T, io::MouseScrollEvent>) {
                    std::cout << "Mouse Scroll (" << arg.scrollX << "," << arg.scrollY << ")" << std::endl;
                } else if constexpr (std::is_same_v<T, io::GamepadButtonEvent>) {
                    const char* action = arg.action == io::Action::Press ? "Press" : "Release";
                    std::cout << "Gamepad " << arg.gamepadIndex << " Button " << action
                              << " [btn " << static_cast<int>(arg.button) << "]" << std::endl;
                } else if constexpr (std::is_same_v<T, io::GamepadAxisEvent>) {
                    std::cout << "Gamepad " << arg.gamepadIndex << " Axis "
                              << static_cast<int>(arg.axis) << " = " << arg.value << std::endl;
                }
            }, ev);
        };

        while (auto ev = _keyboard.poll()) printEvent(*ev);
        while (auto ev = _mouse.poll())    printEvent(*ev);
        while (auto ev = _gamepad.poll())  printEvent(*ev);
    }

    GameState _gamestate{GameState::PLAY};

    SDL_Window* _window = nullptr;
    std::vector<SDL_Joystick*> _joysticks;
    int _win_width {1024};
    int _win_height {768};

    io::SDLKeyboard _keyboard;
    io::SDLMouse    _mouse;
    io::SDLGamepad  _gamepad;
};

int main (int argc, char ** argv) {
    Game game;
    game.run();
    return EXIT_SUCCESS;
}