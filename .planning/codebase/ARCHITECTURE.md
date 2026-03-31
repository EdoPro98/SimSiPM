# Architecture

**Analysis Date:** 2026-04-01

## Pattern Overview

**Overall:** Domain-Specific Simulation Engine with Layered Responsibility Pattern

**Key Characteristics:**
- Single main simulation class (`SiPMSensor`) orchestrates the complete event simulation
- Header/Implementation separation with all logic in `include/` headers and `src/` implementations
- Heavy reliance on value semantics and const-correctness for thread safety
- Custom utility types (`SiPMTypes.h`) provide performance-critical infrastructure

## Layers

**Core Domain Layer:**
- Purpose: Contains all SiPM-specific simulation logic
- Location: `include/`, `src/`
- Contains: `SiPMAnalogSignal`, `SiPMHit`, `SiPMDebugInfo`, `SiPMProperties`, `SiPMRandom`, `SiPMSensor`
- Depends on: Standard C++ library (`<vector>`, `<cmath>`, `<map>`, `<random>`)
- Used by: Simulation consumers, Python bindings (`python/`)

**Signal Processing Layer:**
- Purpose: Generate and process analog waveforms from discrete hits
- Location: `src/SiPMAnalogSignal.cpp`
- Contains: `integral()`, `peak()`, `tot()`, `toa()`, `top()`, `lowPassFilter()`
- Depends on: `SiPMAnalogSignal` class, `m_SignalShape` from sensor
- Used by: `SiPMSensor::generateSignal()`

**Noise Generation Layer:**
- Purpose: Generate stochastic effects (DCR, XT, AP)
- Location: `src/SiPMSensor.cpp`
- Contains: `addDcrEvents()`, `addCorrelatedNoise()`, `generateXtHit()`, `generateApHit()`
- Depends on: `SiPMRandom`, `SiPMProperties`, `SiPMHit`
- Used by: `SiPMSensor::runEvent()`

**Configuration Layer:**
- Purpose: Store and validate SiPM sensor parameters
- Location: `include/SiPMProperties.h`, `src/SiPMProperties.cpp`
- Contains: All sensor parameters (PDE, gain, timing constants, noise rates)
- Depends on: Standard library containers
- Used by: All other layers for parameter lookup

**Random Number Generation Layer:**
- Purpose: Provide high-performance PRNG for simulation stochasticity
- Location: `include/SiPMRandom.h`, `src/SiPMRandom.cpp`
- Contains: `Xorshift256plus` (xoshiro256+ 1.0), distribution wrappers
- Depends on: AVX512 intrinsics (optional), custom allocation (`sipmAlloc`)
- Used by: All random number generation calls throughout simulation

## Data Flow

**Event Simulation Flow:**

1. **Initialization** (`SiPMSensor::SiPMSensor(const SiPMProperties&)`):
   - Create sensor with properties
   - Build signal shape waveform via `signalShape()`

2. **Photon Addition** (`SiPMSensor::addPhoton()` / `addPhotons()`):
   - Store photon arrival times and optional wavelengths
   - Build photon list in `m_PhotonTimes`, `m_PhonWavelengths`

3. **Event Execution** (`SiPMSensor::runEvent()`):
   ```
   1. Generate electronic noise baseline
   2. Add DCR events (if enabled)
   3. Add photoelectron hits (from photon list, considering PDE)
   4. Add correlated noise (XT, AP)
   5. Calculate signal amplitudes (recovery dynamics)
   6. Generate final signal waveform
   ```

4. **Signal Generation** (`SiPMSensor::generateSignal()`):
   - Convert hit times to sample indices
   - Accumulate scaled signal shape at each hit location
   - Output: `SiPMAnalogSignal` with complete waveform

**Signal Processing Flow:**

1. `SiPMAnalogSignal` stores sampled waveform in `m_Waveform`
2. Feature extraction functions work on `m_Sampling`-indexed data:
   - `integral()`: Sum over window, threshold-gated
   - `peak()`: Max amplitude in window, threshold-gated
   - `tot()`: Time-over-threshold duration
   - `toa()`: Time-of-arrival (linear interpolation)
   - `top()`: Time-of-peak
3. `lowPassFilter()` applies RC-like filtering with frequency response

## Key Abstractions

**SiPMHit:**
- Purpose: Represents a single detection event (photoelectron, DCR, XT, AP)
- File: `include/SiPMHit.h`
- Key fields: `m_Time`, `m_Amplitude`, `m_Row`, `m_Col`, `m_HitType`, `m_ParentPtr`
- Parent pointer enables tracking XT/AP chains

**SiPMAnalogSignal:**
- Purpose: Container for sampled analog waveform
- File: `include/SiPMAnalogSignal.h`
- Key fields: `m_Waveform` (std::vector<float>), `m_Sampling` (ns)
- Provides accessor operators: `[]` for indexing, `data()` for pointer access

**SiPMProperties:**
- Purpose: Configuration container for all SiPM parameters
- File: `include/SiPMProperties.h`
- Key abstractions: `PdeType`, `HitDistribution` enums for parameterization
- Provides read-only getters for all parameters

**SiPMDebugInfo:**
- Purpose: MC-Truth metadata for generated events
- File: `include/SiPMDebugInfo.h`
- Key fields: Photon count, PE count, DCR count, XT count, DXT count, AP count

## Entry Points

**Main Simulation Entry:**
- Location: `src/SiPMSensor.cpp`
- Triggers: User calls `SiPMSensor::runEvent()` after configuring via `addPhoton()`
- Responsibilities: Complete event simulation from photon input to signal output

**Python Binding Entry:**
- Location: `python/SiPMSensorPy.cpp`
- Triggers: Python module import
- Responsibilities: Wrap C++ sensor for Python interoperability

## Error Handling

**Strategy:** Value-based return with sentinel values
- **No exceptions used** - simulation prioritizes performance
- Negative return values indicate failure: `-1` for `integral()`, `peak()`, `toa()`, `top()` when signal below threshold
- **`-1ULL`** for modular arithmetic in RNG (LCG implementation)
- Division-by-zero guards in PDE evaluation, hit positioning

**Patterns:**
- Threshold-gated functions: Check threshold, return `-1` if not exceeded
- Range checks: `m_PhotonTimes[i] < 0 || m_PhotonTimes[i] > sigLen` → `continue`
- PDE clamping: `(newy < 0) ? 0 : newy`

## Cross-Cutting Concerns

**Memory Management:**
- **Smart pointers avoided** for performance-critical paths
- `SiPMHit*` stored in `std::vector<SiPMHit*> m_Hits` with manual `new`/`delete`
- Cleanup in `SiPMSensor::resetState()`
- Custom allocator (`sipmAlloc`/`sipmFree`) for RNG buffers

**Thread Safety:**
- **No concurrent access guarantees** - `m_rng` returned by value
- All state accessed through const or local copies
- `mutable SiPMRandom m_rng` in sensor allows per-event state

**Validation:**
- Hit position bounds: `isInSensor()` check before hit creation
- PDE wavelength clamping: Boundary handling in `evaluatePde()`
- Photon time range validation: Early continue if out of signal window

**Optimization Patterns:**
- **Value semantics** - `SiPMDebugInfo` passed by value, `const&` where large
- **Avoid heap** - `SiPMSmallVector` with small-buffer optimization
- **SIMD** - AVX512 intrinsics for RNG (conditional on `__AVX512F__`)
- **Template specialization** - `Rand<float>`, `Rand<double>` for type safety
- **Move semantics** - `std::move(wav)` in `SiPMAnalogSignal` constructor

## Module Dependencies

```
SiPMSensor (main orchestrator)
├── SiPMProperties (configuration)
├── SiPMRandom (PRNG)
├── SiPMAnalogSignal (waveform storage)
├── SiPMHit (event representation)
├── SiPMDebugInfo (MC-truth)
└── SiPMTypes (utility types, allocator)
```

---

*Architecture analysis: 2026-04-01*
