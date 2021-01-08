#include <math.h>
#include <stdint.h>
#include <vector>

#ifndef SIPM_RANDOM_H
#define SIPM_RANDOM_H

namespace sipm {

namespace SiPMRng {
class Xorshift256plus {
public:
  Xorshift256plus() noexcept { seed(); }
  Xorshift256plus(uint64_t aseed) noexcept { seed(aseed); }
  inline uint64_t operator()() noexcept
    __attribute__((hot, optimize("Ofast", "fast-math")));
  void jump();
  void seed();
  void seed(uint64_t);

private:
  uint64_t s[4];
};

inline uint64_t Xorshift256plus::operator()() noexcept {
  const uint64_t result = s[0] + s[3];

  s[2] ^= s[0];
  s[2] ^= s[1] << 17;

  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[3] = (s[3] << 45U) | (s[3] >> (64U - 45U));
  return result;
}
} // namespace SiPMRng

class SiPMRandom {
public:
  SiPMRandom() = default;
  SiPMRandom(uint64_t aseed) noexcept { m_rng.seed(aseed); }
  void seed(uint64_t aseed) noexcept { m_rng.seed(aseed); }

  void seed() { m_rng.seed(); }
  void jump() { m_rng.jump(); }

  inline double Rand() __attribute__((hot, optimize("Ofast", "fast-math")));
  inline uint32_t randInteger(const uint32_t)
    __attribute__((hot, optimize("Ofast", "fast-math")));
  double randGaussian(const double, const double)
    __attribute__((hot, optimize("Ofast", "fast-math")));
  double randExponential(const double)
    __attribute__((hot, optimize("Ofast", "fast-math")));
  uint32_t randPoisson(const double mu)
    __attribute__((hot, optimize("Ofast", "fast-math")));

  std::vector<double> Rand(const uint32_t)
    __attribute__((hot, optimize("Ofast", "fast-math")));
  std::vector<double> randGaussian(const double, const double, const uint32_t)
    __attribute__((hot, optimize("Ofast", "fast-math")));
  std::vector<uint32_t> randInteger(const uint32_t max, const uint32_t n)
    __attribute__((hot, optimize("Ofast", "fast-math")));

private:
  SiPMRng::Xorshift256plus m_rng;
};

// Uniform random in [0,1)
inline double SiPMRandom::Rand() {
  return m_rng() / static_cast<double>(UINT64_MAX);
}

// Integer random in range [0,max]
inline uint32_t SiPMRandom::randInteger(const uint32_t max) {
  return m_rng() % max;
}

} // namespace sipm
#endif /* SIPM_RANDOM_H */
