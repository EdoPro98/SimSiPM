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

#ifndef SIPM_RANDOM_H
#define SIPM_RANDOM_H

#include <cmath>
#include <cstring>
#include <random>
#include <stdint.h>
#include <vector>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#include "SiPMMath.h"

namespace sipm {

namespace SiPMRng {

/// @brief Implementation of xoshiro256++ 1.0 PRNG algorithm
/** Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)
 *
 * To the extent possible under law, the author has dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 * This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
 * It has excellent (sub-ns) speed, a state (256 bits) that is large
 * enough for any parallel application, and it passes all tests we are
 * aware of.
 *
 * For generating just floating-point numbers, xoshiro256+ is even faster.
 * The state must be seeded so that it is not everywhere zero. If you have
 * a 64-bit seed, we suggest to seed a splitmix64 generator and use its
 * output to fill s. */
class Xorshift256plus {
public:
  /// @brief Default contructor for Xorshift256plus
  Xorshift256plus() { seed(); }
  /// @brief Contructor for Xorshift256plus given a seed value
  Xorshift256plus(uint64_t aseed) { seed(aseed); }
  /// @brief Returns a pseud-random 64-bits intger
  inline uint64_t operator()() noexcept {
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
  __attribute__((hot))

  /// @brief Jump function for the alghoritm.
  /** Usefull in case the same generator is used in multiple instancies. The
   * jump function will make sure that pseudo-random values generated from the
   * different instancies are uncorrelated.
   */
  void
  jump();
  /// @brief Sets a random seed generated with rand()
  void seed();
  /// @brief Sets a new seed
  void seed(uint64_t);

private:
  alignas(64) uint64_t s[4];
};

} // namespace SiPMRng

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
   * to 2^128 calls to next(); it can be used to generate 2^128
   * non-overlapping subsequences for parallel computations.*/
  void jump() { m_rng.jump(); }

  inline uint64_t operator()() { return m_rng(); }

  // Uniform random in [0-1]
  inline double Rand() __attribute__((hot));

  // Uniform integer in range [0-max)
  uint32_t randInteger(const uint32_t) __attribute__((hot));
  // Random gaussian given mean and sigma
  double randGaussian(const double, const double) __attribute__((hot));
  // Random exponential given mean
  double randExponential(const double);
  // Random poisson given mean
  uint32_t randPoisson(const double mu);

  /** @brief Vector of random uniforms in [0-1] */
  std::vector<double> Rand(const uint32_t) __attribute__((hot));
  /** @brief Vector of random gaussian given mean an sigma */
  std::vector<double> randGaussian(const double, const double, const uint32_t) __attribute__((hot));
  /** @brief Vector of random integers in range [0-max) */
  std::vector<uint32_t> randInteger(const uint32_t max, const uint32_t n) __attribute__((hot));
  // Vector of random exponential given mean
  std::vector<double> randExponential(const double, const uint32_t);

private:
  SiPMRng::Xorshift256plus m_rng;
};

/** Returns a uniform random in range (0,1)
 * Using getting highest 53 bits from a unit64 for the mantissa,
 * setting the exponent to get values in range (1-2), subtract 1 and type punning
 * to double.
 */
inline double SiPMRandom::Rand() {
  double x;
  static constexpr uint64_t mask = 0x3ff0000000000000;
  const uint64_t u = (m_rng() >> 12) | mask;
  std::memcpy(&x, &u, 8);
  return x - 1;
}
} // namespace sipm
#endif /* SIPM_RANDOM_H */
