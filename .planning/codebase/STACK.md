# Technology Stack

**Analysis Date:** 2026-04-01

## Languages

**Primary:**
- **C++17** - Main implementation language for the C++ simulation library
  - Standard library only (no third-party C++ libraries)
  - Uses STL containers: `std::vector`, `std::map`, `std::unordered_map`, `std::array`
  - Uses STL algorithms: `std::sort`, `std::max_element`, `std::transform`
  - Uses STL utilities: `std::move`, `std::forward`, `std::swap`, `std::pair`

**Secondary:**
- **Python** - Used for package build system and testing configuration
  - `pyproject.toml` with `setuptools` as build backend
  - `pybind11` as C++/Python binding header
  - `pytest` for testing framework

## Runtime

**Environment:**
- **System C++ Library** - Standard C library (glibc on Linux, libc on macOS)
  - Uses `malloc`/`free`/`realloc` with custom `sipmAlloc`/`sipmFree` wrappers for alignment
  - Supports `aligned_alloc` on AVX-512 capable systems

**Package Manager:**
- **setuptools** (Python) - Build backend for Python packaging
- **CMake** - Build system for C++ project (via `cmake-single-platform.yml`)

## Frameworks

**Core:**
- **None** - Pure C++ library with no external frameworks
- Uses only C++ standard library components

**Testing:**
- **pytest** - Python testing framework (configured in `pyproject.toml`)

**Build/Dev:**
- **CMake** - Cross-platform build system
  - Configured via `cmake-single-platform.yml` CI workflow
  - Single-configuration generator (make/ninja)
- **pybind11** - Header-only C++/Python bindings library
- **setuptools** - Python package management

## Key Dependencies

**Critical:**
- **STL (C++ Standard Library)**
  - `<vector>`, `<map>`, `<unordered_map>`, `<array>`, `<cstdint>`, `<cmath>`, `<random>`, `<algorithm>`
  - Provides all core functionality with no external dependencies

- **pybind11>=2.10.0**
  - Provides C++/Python interop layer
  - Header-only, no build-time dependencies beyond STL
  - Used to expose C++ classes to Python runtime

- **setuptools>=42**
  - Python packaging and build system
  - Handles `pyproject.toml` and `pybind11` build integration

**Infrastructure:**
- **CMake**
  - Build system configured in `.github/workflows/cmake-single-platform.yml`
  - Supports testing via `-DSIPM_ENABLE_TEST=ON` flag
  - Uses single-configuration generators

- **pytest**
  - Configured in `pyproject.toml` via `[tool.pytest.ini_options]`
  - Uses `-ra` flag for regression mode
  - Test paths configured in `tests/` directory

## Configuration

**Environment:**
- No `.env` files detected
- Uses system-defined random device (`std::random_device`) for seeding
- Compiles with C++17 standard features (deduced return types, `if constexpr`, etc.)

**Build:**
- **CMake** configuration files:
  - `.github/workflows/cmake-single-platform.yml`
  - Configured build: `cmake -B build -DCMAKE_BUILD_TYPE=Release -DSIPM_ENABLE_TEST=ON`
  - Supports both single-platform and multi-platform workflows
- **pyproject.toml**:
  - Build backend: `setuptools.build_meta`
  - Test configuration via `pytest.ini_options`

## Platform Requirements

**Development:**
- C++17 compliant compiler
  - GCC 7+ (based on RNG optimizations commit history)
  - Clang 5+
  - MSVC 2017+
- Build tools: `cmake`, `make`/`ninja`
- Python 3.8+ for Python bindings
- `pybind11` development headers

**Production:**
- Any platform with:
  - C++17 compiler
  - System C++ runtime library
  - Standard C library (libc)
- Deployment targets:
  - Linux (glibc-based distributions)
  - macOS (macOS SDK)
  - Windows (MSVC/MinGW)
- Optional: AVX-512 support for optimized RNG performance

## Compiler Optimizations

**Detected SIMD Support:**
- **AVX-512** (`__AVX512F__`) - Conditional compilation for AVX-512 capable CPUs
  - Used in `SiPMRandom.cpp` for parallel RNG state management
  - Enables batch processing of random number generation
- **AVX2** - Implicitly supported via standard `<immintrin.h>` usage

**Compile-Time Checks:**
- `#ifdef __APPLE__` - Platform-specific sincos implementations
- `#ifdef __AVX512F__` - SIMD-optimized RNG variants

---

*Stack analysis: 2026-04-01*
