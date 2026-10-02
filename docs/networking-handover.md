# Networking Handover

## Current boundary

`libnet` is intentionally absent from this branch. Keep `libphysics` transport-agnostic: it owns
one local Box3D world and exposes value-based body registration, input, state reads, and collision
records. Do not put sockets, RPC types, service discovery, or remote ownership into `PhysicsWorld`.

BattleGrid's authority flow is:

1. Controllers submit intent in `BattleGridWorld::stepCollisions()`.
2. `PhysicsWorld::step()` advances the local fixed-step solver.
3. `BattleGridWorld` applies solved transforms and emits `CollisionEvent`s.

This all runs inside `BaseEngine::withAgentsLock()`. Render code consumes
`snapshotAgentPositions()` rather than reading/mutating live agents.

## Recommended first network model

Use **single-writer entity ownership**. Each entity has one simulation service responsible for its
physics body, health, mount state, and collision/gameplay events. Services exchange versioned,
tick-stamped value snapshots and input intents; consumers interpolate or predict remote entities.
Do not let two services independently solve the same contact.

The initial wire state should contain stable entity ID, owner ID, simulation tick, position, linear
velocity, yaw, and relevant gameplay state. Treat collision events as owner-authored outcomes.
Reconcile remote transforms in an application-level sync adapter, then update local agent/physics
state atomically under the engine lock.

## Box3D-specific notes

- Keep each land vehicle's chassis and four wheel bodies on the same owner. Wheels and suspension
  are local implementation detail; replicate the chassis state and controls, not independent wheel
  authority.
- BattleGrid transmits authoritative land-vehicle wheel centers as an optional presentation payload
  alongside its chassis snapshot. Clients render those transforms but never create distributed wheel
  authority or step remote suspension; the owning server remains the only Box3D solver for all five
  bodies.
- The solver uses a fixed timestep, but do not assume bit-for-bit determinism across platforms or
  Box3D versions. Prefer authoritative snapshots and correction over lockstep simulation until
  cross-platform determinism is demonstrated.
- BattleGrid's interim cross-service contact policy uses snapshot-driven kinematic proxies on the
  server for compute-owned actors. Server-owned players resolve against those proxies, so they
  cannot pass through a delegated actor; the compute node remains the only solver advancing the
  delegated actor. The server routes an idempotent `ContactDecision` to the compute owner when the
  proxy is contacted; it carries a unique contact ID, the current owner epoch, and the server tick.
  Compute nodes reject replayed or stale-epoch decisions before applying the impulse locally. The
  proxy still cannot receive gameplay damage; distributed damage requires a region/contact-owner
  handoff protocol.

## Integration points

- `libs/libphysics/include/libphysics/physics_world.hpp`: local physics API; preserve its
  Box3D-free public value types.
- `apps/battlegrid/src/battlegrid_world.cpp`: converts gameplay intent to solver state and solver
  state to agent/collision events; the right location for a future sync adapter.
- `libs/libsim/include/libsim/base_engine.hpp`: owns the agent registry/event bus and exposes
  `withAgentsLock()` plus position snapshots.

Build focused regressions with:

```bash
cmake --build build --target test_physics test_battlegrid --parallel 2
./build/bin/test_physics
./build/bin/test_battlegrid
```
