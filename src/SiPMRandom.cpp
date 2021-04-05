#include "SiPMRandom.h"
#include <random>

namespace sipm {

namespace SiPMRng {
void Xorshift256plus::seed() {
  std::random_device rd;
  s[0] = rd();
  s[1] = rd();
  s[2] = rd();
  s[3] = rd();
  this->operator()();
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
}  // namespace SiPMRng

// SCALAR //

/** @brief Returns a value from a poisson distribution given its mean value.
 * @param mu Mean value of the poisson distribution
 */
uint32_t SiPMRandom::randPoisson(const double mu) {
  if (mu == 0) {
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


/** @brief Returns a value from a gaussian distribution given its mean value and
 * sigma.
 *
 * This function is based on Box-Muller algorithm for random gaussian generation.
 * @param mu Mean value of the gaussian distribution
 * @param sigma Standard deviation of the gaussian distribution
 */
double SiPMRandom::randGaussian(const double mu, const double sigma) {
  static double spare;
  static bool hasSpare = false;

  if (hasSpare) {
    hasSpare = false;
    return spare * sigma + mu;
  } else {
    double u, v, s;
    do {
      u = Rand() * 2.0 - 1.0;
      v = Rand() * 2.0 - 1.0;
      s = (u * u) + (v * v);
    } while (s >= 1.0 || s == 0.0);
    s = sqrt(log(s) * (-2. / s));
    spare = v * s;
    hasSpare = true;
    return u * s * sigma + mu;
  }
}


/** @brief Returns a random integer in range [0,max]
 * @param max Maximum value of integer to generate
 */
uint32_t SiPMRandom::randInteger(const uint32_t max) { return static_cast<uint32_t>(Rand() * (max + 1)); }

// VECTORS //

/**
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::Rand(const uint32_t n) {
  std::vector<double> out(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = m_rng();
  }
  for (uint32_t i = 0; i < n; ++i) {
    out[i] *= M_UINT64_MAX_RCP;
  }
  return out;
}


/**
 * @param mu Mean value of the gaussuan
 * @param sigma Standard deviation value of the gaussuan
 * @param n Number of values to generate
 */
std::vector<double> SiPMRandom::randGaussian(const double mu, const double sigma, const uint32_t n) {
  if (n == 0) {
    return {};
  }
  alignas(64) double out[n];
  alignas(64) double ss[n];
  alignas(64) double uu[n];

  for (uint32_t i = 0; i < n - 1; i += 2) {
    double s, u, v;
    do {
      u = Rand() * 2.0 - 1.0;
      v = Rand() * 2.0 - 1.0;
      s = (u * u) + (v * v);
    } while (s >= 1.0 || s == 0.0);

    ss[i] = sqrt(log(s) * (-2. / s));
    ss[i + 1] = ss[i];
    uu[i] = u;
    uu[i + 1] = v;
  }
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = ss[i] * uu[i] * sigma + mu;
  }
  std::vector<double> res(out, out + n);
  return res;
}


/**
 * @param max Max value to generate
 * @param n Number of values to generate
 */
std::vector<uint32_t> SiPMRandom::randInteger(const uint32_t max, const uint32_t n) {
  std::vector<uint32_t> out(n);

  for (uint32_t i = 0; i < n; ++i) {
    out[i] = static_cast<uint32_t>(Rand() * (max + 1));
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
}  // namespace sipm
