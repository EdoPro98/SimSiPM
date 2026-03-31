# Codebase Concerns

**Analysis Date:** 2026-04-01

## Threading/Concurrency

**Current State:** No multi-threading, mutexes, or concurrent constructs found in codebase.

**Observations:**
- Single-threaded simulation model throughout
- No `std::thread`, `std::mutex`, `std::atomic`, or other concurrency primitives
- Random number generator (`SiPMRng::Xorshift256plus`) has internal state that would be corrupted in multi-threaded use
- No thread-safe random number generator implementation
- No parallel random number generation for batch simulations

**Risk:** Using the library in a multi-threaded application without proper RNG state isolation per thread would lead to:
- Corrupted random number sequences
- Non-deterministic simulation results
- Potential data races in the RNG state

**Recommendations:**
- Implement thread-local RNG state in `SiPMRandom` class
- Provide a thread-safe random number generation interface
- Consider `std::jthread` or similar for future parallel workloads

## Error Handling

**Current State:** No exception handling patterns found in codebase.

**Observations:**
- All functions return default values (e.g., `-1.0`, `-1`) on error or below-threshold conditions
- No `throw` statements, `try`/`catch` blocks, or C++ exceptions used
- File I/O errors handled via `std::cerr` messages and returning default-initialized objects
- Example file read error (lines 129-131 in `src/SiPMDebugInfo.cpp`):
  ```cpp
  } else {
    std::cerr << "Could not open " << fname << " for reading!" << std::endl;
  }
  return retval;
  ```
- Integer underflow/overflow not checked (e.g., bit shifts in `SiPMTypes.h`)
- Vector bounds checks rely on standard library (compiler optimization friendly)
- Memory allocation via custom `sipmAlloc`/`sipmFree` with no error checking

**Risk:** Silent failures where errors return default values may lead to:
- Misinterpretation of results (e.g., peak = `-1` treated as valid value)
- Missing file read failures in production
- Memory allocation failures going unnoticed

**Recommendations:**
- Add assertions for debug builds to catch invalid states
- Return status codes or use `std::optional` for critical operations
- Document all default return values clearly in API documentation

## Memory Management

**Current State:** Custom memory management with potential issues.

**Observations:**
- Custom allocators `sipmAlloc`/`sipmFree` in `SiPMTypes.h` (lines 50-61)
- `sipmAlloc` uses `aligned_alloc` for AVX-512 alignment (64 bytes) or `malloc` otherwise
- `SiPMSmallVector` class manages memory internally with heap fallback (lines 63-154)
- Manual `sipmFree` calls throughout `SiPMRandom.cpp` (lines 333, 386, 442)
- No RAII wrappers around `sipmAlloc` objects
- `SiPMSmallVector` destructor only frees `m_HeapStorage` (lines 93-97)

**Risk:**
- Memory leaks if `sipmFree` is not called explicitly (though it appears to be in all current paths)
- `sipmAlloc` alignment requirement may fail on non-aligned memory (e.g., heap fragmentation)
- No detection of memory allocation failures from `aligned_alloc`

**Recommendations:**
- Wrap `sipmAlloc`/`sipmFree` in RAII smart pointer equivalents
- Add allocation failure checks in debug builds
- Consider replacing custom allocators with `std::aligned_allocator` when available

## Platform-Specific Code

**Current State:** Multiple platform-specific code paths found.

### ARM/AArch64 Concerns

**Commit History:**
- Commit `b08cc27`: "[RNG] Using different way to seed PRNG making it compatible for ARM"

**ARM-Specific Code:**
- In `src/SiPMRandom.cpp` (lines 370-380), Apple-specific sincos usage:
  ```cpp
  #ifdef __APPLE__
  __sincos(TWO_PI * u[i + 1], ptr, ptr + 1);
  #else
  sincos(TWO_PI * u[i + 1], ptr, ptr + 1);
  #endif
  ```

**AVX-512 Code:**
- Conditional compilation for AVX-512 in `src/SiPMRandom.h` (lines 19-21, 78-81) and `src/SiPMRandom.cpp` (lines 25-35)
- AVX-512 path uses `_mm512_*` intrinsics when `__AVX512F__` defined
- Non-AVX512 path uses scalar loops

**Platform-Specific Issues:**
- `aligned_alloc` only available on POSIX systems, not Windows
- Apple's `__sincos` vs standard library `sincos` differences
- AVX-512 not available on all x86_64 systems

**Recommendations:**
- Add ARM64 detection with `__aarch64__` or similar
- Consider using `std::sincos` with appropriate precision flags
- Document platform requirements in README

### Compiler Flags

**CMakeLists.txt (lines 2-3):**
```cmake
set(CMAKE_CXX_FLAGS_INIT -ffast-math)
```

**Python bindings (CMakeLists.txt lines 83):**
```cmake
target_compile_options(SiPM PRIVATE -fvisibility=hidden -ffast-math -O3)
```

**Issue:** `-ffast-math` may break strict IEEE-754 compliance, affecting:
- Floating-point reproducibility across platforms
- NaN/Inf handling in statistical calculations
- Results may differ between compilers (GCC vs Clang vs MSVC)

**Recommendations:**
- Document that results may not be bit-exact across platforms
- Provide a `-DENABLE_FAST_MATH=OFF` option for reproducibility mode

## Performance Considerations

**Current State:** Aggressive optimization with potential side effects.

**Observations:**
- All random number generation uses custom `Xorshift256plus` implementation
- Vectorized generation via AVX-512 when available
- Custom `SiPMAnalogSignal` integral/peak/tot/toa/top methods avoid unnecessary allocations
- `SiPMSmallVector` uses stack allocation before falling back to heap
- Signal processing in `src/SiPMAnalogSignal.cpp` (lines 16-115) operates on `std::vector<float>` directly

**Benchmark Files:**
- `bench/sensor.cpp` - sensor simulation benchmarks
- `bench/random.cpp` - random number generation benchmarks
- `tests/xorshift.cpp` - RNG quality tests

**Performance Risks:**
- Cache-line aligned structures (`alignas(64)`) may not be necessary for all workloads
- `-ffast-math` may cause floating-point inconsistencies
- Custom allocators may be slower than `malloc` due to lack of pool optimization
- No NUMA awareness for multi-socket systems

**Recommendations:**
- Add performance profiling markers (e.g., `std::chrono::high_resolution_clock`)
- Consider cache-line padding in hot paths
- Profile on target hardware before committing optimizations

## Signal Processing Specific Concerns

**Current State:** SiPM signal processing with numerical precision considerations.

**Observations:**

**Signal Shape (from `src/SiPMSensor.cpp`):**
- Uses exponential decay model with configurable slow component
- `SiPMAnalogSignal::toa` (lines 81-93) uses linear interpolation for time-over-threshold
- `SiPMAnalogSignal::top` (lines 102-115) finds peak within integration gate
- Signal amplitude scaled such that 1 photoelectron = 1.0

**Numerical Issues:**
- Division by `m_Waveform[i] - m_Waveform[i - 1]` in `toa` could cause NaN if consecutive samples are equal
- Threshold comparisons assume signal monotonicity within integration window
- No validation that integration gates are within signal bounds
- `peak()` returns `-1` for signals below threshold, but this could conflict with valid signal amplitudes

**Memory Patterns:**
- Signal processing iterates over `m_Waveform` with simple sequential access
- No stride access or cache-unfriendly patterns detected
- Copy-on-write for filtered signals (`lowPassFilter` creates new signal objects)

**Recommendations:**
- Add bounds checking in signal feature extraction methods
- Consider using `std::span` instead of `std::vector` for zero-copy operations
- Document valid signal amplitude ranges

## Domain-Specific Concerns

**SiPM Simulation Model:**

**Physical Effects Modeled:**
- Dark counts (DCR) - configurable rate in Hz
- Optical crosstalk (XT) - probability of secondary avalanches
- Delayed optical crosstalk (DXT) - delayed XT with tau
- Afterpulsing (AP) - fast and slow components with probabilities
- Cell recovery time - configurable dead time
- Cell-to-cell gain variation (CCGV)
- Signal shape parameters (rise/fall times, slow component fraction)

**Domain Risks:**
- All stochastic processes use same RNG, potentially introducing correlations
- DCR events may correlate with crosstalk due to shared RNG state
- Afterpulsing probability assumes independence between cells
- No validation that physical parameters are in valid ranges
- Example: CCGV > 1.0 would indicate unphysical gain variation

**Configuration Concerns:**
- `readSettings` (lines 107-132 in `src/SiPMDebugInfo.h`) parses simple key=value format
- File read errors don't validate parameter ranges
- No schema validation for configuration files

**Recommendations:**
- Add parameter validation after configuration load
- Document physical constraints on SiPM parameters
- Consider domain-specific exceptions for invalid configurations

## Missing Critical Features

**Not Implemented:**
- Event-based simulation for Geant4 integration beyond basic photon time lists
- Persistent storage for signal waveforms (e.g., HDF5, ROOT)
- Multi-sensor array simulation
- Time-correlated single photon counting (TCSPC) functionality
- FPGA-compatible output formats

**Blocks:**
- Integration into larger detector frameworks without custom wrappers
- Real-time signal processing applications
- Standard data analysis tools expecting specific formats

## Test Coverage Gaps

**Current Test Files:**
- `tests/sensor.cpp` - sensor simulation tests
- `tests/xorshift.cpp` - RNG quality tests
- `tests/properties.cpp` - parameter tests
- `tests/rand.cpp` - random number tests

**Gaps:**
- No tests for multi-threaded usage
- No tests for edge cases (e.g., zero-length signals, empty photon lists)
- No tests for platform-specific code paths (ARM, AVX-512)
- No integration tests with Geant4 or other frameworks
- No performance regression tests

**Risk:** Changes to core algorithms may introduce subtle bugs that go undetected.

---

*Concerns audit: 2026-04-01*
