#include "SiPMRandom.h"
#include <SiPMTypes.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sipm {
namespace SiPMRng {
void Xorshift256plus::seed() {
  for (uint8_t i = 0; i < 4; ++i) {
    s[i] = std::random_device{}();
  }
  // Call rng few times
  this->operator()();
  this->operator()();
  this->operator()();
  this->operator()();
}

void Xorshift256plus::seed(const uint64_t aseed) {
  for (uint8_t i = 0; i < 4; ++i) {
    s[i] = aseed + i;
  }
}

void Xorshift256plus::jump() {
  static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
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
uint32_t SiPMRandom::randPoisson(const double mu) {
  const double q = exp(-mu);
  double p = 1;
  int32_t out = -1;

  while (p > q) {
    ++out;
    p *= Rand();
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @return double value from exponential distribution
 */
double SiPMRandom::randExponential(const double mu) { return -log(Rand()) * mu; }

/**
 * @param mu Mean value of the exponential distribution
 * @return float value from exponential distribution
 */
float SiPMRandom::randExponentialF(const float mu) { return -log(RandF()) * mu; }

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
double SiPMRandom::randGaussian(const double mu, const double sigma) {
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

    else if (y < kZm) {
      const double rn = kZp * y - 1;
      const double result = (rn > 0) ? (1 + rn) : (-1 + rn);
      return result * sigma + mu;
    }

    else if (y < kHm) {
      const double rn = 2 * Rand() - 1;
      const double z = (rn > 0) ? 2 - rn : -2 - rn;
      if ((kC1 - y) * (kC3 + abs(z)) < kC2) {
        return z * sigma + mu;
      } else {
        const double x = rn * rn;
        if ((y + kD1) * (kD3 + x) < kD2) {
          return rn * sigma + mu;
        } else if (kHzmp - y < exp(-(z * z + kPhln) * 0.5)) {
          return z * sigma + mu;
        } else if (y + kHzm < exp(-(x + kPhln) * 0.5)) {
          return rn * sigma + mu;
        }
      }
    }

    while (1) {
      double x = Rand();
      double y = kYm * Rand();
      const double z = kX0 - kS * x - y;
      double rn;
      if (z > 0)
        rn = 2 + y / x;
      else {
        x = 1 - x;
        y = kYm - y;
        rn = -(2 + y / x);
      }
      if ((y - kAs + x) * (kCs + x) + kBs < 0) {
        return rn * sigma + mu;
      } else if (y < x + kT) {
        if (rn * rn < 4 * (kB - log(x))) {
          return rn * sigma + mu;
        }
      }
    }
  } while (0);
}

/**
 * Float implemented of @ref randGaussian.
 * It is not faster than double version but is kept to avoid
 * casting double to float.
 */
float SiPMRandom::randGaussianF(const float mu, const float sigma) {
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

    else if (y < kZm) {
      const float rn = kZp * y - 1;
      const float result = (rn > 0) ? (1 + rn) : (-1 + rn);
      return result * sigma + mu;
    }

    else if (y < kHm) {
      const float rn = 2 * RandF() - 1;
      const float z = (rn > 0) ? 2 - rn : -2 - rn;
      if ((kC1 - y) * (kC3 + abs(z)) < kC2) {
        return z * sigma + mu;
      } else {
        const float x = rn * rn;
        if ((y + kD1) * (kD3 + x) < kD2) {
          return rn * sigma + mu;
        } else if (kHzmp - y < exp(-(z * z + kPhln) * 0.5)) {
          return z * sigma + mu;
        } else if (y + kHzm < exp(-(x + kPhln) * 0.5)) {
          return rn * sigma + mu;
        }
      }
    }

    while (1) {
      float x = RandF();
      float y = kYm * RandF();
      const float z = kX0 - kS * x - y;
      float rn;
      if (z > 0)
        rn = 2 + y / x;
      else {
        x = 1 - x;
        y = kYm - y;
        rn = -(2 + y / x);
      }
      if ((y - kAs + x) * (kCs + x) + kBs < 0) {
        return rn * sigma + mu;
      } else if (y < x + kT) {
        if (rn * rn < 4 * (kB - log(x))) {
          return rn * sigma + mu;
        }
      }
    }
  } while (0);
}

/**
 * @param max Maximum value of integer to generate
 * @return uint32_t value from random integer distribution
 */
uint32_t SiPMRandom::randInteger(const uint32_t max) { return static_cast<uint32_t>(Rand() * max); }

/**
 * @param n Number of values to generate
 */
template <> SiPMVector<double> SiPMRandom::Rand<SiPMVector<double>>(const uint32_t n) {
  SiPMVector<double> out(n);
  std::generate(out.begin(), out.end(), [this] { return this->Rand(); });
  return out;
}

/**
 * @param n Number of values to generate
 */
template <> SiPMVector<float> SiPMRandom::RandF<SiPMVector<float>>(const uint32_t n) {
  SiPMVector<float> out(n);
  std::generate(out.begin(), out.end(), [this] { return this->RandF(); });
  return out;
}

/**
 * @param n Number of values to generate
 */
template <> std::vector<double> SiPMRandom::Rand<std::vector<double>>(const uint32_t n) {
  std::vector<double> out(n);
  std::generate(out.begin(), out.end(), [this] { return this->Rand(); });
  return out;
}

/**
 * @param n Number of values to generate
 */
template <> std::vector<float> SiPMRandom::RandF<std::vector<float>>(const uint32_t n) {
  std::vector<float> out(n);
  std::generate(out.begin(), out.end(), [this] { return this->RandF(); });
  return out;
}

/**
 * @param mu Mean value of the gaussuain
 * @param sigma Standard deviation value of the gaussuan
 * @param n Number of values to generate
 */
template <>
SiPMVector<double> SiPMRandom::randGaussian<SiPMVector<double>>(const double mu, const double sigma, const uint32_t n) {
  SiPMVector<double> out(n);
  SiPMVector<double> s(n);

  for (uint32_t i = 0; i < n - 1; i += 2) {
    double z, u, v;
    do {
      u = Rand() * 2.0 - 1.0;
      v = Rand() * 2.0 - 1.0;
      z = u * u + v * v;
    } while (z >= 1.0);
    s[i] = log(z) * math::reciprocal(z);
    s[i + 1] = s[i];
    out[i] = u;
    out[i + 1] = v;
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
SiPMVector<float> SiPMRandom::randGaussianF<SiPMVector<float>>(const float mu, const float sigma, const uint32_t n) {
  SiPMVector<float> out(n);
  SiPMVector<float> s(n);

  for (uint32_t i = 0; i < n - 1; i += 2) {
    float z, u, v;
    do {
      u = RandF() * 2.0f - 1.0f;
      v = RandF() * 2.0f - 1.0f;
      z = u * u + v * v;
    } while (z >= 1.0);
    s[i] = log(z) * math::reciprocal(z);
    s[i + 1] = s[i];
    out[i] = u;
    out[i + 1] = v;
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
std::vector<double> SiPMRandom::randGaussian<std::vector<double>>(const double mu, const double sigma,
                                                                  const uint32_t n) {
  SiPMVector<double> out = randGaussian<SiPMVector<double>>(mu, sigma, n);
  return std::vector<double>(out.begin(), out.end());
}

/**
 * @param mu Mean value of the gaussuan
 * @param sigma Standard deviation value of the gaussian
 * @param n Number of values to generate
 */
template <>
std::vector<float> SiPMRandom::randGaussianF<std::vector<float>>(const float mu, const float sigma, const uint32_t n) {
  SiPMVector<float> out = randGaussianF<SiPMVector<float>>(mu, sigma, n);
  return std::vector<float>(out.begin(), out.end());
}

/**
 * @param max Max value to generate
 * @param n Number of values to generate
 */
std::vector<uint32_t> SiPMRandom::randInteger(const uint32_t max, const uint32_t n) {
  std::vector<uint32_t> out(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = static_cast<uint32_t>(Rand() * max);
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
template <> SiPMVector<double> SiPMRandom::randExponential<SiPMVector<double>>(const double mu, const uint32_t n) {
  SiPMVector<double> out(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -log(Rand()) * mu;
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
template <> SiPMVector<float> SiPMRandom::randExponentialF<SiPMVector<float>>(const float mu, const uint32_t n) {
  SiPMVector<float> out(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -log(RandF()) * mu;
  }
  return out;
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
template <> std::vector<double> SiPMRandom::randExponential<std::vector<double>>(const double mu, const uint32_t n) {
  SiPMVector<double> out = randExponential<SiPMVector<double>>(mu, n);
  return std::vector<double>(out.begin(), out.end());
}

/**
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
template <> std::vector<float> SiPMRandom::randExponentialF<std::vector<float>>(const float mu, const uint32_t n) {
  SiPMVector<float> out = randExponentialF<SiPMVector<float>>(mu, n);
  return std::vector<float>(out.begin(), out.end());
}

} // namespace sipm
