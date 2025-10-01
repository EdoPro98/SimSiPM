#include "SiPMRandom.h"

#include "SiPMTypes.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <vector>
#include <random>


// Random seed
static uint64_t rngInit() {
  std::random_device rd;
  return (static_cast<uint64_t>(rd()) << 32) ^ rd();
}

namespace sipm {
namespace SiPMRng {
void Xorshift256plus::seed() {
  s[0] = lcg64(rngInit());
  for (uint8_t i = 1; i < 4; ++i) {
    s[i] = lcg64(s[i - 1]);
  }
#ifdef __AVX512F__
  __s[0][0] = lcg64(rngInit());
  for (int i = 1; i < 8; ++i) {
    __s[0][i] = lcg64(__s[0][i - 1]);
  }
  for (uint8_t i = 1; i < 4; ++i) {
    for (int j = 0; j < 8; ++j) {
      __s[i][j] = lcg64(__s[i - 1][j]);
    }
  }
#endif
  index = N;
  // Call rng few times
  for (uint32_t i = 0; i < 1 << 16; ++i) {
    this->operator()();
  }
}

void Xorshift256plus::seed(const uint64_t aseed) {
  s[0] = lcg64(aseed);
  for (uint8_t i = 1; i < 4; ++i) {
    s[i] = lcg64(s[i - 1]);
  }
#ifdef __AVX512F__
  __s[0][0] = lcg64(s[3]);
  for (int i = 1; i < 8; ++i) {
    __s[0][i] = lcg64(__s[0][i - 1]);
  }
  for (uint8_t i = 1; i < 4; ++i) {
    for (int j = 0; j < 8; ++j) {
      __s[i][j] = lcg64(__s[i - 1][j]);
    }
  }
#endif
  index = N;
  // Call rng few
  for (uint32_t i = 0; i < 1 << 16; ++i) {
    this->operator()();
  }
}
} // namespace SiPMRng

// SCALAR //

// Generate two 32 bit floating from one 64 bit integer
pair<float, float> SiPMRandom::RandF2() noexcept {
  const uint64_t u64 = m_rng();
  const uint32_t lo = u64 & 0xffffffff;
  const uint32_t hi = u64 >> 32;
  const float first = (lo >> 8) * 0x1p-24f;
  const float second = (hi >> 8) * 0x1p-24f;
  return {first, second};
}

/**
 * @param mu Mean value of the poisson distribution
 */
uint32_t SiPMRandom::randPoisson(const double mu) noexcept {
  if (mu <= 0) {
    return 0;
  }

  const double emu = exp(-mu);
  double prod = 1.0;
  uint32_t out = 0;

  while (1) {
    const double U = Rand();
    prod *= U;
    if (prod > emu) {
      out++;
    } else {
      return out;
    }
  }
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
uint32_t SiPMRandom::randInteger(const uint32_t max) noexcept { return ((m_rng() >> 32) * max) >> 32; }

pair<uint32_t> SiPMRandom::randInteger2(const uint32_t max) noexcept {
  const uint64_t u64 = m_rng();
  const uint32_t first = ((u64 >> 32) * max) >> 32;
  const uint32_t second = ((u64 & 0xffffffff) * max) >> 32;
  return {first, second};
}

/**
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::Rand(const uint32_t n) {
  // Integer multiple of 64 grater than n*sizeof(double)
  std::vector<double> out(n);

  uint64_t* u64 = (uint64_t*)sipmAlloc(sizeof(uint64_t) * n);
  m_rng.getRand(u64, n);
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = (u64[i] >> 11) * 0x1p-53;
  }
  sipmFree(u64);

  return out;
}

/**
 * @param n Number of values to generate
 */
std::vector<float> SiPMRandom::RandF(const uint32_t n) {
  std::vector<float> out(n);

  // simd8 allocates data
  uint32_t* u32 = (uint32_t*)sipmAlloc(sizeof(float) * n);
  m_rng.getRand(u32, n);
  for (uint32_t i = 0; i < n; ++i) {
    // See SiPMRandom.h for details
    out[i] = (u32[i] >> 8) * 0x1p-24f;
  }
  sipmFree(u32);
  return out;
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
  double* r = (double*)sipmAlloc(sizeof(double) * n);

  for (uint32_t i = 0; i < n - 1; i += 2) {
    const double R = -2 * log(u[i]);
    r[i] = R;
    r[i + 1] = R;
  }
#ifdef __APPLE__
  for (uint32_t i = 0; i < n - 1; i += 2) {
    double* ptr = out.data() + i;
    __sincos(TWO_PI * u[i + 1], ptr, ptr + 1);
  }
#else
  for (uint32_t i = 0; i < n - 1; i += 2) {
    double* ptr = out.data() + i;
    sincos(TWO_PI * u[i + 1], ptr, ptr + 1);
  }
#endif
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = out[i] * sqrt(r[i]) * sigma + mu;
  }

  out[n - 1] = randGaussian(mu, sigma);
  sipmFree(r);
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
  float* r = (float*)sipmAlloc(sizeof(float) * n);

  for (uint32_t i = 0; i < n - 1; i += 2) {
    const float R = -2 * logf(u[i]);
    r[i] = R;
    r[i + 1] = R;
  }
#ifdef __APPLE__
  for (uint32_t i = 0; i < n - 1; i += 2) {
    float* ptr = out.data() + i;
    __sincosf(TWO_PI * u[i + 1], ptr, ptr + 1);
  }
#else
  for (uint32_t i = 0; i < n - 1; i += 2) {
    float* ptr = out.data() + i;
    sincosf(TWO_PI * u[i + 1], ptr, ptr + 1);
  }
#endif
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = out[i] * sqrtf(r[i]) * sigma + mu;
  }

  out[n - 1] = randGaussianF(mu, sigma);
  sipmFree(r);
  return out;
}

/**
 * @param max Max value to generate
 * @param n Number of values to generate
 */
std::vector<uint32_t> SiPMRandom::randInteger(const uint32_t max, const uint32_t n) {
  std::vector<uint32_t> out(n);

  // Remember to free u32 ptr
  uint32_t* u32 = (uint32_t*)sipmAlloc(sizeof(uint32_t) * n);
  m_rng.getRand(u32, n);

  // Sort of fixed point arithmetic
  // Avoids division and float numbers
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = (uint64_t(u32[i]) * max) >> 32;
  }
  sipmFree(u32);

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
