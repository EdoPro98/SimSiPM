# Testing Patterns

**Analysis Date:** 2026-04-01

## Test Framework

**Runner:**
- Framework: Google Test (gtest)
- Config: `tests/CMakeLists.txt` (included via `SIPM_ENABLE_TEST` CMake option)

**Assertion Library:**
- Google Test assertions (`EXPECT_EQ`, `EXPECT_DOUBLE_EQ`, `EXPECT_TRUE`, `EXPECT_NE`, etc.)

**Run Commands:**
```bash
# Build tests
cmake -B build -S . -DSIPM_ENABLE_TEST=ON
make -C build

# Run all tests
ctest -C build --output-on-failure

# Watch mode
ctest -C build --output-on-failure -j

# Run specific test file
./tests/sensor
./tests/rand

# Run with coverage (requires lcov/gcov)
gcov -o build -d build
```

## Test File Organization

**Location:**
- All tests colocated in `tests/` directory

**Naming:**
- `<module>.cpp` pattern (e.g., `sensor.cpp`, `rand.cpp`, `properties.cpp`, `xorshift.cpp`)

**Structure:**
```
tests/
├── sensor.cpp     # SiPMSensor tests
├── rand.cpp       # SiPMRandom tests
├── properties.cpp # SiPMProperties tests
└── xorshift.cpp   # RNG algorithm tests
```

## Test Structure

**Suite Organization:**
```cpp
#include "SiPM.h"
#include <gtest/gtest.h>

struct TestClassName : public ::testing::Test {
  ClassName sut;           // SUT = System Under Test
  SiPMRandom rng;          // Common fixture data
};

TEST_F(TestClassName, MethodName) {
  // Tests using fixture
}

TEST_F(TestClassName, AnotherTest) {
  // Another test
}

TEST_F(TestClassName, EdgeCase) {
  // Edge case test
}
```

**Common Fixture Pattern:**
```cpp
struct TestSiPMProperties : public ::testing::Test {
  SiPMProperties sut;
  SiPMRandom rng;
};

TEST_F(TestSiPMProperties, Constructor) {
  EXPECT_EQ(sut.size(), 1);
  EXPECT_EQ(sut.pitch(), 25);
}
```

**Patterns:**
- Use `TEST_F` for tests that use fixture data (fixture-based tests)
- Use `TEST` for tests without fixtures (free tests)
- Fixtures store `sut` (System Under Test) and `rng` (Random number generator)
- Tests verify both constructor defaults and property setters

## Mocking

**Framework:**
- No mocking framework used
- All dependencies are in-memory or stateful

**Patterns:**
```cpp
// Direct testing - no mocking needed
TEST_F(TestSiPMProperties, SetPropertyWithSetter) {
  SiPMProperties lsut = sut;
  for (int i = 0; i < 1000; ++i) {
    double dcr = rng.Rand() * 100e3;
    lsut.setDcr(dcr);
    EXPECT_DOUBLE_EQ(lsut.dcr(), dcr);
  }
}
```

**What to Mock:**
- Not applicable - tests use direct in-memory objects

**What NOT to Mock:**
- Not applicable - all code paths are directly testable
- RNG is a fixture member for reproducibility

## Fixtures and Factories

**Test Data:**
```cpp
// RNG fixture for reproducible tests
struct TestSiPMProperties : public ::testing::Test {
  SiPMProperties sut;
  SiPMRandom rng;
};

// Test with many iterations for statistical properties
TEST_F(TestSiPMRandom, RandGeneration) {
  sipm::SiPMRandom rng;
  for (int i = 0; i < N; ++i) {  // N = 10000000
    double x = rng.Rand();
    EXPECT_GE(x, 0);
    EXPECT_LE(x, 1);
  }
}
```

**Location:**
- Fixtures defined in test files themselves
- No separate fixture factory files

## Coverage

**Requirements:**
- Not enforced in current codebase
- No coverage targets specified

**View Coverage:**
```bash
# Generate coverage reports
cmake -B build -S . -DSIPM_ENABLE_TEST=ON -DCMAKE_BUILD_TYPE=Debug
make -C build
ctest -C build
lcov --capture --directory build --output-file coverage.info
lcov --summary coverage.info
```

## Test Types

**Unit Tests:**
- Scope: Individual methods and constructors
- Approach: Constructor tests verify default values, setter tests verify parameter changes
- Example: `TEST_F(TestSiPMProperties, Constructor)` checks all default parameter values

**Integration Tests:**
- Scope: Full event simulation workflow
- Approach: Test end-to-end signal generation with noise effects
- Example: `TEST_F(TestSiPMSensor, SignalGeneration)` tests complete simulation with peak validation

**E2E Tests:**
- Not applicable for this library
- Python bindings can be tested with pytest separately

**Property Tests:**
- Scope: Statistical properties over many events
- Approach: Run many iterations (100k-10M) to verify distributions
- Example: `TEST_F(TestSiPMRandom, RandAverage)` verifies mean of uniform distribution

## Common Patterns

**Async Testing:**
- Not applicable (single-threaded simulation)

**Error Testing:**
```cpp
// Test that returns -1 for invalid inputs
double SiPMAnalogSignal::peak(const double intstart, const double intgate, const double threshold) const {
  // ...
  return peak;  // peak starts at -1
}
```

**Signal Validation:**
```cpp
TEST_F(TestSiPMSensor, SignalGeneration) {
  static constexpr int N = 25;
  static constexpr int R = 10000;  // Repetitions for averaging
  SiPMSensor sensor;
  // Configure with minimal noise
  auto prop = sensor.properties();
  prop.setXtOff();
  prop.setDcrOff();
  prop.setApOff();
  prop.setSnr(40);
  sensor.setProperties(prop);

  for (int i = 1; i < N; ++i) {
    double avg_peak = 0;
    for (int j = 0; j < R; ++j) {
      sensor.resetState();
      const std::vector<double> t = rng.randGaussian(10, 0.1, i);
      sensor.addPhotons(t);
      sensor.runEvent();
      avg_peak += sensor.signal().peak(0, 20, 0);
    }
    avg_peak /= R;
    EXPECT_GE(avg_peak + 0.5, i);  // Verify signal scales with input
    EXPECT_LE(avg_peak - 0.5, i);
  }
}
```

**Reproducibility Testing:**
```cpp
TEST_F(TestSiPMXorshift256, GenerationSmallWindowTest) {
  static constexpr int n = 16;
  uint64_t first_run[n];
  uint64_t second_run[n];

  sipm::SiPMRng::Xorshift256plus rng;

  for (int t = 0; t < 2; ++t) {
    rng.seed(1234567890); // Set seed
    for (int i = 0; i < n; ++i) {
      first_run[i] = rng();
    }
    rng.seed(1234567890); // Set same seed
    for (int i = 0; i < n; ++i) {
      second_run[i] = rng();
    }
    for (int i = 0; i < n; ++i) {
      EXPECT_EQ(first_run[i], second_run[i]) << ">> Generator with same seed produces different values";
    }
  }
}
```

**Distribution Testing:**
```cpp
// Verify exponential distribution average
TEST_F(TestSiPMRandom, ExponentialAverageSmall) {
  sipm::SiPMRandom rng;
  double x = 0;
  static const double std = muSmall / std::sqrt(N);
  for (int i = 0; i < N; ++i) {
    x += rng.randExponential(muSmall);
  }
  x = x / N;
  EXPECT_GE(x, muSmall - 3 * std);  // Within 3 sigma
  EXPECT_LE(x, muSmall + 3 * std);
}
```

**Dark Count Rate Testing:**
```cpp
TEST_F(TestSiPMSensor, AddDcr) {
  static constexpr int N = 1000000;
  int ndcr = 0;
  SiPMSensor sensor;
  sensor.rng().rng().seed();
  for (int i = 0; i < N; ++i) {
    sensor.resetState();
    sensor.runEvent();
    ndcr += sensor.debug().nDcr;
  }
  const double rate = 1e9 * ((double)ndcr / N / sensor.properties().signalLength());
  EXPECT_GE(rate, sensor.properties().dcr() * 0.95);
  EXPECT_LE(rate, sensor.properties().dcr() * 1.05);
}
```

**Optimization Tests:**
```cpp
// Stress test large input sizes
TEST_F(TestSiPMSensor, AddPhoton) {
  static constexpr int N = 100000;
  for (int i = 0; i < N; ++i) {
    sut.resetState();
    const double t = rng.randGaussian(100, 0.1);
    sut.addPhoton(t);
  }
}
```

## Random Number Generator Tests

**Deterministic Seed Testing:**
- Same seed produces same sequence
- Verified with small, medium, and large windows (16 to 65536 elements)

**Independence Testing:**
- Consecutive values should not be equal
- Correlation tests between paired random values
- Covariance should be near zero

**Distribution Testing:**
- Uniform: Verify bounds [0,1]
- Exponential: Verify mean within 3 sigma
- Gaussian: Verify mean near 0
- Poisson: Verify mean matches lambda
- Integer: Verify bounded range

## Benchmark Tests

**Location:**
- `bench/` directory contains performance tests

**Files:**
- `sensor.cpp` - Sensor performance benchmarks
- `random.cpp` - RNG performance benchmarks

**Use Case:**
- Measure performance of different algorithms
- Compare optimization levels
- Not used as unit tests but for profiling

## Python Bindings Testing

**Note:**
- Python bindings compiled separately with `-DCOMPILE_PYTHON_BINDINGS=ON`
- Not included in C++ test suite
- Can be tested with pytest if configured

## Test Compilation

**CMake Configuration:**
```cmake
# In CMakeLists.txt
add_subdirectory(tests)  # Only if SIPM_ENABLE_TEST=ON
```

**Dependencies:**
- `gtest` linked to all test files
- Tests include `<gtest/gtest.h>`

---

*Testing analysis: 2026-04-01*
