# Grid Sim Framework documentation {#mainpage}

Grid Sim Framework is a modular C++20 simulation framework built from reusable
libraries and a small set of focused demo applications.

## Applications

- **battlegrid** — flagship 3D sandbox/demo application
- **demos** — focused examples for event and input subsystems

## Libraries

- **libevent** — event bus and event components
- **libsim** — simulation engine, agents, behaviours, spatial senses, logs
- **libio** — SDL, ncurses, and evdev input abstraction
- **libphysics** — collision bodies and fixed-step physics world
- **libmap** — map tiles, projections, ASCII/OSM import, world metadata

## Functional decomposition

The detailed source-level decomposition for every retained library lives in
`docs/architecture.md`.
