# Grid Sim Framework

Grid Sim Framework is a **C++20 agent-based simulation framework** built around
reusable libraries for simulation, events, input, terrain-aware movement, and
map import/export. This export is the focused public framework cut of the larger
workspace, with `battlegrid` kept as the flagship demo application.

## Included in this export

### Core libraries

- `libsim` — simulation engine, agents, behaviours, spatial sensing, game log
- `libevent` — typed event bus and threaded/poll-based event components
- `libio` — normalized keyboard, mouse, gamepad, ncurses, SDL, and evdev input
- `libphysics` — collision events, fixed-step collision world, kinematic bodies
- `libmap` — terrain layers, geographic projection, ASCII maps, OSM import

### Demo applications

- `apps/battlegrid` — 3D sandbox/demo app for exercising the framework
- `apps/demos` — focused examples for event and input subsystems

## Not included in the initial public import

`libnet` is intentionally **parked outside this export** and will be brought in
later on its own branch. This keeps the initial public repo focused on the core
framework and standalone demos.

## Features

- **Modern C++20** with templates, smart pointers, and strong type usage
- **Functional decomposition** across reusable libraries
- **Event-driven architecture** for decoupled simulation behavior
- **Multiple I/O backends** via SDL2, ncurses, and Linux evdev
- **Terrain-aware movement and collisions**
- **Map pipeline** with ASCII maps and OpenStreetMap XML rasterization
- **Documented and unit-tested libraries**

## Requirements

- CMake 3.14+
- C++20-capable compiler
- SDL2 development headers
- ncurses development headers
- `lcov` for coverage reports
- `doxygen` for API docs

This export is developed and documented around Linux/Ubuntu-style setup. The
framework itself is modular, but some demos and input backends assume Linux or
desktop SDL/OpenGL availability.

## Guiding principles

The public export is shaped by a few architectural rules:

- **Framework-first design** — reusable libraries own the core logic; apps are
  thin composition layers rather than the center of the codebase
- **Clear subsystem boundaries** — simulation, events, input, physics, and maps
  are kept in separate libraries with narrow responsibilities
- **Testable decomposition** — code is split so behavior can be exercised with
  focused unit tests instead of only through large end-to-end apps
- **Backend abstraction** — platform-specific input and rendering concerns are
  isolated behind stable interfaces where practical
- **Battle-tested demos** — `battlegrid` and the smaller demos exist to prove
  library integration without becoming hidden framework dependencies
- **Progressive complexity** — the repo supports simple local simulations first,
  while leaving room for richer extensions like networking or larger worlds

## Build

### Dependencies (Ubuntu/Debian)

```bash
sudo apt-get install build-essential cmake g++ lcov doxygen libsdl2-dev libncurses-dev
```

`libmap` fetches `tinyxml2` automatically with CMake if it is not already
available on the system.

### Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

### Generate coverage

```bash
cmake --build build --target coverage --parallel 2
```

## Project layout

```text
grid-sim-framework/
├── libs/
│   ├── libevent/
│   ├── libio/
│   ├── libmap/
│   ├── libphysics/
│   └── libsim/
├── apps/
│   ├── battlegrid/
│   └── demos/
├── cmake/
└── docs/
```

## Documentation

- `docs/architecture.md` — architecture overview and source-level decomposition
- `docs/networking-handover.md` — ownership and synchronization guidance for future network work
- `docs/mainpage.md` — Doxygen landing page
- `CONTRIBUTING.md` — contributor setup and expectations
- `CHANGELOG.md` — staged public-export change history

## BattleGrid demo

Run the included demo app with the shipped ASCII map:

```bash
./build/bin/battlegrid --map apps/battlegrid/maps/default.map
```

Controls include:

- `W/A/S/D` move
- `Shift` sprint
- `Space` jump
- `E` mount/dismount
- `T` shout
- `Tab` toggle camera
- `M` toggle map overlay
- `P` pause
- `Q` or `Esc` quit

## License

This export is licensed under the **GNU Affero General Public License v3.0**.
See `LICENSE`.
