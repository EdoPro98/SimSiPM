# External Integrations

**Analysis Date:** 2026-04-01

## APIs & External Services

**None**

This is a standalone C++ library with no external API integrations. All random number generation and signal processing algorithms are implemented from scratch within the codebase.

### Random Number Generation

**Implementation:** Custom implementation of PRNG algorithms
- **Location:** `include/SiPMRandom.h`, `src/SiPMRandom.cpp`
- **Algorithm:** Xorshift256+ (xoshiro256+)
- **Seeding:** Uses `std::random_device` combined with LCG64 for entropy gathering
  - `static uint64_t lcg64(const uint64_t x)` - Custom LCG implementation
  - `std::random_device rd` - System random device for initial seeding

**Distribution Implementations:**
- **Poisson:** `randPoisson()` - Inverse transform method
- **Exponential:** `randExponential()` - `-log(U) * mu` method
- **Gaussian:** `randGaussian()` - Acceptance-complement ratio (ACR) method
  - Reference: Hoermann & Derflinger (1990) ACR Method
  - Optimized for speed (2/3 faster than polar/Box-Muller)
- **Uniform:** Bit-extraction from PRNG output
- **Integer:** Bit-shifting and multiplication methods

**Distributions Available:**
```cpp
// Double precision randoms
double Rand();                                    // Uniform (0, 1]
double randPoisson(double mu);                    // Poisson distribution
double randExponential(double mu);                // Exponential distribution
double randGaussian(double mu, double sigma);     // Gaussian distribution

// Single-precision randoms
float Rand<float>();                               // Uniform (0, 1]
float randExponentialF(float mu);                  // Exponential distribution
float randGaussianF(float mu, float sigma);        // Gaussian distribution

// Bulk generation
std::vector<double> Rand(const uint32_t n);
std::vector<float> RandF(const uint32_t n);
std::vector<double> randGaussian(double, double, uint32_t);
std::vector<float> randGaussianF(float, float, uint32_t);
std::vector<double> randExponential(double, uint32_t);
std::vector<float> randExponentialF(float, uint32_t);
std::vector<uint32_t> randInteger(uint32_t max, uint32_t n);
```

**Vectorized Generation:**
- Bulk generation methods avoid per-call overhead
- Uses internal buffer (`uint64_t buffer[N]` with `N = 1 << 16`)
- SIMD-optimized paths for AVX-512 capable CPUs
- Memory alignment with `alignas(64)` for cache efficiency

**Location:** `src/SiPMRandom.cpp`

## Data Storage

**Databases:**
- **None** - Pure in-memory simulation
- Uses STL containers for data:
  - `std::vector` for waveforms and photon lists
  - `std::map<double, double>` for PDE wavelength spectra
  - `std::unordered_map` for hit hash tables
  - `std::array` (via `SiPMSmallVector`) for small buffers

**File Storage:**
- **Local filesystem only**
- Properties read from file via `SiPMProperties::readSettings(const std::string& fname)`
  - Format: Key-value pairs with `=` delimiter
  - Supports comments starting with `#` or `/`
  - Location: Any accessible file path passed to method

**In-Memory Structures:**
- **SmallVector:** `SiPMSmallVector<N = 3>` template class
  - Stack-allocated storage for small counts
  - Auto-promotes to heap when exceeding capacity
  - Location: `include/SiPMTypes.h`
  - Uses custom heap allocation to avoid `<vector>` overhead

- **Pair:** Custom `pair` template (no runtime checks)
  - Location: `include/SiPMTypes.h`
  - Optimized for performance (no bounds checking)

## Authentication & Identity

**None** - No authentication required (pure simulation library)

## Monitoring & Observability

**Error Tracking:**
- **None** - No external error tracking service
- Error reporting via `std::cerr`:
  - Property not found messages
  - File open failures
  - Platform detection warnings

**Logs:**
- **Console output via `std::ostream`**
- All logging uses standard I/O streams
- `operator<<` overloads for debug output
- Located throughout header files and implementation

**Diagnostic Methods:**
- `SiPMDebugInfo` struct with MC-truth values:
  - Number of photons, photoelectrons, dark counts, crosstalk, afterpulses
  - Location: `include/SiPMDebugInfo.h`

## CI/CD & Deployment

**Hosting:**
- **GitHub Actions** (`.github/workflows/`)
  - `cmake-single-platform.yml` - Single platform CI/CD
  - `cmake-multi-platform.yml` - Multi-platform matrix
  - `deployonTestPyPi.yml` - PyPI deployment on test
  - `deploy.yml` - Production deployment

**CI Pipeline:**
- **GitHub Actions workflows:**
  - **cmake-single-platform.yml:**
    - Runs on `macos-latest`
    - CMake configure: `cmake -B build -DCMAKE_BUILD_TYPE=Release -DSIPM_ENABLE_TEST=ON`
    - Build: `cmake --build build --config Release`
    - Test: `ctest -C Release --test-dir tests`

  - **cmake-multi-platform.yml:**
    - Cross-platform matrix (Linux, macOS, Windows)
    - Multiple build configurations

  - **deployonTestPyPi.yml:**
    - Publishes to TestPyPI for validation
    - Uses `twine` for upload

  - **deploy.yml:**
    - Production PyPI deployment
    - Requires PyPI token or permissions

**Testing Workflow:**
- Configure: `cmake -B build -DSIPM_ENABLE_TEST=ON`
- Build: `cmake --build build`
- Test: `ctest --test-dir build/tests`

## Environment Configuration

**Required env vars:**
- **None** - No environment variables required for runtime
- Build-time: `CMAKE_BUILD_TYPE` (Release/Debug/RelWithDebInfo)
- Build-time: `SIPM_ENABLE_TEST` (ON/OFF for test suite)

**Secrets location:**
- **Not applicable** - No secrets stored
- RNG seeding uses system `random_device` for entropy
- No external API keys or credentials required

## Webhooks & Callbacks

**Incoming:**
- **None** - No webhook endpoints

**Outgoing:**
- **None** - No outgoing callbacks

## Python Bindings

**pybind11 Integration:**
- **Location:** `pyproject.toml` references `pybind11>=2.10.0`
- **Purpose:** Exposes C++ classes to Python runtime
- **Build Backend:** `setuptools.build_meta`
- **Build Requirements:**
  ```toml
  [build-system]
  requires = [
      "setuptools>=42",
      "pybind11>=2.10.0",
  ]
  build-backend = "setuptools.build_meta"
  ```

**Exposure Pattern:**
- Python imports C++ headers via pybind11's `pybind11_MODULE` macro
- C++ types accessible from Python through binding layer
- Enables scientific Python workflows (NumPy integration potential)

## External Math Libraries

**None** - All mathematical operations implemented internally:
- `<cmath>` functions: `exp()`, `log()`, `sqrt()`, `sin()`, `cos()`
- Platform-specific: `__sincos()`, `sincos()` for trig functions
- `<random>` standard library RNG interface (used for seeding only)
- No external math libraries (Boost, GSL, etc.)

---

*Integration audit: 2026-04-01*
