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
#include <cstdint>
#include <stdint.h>
#include <vector>
#include <string.h>

#include "SiPMMath.h"
#include "SiPMTypes.h"

// Musl implementation of lcg64
static constexpr uint64_t lcg64(const uint64_t x) { return (x * 10419395304814325825ULL + 1) % -1ULL; }

namespace sipm {
namespace SiPMRng {
/// @brief Implementation of xoshiro256+ 1.0 PRNG algorithm
/** This is xoshiro256+ 1.0, our best and fastest generator for floating-point
 * numbers. We suggest to use its upper bits for floating-point
 * generation, as it is slightly faster than xoshiro256++/xoshiro256**. It
 * passes all tests we are aware of except for the lowest three bits,
 * which might fail linearity tests (and just those), so if low linear
 * complexity is not considered an issue (as it is usually the case) it
 * can be used to generate 64-bit outputs, too.
 *
 * We suggest to use a sign test to extract a random Boolean value, and
 * right shifts to extract subsets of bits.
 *
 * The state must be seeded so that it is not everywhere zero. If you have
 * a 64-bit seed, we suggest to seed a splitmix64 generator and use its
 * output to fill s. */
class Xorshift256plus {
public:
  /// @brief Default contructor for Xorshift256plus
  /// It cretates an instance of Xorhift256plus and sets the seed using 
  /// a 64 bit LCG and a random value from system randomd device
  Xorshift256plus() { seed(); }
  
  /// @brief Constructor with seed for Xorhift256plus
  /// It cretates an instance of Xorshift256plus and sets the seed 
  /// using a 64 bit LCG and the seed value provided
  /// @param sd uint64_t User provided seed. Must not be 0!
  Xorshift256plus(const uint64_t sd) { seed(sd); }

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
  } __attribute__((hot))

  /// @brief Jump function for the alghoritm.
  /**This is the jump function for the generator. It is equivalent
   * to 2^128 calls to next(); it can be used to generate 2^128
   * non-overlapping subsequences for parallel computations. */
  void jump();

  /// @brief Sets a random seed generated using system random device.
  void seed();

  /// @brief Manually set a seed
  void seed(const uint64_t);

  /// @brief Return internal state of rng.
  const uint64_t* getState() const { return s; }

private:
  alignas(64) uint64_t s[4];
};
} // namespace SiPMRng

class SiPMRandom {
public:
  SiPMRandom() = default;

  /// @brief Get a reference to the underlying PRNG */
  /// Use this method to seed the PRNG and to get the status
  SiPMRng::Xorshift256plus& rng() { return m_rng; }

  /// @brief Gives an uniformly distributed random double
  inline double Rand() noexcept;
  /// @brief Gives an uniformly distributed random float
  inline float RandF() noexcept;
  /// @brief Gives an uniform random integer
  uint32_t randInteger(const uint32_t) noexcept;

  /// @brief Gives random double with gaussian distribution
  double randGaussian(const double, const double) noexcept;
  /// @brief Gives random float with gaussian distribution
  float randGaussianF(const float, const float) noexcept;
  /// @brief Gives random double with exponential distribution
  double randExponential(const double) noexcept;
  /// @brief Gives random float with exponential distribution
  float randExponentialF(const float) noexcept;
  /// @brief Gives random value with poisson distribution
  uint32_t randPoisson(const double mu) noexcept;

  /// @brief Vector version of @ref Rand()
  template <typename T = std::vector<double>> T Rand(const uint32_t);
  /// @brief Vector version of @ref RandF()
  template <typename T = std::vector<float>> T RandF(const uint32_t);
  /// @brief Vector version of @ref randGaussian()
  template <typename T = std::vector<double>>
  T randGaussian(const double, const double, const uint32_t);
  template <typename T = std::vector<float>>
  /// @brief Vector version of @ref randGaussianF()
  T randGaussianF(const float, const float, const uint32_t);
  /// @brief Vector version of @ref randInteger()
  std::vector<uint32_t> randInteger(const uint32_t max, const uint32_t n);
  /// @brief Vector version of @ref randExponential()
  template <typename T = std::vector<double>> T randExponential(const double, const uint32_t);
  /// @brief Vector version of @ref randExponentialF()
  template <typename T = std::vector<float>> T randExponentialF(const float, const uint32_t);

private:
  SiPMRng::Xorshift256plus m_rng;
};

/**
 * This method uses the internal PRNG to get a random uint64
 * then sets its sign bit to 0 and the exponent bits are set to
 * 0x3fff. By aliasing the uint to a double and subtracting 1
 * the result is a random double in range (0-1].
 */
inline double SiPMRandom::Rand() noexcept {
  const uint64_t x = (0x3ffull << 52) | (m_rng() >> 12);
  return *(double*)(&x) - 1;
}

/**
 * This method uses the internal PRNG to get a random uint64
 * then sets its sign bit to 0 and the exponent bits are set to
 * 0x3f8. By aliasing the uint to a double and subtracting 1
 * the result is a random float in range (0-1].
 */
inline float SiPMRandom::RandF() noexcept {
  const uint32_t x = (0x3f8ul << 20) | (static_cast<uint32_t>(m_rng()) >> 8);
  return *(float*)(&x) - 1;
}
} // namespace sipm
#endif /* SIPM_RANDOM_H */
