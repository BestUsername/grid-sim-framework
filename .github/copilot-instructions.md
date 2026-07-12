# Grid Sim Framework Copilot Instructions

## Project context

- The original, hand-crafted core is a templated C++ simulation library designed
  around configurable coordinate dimensions and numeric data types. AI-assisted
  development accelerated the demo applications and broadened the framework's
  usage; describe contributions accurately and do not imply that all project
  code has the same authorship or origin.
- Distributed simulation/networking is being developed on a separate branch.
  `libnet` is not part of this public export, so do not introduce networking
  dependencies or assume its APIs exist on this branch.
- A separate upcoming branch will evaluate replacing the current rudimentary
  physics system with Box2D and Box3D. Until that work is explicitly in scope,
  preserve the `libphysics` public contracts and the BattleGrid collision
  integration.
- Box3D's sample application requires a newer OpenGL capability than the
  development laptop supports. Do not adopt its Sokol/OpenGL sample renderer;
  translate any useful physics/sample logic into BattleGrid's existing
  SDL2/OpenGL rendering system.

## Build, test, and analysis

- This is a C++20 CMake project. Configure a Debug build (also enables coverage instrumentation), then build:
  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build build --parallel 2
  ```
- Run the complete suite:
  ```bash
  ctest --test-dir build --output-on-failure
  ```
- Build a focused test executable, then run one GoogleTest case:
  ```bash
  cmake --build build --target test_sim
  ./build/libs/libsim/tests/test_sim --gtest_filter=EngineTest.TestRunStep
  ```
  Other test targets are `test_libevent`, `test_libio_basic`, `test_libio_sdl`,
  `test_libio_evdev`, `test_libio_ncurses`, `test_map`, `test_physics`, and
  `test_battlegrid`. Backend-specific `libio` targets are present only when
  SDL2, ncurses, or Linux evdev support is detected at configure time.
- Build coverage (Debug only; enforces 80% line/function coverage for `libsim`,
  `libevent`, `libphysics`, and `libmap`, and 50% for `libio`):
  ```bash
  cmake --build build --target coverage --parallel 2
  ```
- Format with the repository configuration:
  ```bash
  clang-format -i path/to/file.cpp
  ```
- Run clang-tidy against a source file with the relevant library include path:
  ```bash
  clang-tidy path/to/file.cpp -- -I./libs/libsim/include
  ```

## Architecture

- The framework is library-first. `libs/` defines reusable targets with public
  headers under `include/<library>/`; applications in `apps/` compose them.
  CMake target names do not use the directory names: `event`, `sim`, `io`,
  `physics`, and `map`.
- `libevent` provides explicit, per-instance `EventBus` pub/sub domains.
  `EventBus` clones each event for every subscriber; event subclasses must
  override `clone()` to preserve their derived payload. Prefer constructing
  `EventComponent` with an explicit bus rather than using its unattached form.
  Poll-based components process queues through `PollEvents()`; use
  `ThreadEventComponent` only when the component must own a worker thread.
- `libsim` is the core agent runtime. `BaseEngine` owns its event bus and an
  agent registry; each tick polls queued events, forwards them to
  `IAgent::on_event()`, then calls `IAgent::update()`. Agents derive from
  `BaseAgent` and own `IBehaviour` instances through `unique_ptr`. Hearing is
  delivered as a spatially attenuated `SenseEvent`; other senses currently log
  directly to `GameLog`.
- `BaseEngine::run()` executes on a separate engine thread. Render/client code
  must read positions with `snapshotAgentPositions()` and wrap agent-state
  mutations in `withAgentsLock()` rather than retaining or changing state
  concurrently.
- `libphysics` is separate from the engine and depends only on `libevent`.
  `PhysicsWorld` owns registered `CollisionBody` values, advances via fixed
  substeps, and returns collision records. The application syncs positions,
  calls `step(dt)`, applies gameplay separation, then sends a distinct
  `CollisionEvent` to each participant with the normal and relative velocity
  reversed for the second entity. `maxSubSteps == 0` selects accuracy mode;
  positive values cap real-time work.
- `libmap` is standalone. `MapWorld` owns a row-major `MapLayer<TerrainTile>`
  indexed as `z * width + x`: X is east and Z is south. Parse/import through
  `IMapReader` implementations (`AsciiMapFormat`, `OsmFormat`) and return
  failure as `std::optional<MapWorld>`; OSM parsing depends on tinyxml2, which
  CMake fetches when not installed.
- `apps/battlegrid` is the integration example: it owns the engine, legacy
  `TerrainMap`, rich `MapWorld`, and `PhysicsWorld`. Keep its two map forms in
  sync: `TerrainMap` remains the collision/movement source, while `MapWorld`
  supports display and import/export. It starts the engine thread, processes
  SDL input and rendering on the main thread, and performs player updates plus
  collision stepping inside `withAgentsLock()`.

## Repository conventions

- Namespaces are subsystem-specific: simulation and maps use
  `grid::libsim`/`grid::libmap`, physics uses `grid::physics`, events use
  `grid::libevent`, and input uses `io`.
- Follow the configured naming rules: lower-case namespaces/functions/variables,
  `CamelCase` classes and structs, `UPPER_CASE` template parameters and
  constants, and `m_` private/protected members. Use 4 spaces, left-aligned
  pointers, a 120-character limit, and let clang-format sort/regroup includes.
- Public APIs use Doxygen comments. Keep public interfaces in the library's
  `include/` directory and wire implementation and tests through that library's
  `CMakeLists.txt`.
- Use `shared_ptr` for agents registered with `BaseEngine` and `unique_ptr` for
  exclusively owned behaviours/controllers. Raw pointers in these APIs are
  non-owning.
- Library test sources are globbed into their target, so adding a `.cpp` under
  an existing `tests/` directory normally needs no test-source-list change.
  `libmap` fixtures are exposed to tests through `LIBMAP_TEST_FIXTURE_DIR`; do
  not rely on the test process working directory.
