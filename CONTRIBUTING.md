# Contributing to Grid Sim Framework

Thank you for your interest in contributing to Grid Sim Framework. This
document provides guidelines and instructions for contributing to the project.

## Code of Conduct

- Be respectful and constructive in all interactions
- Welcome newcomers and help them get started
- Focus on what's best for the community and the project

## Getting Started

### Development Environment Setup

1. **Fork and clone the repository:**
```bash
git clone https://github.com/YOUR_USERNAME/grid-sim-framework.git
cd grid-sim-framework
```

2. **Install dependencies** (Ubuntu/Debian):
```bash
sudo apt-get install build-essential cmake g++ lcov doxygen libsdl2-dev libncurses-dev
```
> **Note:** [tinyxml2](https://github.com/leethomason/tinyxml2) (used by `libmap` for OSM XML parsing) is fetched automatically by CMake via `FetchContent` if not found on the system. No manual installation is needed.

3. **Build in debug mode:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

4. **Run tests:**
```bash
make test
```

## Development Workflow

### Branch Strategy

- `main` - Default public branch
- `feature/*` - Feature branches
- `bugfix/*` - Bug fix branches

### Creating a Feature Branch

```bash
git checkout -b feature/your-feature-name
```

### Making Changes

1. **Write clean, documented code:**
   - Follow the existing code style (see `.clang-format`)
   - Add Doxygen comments for public APIs
   - Keep functions focused and testable

2. **Add tests:**
   - Write unit tests for new functionality
   - Ensure existing tests still pass
   - Aim for >80% code coverage

3. **Format your code:**
```bash
clang-format -i path/to/your/file.cpp
```

4. **Run static analysis:**
```bash
clang-tidy path/to/your/file.cpp -- -I./libs/libsim/include
```

### Commit Messages

Write clear, descriptive commit messages:

```
Add feature: Short description (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain what and why, not how.

- Bullet points are fine
- Reference issues: Fixes #123
```

**Good examples:**
- `Fix memory leak in BaseEngine agent cleanup`
- `Add configurable FPS to engine constructor`
- `Refactor velocity behavior to use smart pointers`

**Avoid:**
- `Fix bug`
- `Update code`
- `WIP`

## Code Standards

### C++ Style Guidelines

- **Language version:** C++20
- **Naming conventions:**
  - Classes/Structs: `PascalCase`
  - Functions/Methods: `snake_case`
  - Member variables: `m_snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Template parameters: `UPPER_CASE`

- **Smart Pointers:**
  - Use `std::shared_ptr` for shared ownership
  - Use `std::unique_ptr` for exclusive ownership
  - Avoid raw pointers except for non-owning references

- **Include order:**
  1. Corresponding header
  2. Project headers (`libsim/`, `libio/`, `libevent/`, `libmap/`, etc.)
  3. System C headers (`<cmath>`, etc.)
  4. System C++ headers (`<vector>`, etc.)

### Documentation

- Add Doxygen comments to all public APIs:
```cpp
/**
 * @brief Brief description of the function.
 * 
 * More detailed description if needed.
 * 
 * @param param1 Description of param1
 * @param param2 Description of param2
 * @return Description of return value
 */
```

### Testing

- Write tests for all new features
- Test edge cases and error conditions
- Use descriptive test names: `TEST(ComponentTest, DescriptiveTestName)`
- Mock external dependencies when appropriate

### Coverage Requirements

- **libsim:**     80% line coverage, 80% function coverage
- **libevent:**   80% line coverage, 80% function coverage
- **libphysics:** 80% line coverage, 80% function coverage
- **libmap:**     80% line coverage, 80% function coverage
- **libio:**      50% line coverage, 50% function coverage (due to hardware dependencies)

Check coverage:
```bash
make coverage
# Open build/coverage_report/index.html
```

## Pull Request Process

1. **Before submitting:**
   - Ensure all tests pass: `make test`
   - Check code coverage: `make coverage`
   - Run static analysis (clang-tidy)
   - Format code (clang-format)
   - Update documentation if needed

2. **Create pull request:**
   - Target the default branch used by the public repo
   - Fill out the PR template completely
   - Link related issues
   - Add screenshots for UI changes

3. **PR requirements:**
   - All tests must pass
   - Coverage thresholds must be met
   - At least one approval from maintainers
   - No merge conflicts

4. **After approval:**
   - Squash commits if requested
   - Maintainer will merge when ready

## Project Structure

```
grid-sim-framework/
├── libs/               # Core libraries
│   ├── libsim/        # Simulation engine (BaseEngine, IAgent, IBehaviour)
│   │   ├── include/   # Public headers
│   │   ├── src/       # Implementation
│   │   └── tests/     # Unit tests
│   ├── libevent/      # Event system (TEvent, EventBus, ThreadEventComponent)
│   ├── libio/         # I/O abstraction (keyboard, mouse, gamepad)
│   ├── libphysics/    # Collision detection (PhysicsWorld, KinematicBody)
│   └── libmap/        # Map system (MapWorld, GeoProjection, AsciiFormat, OsmFormat)
├── apps/              # Applications
│   ├── battlegrid/    # 3D battle simulation (SDL2 + OpenGL)
│   │   └── tests/     # Integration tests
│   └── demos/         # Demo programs
├── cmake/             # CMake modules (coverage, etc.)
└── docs/              # Architecture and Doxygen documentation
```

## Adding New Features

### Adding a New Library

1. Create directory structure: `libs/newlib/{include/newlib/,src/,tests/}`
2. Add CMakeLists.txt (follow existing pattern)
3. Update `libs/CMakeLists.txt`
4. Add tests with appropriate coverage

### Adding a New Agent Behavior

1. Create header in `libs/libsim/include/libsim/`
2. Inherit from `IBehaviour`
3. Implement `execute(DeltaType delta)` method
4. Add tests in `libs/libsim/tests/`
5. Document in Doxygen format

### Adding a New Input Method

1. Add implementation to `libs/libio/src/`
2. Add header to `libs/libio/include/libio/`
3. Update `libs/libio/CMakeLists.txt` with find_package
4. Add conditional compilation support

## Reporting Issues

### Bug Reports

Include:
- OS and version
- Compiler version
- CMake version
- Steps to reproduce
- Expected vs actual behavior
- Relevant code snippets or logs

### Feature Requests

Include:
- Clear use case description
- Why existing features don't suffice
- Proposed API or interface
- Examples of usage

## Questions?

- Open a GitHub issue with the `question` label
- Check existing documentation in `docs/`
- Review code examples in `apps/demos/`

## License

By contributing, you agree that your contributions will be licensed under the
GNU Affero General Public License v3.0.

## Recognition

Contributors will be acknowledged in release notes and the project README.

Thank you for contributing to Grid Sim Framework.
