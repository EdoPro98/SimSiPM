/** @class sipm::SiPMRandom SimSiPM/src/components/SiPMRandom.h SiPMRandom.h
 *
 * @brief Class for random number generation.
 *
 * Class used for random number generation. The simulation needs very fast
 * pseudo-random number generation, Xorhift256+ algorithm is used as it is one
 * of the fastest considering modern x86-64 architectures.
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */

/** @class sipm::SiPMRng::Xorshift256plus SimSiPM/src/components/SiPMRandom.h
 * SiPMRandom.h
 *
 * @brief Implementation of Xorshift256+ algorithm
 *
 *  @author Edoardo Proserpio
 *  @date 2020
 */
#include <vector>

#include <math.h>
#include <stdint.h>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifndef SIPM_RANDOM_H
#define SIPM_RANDOM_H

namespace sipm {

namespace SiPMRng {
class Xorshift256plus {
public:
  /// @brief Default contructor for Xorshift256plus
  Xorshift256plus() noexcept { seed(); }
  /// @brief Contructor for Xorshift256plus given a seed value
  Xorshift256plus(uint64_t aseed) noexcept { seed(aseed); }
  /// @brief Returns a pseud-random 64-bits intger
  inline uint64_t operator()() noexcept __attribute__((hot));
  /// @brief Jump function for the alghoritm.
  /** Usefull in case the same generator is used in multiple instancies. The
   * jump function will make sure that pseudo-random values generated from the
   * different instancies are uncorrelated.
   */
  void jump();
  /// @brief Sets a random seed generated with rand()
  void seed();
  /// @brief Sets a new seed
  void seed(uint64_t);

private:
  uint64_t s[4];
};

inline uint64_t Xorshift256plus::operator()() noexcept {
  const uint64_t result = s[0] + s[3];

  const uint64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = (s[3] << 45U) | (s[3] >> (64U - 45U));
  return result;
}

}  // namespace SiPMRng

class SiPMRandom {
public:
  SiPMRandom() = default;
  SiPMRandom(uint64_t aseed) noexcept { m_rng.seed(aseed); }

  /** @brief Sets a seed for the rng.
   * @param aSeed Seed used to initialize the rng algorithm
   */
  void seed(const uint64_t aSeed) { m_rng.seed(aSeed); }
  /** @brief Sets a seed for the rng obtained from rand().*/
  void seed() { m_rng.seed(); }
  /** @brief This is the jump function for the generator. It is equivalent
   to 2^128 calls to next(); it can be used to generate 2^128
   non-overlapping subsequences for parallel computations.*/
  void jump() { m_rng.jump(); }

  // Uniform random in [0-1]
  inline double Rand() __attribute__((hot));

  // Uniform integer in range [0-max]
  inline uint32_t randInteger(const uint32_t) __attribute__((hot));
  // Random gaussian given mean and sigma
  double randGaussian(const double, const double) __attribute__((hot));
  // Random gaussian given mean and sigma
  double randNormal() __attribute__((hot));
  // Random exponential given mean
  double randExponential(const double) __attribute__((hot));
  // Random poisson given mean
  uint32_t randPoisson(const double mu) __attribute__((hot));

  /** @brief Vector of random uniforms in [0-1] */
  std::vector<double> Rand(const uint32_t) __attribute__((hot));
  /** @brief Vector of random gaussian given mean an sigma */
  std::vector<double> randGaussian(const double, const double, const uint32_t) __attribute__((hot));
  /** @brief Vector of random integers in range [0-max] */
  std::vector<uint32_t> randInteger(const uint32_t max, const uint32_t n) __attribute__((hot));

private:
  SiPMRng::Xorshift256plus m_rng;
  static constexpr double M_UINT64_MAX_RCP = 1 / static_cast<double>(UINT64_MAX);
};

/** Returns a uniform random in range [0,1] */
inline double SiPMRandom::Rand() { return m_rng() * M_UINT64_MAX_RCP; }

/** @brief Returns a random integer in range [0,max]
 * @param max Maximum value of integer to generate
 */
inline uint32_t SiPMRandom::randInteger(const uint32_t max) { return static_cast<uint32_t>(Rand() * (max + 1)); }

}  // namespace sipm
#endif /* SIPM_RANDOM_H */

// ABOUT XOROSHIRO256+
/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */
