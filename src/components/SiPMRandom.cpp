#include "SiPMRandom.h"

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace sipm {

namespace SiPMRng {

void Xorshift256plus::jump() {
  static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c,
                                  0xa9582618e03fc9aa, 0x39abdc4529b1661c};
  uint64_t s0 = 0;
  uint64_t s1 = 0;
  uint64_t s2 = 0;
  uint64_t s3 = 0;

  for (int i = 0; i < 4; ++i)
    for (int b = 0; b < 64; b++) {
      if (JUMP[i] & UINT64_C(1) << b) {
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

void Xorshift256plus::seed() {
  s[0] = rand();
  s[1] = rand();
  s[2] = rand();
  s[3] = rand();
}

void Xorshift256plus::seed(uint64_t aseed) {
  s[0] = aseed;
  s[1] = aseed + 1;
  s[2] = aseed + 2;
  s[3] = aseed + 3;
}
} // namespace SiPMRng

// Poisson random with mean value "mu"
uint32_t SiPMRandom::randPoisson(const double mu) {
  if (mu == 0) { return 0; }
  const double q = exp(-mu);
  double p = 1;
  int32_t out = -1;

  while (p > q) {
    ++out;
    p *= Rand();
  }
  return out;
}

// Exponential random with mean value "mu"
double SiPMRandom::randExponential(const double mu) {
  return -log(Rand()) * mu;
}

// Gaussian random value with mean "mu" and sigma "sigma"
// Using Box-Muller transform
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
      s = u * u + v * v;
    } while (s >= 1.0 || s == 0.0);
    s = sqrt(-2.0 * log(s) / s);
    spare = v * s;
    hasSpare = true;
    return u * s * sigma + mu;
  }
}

std::vector<double> SiPMRandom::Rand(const uint32_t n) {
  std::vector<double> out(n);

  for (int i = 0; i < n; ++i) {
    out[i] = m_rng() / static_cast<double>(UINT64_MAX);
  }
  return out;
}

// Generate n random gaussian values with mean mu and std sigma
std::vector<double> SiPMRandom::randGaussian(const double mu,
                                             const double sigma,
                                             const uint32_t n) {
  if (n == 0) { return {}; }

  std::vector<double> out(n);

  for (uint32_t i = 0; i < n - 1; ++i) {
    double s, u, v;
    do {
      u = Rand() * 2.0 - 1.0;
      v = Rand() * 2.0 - 1.0;
      s = u * u + v * v;
    } while (s >= 1 || s == 0.0);
    s = sqrt(-2 * log(s) / s);
    out[i] = u * s;
    ++i;
    out[i] = v * s;
  }

  // for(uint32_t i=0;i<n-1;++i){
  //   double u = sqrt(-2*log(Rand()));
  //   double v = 2*M_PI * Rand();
  //   out[i] = u * sin(v);
  //   ++i;
  //   out[i] = u * cos(v);
  // }

  for (uint32_t i = 0; i < n; ++i) { out[i] = out[i] * sigma + mu; }
  return out;
}

// Generate random integers in range [0 - max]
std::vector<uint32_t> SiPMRandom::randInteger(const uint32_t max,
                                              const uint32_t n) {
  std::vector<uint32_t> out(n);

  for (int i = 0; i < n; ++i) { out[i] = m_rng() % max; }
  return out;
}
} // namespace sipm
