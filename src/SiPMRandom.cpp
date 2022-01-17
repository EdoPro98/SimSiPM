#include "SiPMRandom.h"

namespace sipm {

namespace SiPMRng {
void Xorshift256plus::seed() {
  s[0] = std::random_device{}();
  s[1] = std::random_device{}();
  s[2] = std::random_device{}();
  s[3] = std::random_device{}();
}

void Xorshift256plus::seed(const uint64_t aseed) {
  s[0] = aseed;
  s[1] = aseed + 1;
  s[2] = aseed + 2;
  s[3] = aseed + 3;
}

void Xorshift256plus::jump() {
  static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
  uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;

  for (int i = 0; i < 4; ++i)
    for (int b = 0; b < 64; ++b) {
      if (JUMP[i] & 1UL << b) {
        s0 ^= s[0];
        s1 ^= s[1];
        s2 ^= s[2];
        s3 ^= s[3];
      }
      this->operator()();
    }

  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  s[3] = s3;
}
} // namespace SiPMRng

// SCALAR //

/** @brief Returns a value from a poisson distribution given its mean value.
 * @param mu Mean value of the poisson distribution
 */
uint32_t SiPMRandom::randPoisson(const double mu) {
  if (mu <= 0) {
    return 0;
  }
  const double q = exp(-mu);
  double p = 1;
  int32_t out = -1;

  while (p > q) {
    ++out;
    p *= Rand();
  }
  return out;
}

/** @brief Returns a value from a exponential distribution given its mean value.
 * @param mu Mean value of the exponential distribution
 */
double SiPMRandom::randExponential(const double mu) { return -log(Rand()) * mu; }

/**
*   @brief Samples a random number from the standard Normal (Gaussian) Distribution with the given mean and sigma.
*
* Uses the Acceptance-complement ratio from W. Hoermann and G. Derflinger
* This is one of the fastest existing method for generating normal random variables.
* It is a factor 2/3 faster than the polar (Box-Muller) method used in the previous
* version of TRandom::Gaus. The speed is comparable to the Ziggurat method (from Marsaglia)
* implemented for example in GSL and available in the MathMore library.
*
* REFERENCE:  - W. Hoermann and G. Derflinger (1990):
*              The ACR Method for generating normal random variables,
*              OR Spektrum 12 (1990), 181-185.
*
* Implementation taken from
* UNURAN (c) 2000  W. Hoermann & J. Leydold, Institut f. Statistik, WU Wien

* @param mu Mean value of the gaussian distribution
* @param sigma Standard deviation of the gaussian distribution
* @return Double value from gaussian distribution
*/
double SiPMRandom::randGaussian(const double mu, const double sigma) {
  constexpr double kC1 = 1.448242853;
  constexpr double kC2 = 3.307147487;
  constexpr double kC3 = 1.46754004;
  constexpr double kD1 = 1.036467755;
  constexpr double kD2 = 5.295844968;
  constexpr double kD3 = 3.631288474;
  constexpr double kHm = 0.483941449;
  constexpr double kZm = 0.107981933;
  constexpr double kHp = 4.132731354;
  constexpr double kZp = 18.52161694;
  constexpr double kPhln = 0.4515827053;
  constexpr double kHm1 = 0.516058551;
  constexpr double kHp1 = 3.132731354;
  constexpr double kHzm = 0.375959516;
  constexpr double kHzmp = 0.591923442;
  /*zhm 0.967882898*/

  constexpr double kAs = 0.8853395638;
  constexpr double kBs = 0.2452635696;
  constexpr double kCs = 0.2770276848;
  constexpr double kB = 0.5029324303;
  constexpr double kX0 = 0.4571828819;
  constexpr double kYm = 0.187308492;
  constexpr double kS = 0.7270572718;
  constexpr double kT = 0.03895759111;

  double result;
  double rn, x, y, z;

  do {
    y = Rand();

    if (y > kHm1) {
      result = kHp * y - kHp1;
      break;
    }

    else if (y < kZm) {
      rn = kZp * y - 1;
      result = (rn > 0) ? (1 + rn) : (-1 + rn);
      break;
    }

    else if (y < kHm) {
      rn = Rand();
      rn = rn - 1 + rn;
      z = (rn > 0) ? 2 - rn : -2 - rn;
      if ((kC1 - y) * (kC3 + abs(z)) < kC2) {
        result = z;
        break;
      } else {
        x = rn * rn;
        if ((y + kD1) * (kD3 + x) < kD2) {
          result = rn;
          break;
        } else if (kHzmp - y < exp(-(z * z + kPhln) * 0.5)) {
          result = z;
          break;
        } else if (y + kHzm < exp(-(x + kPhln) * 0.5)) {
          result = rn;
          break;
        }
      }
    }

    while (1) {
      x = Rand();
      y = kYm * Rand();
      z = kX0 - kS * x - y;
      if (z > 0)
        rn = 2 + y / x;
      else {
        x = 1 - x;
        y = kYm - y;
        rn = -(2 + y / x);
      }
      if ((y - kAs + x) * (kCs + x) + kBs < 0) {
        result = rn;
        break;
      } else if (y < x + kT)
        if (rn * rn < 4 * (kB - log(x))) {
          result = rn;
          break;
        }
    }
  } while (0);

  return mu + sigma * result;
}

/** @brief Returns a random integer in range [0,max)
 * @param max Maximum value of integer to generate
 */
uint32_t SiPMRandom::randInteger(const uint32_t max) { return static_cast<uint32_t>(Rand() * max); }

// VECTORS //

/**
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::Rand(const uint32_t n) {
  std::vector<double> out(n);
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = Rand();
  }
  return out;
}

/**
 * @param mu Mean value of the gaussuan
 * @param sigma Standard deviation value of the gaussuan
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::randGaussian(const double mu, const double sigma, const uint32_t n) {

  std::vector<double> out(n);
  std::vector<double> s(n);

  for (uint32_t i = 0; i < n - 1; i += 2) {
    double z, u, v;
    do {
      u = Rand() * 2.0 - 1.0;
      v = Rand() * 2.0 - 1.0;
      z = (u * u) + (v * v);
    } while (z > 1.0 || z == 0.0);
    s[i] = log(z) * math::rec(z);
    s[i + 1] = s[i];
    out[i] = u;
    out[i + 1] = v;
  }
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = math::sqrt(-2 * s[i]) * (out[i] * sigma) + mu;
  }
  // If n is odd we miss last value so recalculate it anyway
  out[n - 1] = randGaussian(mu, sigma);
  return out;
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

/** @brief Returns a vector from a exponential distribution given its mean value.
 * @param mu Mean value of the exponential distribution
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::randExponential(const double mu, const uint32_t n) {
  std::vector<double> out(n);
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = -log(Rand()) * mu;
  }
  return out;
}
} // namespace sipm
