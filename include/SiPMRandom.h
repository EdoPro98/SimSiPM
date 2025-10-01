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

#include <array>
#include <cstddef>
#include <cstdlib>
#ifdef __AVX512F__
#include <immintrin.h>
#endif

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "SiPMTypes.h"

// Musl implementation of lcg64
// Keeping this function limited to this translation unit
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
  explicit Xorshift256plus(const uint64_t sd) { seed(sd); }

  /// @brief Sets a random seed generated using system random device.
  void seed();

  /// @brief Manually set a seed
  void seed(const uint64_t);

private:
  // Generator state
  alignas(64) uint64_t s[4];
  // Size of the buffer
  static constexpr uint32_t N = 1 << 16;
  // Buffer for random values
  alignas(64) uint64_t buffer[N];
  uint32_t index = N;
#ifdef __AVX512F__
  // Generator state for AVX512
  __m512i __s[4];
#endif

public:
  /// @brief Returns a pseud-random 64-bits integer
  inline uint64_t operator()() noexcept {
    if (index == N) {
      getRand(buffer, N);
      index = 0;
    }
    return buffer[index++];
  }

#ifdef __AVX512F__
  // Overload for uint64_t
  inline void getRand(uint64_t* __restrict array, const uint32_t n) noexcept {
    size_t i = 0;
    // Generate 8 uint64_t values per iteration
    for (; i + 8 <= n; i += 8) {
      const __m512i __result = _mm512_add_epi64(__s[0], __s[3]);
      _mm512_store_si512(array + i, __result);

      const __m512i __t = _mm512_slli_epi64(__s[1], 17);

      __s[2] = _mm512_xor_epi64(__s[2], __s[0]);
      __s[3] = _mm512_xor_epi64(__s[3], __s[1]);
      __s[1] = _mm512_xor_epi64(__s[1], __s[2]);
      __s[0] = _mm512_xor_epi64(__s[0], __s[3]);

      __s[2] = _mm512_xor_si512(__s[2], __t);

      __s[3] = _mm512_rol_epi64(__s[3], 45);
    }
    // Handle leftover if n is not a multiple of 8
    if (i < n) {
      alignas(64) uint64_t buffer[8];
      const __m512i __result = _mm512_add_epi64(__s[0], __s[3]);
      _mm512_store_si512(buffer, __result);

      const __m512i __t = _mm512_slli_epi64(__s[1], 17);

      __s[2] = _mm512_xor_si512(__s[2], __s[0]);
      __s[3] = _mm512_xor_si512(__s[3], __s[1]);
      __s[1] = _mm512_xor_si512(__s[1], __s[2]);
      __s[0] = _mm512_xor_si512(__s[0], __s[3]);

      __s[2] = _mm512_xor_si512(__s[2], __t);

      __s[3] = _mm512_rol_epi64(__s[3], 45);
      for (size_t j = 0; j < n - i; ++j) {
        array[i + j] = buffer[j];
      }
    }
  }

  // Overload for uint32_t
  inline void getRand(uint32_t* array, const uint32_t n) noexcept {
    size_t i = 0;

    // Generate 8 uint64_t values per iteration and split them into 16 uint32_t values
    for (; i + 16 <= n; i += 16) {
      const __m512i __result = _mm512_add_epi64(__s[0], __s[3]);

      _mm512_store_si512(array + i, __result);

      const __m512i __t = _mm512_slli_epi64(__s[1], 17);

      __s[2] = _mm512_xor_si512(__s[2], __s[0]);
      __s[3] = _mm512_xor_si512(__s[3], __s[1]);
      __s[1] = _mm512_xor_si512(__s[1], __s[2]);
      __s[0] = _mm512_xor_si512(__s[0], __s[3]);

      __s[2] = _mm512_xor_si512(__s[2], __t);

      __s[3] = _mm512_rol_epi64(__s[3], 45);
    }

    // Handle leftover if n is not a multiple of 16
    if (i < n) {
      alignas(64) uint32_t buffer[16];
      const __m512i __result = _mm512_add_epi64(__s[0], __s[3]);
      _mm512_store_si512(buffer, __result);

      const __m512i __t = _mm512_slli_epi64(__s[1], 17);

      __s[2] = _mm512_xor_si512(__s[2], __s[0]);
      __s[3] = _mm512_xor_si512(__s[3], __s[1]);
      __s[1] = _mm512_xor_si512(__s[1], __s[2]);
      __s[0] = _mm512_xor_si512(__s[0], __s[3]);

      __s[2] = _mm512_xor_si512(__s[2], __t);

      __s[3] = _mm512_rol_epi64(__s[3], 45);

      // Fill the remaining uint32_t values
      for (size_t j = 0; j < n - i; ++j) {
        array[i + j] = buffer[j];
      }
    }
  }
#else

  // Overload for uint64_t (most of the times this one is used)
  inline void getRand(uint64_t* __restrict array, const uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) {
      const uint64_t randVal = s[0] + s[3];
      const uint64_t t = s[1] << 17;
      s[2] ^= s[0];
      s[3] ^= s[1];
      s[1] ^= s[2];
      s[0] ^= s[3];

      s[2] ^= t;
      s[3] = (s[3] << 45) | (s[3] >> (64 - 45));

      array[i] = randVal;
    }
  }

  // Overload for uint32_t
  inline void getRand(uint32_t* __restrict array, const uint32_t n) {
    for (uint32_t i = 0; i < n; i += 2) {
      const uint64_t randVal = s[0] + s[3];
      const uint64_t t = s[1] << 17;
      s[2] ^= s[0];
      s[3] ^= s[1];
      s[1] ^= s[2];
      s[0] ^= s[3];

      s[2] ^= t;
      s[3] = (s[3] << 45) | (s[3] >> (64 - 45));

      array[i] = static_cast<uint32_t>(randVal); // Lower 32 bits
      if (i + 1 < n) {
        array[i + 1] = static_cast<uint32_t>(randVal >> 32); // Upper 32 bits
      }
    }
  }
#endif
};
} // namespace SiPMRng

class SiPMRandom {
public:
  SiPMRandom() = default;

  /// @brief Get a reference to the underlying PRNG */
  /// Use this method to seed the PRNG and to get the status
  SiPMRng::Xorshift256plus& rng() { return m_rng; }

  // Seed underlying rng
  void seed(const uint64_t x) { m_rng.seed(x); }

  /// @brief Gives an uniformly distributed random
  template <typename T = double>
  inline T Rand() noexcept;
  pair<float, float> RandF2() noexcept;
  /// @brief Gives an uniform random integer
  uint32_t randInteger(const uint32_t) noexcept;
  pair<uint32_t> randInteger2(const uint32_t) noexcept;

  /// @brief Gives random value with gaussian distribution
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
  std::vector<double> Rand(const uint32_t);
  /// @brief Vector version of @ref RandF()
  std::vector<float> RandF(const uint32_t);
  /// @brief Vector version of @ref randGaussian()
  std::vector<double> randGaussian(const double, const double, const uint32_t);
  /// @brief Vector version of @ref randGaussianF()
  std::vector<float> randGaussianF(const float, const float, const uint32_t);
  /// @brief Vector version of @ref randInteger()
  std::vector<uint32_t> randInteger(const uint32_t max, const uint32_t n);
  /// @brief Vector version of @ref randExponential()
  std::vector<double> randExponential(const double, const uint32_t);
  /// @brief Vector version of @ref randExponentialF()
  std::vector<float> randExponentialF(const float, const uint32_t);

private:
  SiPMRng::Xorshift256plus m_rng;
};

/**
 * This method uses the internal PRNG to get a random uint64
 * then sets its sign bit to 0 and the exponent bits are set to
 * 0x3fff. By aliasing the uint to a double and subtracting 1
 * the result is a random double in range (0-1].
 */
template <>
inline double SiPMRandom::Rand<double>() noexcept {
  return (m_rng() >> 11) * 0x1p-53;
}

/**
 * This method uses the internal PRNG to get a random uint64
 * then sets its sign bit to 0 and the exponent bits are set to
 * 0x3f8. By aliasing the uint to a double and subtracting 1
 * the result is a random float in range (0-1].
 */
template <>
inline float SiPMRandom::Rand<float>() noexcept {
  return (m_rng() >> 40) * 0x1p-24f;
}
} // namespace sipm
#endif /* SIPM_RANDOM_H */
