#include "SiPMRandom.h"
#include "SiPMTypes.h"
#include <cstdint>

// Random seed
#include <x86intrin.h>
static uint64_t rdtsc() {
  _mm_lfence();
  const uint64_t t = __rdtsc();
  _mm_lfence();
  return t;
}

namespace sipm {
namespace SiPMRng {
void Xorshift256plus::seed() {
  s[0] = lcg64(rdtsc());
  __s[0][0] = lcg64(s[0]);
  for (int i = 1; i < 8; ++i) {
    __s[0][i] = lcg64(__s[0][i - 1]);
  }
  for (uint8_t i = 1; i < 4; ++i) {
    s[i] = lcg64(s[i - 1]);
    for (int j = 0; j < 8; ++j) {
      __s[i][j] = lcg64(__s[i - 1][j]);
    }
  }
  // Call rng few times
  for (uint16_t i = 0; i < 1024; ++i) {
    this->operator()();
  }
  free(this->simd8(1024));
}

void Xorshift256plus::seed(const uint64_t aseed) {
  s[0] = aseed;
  __s[0][0] = aseed;
  for (int i = 1; i < 8; ++i) {
    __s[0][i] = lcg64(__s[0][i - 1]);
  }
  for (uint8_t i = 1; i < 4; ++i) {
    s[i] = lcg64(s[i - 1]);
    for (int j = 0; j < 8; ++j) {
      __s[i][j] = lcg64(__s[i - 1][j]);
    }
  }
  // Call rng few times
  for (uint16_t i = 0; i < 1024; ++i) {
    this->operator()();
  }
  free(this->simd8(1024));
}
} // namespace SiPMRng

// SCALAR //

// Generate two 32 bit floating from one 64 bit integer
pair<float, float> SiPMRandom::RandF2() noexcept {
  const uint64_t u64 = m_rng();
  const uint32_t lo = u64 & 0xffffffff;
  const uint32_t hi = u64 >> 32;
  const float first = static_cast<float>(lo & ((1ULL << 24) - 1)) * 0x1p-24;
  const float second = static_cast<float>(hi & ((1ULL << 24) - 1)) * 0x1p-24;
  return {first, second};
}

/**
 * @param mu Mean value of the poisson distribution
 */
uint32_t SiPMRandom::randPoisson(const double mu) noexcept {
  if (mu == 0) {
    return 0;
  }
  const double q = exp(-mu);
  double p = 1.0;
  uint32_t out = 0;

  while (p > q) {
    ++out;
    p *= Rand();
  }
  return out - 1;
}

/**
 * @param mu Mean value of the exponential distribution
 * @return double value from exponential distribution
 */
double SiPMRandom::randExponential(const double mu) noexcept { return -log(Rand()) * mu; }

/**
 * @param mu Mean value of the exponential distribution
 * @return float value from exponential distribution
 */
float SiPMRandom::randExponentialF(const float mu) noexcept { return -logf(Rand<float>()) * mu; }
/**
*   @brief Samples a random number from the standard Normal (Gaussian) Distribution with the given mean and sigma.
*
* Uses the Acceptance-complement ratio from W. Hoermann and G. Derflinger
* This is one of the fastest existing method for generating normal random variables.
* It is a factor 2/3 faster than the polar (Box-Muller) method used in the previous
* version of TRandom::Gaus. The speed is comparable to the Ziggurat method (from Marsaglia)
* implemented for example in GSL.
*
* REFERENCE:  - W. Hoermann and G. Derflinger (1990):
*              The ACR Method for generating normal random variables,
*              OR Spektrum 12 (1990), 181-185.
*
* Implementation taken from
* UNURAN (c) 2000  W. Hoermann & J. Leydold, Institut f. Statistik, WU Wien

* @param mu Mean value of the gaussian distribution
* @param sigma Standard deviation of the gaussian distribution
* @return double value from gaussian distribution
*/
double SiPMRandom::randGaussian(const double mu, const double sigma) noexcept {
  static constexpr double kC1 = 1.448242853;
  static constexpr double kC2 = 3.307147487;
  static constexpr double kC3 = 1.46754004;
  static constexpr double kD1 = 1.036467755;
  static constexpr double kD2 = 5.295844968;
  static constexpr double kD3 = 3.631288474;
  static constexpr double kHm = 0.483941449;
  static constexpr double kZm = 0.107981933;
  static constexpr double kHp = 4.132731354;
  static constexpr double kZp = 18.52161694;
  static constexpr double kPhln = 0.4515827053;
  static constexpr double kHm1 = 0.516058551;
  static constexpr double kHp1 = 3.132731354;
  static constexpr double kHzm = 0.375959516;
  static constexpr double kHzmp = 0.591923442;
  /*zhm 0.967882898*/

  static constexpr double kAs = 0.8853395638;
  static constexpr double kBs = 0.2452635696;
  static constexpr double kCs = 0.2770276848;
  static constexpr double kB = 0.5029324303;
  static constexpr double kX0 = 0.4571828819;
  static constexpr double kYm = 0.187308492;
  static constexpr double kS = 0.7270572718;
  static constexpr double kT = 0.03895759111;

  do {
    const double y = Rand();

    if (y > kHm1) {
      const double result = kHp * y - kHp1;
      return result * sigma + mu;
    }

    if (y < kZm) {
      const double rn = kZp * y - 1;
      const double result = (rn > 0) ? (1 + rn) : (-1 + rn);
      return result * sigma + mu;
    }

    if (y < kHm) {
      const double rn = 2 * Rand() - 1;
      const double z = (rn > 0) ? 2 - rn : -2 - rn;
      if ((kC1 - y) * (kC3 + abs(z)) < kC2) {
        return z * sigma + mu;
      }
      const double x = rn * rn;
      if ((y + kD1) * (kD3 + x) < kD2) {
        return rn * sigma + mu;
      }
      if (kHzmp - y < exp(-(z * z + kPhln) * 0.5)) {
        return z * sigma + mu;
      }
      if (y + kHzm < exp(-(x + kPhln) * 0.5)) {
        return rn * sigma + mu;
      }
    }

    while (true) {
      double x = Rand();
      double y = kYm * Rand();
      const double z = kX0 - kS * x - y;
      double rn;
      if (z > 0) {
        rn = 2 + y / x;
      } else {
        x = 1 - x;
        y = kYm - y;
        rn = -(2 + y / x);
      }
      if ((y - kAs + x) * (kCs + x) + kBs < 0) {
        return rn * sigma + mu;
      }
      if (y < x + kT) {
        if (rn * rn < 4 * (kB - log(x))) {
          return rn * sigma + mu;
        }
      }
    }
  } while (false);
}

/**
 * Float implemented of @ref randGaussian.
 * It is not faster than double version but is kept to avoid
 * casting double to float.
 */
float SiPMRandom::randGaussianF(const float mu, const float sigma) noexcept {
  static constexpr float kC1 = 1.448242853;
  static constexpr float kC2 = 3.307147487;
  static constexpr float kC3 = 1.46754004;
  static constexpr float kD1 = 1.036467755;
  static constexpr float kD2 = 5.295844968;
  static constexpr float kD3 = 3.631288474;
  static constexpr float kHm = 0.483941449;
  static constexpr float kZm = 0.107981933;
  static constexpr float kHp = 4.132731354;
  static constexpr float kZp = 18.52161694;
  static constexpr float kPhln = 0.4515827053;
  static constexpr float kHm1 = 0.516058551;
  static constexpr float kHp1 = 3.132731354;
  static constexpr float kHzm = 0.375959516;
  static constexpr float kHzmp = 0.591923442;
  /*zhm 0.967882898*/

  static constexpr float kAs = 0.8853395638;
  static constexpr float kBs = 0.2452635696;
  static constexpr float kCs = 0.2770276848;
  static constexpr float kB = 0.5029324303;
  static constexpr float kX0 = 0.4571828819;
  static constexpr float kYm = 0.187308492;
  static constexpr float kS = 0.7270572718;
  static constexpr float kT = 0.03895759111;

  do {
    const float y = Rand<float>();

    if (y > kHm1) {
      const float result = kHp * y - kHp1;
      return result * sigma + mu;
    }

    if (y < kZm) {
      const float rn = kZp * y - 1;
      const float result = (rn > 0) ? (1 + rn) : (-1 + rn);
      return result * sigma + mu;
    }

    if (y < kHm) {
      const float rn = 2 * Rand<float>() - 1;
      const float z = (rn > 0) ? 2 - rn : -2 - rn;
      if ((kC1 - y) * (kC3 + abs(z)) < kC2) {
        return z * sigma + mu;
      }
      const float x = rn * rn;
      if ((y + kD1) * (kD3 + x) < kD2) {
        return rn * sigma + mu;
      }
      if (kHzmp - y < exp(-(z * z + kPhln) * 0.5)) {
        return z * sigma + mu;
      }
      if (y + kHzm < exp(-(x + kPhln) * 0.5)) {
        return rn * sigma + mu;
      }
    }

    while (true) {
      float x = Rand<float>();
      float y = kYm * Rand<float>();
      const float z = kX0 - kS * x - y;
      float rn;
      if (z > 0) {
        rn = 2 + y / x;
      } else {
        x = 1 - x;
        y = kYm - y;
        rn = -(2 + y / x);
      }
      if ((y - kAs + x) * (kCs + x) + kBs < 0) {
        return rn * sigma + mu;
      }
      if (y < x + kT) {
        if (rn * rn < 4 * (kB - logf(x))) {
          return rn * sigma + mu;
        }
      }
    }
  } while (false);
}

/**
 * @param max Maximum value of integer to generate
 * @return uint32_t value from random integer distribution
 */
uint32_t SiPMRandom::randInteger(const uint32_t max) noexcept {
  uint32_t mask = ~uint32_t(0);
  uint32_t range = max - 1;
  mask >>= __builtin_clz(range | 1);
  uint32_t x;
  do {
    x = m_rng() & mask;
  } while (x > range);
  return x;
}

/**
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::Rand(const uint32_t n) {
  std::vector<double> f64(n);
  uint64_t* u64; // Free at end of function

  if (n > 8) {
    // simd8 allocates data
    u64 = m_rng.simd8(n);
  } else {
    // We do allocation
    u64 = (uint64_t*)aligned_alloc(64, n * sizeof(uint64_t));
    for (uint32_t i = 0; i < n; ++i) {
      u64[i] = m_rng();
    }
  }

  for (uint32_t i = 0; i < n; ++i) {
    f64[i] = static_cast<double>(u64[i]) * 0x1p-64;
  }

  free(u64);

  return f64;
}

/**
 * @param n Number of values to generate
 */
std::vector<float> SiPMRandom::RandF(const uint32_t n) {
  std::vector<float> f32(n);
  uint32_t* u32; // Free before end of function

  if (n > 16) {
    // simd8 allocates data
    // need n/2 uint64_t to generate n uint32_t
    // odd case requires one more generation for tail
    u32 = (uint32_t*)m_rng.simd8((n + 1) / 2);
  } else {
    // We do allocation
    u32 = (uint32_t*)aligned_alloc(64, n * sizeof(uint32_t));
    for (uint32_t i = 0; i < n - 1; i += 2) {
      const uint64_t u64 = m_rng();
      u32[i] = u64 >> 32;
      u32[i + 1] = u64 & 0xffffffff;
    }
  }

  for (uint32_t i = 0; i < n; ++i) {
    // See SiPMRandom.h for details
    f32[i] = static_cast<float>(u32[i]) * 0x1p-32;
  }
  f32[n - 1] = Rand<float>();

  free(u32);

  return f32;
}

/**
 * @param mu Mean value of the gaussuain
 * @param sigma Standard deviation value of the gaussuan
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::randGaussian(const double mu, const double sigma, const uint32_t n) {
  std::vector<double> out(n);
  const std::vector<double> u = Rand(n);
  constexpr double TWO_PI = 2 * M_PI;

  for (uint32_t i = 0; i < n - 1; i += 2) {
    const double R = sqrt(-2.0 * log(u[i]));
    out[i] = cos(TWO_PI * u[i]) * R;
    out[i + 1] = sin(TWO_PI * u[i + 1]) * R;
  }

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = out[i] * sigma + mu;
  }

  out[n - 1] = randGaussian(mu, sigma);

  return out;
}

/**
 * @param mu Mean value of the gaussian
 * @param sigma Standard deviation value of the gaussian
 * @param n Number of values to generate
 */
std::vector<float> SiPMRandom::randGaussianF(const float mu, const float sigma, const uint32_t n) {
  std::vector<float> out(n);
  const std::vector<float> u = RandF(n);
  constexpr float TWO_PI = 2 * M_PI;

  for (uint32_t i = 0; i < n - 1; i += 2) {
    const float R = sqrtf(-2.0 * logf(u[i]));
    out[i] = cosf(TWO_PI * u[i]) * R;
    out[i + 1] = sinf(TWO_PI * u[i + 1]) * R;
  }

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = out[i] * sigma + mu;
  }

  out[n - 1] = randGaussianF(mu, sigma);
  return out;
}

/**
 * @param max Max value to generate
 * @param n Number of values to generate
 */
std::vector<uint32_t> SiPMRandom::randInteger(const uint32_t max, const uint32_t n) {
  std::vector<uint32_t> out(n);
  uint32_t* u32;

  if (n < 16) {
    u32 = (uint32_t*)aligned_alloc(64, n * sizeof(uint32_t));
    for (uint32_t i = 0; i < n; ++i) {
      u32[i] = m_rng() >> 32;
    }
  } else {
    u32 = (uint32_t*)m_rng.simd8((n + 1) / 2);
  }

  // Sort of fixed point arithmetic
  // Avoids division and float numbers
  for (uint32_t i = 0; i < n; ++i) {
    const uint64_t m = uint64_t(u32[i]) * uint64_t(max);
    out[i] = m >> 32;
  }

  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::randExponential(const double mu, const uint32_t n) {
  std::vector<double> out(n);
  const std::vector<double> buffer = Rand(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -log(buffer[i]) * mu;
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
std::vector<float> SiPMRandom::randExponentialF(const float mu, const uint32_t n) {
  std::vector<float> out(n);
  const std::vector<float> buffer = RandF(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -logf(buffer[i]) * mu;
  }
  return out;
}
} // namespace sipm
