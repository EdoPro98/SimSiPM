# Codebase Structure

**Analysis Date:** 2026-04-01

## Directory Layout

```
/root/SimSiPM/
├── include/               # Public header files (interface layer)
│   ├── SiPM.h            # Main include, aggregates all headers
│   ├── SiPMTypes.h       # Custom types: pair, SiPMSmallVector, sipmAlloc
│   ├── SiPMAnalogSignal.h # Signal waveform class
│   ├── SiPMHit.h         # Detection event class
│   ├── SiPMDebugInfo.h   # MC-truth metadata
│   ├── SiPMProperties.h  # Sensor configuration
│   ├── SiPMRandom.h      # PRNG and distributions
│   └── SiPMSensor.h      # Main simulation orchestrator
├── src/                  # Implementation files (internal logic)
│   ├── SiPMAnalogSignal.cpp # Signal processing algorithms
│   ├── SiPMRandom.cpp     # PRNG implementation
│   ├── SiPMProperties.cpp # Configuration parsing
│   └── SiPMSensor.cpp     # Sensor simulation logic
├── python/               # Python bindings
│   ├── SiPMSensorPy.cpp    # Main sensor binding
│   ├── SiPMDebugInfoPy.cpp
│   ├── SiPMPropertiesPy.cpp
│   ├── SiPMHitPy.cpp
│   ├── SiPMAnalogSignalPy.cpp
│   └── SiPMRandomPy.cpp
├── tests/                # Unit tests
│   ├── sensor.cpp
│   ├── properties.cpp
│   ├── rand.cpp
│   └── xorshift.cpp
├── bench/                # Performance benchmarks
│   ├── sensor.cpp
│   └── random.cpp
├── CMakeLists.txt       # Build configuration
├── cmake-single-platform.yml # CI configuration
└── README.md           # Documentation
```

## Directory Purposes

**include/:**
- Purpose: Public interface headers for simulation API
- Contains: All `.h` files exposing C++ classes and functions
- Key files: `SiPM.h` (aggregation), `SiPMAnalogSignal.h`, `SiPMSensor.h`

**src/:**
- Purpose: Implementation files for declared interfaces
- Contains: All `.cpp` files implementing logic in headers
- Key files: `SiPMSensor.cpp` (main simulation), `SiPMRandom.cpp` (PRNG)

**python/:**
- Purpose: Python bindings for SiPM simulation classes
- Contains: SWIG/cython-style wrapper implementations
- Key files: `SiPMSensorPy.cpp`, `SiPMSensorPy.py` (generated module)

**tests/:**
- Purpose: Unit test suite for validation
- Contains: Google Test/Catch2 test cases
- Key files: `sensor.cpp`, `properties.cpp`, `rand.cpp`

**bench/:**
- Purpose: Performance benchmarking
- Contains: Microbenchmarks for profiling
- Key files: `sensor.cpp`, `random.cpp`

## Key File Locations

**Entry Points:**
- `src/SiPMSensor.cpp`: Main simulation entry via `runEvent()`
- `src/SiPMAnalogSignal.cpp`: Signal processing utilities
- `python/SiPMSensorPy.cpp`: Python module entry

**Configuration:**
- `src/SiPMProperties.cpp`: Property parsing and validation
- `include/SiPMTypes.h`: Custom types and memory allocation

**Core Logic:**
- `src/SiPMSensor.cpp`: Complete event simulation pipeline
- `include/SiPMProperties.h`: All sensor parameters as getters/setters

**Testing:**
- `tests/sensor.cpp`: Integration tests for complete simulation
- `tests/properties.cpp`: Property configuration tests

## Naming Conventions

**Files:**
- Pattern: `{Component}.h` / `{Component}.cpp`
- Example: `SiPMAnalogSignal.h` / `SiPMAnalogSignal.cpp`

**Directories:**
- Pattern: Function-based grouping (not dependency-based)
- Example: All headers in `include/`, implementations in `src/`

**Classes:**
- Pattern: PascalCase for class names (`SiPMSensor`, `SiPMHit`)
- Example: `SiPMAnalogSignal`, `SiPMDebugInfo`

**Functions:**
- Pattern: camelCase for functions
- Example: `signalShape()`, `addPhotoelectrons()`

**Variables:**
- Pattern: m_ prefix for member variables
- Example: `m_Waveform`, `m_Sampling`, `m_Hits`

**Constants:**
- Pattern: static constexpr with descriptive names
- Example: `static constexpr uint32_t N = 1 << 16;`

## Where to Add New Code

**New Feature:**
- Primary code: Add implementation to `src/` directory
- Tests: Add to `tests/` directory
- Follow: Header declaration in `include/` or create new header

**New Component/Module:**
- Implementation: Add `.cpp` to `src/` with corresponding `.h` in `include/`
- Example: `src/NewComponent.cpp` → `include/NewComponent.h`

**Utilities:**
- Shared helpers: Add to `include/SiPMTypes.h` (custom allocator, small vectors)
- Signal processing: Add to `src/SiPMAnalogSignal.cpp`
- Random distributions: Add to `src/SiPMRandom.cpp`

**New Distribution:**
- Location: Add to `include/SiPMRandom.h` (template) + `src/SiPMRandom.cpp` (implementation)
- Pattern: Follow existing `randGaussian()`, `randPoisson()` style

## Special Directories

**python/:**
- Purpose: Generated Python bindings
- Generated: Partially generated (needs build system)
- Committed: Yes (custom bindings)

**tests/:**
- Purpose: Unit test suite
- Generated: No
- Committed: Yes

**bench/:**
- Purpose: Performance benchmarks
- Generated: No
- Committed: Yes

---

*Structure analysis: 2026-04-01*
