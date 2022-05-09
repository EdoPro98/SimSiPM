#include "SiPMRandom.h"
#include "SiPMTypes.h"
#include "SiPMMath.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <cstring>
#include <iostream>
#include <vector>

// Musl implementation of lcg64
static constexpr uint64_t lcg64(const uint64_t x) { return (x * 10419395304814325825ULL + 1) % -1ULL; }

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifdef __AVX2__
static uint64_t rdtsc() { return _rdtsc(); }
#else
static uint64_t rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}
#endif

namespace sipm {
namespace SiPMRng {
void Xorshift256plus::seed() {
  s[0] = lcg64(rdtsc());
  for (uint8_t i = 1; i < 4; ++i) {
    s[i] = lcg64(s[i - 1]);
  }
  // Call rng few times
  this->operator()();
  this->operator()();
  this->operator()();
  this->operator()();
}

void Xorshift256plus::seed(const uint64_t aseed) {
  s[0] = lcg64(aseed);
  for (uint8_t i = 1; i < 4; ++i) {
    s[i] = lcg64(s[i - 1]);
  }
}

void Xorshift256plus::jump() {
  static constexpr uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
  uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
  for (int i = 0; i < 4; ++i) {
    for (int b = 0; b < 64; ++b) {
      if (JUMP[i] & 1UL << b) {
        s0 ^= s[0];
        s1 ^= s[1];
        s2 ^= s[2];
        s3 ^= s[3];
      }
      this->operator()();
    }
  }
  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  s[3] = s3;
}
} // namespace SiPMRng

// SCALAR //

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
float SiPMRandom::randExponentialF(const float mu) noexcept { return -log(RandF()) * mu; }

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
    const float y = RandF();

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
      const float rn = 2 * RandF() - 1;
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
      float x = RandF();
      float y = kYm * RandF();
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
        if (rn * rn < 4 * (kB - log(x))) {
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
uint32_t SiPMRandom::randInteger(const uint32_t max) noexcept { return static_cast<uint32_t>(Rand() * max); }

/**
 * @param n Number of values to generate
 */
template <> auto SiPMRandom::Rand<SiPMVector<double>>(const uint32_t n) -> SiPMVector<double> {
  SiPMVector<uint64_t> uVec(n);
  SiPMVector<double> dVec(n);

  for (uint32_t i = 0; i < n; ++i) {
    uVec[i] = 0x3FFULL << 52ULL | m_rng() >> 12ULL;
  }
  std::memcpy(dVec.data(), uVec.data(), n * sizeof(uint64_t));
  for (uint32_t i = 0; i < n; ++i) {
    dVec[i] = dVec[i] - 1;
  }
  return dVec;
}

/**
 * @param n Number of values to generate
 */
template <> auto SiPMRandom::RandF<SiPMVector<float>>(const uint32_t n) -> SiPMVector<float> {
  SiPMVector<float> out(n);
  std::generate(out.begin(), out.end(), [this] { return this->RandF(); });
  return out;
}

/**
 * @param n Number of values to generate
 */
template <> auto SiPMRandom::Rand<std::vector<double>>(const uint32_t n) -> std::vector<double> {
  SiPMVector<double> out = Rand<SiPMVector<double>>(n);
  return std::vector<double>(out.begin(), out.end());
}

/**
 * @param n Number of values to generate
 */
template <> auto SiPMRandom::RandF<std::vector<float>>(const uint32_t n) -> std::vector<float> {
  SiPMVector<float> out = RandF<SiPMVector<float>>(n);
  return std::vector<float>(out.begin(), out.end());
}

/**
 * @param mu Mean value of the gaussuain
 * @param sigma Standard deviation value of the gaussuan
 * @param n Number of values to generate
 */
template <>
auto SiPMRandom::randGaussian<SiPMVector<double>>(const double mu, const double sigma, const uint32_t n)
  -> SiPMVector<double> {
  SiPMVector<double> out(n);
  SiPMVector<double> s(n);

  for (uint32_t i = 0; i < n - 1; i += 2) {
    double z, u, v;
    do {
      u = Rand() * 2.0 - 1.0;
      v = Rand() * 2.0 - 1.0;
      z = u * u + v * v;
    } while (z > 1.0);
    s[i] = z;
    s[i + 1] = z;
    out[i] = u;
    out[i + 1] = v;
  }

  for (uint32_t i = 0; i < n; ++i) {
    s[i] = log(s[i]) * math::reciprocal(s[i]);
  }

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = sqrt(-2 * s[i]) * out[i] * sigma + mu;
  }
  // If n is odd we miss last value so recalculate it anyway
  out[n - 1] = randGaussian(mu, sigma);

  return out;
}

/**
 * @param mu Mean value of the gaussian
 * @param sigma Standard deviation value of the gaussian
 * @param n Number of values to generate
 */
template <>
auto SiPMRandom::randGaussianF<SiPMVector<float>>(const float mu, const float sigma, const uint32_t n)
  -> SiPMVector<float> {
  SiPMVector<float> out(n);
  SiPMVector<float> s(n);

  for (uint32_t i = 0; i < n - 1; i += 2) {
    float z, u, v;
    do {
      u = RandF() * 2.0f - 1.0f;
      v = RandF() * 2.0f - 1.0f;
      z = u * u + v * v;
    } while (z > 1.0);
    s[i] = z;
    s[i + 1] = z;
    out[i] = u;
    out[i + 1] = v;
  }

  for (uint32_t i = 0; i < n; ++i) {
    s[i] = log(s[i]) * math::reciprocal(s[i]);
  }

  // If compiler is clever this loop should be vectorized
  // using vsqrtps and vfmadd instructions on ymm registers
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = sqrt(-2 * s[i]) * out[i] * sigma + mu;
  }
  // If n is odd we miss last value so recalculate it anyway
  out[n - 1] = randGaussianF(mu, sigma);

  return out;
}

/**
 * @param mu Mean value of the gaussuan
 * @param sigma Standard deviation value of the gaussian
 * @param n Number of values to generate
 */
template <>
auto SiPMRandom::randGaussian<std::vector<double>>(const double mu, const double sigma, const uint32_t n)
  -> std::vector<double> {
  SiPMVector<double> out = randGaussian<SiPMVector<double>>(mu, sigma, n);
  return std::vector<double>(out.begin(), out.end());
}

/**
 * @param mu Mean value of the gaussuan
 * @param sigma Standard deviation value of the gaussian
 * @param n Number of values to generate
 */
template <>
auto SiPMRandom::randGaussianF<std::vector<float>>(const float mu, const float sigma, const uint32_t n)
  -> std::vector<float> {
  SiPMVector<float> out = randGaussianF<SiPMVector<float>>(mu, sigma, n);
  return std::vector<float>(out.begin(), out.end());
}

/**
 * @param max Max value to generate
 * @param n Number of values to generate
 */
std::vector<uint32_t> SiPMRandom::randInteger(const uint32_t max, const uint32_t n) {
  std::vector<uint32_t> out(n);
  const SiPMVector<double> buffer = Rand<SiPMVector<double>>(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = buffer[i] * max;
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
template <>
auto SiPMRandom::randExponential<SiPMVector<double>>(const double mu, const uint32_t n) -> SiPMVector<double> {
  SiPMVector<double> out(n);
  const SiPMVector<double> buffer = Rand<SiPMVector<double>>(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -log(buffer[i]) * mu;
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
template <>
auto SiPMRandom::randExponentialF<SiPMVector<float>>(const float mu, const uint32_t n) -> SiPMVector<float> {
  SiPMVector<float> out(n);
  const SiPMVector<float> buffer = RandF<SiPMVector<float>>(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -log(buffer[i]) * mu;
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
template <>
auto SiPMRandom::randExponential<std::vector<double>>(const double mu, const uint32_t n) -> std::vector<double> {
  SiPMVector<double> out = randExponential<SiPMVector<double>>(mu, n);
  return std::vector<double>(out.begin(), out.end());
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
template <>
auto SiPMRandom::randExponentialF<std::vector<float>>(const float mu, const uint32_t n) -> std::vector<float> {
  SiPMVector<float> out = randExponentialF<SiPMVector<float>>(mu, n);
  return std::vector<float>(out.begin(), out.end());
}

} // namespace sipm
