# Changelog

All notable changes to this public export will be documented in this file.

## [Unreleased]

### Added
- Initial AGPL public export of the core simulation framework
- `libsim`, `libevent`, `libio`, `libphysics`, `libnet`, and `libmap`
- `battlegrid` as the primary demo application
- Focused event/input/network demos under `apps/demos`
- Source-level architecture and decomposition documentation

### Changed
- Switched test dependency setup to CMake `FetchContent` for GoogleTest
- Kept the export framework-first while restoring `libnet` as an optional distributed-simulation layer
- Re-enabled `battlegrid` server/client/headless/compute modes on the distributed simulation branch
