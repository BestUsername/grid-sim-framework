# Changelog

All notable changes to this public export will be documented in this file.

## [Unreleased]

### Added
- Initial AGPL public export of the core simulation framework
- `libsim`, `libevent`, `libio`, `libphysics`, and `libmap`
- `battlegrid` as the primary demo application
- Focused event/input demos under `apps/demos`
- Source-level architecture and decomposition documentation

### Changed
- Switched test dependency setup to CMake `FetchContent` for GoogleTest
- Trimmed the export to a framework-first shape for public release
- Simplified `battlegrid` in this export to a standalone demo app

### Deferred
- `libnet` is intentionally excluded from the initial import and staged
  separately for a later branch-based addition
