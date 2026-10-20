# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SimSiPM is a C++17 library for simulating Silicon Photomultiplier (SiPM) sensors. It has zero runtime external dependencies and is designed for integration into physics frameworks like Geant4 or DD4HEP.

## Build & Test

- CMake cache flags: `-DSIPM_ENABLE_TEST=ON` (tests), `-DSIPM_ENABLE_BENCH=ON` (benchmarks, requires Google Benchmark), `-DSIPM_BUILD_PYTHON=ON` (bindings, requires pybind11).
- Test executables are built in `build/tests/` (e.g. `build/tests/TestSiPMSensor`).
- CMake enforces `-ffast-math` globally. AVX512 SIMD in the RNG is optional.

## Conventions

- Test fixtures follow the `sut` (System Under Test) naming pattern.
- `SiPMRandom` is marked `mutable` in `SiPMSensor` for const-correct event simulation.

### No Exceptions

The codebase avoids C++ exceptions for performance. Error conditions use sentinel return values (typically `-1`).
