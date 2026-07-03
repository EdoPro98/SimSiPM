# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SimSiPM is a C++17 library for simulating Silicon Photomultiplier (SiPM) sensors. It has zero runtime external dependencies and is designed for integration into physics frameworks like Geant4 or DD4HEP.

## Build Commands

```bash
# Standard build
cmake -B build -S . && make -C build

# Build with tests
cmake -B build -S . -DSIPM_ENABLE_TEST=ON && make -C build

# Build with Python bindings (requires pybind11)
cmake -B build -S . -DCOMPILE_PYTHON_BINDINGS=ON && make -C build

# Build with benchmarks (requires Google Benchmark)
cmake -B build -S . -DSIPM_ENABLE_BENCH=ON && make -C build

# Install
make -C build install
```

CMake enforces `-ffast-math` globally. AVX512 SIMD in the RNG is optional.

## Test Commands

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test suite
ctest --test-dir build -R TestSiPMSensor -VV

# Run test executable directly
build/bin/TestSiPMSensor
```

Test suites: `TestSiPMSensor`, `TestSiPMProperties`, `TestSiPMRandom`, `TestSiPMRng`. Uses Google Test (auto-fetched if not installed). Test fixtures follow the `sut` (System Under Test) naming pattern.

## Architecture

### Event Simulation Data Flow

```
addPhoton(s) → runEvent() → signal() / debug()
```

`runEvent()` proceeds in order: electronic noise baseline → DCR (dark counts) → photoelectron hits (gated by PDE) → correlated noise chains (XT/AP) → cell recovery amplitude calculation → waveform generation.

### Core Classes

- **`SiPMSensor`** (`src/SiPMSensor.cpp`) — Main orchestrator. Owns a `SiPMProperties` and `SiPMRandom`. Call `addPhoton()` then `runEvent()` per event.
- **`SiPMProperties`** (`src/SiPMProperties.cpp`) — All sensor parameters (geometry, timing constants, noise rates). Supports `PdeType` enum (`kNoPde`, `kSimplePde`, `kSpectrumPde`) and `HitDistribution` enum (`kUniform`, `kCircle`, `kGaussian`). Can be loaded from JSON.
- **`SiPMAnalogSignal`** (`src/SiPMAnalogSignal.cpp`) — Wraps `std::vector<float>` for the output waveform. Feature extraction: `integral()`, `peak()`, `toa()`, `tot()`, `top()`.
- **`SiPMRandom`** (`src/SiPMRandom.cpp`) — Xorshift256+ (xoshiro256+) PRNG with optional AVX512 path. Provides uniform, Gaussian, exponential, Poisson, binomial distributions. Marked `mutable` in `SiPMSensor` for const-correct event simulation.
- **`SiPMHit`** (`include/SiPMHit.h`) — Represents a detection event with time, amplitude, cell index, and type (photoelectron/DCR/XT/AP). Supports parent-child chaining for correlated noise.
- **`SiPMTypes`** (`include/SiPMTypes.h`) — Custom performance types: `SiPMSmallVector<T,N>` (small-buffer-optimized vector), custom `pair<T,U>`, and `sipmAlloc()`/`sipmFree()` with optional 64-byte alignment.
- **`SiPMDebugInfo`** (`include/SiPMDebugInfo.h`) — MC-truth counters accessible via `sensor.debug()`.

### Python Bindings

Six `*Py.cpp` files in `python/` expose the C++ API via pybind11. Examples in `examples/` show typical usage patterns.

### No Exceptions

The codebase avoids C++ exceptions for performance. Error conditions use sentinel return values (typically `-1`).
