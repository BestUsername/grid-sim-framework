# Grid Sim Framework

Grid Sim Framework is a **C++20 agent-based simulation framework** built around
reusable libraries for simulation, events, input, terrain-aware movement,
networking, and map import/export, with `battlegrid` kept as the flagship demo
application.

## Included in this export

### Core libraries

- `libsim` — simulation engine, agents, behaviours, spatial sensing, game log
- `libevent` — typed event bus and threaded/poll-based event components
- `libio` — normalized keyboard, mouse, gamepad, ncurses, SDL, and evdev input
- `libphysics` — collision events, fixed-step collision world, kinematic bodies
- `libnet` — TCP transport, state snapshots, player input forwarding, network bridge
- `libmap` — terrain layers, geographic projection, ASCII maps, OSM import

### Demo applications

- `apps/battlegrid` — 3D sandbox/demo app for exercising the framework
- `apps/demos` — focused examples for event and input subsystems

## Features

- **Modern C++20** with templates, smart pointers, and strong type usage
- **Functional decomposition** across reusable libraries
- **Event-driven architecture** for decoupled simulation behavior
- **Multiple I/O backends** via SDL2, ncurses, and Linux evdev
- **Terrain-aware movement and collisions**
- **Distributed simulation support** through optional TCP networking and state sync
- **Map pipeline** with ASCII maps and OpenStreetMap XML rasterization
- **Documented and unit-tested libraries**

## Requirements

- CMake 3.14+
- C++20-capable compiler
- SDL2 development headers
- Boost.System development headers
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
  while leaving room for richer extensions like distributed networking or larger worlds

## Build

### Dependencies (Ubuntu/Debian)

```bash
sudo apt-get install build-essential cmake g++ lcov doxygen libsdl2-dev libncurses-dev libboost-system-dev
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
│   ├── libnet/
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

Distributed modes:

```bash
./build/bin/battlegrid --server 4000 --map apps/battlegrid/maps/default.map
./build/bin/battlegrid --client 127.0.0.1:4000
./build/bin/battlegrid --headless 4000 --map apps/battlegrid/maps/default.map
./build/bin/battlegrid --compute 127.0.0.1:4000
./build/bin/demo_net_bridge
```

### Ability and distributed-physics testing

`apps/battlegrid/maps/all-abilities.map` has water, land, bumps, every ramp
direction, a hill, and mountains for manual testing of movement, jumping,
mounting/dismounting, driving, and terrain collision:

```bash
./build/bin/battlegrid --map apps/battlegrid/maps/all-abilities.map
```

Run the automated Box3D and networking test suite plus a headless
server/compute-node handoff with:

```bash
./scripts/e2e-network-physics.sh
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
