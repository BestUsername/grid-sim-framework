# Grid — Architecture

## Overview

Grid Sim Framework is a modular C++ simulation framework composed of reusable
libraries and an application layer built around the `battlegrid` sandbox and
the examples under `apps/demos`.

```
┌──────────────────────────────────────────────────────────────────┐
│                        apps/battlegrid                           │
│  BattleGridWorld · PlayerController · GLDisplay · MapOverlay     │
├───────────┬────────────┬──────────────┬────────────┬────────────┤
│ libsim    │ libevent   │ libphysics   │ libio      │ libmap     │
│ Engine    │ EventBus   │ KinematicBody│ Input abs. │ MapWorld   │
│ Agents    │ Event      │ Gravity      │ Kbd/Gamepad│ GeoProj.   │
│ Behaviours│ Pub/Sub    │ Slope checks │            │ OSM/ASCII  │
└───────────┴────────────┴──────────────┴────────────┴────────────┘
                              libnet (optional — TCP networking)
```

### Libraries

| Library      | Purpose |
|--------------|---------|
| **libevent** | Generic pub/sub event bus with clone-based delivery, typed events (`TEvent<T>`), and optional threaded components. |
| **libsim**   | Simulation engine (`BaseEngine`), entity/agent hierarchy (`IEntity → IAgent → BaseAgent`), behaviours, spatial sense events, game log. |
| **libphysics** | Gravity, jump, slope/wall checks (`KinematicBody`). Designed to grow into a full physics layer (collision resolution, flight dynamics). |
| **libio**    | Input abstraction — keyboard (console, ncurses, SDL, evdev), gamepad, configurable action mapping (`InputMap`). |
| **libnet**   | TCP client/server (`TcpClient`, `TcpServer`), `NetworkBridge` for distributing simulation state; binary serialisation helpers. |
| **libmap**   | Rich geographic map data: `MapLayer<TerrainTile>` grid, `MapWorld` with optional WGS-84 `GeoOrigin`, equirectangular `GeoProjection`, `AsciiMapFormat` (legacy `.map` files), `OsmFormat` (OpenStreetMap XML rasteriser). |

### Applications

| Application | Purpose |
|-------------|---------|
| **battlegrid** | Main 3D sandbox app wiring rendering, input, terrain, physics, and simulation into a playable environment. |
| **demos** | Small standalone examples used to exercise specific libraries and backend integrations with minimal app scaffolding. |

## Source-level functional decomposition

The tables below break each library down by the responsibilities carried by its
public headers, implementation files, and current tests. Together they provide
the code-level map of where each reusable subsystem lives.

### libevent

| Functional area | Primary files | Responsibility |
|-----------------|---------------|----------------|
| Core event model | `libs/libevent/include/libevent/event.hpp` | Defines the base `Event`, typed `TEvent<T>`, and clone-based polymorphic delivery contract used everywhere else. |
| Bus orchestration | `libs/libevent/include/libevent/event_bus.hpp`, `libs/libevent/src/event_bus.cpp` | Owns subscriber lists, isolates event domains per bus instance, and fans out cloned events to subscribed components. |
| Poll-based components | `libs/libevent/include/libevent/event_component.hpp`, `libs/libevent/src/event_component.cpp` | Provides the queue, subscribe/unsubscribe API, and synchronous `PollEvents()` path for simulation-loop-driven consumers. |
| Thread-owned components | `libs/libevent/include/libevent/thread_event_component.hpp`, `libs/libevent/src/thread_event_component.cpp` | Extends `EventComponent` with a worker thread for asynchronous event draining and clean start/stop semantics. |
| Regression coverage | `libs/libevent/tests/test_thread_event_component.cpp` | Exercises bus subscription, typed event delivery, clone semantics, and threaded delivery/teardown behavior. |

### libsim

| Functional area | Primary files | Responsibility |
|-----------------|---------------|----------------|
| Shared simulation types | `libs/libsim/include/libsim/types.hpp`, `libs/libsim/src/types.cpp` | Defines core aliases, sensory enums, vector/range helpers, and formatting utilities shared across the engine and agents. |
| Entity and agent contracts | `libs/libsim/include/libsim/i_entity.hpp`, `libs/libsim/include/libsim/i_agent.hpp`, `libs/libsim/include/libsim/i_environment.hpp` | Establishes the interfaces between agents and the environment: naming, location, updates, event handling, and environment queries. |
| Base agent implementation | `libs/libsim/include/libsim/base_agent.hpp` | Implements lifecycle defaults, behaviour ownership, communication helpers, and the common mutable state carried by concrete agents. |
| Behaviour system | `libs/libsim/include/libsim/i_behaviour.hpp`, `libs/libsim/include/libsim/velocity_behaviour.hpp` | Encapsulates pluggable agent logic, including the built-in velocity-driven movement behaviour. |
| Engine runtime | `libs/libsim/include/libsim/base_engine.hpp`, `libs/libsim/src/base_engine.cpp` | Owns the simulation loop, agent registry, event proxying, sense-event fan-out, snapshots, and thread-safe mutation points. |
| Spatial sensing | `libs/libsim/include/libsim/sense_event.hpp`, `libs/libsim/include/libsim/shapes.hpp` | Models spatially-bounded perception events and the reusable geometric primitives used to evaluate containment and attenuation. |
| Logging and diagnostics | `libs/libsim/include/libsim/game_log.hpp` | Records communicated events in memory and optionally on disk so simulations can inspect or stream player-visible history. |
| Regression coverage | `libs/libsim/tests/test_base_agent.cpp`, `libs/libsim/tests/test_engine.cpp`, `libs/libsim/tests/test_sense_event.cpp`, `libs/libsim/tests/test_game_log.cpp`, `libs/libsim/tests/test_types.cpp`, `libs/libsim/tests/test_range.cpp`, `libs/libsim/tests/test_shapes.cpp`, `libs/libsim/tests/test_velocity_behaviour.cpp`, `libs/libsim/tests/test_template_instantiations.cpp` | Covers the engine loop, behaviour execution, snapshots, communication, sense propagation, logging, math helpers, geometry primitives, and explicit template instantiations used by the shipped engine variants. |

### libio

| Functional area | Primary files | Responsibility |
|-----------------|---------------|----------------|
| Device-agnostic event model | `libs/libio/include/libio/input_event.hpp`, `libs/libio/include/libio/keycodes.hpp`, `libs/libio/include/libio/input_device.hpp` | Defines the normalized input event variant, shared key/button enums, modifier flags, and the polling interface implemented by every backend. |
| SDL keyboard and key translation | `libs/libio/include/libio/sdl_keyboard.hpp`, `libs/libio/include/libio/sdl_keymap.hpp`, `libs/libio/src/sdl_keyboard.cpp`, `libs/libio/src/sdl_keymap.cpp` | Converts SDL keyboard events and modifier state into the unified `KeyEvent` representation. |
| SDL mouse and gamepad devices | `libs/libio/include/libio/sdl_mouse.hpp`, `libs/libio/include/libio/sdl_gamepad.hpp`, `libs/libio/src/sdl_mouse.cpp`, `libs/libio/src/sdl_gamepad.cpp` | Normalizes motion, button, scroll, and axis input into the shared event model. |
| NCurses keyboard backend | `libs/libio/include/libio/ncurses_keyboard.hpp`, `libs/libio/include/libio/ncurses_keymap.hpp`, `libs/libio/src/ncurses_keyboard.cpp`, `libs/libio/src/ncurses_keymap.cpp` | Adapts terminal `getch()` codes into normalized keyboard events for text-mode applications, with the poll wrapper split from the translation helper for direct unit coverage. |
| Linux evdev backend | `libs/libio/include/libio/evdev_keyboard.hpp`, `libs/libio/include/libio/evdev_keymap.hpp`, `libs/libio/src/evdev_keyboard.cpp`, `libs/libio/src/evdev_keymap.cpp` | Maps Linux input subsystem events into the same cross-platform key representation. |
| Regression coverage | `libs/libio/tests/test_io_basic.cpp`, `libs/libio/tests/test_sdl_devices.cpp`, `libs/libio/tests/test_evdev_keymap.cpp`, `libs/libio/tests/test_ncurses_keymap.cpp` | Covers the shared event model, modifier helpers, SDL device adapters, evdev key translation, and the ncurses key/poll translation paths. |

### libphysics

| Functional area | Primary files | Responsibility |
|-----------------|---------------|----------------|
| Collision data model | `libs/libphysics/include/libphysics/collision_body.hpp`, `libs/libphysics/include/libphysics/collision_event.hpp` | Defines collision bodies, contact payloads, and the event object used to surface resolved contacts to higher layers. |
| Kinematic actor movement | `libs/libphysics/include/libphysics/kinematic_body.hpp`, `libs/libphysics/src/kinematic_body.cpp` | Implements gravity, grounded checks, jumping, and terrain step/slope rules for character-like actors. |
| Fixed-step collision world | `libs/libphysics/include/libphysics/physics_world.hpp`, `libs/libphysics/src/physics_world.cpp` | Owns body registration, broad/narrow phase overlap checks, fixed-timestep stepping, and impulse calculation. |
| Regression coverage | `libs/libphysics/tests/test_kinematic_body.cpp`, `libs/libphysics/tests/test_physics_world.cpp` | Covers terrain-aware kinematics, collision detection, impulse behavior, fixed-step accumulation, and collision-event cloning. |

### libnet

| Functional area | Primary files | Responsibility |
|-----------------|---------------|----------------|
| Wire protocol | `libs/libnet/include/libnet/message.hpp` | Defines the length-prefixed `Message` envelope and the message-type taxonomy shared by clients, servers, and bridges. |
| Domain serialisation | `libs/libnet/include/libnet/serializer.hpp` | Packs and unpacks agent snapshots, input state, control messages, and assignment payloads onto the wire protocol. |
| TCP transport | `libs/libnet/include/libnet/tcp_client.hpp`, `libs/libnet/include/libnet/tcp_server.hpp`, `libs/libnet/src/tcp_client.cpp`, `libs/libnet/src/tcp_server.cpp` | Implements asynchronous Boost.Asio client/server transport, session tracking, and queued message I/O with runtime behavior moved out of public headers and into compiled sources. |
| Simulation bridge | `libs/libnet/include/libnet/network_bridge.hpp`, `libs/libnet/src/network_bridge.cpp` | Wraps the raw transport in a server/client façade tailored to Grid's state-sync and input-forwarding flows while preserving a narrower public API surface. |
| Regression coverage | `libs/libnet/tests/message_test.cpp`, `libs/libnet/tests/serializer_test.cpp`, `libs/libnet/tests/transport_test.cpp` | Covers the message envelope, serialisation helpers, and transport/bridge lifecycle behavior that sit underneath distributed state sync. |

### libmap

| Functional area | Primary files | Responsibility |
|-----------------|---------------|----------------|
| Tile and layer primitives | `libs/libmap/include/libmap/map_tile.hpp`, `libs/libmap/include/libmap/map_layer.hpp` | Defines terrain surface semantics and the generic row-major 2D layer container used to store map cells. |
| Geographic transforms | `libs/libmap/include/libmap/geo_projection.hpp` | Converts between WGS-84 latitude/longitude and local grid coordinates anchored by a `GeoOrigin`. |
| World aggregate | `libs/libmap/include/libmap/map_world.hpp`, `libs/libmap/src/map_world.cpp` | Aggregates terrain layers, metadata, cell size, and optional geographic origin into a single map object. |
| Format abstraction | `libs/libmap/include/libmap/map_format.hpp` | Defines the reader/writer contracts shared by concrete import/export implementations. |
| ASCII map import/export | `libs/libmap/include/libmap/ascii_format.hpp`, `libs/libmap/src/ascii_format.cpp` | Reads and writes the legacy tile-based `.map` representation. |
| OSM ingest | `libs/libmap/include/libmap/osm_format.hpp`, `libs/libmap/src/osm_format.cpp` | Parses OpenStreetMap XML, projects it into a tile grid, and rasterises tagged features into terrain layers. |
| Regression coverage | `libs/libmap/tests/test_map_layer.cpp`, `libs/libmap/tests/test_map_world.cpp`, `libs/libmap/tests/test_geo_projection.cpp`, `libs/libmap/tests/test_ascii_format.cpp`, `libs/libmap/tests/test_osm_format.cpp` | Covers the layer container, world metadata, projection rules, ASCII format round-trips, and OSM ingestion. |

### Threading model

- **Engine thread** — owns the simulation loop (`BaseEngine::run()`), ticks at
  target FPS. Each tick: poll events → update all agents. Protected by
  `_agents_mutex` (recursive).
- **Render thread** — reads agent positions via `snapshotAgentPositions()`,
  writes player input via `withAgentsLock()`.
- **Event threads** (optional) — `ThreadEventComponent` owns a dedicated thread
  for event processing; base `EventComponent` is poll-based.

### Event system

```
Event (base — holds EventKey string, virtual clone())
  ├── TEvent<T>        — typed container for arbitrary payload
  └── SenseEvent       — spatially-attenuated, carries source/origin/intensity/range
       └── AudioEvent  — radial sound emission, linear falloff
```

Events flow through `EventBus` → `EventComponent::PollEvents()` →
`ProcessEvent()`. The engine's private `EventProxy` dispatches to all agents via
`IAgent::on_event()`. Sense events bypass the bus and are delivered directly with
per-receiver attenuation.

---

## Map System

**Status:** Implemented — April 2026

### Overview

`libmap` is a standalone library (no dependencies on other Grid libs) that
provides a rich geographic map representation and import/export pipeline.

### Data model

```
MapWorld
  ├── MapLayer<TerrainTile>   — 2D grid, row-major (z * width + x)
  │     └── TerrainTile       — {SurfaceType, elevation, OSM tags}
  ├── GeoOrigin (optional)    — {maxLat, minLon, metersPerCell}
  ├── cellSize (double)       — real-world metres per grid cell
  └── name (string)
```

`MapLayer<T>` is a generic template; `TerrainTile` is the default tile type.
Additional layers (elevation rasters, road networks) can be added as new
`MapLayer` members of `MapWorld` without breaking existing code.

### Geographic projection

`GeoProjection` implements an equirectangular (plate carrée) projection:

```
x = (lon − origin.lon) · cos(origin.lat) · R / cellSize    [east]
z = (origin.lat − lat) · R / cellSize                      [south]
```

`R = 6,371,000 m` (mean Earth radius).  Accuracy is ±0.3 % for areas up to
~200 km across.  For larger extents a UTM or Web-Mercator projection should
replace this.  The `GeoOrigin` is always set to the **NW corner** of the
bounding box (maxLat, minLon) so all projected cell coordinates are ≥ 0.

### Format support

| Format | Class | Read | Write |
|--------|-------|------|-------|
| ASCII `.map` | `AsciiMapFormat` | ✓ | ✓ |
| OpenStreetMap XML | `OsmFormat` | ✓ | — |

`OsmFormat` rasterises OSM vector data into the tile grid:

1. Parse all `<node>` elements → id → {lat, lon} map.
2. Parse all `<way>` elements → collect refs + tags; skip ways with no
   recognised surface-type tag.
3. Derive grid dimensions from the node bounding box and `cellSize`.
4. For each way: determine `SurfaceType` from tags; rasterise as a filled
   polygon (closed area ways) or a Bresenham line (roads, rivers).
5. Priority rule prevents lower-priority surfaces overwriting higher ones:
   Building > Road > Water > Forest > Swamp > Sand > Mountain > Land.

tinyxml2 (fetched automatically via CMake `FetchContent`) handles XML parsing.

### Integration in battlegrid

`BattleGridWorld` maintains a `MapWorld` alongside the legacy `TerrainMap`:

- `TerrainMap` is unchanged — used by `PhysicsWorld` for collision.
- `MapWorld` is populated by `terrainMapToMapWorld()` from the same ASCII file
  and exposed via `BattleGridWorld::mapWorld()`.
- `GLDisplay` owns a `MapOverlay` that renders a fullscreen 2D HUD when the
  player presses **M** (`GameAction::ToggleMap`).
- `MapOverlay` uses the existing single-shader pipeline (MVP + uColor) — no
  new shaders required.  Terrain is batched by surface type (one draw call per
  type); agent dots are rendered per-frame using the shared unit-quad VAO.

---

## Collision Architecture (ADR-001)

**Status:** Implemented — March 2026

### Context

Entities previously used single-point terrain sampling for movement collision. A
volume-based footprint (`maxHeightInRadius`) was added to prevent entities from
clipping into walls. The next step is full entity-vs-entity collision with
physics outcomes — required for vehicle/pedestrian impacts, projectile hits, and
future distributed simulation.

### Design constraints

1. **Distributed simulation** — the architecture must work across a federation
   of simulators (DIS/HLA-style). Each federate is authoritative for its own
   entities. There is no guaranteed central server.

2. **Dual simulation modes** — the engine must support two overload strategies:
   - **Real-time mode:** wall-clock time is king. When the simulation is too
     heavy, fidelity degrades (collision shape LOD: concave mesh → convex hull →
     AABB → bounding sphere; tick rate reduction for distant entities; reduced
     event fan-out).
   - **Accuracy mode:** simulation correctness is king. When the simulation is
     too heavy, the simulation clock slows relative to wall time so every
     collision is fully resolved at maximum fidelity.

### Decision: Hybrid impulse + entity response (Option C)

We evaluated three alternatives:

| Approach | Summary |
|----------|---------|
| **A — Engine-authoritative** | Central collision server computes outcomes and broadcasts results. |
| **B — Entity-authoritative** | Each entity (or its owning federate) independently resolves its own collision outcome from raw contact data. |
| **C — Hybrid** | The detecting side computes a deterministic physics impulse from agreed-upon rules and published entity state. Each entity independently decides its gameplay response to that impulse. |

**Option C was chosen** for the following reasons:

- **Consistency without a bottleneck.** The physics impulse is deterministic —
  any federate computing it from the same inputs gets the same result. No
  central server is needed, and no round-trip latency is added.

- **Separation of concerns.** The physics layer owns impulse computation (mass,
  velocity, contact normal, restitution). The entity/gameplay layer owns
  response (damage, knockback magnitude, animation, sound). New entity types
  only need to implement their response handler.

- **Clean LOD degradation.** In real-time mode, only the collision shape used
  for impulse calculation is degraded (e.g. swap concave mesh for AABB). The
  entity response layer is unchanged — it receives the same `CollisionEvent`
  regardless of the detection fidelity. In accuracy mode, the full-fidelity
  shape is used and the simulation clock stretches.

- **Distributed simulation fit.** In an HLA federation, the impulse formula and
  collision shape LOD table are part of the Federation Object Model (FOM). Each
  federate detects overlaps between its own entities and dead-reckoned remote
  entities, computes the impulse locally, applies it to its own entity, and
  publishes the collision interaction. The remote federate receiving the
  interaction can verify/apply the same impulse. This maps directly to HLA
  time-stamp-ordered interactions and is compatible with DIS Collision PDUs.

- **Existing architecture fit.** `CollisionEvent` extends `Event` just as
  `SenseEvent` does. Entities handle it via the existing `on_event()` virtual.
  The physics impulse calculation lives in `libphysics` alongside
  `KinematicBody`.

### Consequences

- `libphysics` gains collision detection and impulse resolution.
- A `CollisionEvent` type is added to the event hierarchy.
- Entities must define mass and a collision shape (with LOD variants).
- The engine tick must include a collision detection + impulse dispatch phase.
- A simulation-mode flag must control whether the engine degrades fidelity or
  slows the clock.

---

## Collision Implementation Routes (ADR-002)

**Status:** Implemented (Route 2) — March 2026

Three implementation routes toward the hybrid collision architecture are
described below. All three produce the same external contract (`CollisionEvent`
delivered to entities via `on_event()`); they differ in *where* collision
detection and impulse calculation run and how the work integrates with the
existing `BaseEngine` tick.

### Route 1 — Inline in engine tick

Collision detection and impulse resolution run synchronously inside
`BaseEngine::run_step()`, after all agents have updated and before the next
frame begins. The engine iterates all entity pairs (with broad-phase culling),
detects overlaps, computes impulses, and dispatches `CollisionEvent`s.

```
run_step(dt):
    PollEvents()
    for each agent: agent.update(dt)    ← existing
    detectAndResolve(dt)                ← NEW: broad-phase → narrow-phase → impulse → dispatch
```

### Route 2 — Physics world in libphysics

A new `PhysicsWorld` object in `libphysics` owns all collision bodies and runs
its own fixed-timestep sub-steps, decoupled from the engine tick rate. The
engine calls `physicsWorld.step(dt)` once per tick; `PhysicsWorld` accumulates
time and runs as many fixed sub-steps as needed. After each sub-step, collision
events are queued. The engine drains the queue and dispatches events to agents.

```
run_step(dt):
    PollEvents()
    for each agent: agent.update(dt)
    physicsWorld.step(dt)               ← may run 0–N fixed sub-steps internally
    for each queued collision: dispatch CollisionEvent
```

### Route 3 — Collision as a behaviour

Collision detection is implemented as an `IBehaviour` attached to each entity.
Each entity's `CollisionBehaviour::execute(dt)` queries a shared spatial index
(owned by `BattleGridWorld` or the engine) for nearby entities, runs narrow-phase
checks, computes impulses, and delivers events. No changes to `BaseEngine`.

```
run_step(dt):
    PollEvents()
    for each agent: agent.update(dt)    ← CollisionBehaviour runs here
```

### Comparison

| | **Route 1: Inline in engine tick** | **Route 2: PhysicsWorld in libphysics** | **Route 3: Collision as a behaviour** |
|---|---|---|---|
| **Determinism** | High — single pass after all movement, consistent ordering | Highest — fixed timestep guarantees frame-rate-independent results | Low — execution order depends on agent iteration order; same pair may be detected twice (once per entity) |
| **Accuracy-mode support** | Moderate — must slow the whole engine tick to increase collision fidelity | Excellent — `PhysicsWorld` sub-steps independently of render rate; accuracy mode just increases sub-step count or uses smaller time slices | Poor — no sub-stepping; tied 1:1 to the engine tick |
| **Real-time-mode LOD** | Easy — engine selects collision shape LOD before the detection pass | Easy — `PhysicsWorld` owns shape LOD table, selects per-frame based on budget | Hard — each behaviour would need to independently query and agree on LOD; risk of mismatched decisions |
| **Distributed simulation** | Moderate — detection is centralised on each federate but tightly coupled to the engine; porting to a different federation architecture means moving engine code | Good — `PhysicsWorld` is a standalone object that can be instantiated per-federate with its own entity registry; clean boundary for HLA/DIS integration | Poor — collision logic is scattered across entity behaviours; hard to extract or replicate across federates |
| **Broad-phase efficiency** | Good — single pass builds spatial index once, checks all pairs | Best — `PhysicsWorld` owns the spatial index (grid, BVH, etc.) persistently across frames; incremental updates | Worst — each behaviour must query the index separately; redundant overlap checks |
| **Separation from gameplay** | Moderate — collision code lives in `BaseEngine` (a library class), which must become aware of collision shapes and masses | Best — collision is fully encapsulated in `libphysics`; `BaseEngine` only calls `step()` and drains a queue | Good for entity code, bad for collision code — spread across individual behaviours rather than one cohesive system |
| **Engine changes required** | Moderate — add a detection+dispatch phase to `run_step()`, add entity collision shape registration | Small — add one `physicsWorld.step(dt)` call and event drain loop to `run_step()` | None — behaviours are added per-entity; only `BattleGridWorld::populate()` changes |
| **Duplicate detection avoidance** | Built in — outer loop `i < j` ensures each pair checked once | Built in — `PhysicsWorld` owns pair management | Must be handled manually — need a shared "already processed" set or half-pair convention |
| **Testability** | Requires instantiating `BaseEngine` or extracting collision into a helper | `PhysicsWorld` is independently testable with no engine dependency | Each behaviour is testable in isolation, but integration testing is harder |
| **Complexity** | Low — straightforward addition to existing loop | Moderate — new class, fixed-timestep accumulator, event queue, body registration API | Low initial, high ongoing — simple per-entity, but managing consistency across all behaviours is error-prone |
| **Future networking fit** | Moderate — collision logic locked inside the engine template | Best — `PhysicsWorld` can be instantiated on a dedicated physics federate or replicated across federates with identical inputs | Poor — no single authority; each federate's behaviours run independently with no consistency guarantee |

### Recommendation

**Route 2 (PhysicsWorld in libphysics)** is the strongest choice. It provides
the cleanest separation of concerns, the best support for both simulation modes
(fixed sub-stepping for accuracy, shape LOD for real-time), and the most natural
boundary for distributed simulation. The `PhysicsWorld` object maps directly to
a physics federate in an HLA federation, and its deterministic fixed-timestep
design ensures frame-rate-independent collision results.

Route 1 is a reasonable simpler alternative if the fixed-timestep sub-stepping
is not needed. Route 3 is not recommended — while it avoids engine changes, the
scattered collision logic undermines consistency, distributed simulation
support, and LOD management.

---

## Implementation Notes

Route 2 was implemented with the following components:

### New files in libphysics

| File | Purpose |
|------|--------|
| `collision_body.hpp` | `CollisionBody` struct (name, position, prevPosition, mass, radius) and `Collision` result struct (names, contact normal, relative velocity, impulse, penetration). |
| `collision_event.hpp` | `CollisionEvent` extending `Event` — delivered to each collision participant with impulse data, masses, normal, and the other entity's name. |
| `physics_world.hpp/cpp` | `PhysicsWorld` — body registry, fixed-timestep accumulator, broad-phase O(n²) sphere sweep, narrow-phase sphere-sphere, elastic impulse resolution. Supports real-time mode (capped sub-steps) and accuracy mode (unlimited sub-steps). |
| `tests/test_physics_world.cpp` | 14 unit tests covering body registry, overlap detection, separating-body rejection, contact normals, impulse magnitude, restitution, penetration depth, deterministic ordering, fixed-timestep accumulator, and CollisionEvent clone/data. |

### Integration in battlegrid

- `BattleGridWorld` owns a `PhysicsWorld` member.
- `populate()` registers a `CollisionBody` for every soldier, civilian, and
  vehicle with appropriate mass and radius.
- `stepCollisions(dt, positions)` replaces the old `checkCollisions()`:
  1. Syncs entity positions into `PhysicsWorld`.
  2. Calls `physicsWorld.step(dt)` — runs fixed sub-steps, returns `Collision`
     records.
  3. For each collision, creates two `CollisionEvent`s (one per participant,
     normal flipped) and delivers via `agent->on_event()`.

### Entity collision responses

- **Soldier/Civilian**: knockback proportional to `impulse / mass` along the
  contact normal; logs a Touch sense event.
- **Vehicle**: same formula but high mass means minimal displacement; logs an
  impact event.

### Physics constants

| Entity | Mass (kg) | Collision radius (m) |
|--------|-----------|---------------------|
| Soldier | 80 | 0.4 |
| Civilian | 70 | 0.4 |
| Vehicle | 2000 | 1.0 |
