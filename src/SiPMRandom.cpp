#include "SiPMRandom.h"

#include "SiPMTypes.h"
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <random>


// Random seed
static uint64_t rngInit() {
  std::random_device rd;
  return (static_cast<uint64_t>(rd()) << 32) ^ rd();
}

// Branch-free logf/sincosf (Cephes polynomials, ~1e-7 error) so the bulk
// Box-Muller loop vectorizes: scalar libm calls block auto-vectorization.
// Valid for x in (0, 1], the range of unitFloat.
static inline float logUnit(const float x) {
  uint32_t ix;
  std::memcpy(&ix, &x, sizeof ix);
  int32_t e = static_cast<int32_t>(ix >> 23) - 126;
  ix = (ix & 0x007FFFFFu) | 0x3F000000u; // mantissa in [0.5, 1)
  float m;
  std::memcpy(&m, &ix, sizeof m);
  const bool small = m < 0.70710678f;
  e -= small;
  const float f = small ? m + m - 1 : m - 1;
  const float z = f * f;
  float y = 7.0376836292E-2f;
  y = y * f - 1.1514610310E-1f;
  y = y * f + 1.1676998740E-1f;
  y = y * f - 1.2420140846E-1f;
  y = y * f + 1.4249322787E-1f;
  y = y * f - 1.6668057665E-1f;
  y = y * f + 2.0000714765E-1f;
  y = y * f - 2.4999993993E-1f;
  y = y * f + 3.3333331174E-1f;
  y = y * f * z - 2.12194440E-4f * e - 0.5f * z;
  return f + y + 0.693359375f * e;
}

// sin and cos of 2*pi*u for u in [0, 1]
static inline void sinCosTurn(const float u, float& s, float& c) {
  const float t = u * 4;
  const int32_t k = static_cast<int32_t>(t + 0.5f); // nearest quarter turn
  const float r = (t - k) * 1.5707963267948966f;   // in [-pi/4, pi/4]
  const float z = r * r;
  const float sr = r + r * z * ((-1.9515295891E-4f * z + 8.3321608736E-3f) * z - 1.6666654611E-1f);
  const float cr = 1 - 0.5f * z + z * z * ((2.443315711809948E-5f * z - 1.388731625493765E-3f) * z + 4.166664568298827E-2f);
  const bool swap = k & 1;
  const float ss = swap ? cr : sr;
  const float cc = swap ? sr : cr;
  s = (k & 2) ? -ss : ss;
  c = ((k + 1) & 2) ? -cc : cc;
}

namespace sipm {
namespace SiPMRng {
void Xorshift256plus::seed() {
  s[0] = lcg64(rngInit());
  for (uint8_t i = 1; i < 4; ++i) {
    s[i] = lcg64(s[i - 1]);
  }
#ifdef __AVX512F__
  __sA[0][0] = lcg64(rngInit());
  for (int i = 1; i < 8; ++i) __sA[0][i] = lcg64(__sA[0][i - 1]);
  for (int i = 1; i < 4; ++i)
    for (int j = 0; j < 8; ++j) __sA[i][j] = lcg64(__sA[i - 1][j]);
  __sB[0][0] = lcg64(__sA[3][7]);
  for (int i = 1; i < 8; ++i) __sB[0][i] = lcg64(__sB[0][i - 1]);
  for (int i = 1; i < 4; ++i)
    for (int j = 0; j < 8; ++j) __sB[i][j] = lcg64(__sB[i - 1][j]);
#elif defined(__AVX2__)
  __sA[0][0] = lcg64(rngInit());
  for (int i = 1; i < 4; ++i) __sA[0][i] = lcg64(__sA[0][i - 1]);
  for (int i = 1; i < 4; ++i)
    for (int j = 0; j < 4; ++j) __sA[i][j] = lcg64(__sA[i - 1][j]);
  __sB[0][0] = lcg64(__sA[3][3]);
  for (int i = 1; i < 4; ++i) __sB[0][i] = lcg64(__sB[0][i - 1]);
  for (int i = 1; i < 4; ++i)
    for (int j = 0; j < 4; ++j) __sB[i][j] = lcg64(__sB[i - 1][j]);
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
  __sA[0][0] = lcg64(s[3]);
  for (int i = 1; i < 8; ++i) __sA[0][i] = lcg64(__sA[0][i - 1]);
  for (int i = 1; i < 4; ++i)
    for (int j = 0; j < 8; ++j) __sA[i][j] = lcg64(__sA[i - 1][j]);
  __sB[0][0] = lcg64(__sA[3][7]);
  for (int i = 1; i < 8; ++i) __sB[0][i] = lcg64(__sB[0][i - 1]);
  for (int i = 1; i < 4; ++i)
    for (int j = 0; j < 8; ++j) __sB[i][j] = lcg64(__sB[i - 1][j]);
#elif defined(__AVX2__)
  __sA[0][0] = lcg64(s[3]);
  for (int i = 1; i < 4; ++i) __sA[0][i] = lcg64(__sA[0][i - 1]);
  for (int i = 1; i < 4; ++i)
    for (int j = 0; j < 4; ++j) __sA[i][j] = lcg64(__sA[i - 1][j]);
  __sB[0][0] = lcg64(__sA[3][3]);
  for (int i = 1; i < 4; ++i) __sB[0][i] = lcg64(__sB[0][i - 1]);
  for (int i = 1; i < 4; ++i)
    for (int j = 0; j < 4; ++j) __sB[i][j] = lcg64(__sB[i - 1][j]);
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
  const uint32_t lo = static_cast<uint32_t>(u64);
  const uint32_t hi = static_cast<uint32_t>(u64 >> 32);
  const float first = unitFloat(lo);
  const float second = unitFloat(hi);
  return {first, second};
}

/**
 * @param mu Mean value of the poisson distribution
 */
uint32_t SiPMRandom::randPoisson(const double mu) noexcept {
  if (mu <= 0) {
    return 0;
  }
  // For large mu, Poisson(mu) ≈ Normal(mu, sqrt(mu)). This is O(1) vs O(mu) for Knuth's algorithm.
  if (mu > 50.0) {
    const double x = randGaussian(mu, std::sqrt(mu));
    return x < 0.5 ? 0u : static_cast<uint32_t>(x + 0.5);
  }
  const double emu = std::exp(-mu);
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
      if ((kC1 - y) * (kC3 + std::abs(z)) < kC2) {
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
      if ((kC1 - y) * (kC3 + std::abs(z)) < kC2) {
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
  // Use direct multiplication and bit-shifting
  // Use local variables to help compiler optimize
  uint32_t hi = static_cast<uint32_t>(u64 >> 32);
  uint32_t lo = static_cast<uint32_t>(u64);
  uint32_t first = (uint64_t(hi) * max) >> 32;
  uint32_t second = (uint64_t(lo) * max) >> 32;
  return {first, second};
}

/**
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::Rand(const uint32_t n) {
  std::vector<double> out(n);
  // Generate raw u64 directly into the output buffer (sizeof(uint64_t)==sizeof(double)),
  // then convert each element in-place before reading it as a double.
  auto* const u64 = reinterpret_cast<uint64_t*>(out.data());
  m_rng.getRand(u64, n);
  for (uint32_t i = 0; i < n; ++i) {
    const uint64_t u = u64[i];
    out[i] = unitDouble(u);
  }
  return out;
}

/**
 * @param n Number of values to generate
 */
std::vector<float> SiPMRandom::RandF(const uint32_t n) {
  std::vector<float> out(n);
  // Generate raw u32 directly into the output buffer (sizeof(uint32_t)==sizeof(float)),
  // then convert each element in-place before reading it as a float.
  auto* const u32 = reinterpret_cast<uint32_t*>(out.data());
  m_rng.getRand(u32, n);
  for (uint32_t i = 0; i < n; ++i) {
    const uint32_t u = u32[i];
    out[i] = unitFloat(u);
  }
  return out;
}

/**
 * @param mu Mean value of the gaussuain
 * @param sigma Standard deviation value of the gaussuan
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::randGaussian(const double mu, const double sigma, const uint32_t n) {
  std::vector<double> out(n);
  constexpr double TWO_PI = 2 * M_PI;
  // Fill out with raw u64 bits, then convert each pair in-place (same trick as Rand(n)).
  auto* u64 = reinterpret_cast<uint64_t*>(out.data());
  m_rng.getRand(u64, n);

  const uint32_t pairs = n & ~1u;
  for (uint32_t i = 0; i < pairs; i += 2) {
    const double u0 = unitDouble(u64[i]);
    const double u1 = unitDouble(u64[i + 1]);
    const double sqrtR = std::sqrt(-2.0 * std::log(u0));
    double s, c;
#ifdef __APPLE__
    __sincos(TWO_PI * u1, &s, &c);
#else
    sincos(TWO_PI * u1, &s, &c);
#endif
    out[i]     = s * sqrtR * sigma + mu;
    out[i + 1] = c * sqrtR * sigma + mu;
  }
  if (n & 1u) {
    out[n - 1] = randGaussian(mu, sigma);
  }
  return out;
}

/**
 * @param mu Mean value of the gaussian
 * @param sigma Standard deviation value of the gaussian
 * @param n Number of values to generate
 */
std::vector<float> SiPMRandom::randGaussianF(const float mu, const float sigma, const uint32_t n) {
  std::vector<float> out(n);
  auto* u32 = reinterpret_cast<uint32_t*>(out.data());
  m_rng.getRand(u32, n);

  const uint32_t pairs = n & ~1u;
  for (uint32_t i = 0; i < pairs; i += 2) {
    const float u0 = unitFloat(u32[i]);
    const float u1 = unitFloat(u32[i + 1]);
    const float sqrtR = std::sqrt(-2.0f * logUnit(u0));
    float s, c;
    sinCosTurn(u1, s, c);
    out[i]     = s * sqrtR * sigma + mu;
    out[i + 1] = c * sqrtR * sigma + mu;
  }
  if (n & 1u) {
    out[n - 1] = randGaussianF(mu, sigma);
  }
  return out;
}

/**
 * @param max Max value to generate
 * @param n Number of values to generate
 */
std::vector<uint32_t> SiPMRandom::randInteger(const uint32_t max, const uint32_t n) {
  std::vector<uint32_t> out(n);
  m_rng.getRand(out.data(), n);
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = (uint64_t(out[i]) * max) >> 32;
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::randExponential(const double mu, const uint32_t n) {
  std::vector<double> out(n);
  auto* u64 = reinterpret_cast<uint64_t*>(out.data());
  m_rng.getRand(u64, n);
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -std::log(unitDouble(u64[i])) * mu;
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
std::vector<float> SiPMRandom::randExponentialF(const float mu, const uint32_t n) {
  std::vector<float> out(n);
  auto* u32 = reinterpret_cast<uint32_t*>(out.data());
  m_rng.getRand(u32, n);
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -std::log(unitFloat(u32[i])) * mu;
  }
  return out;
}
} // namespace sipm
