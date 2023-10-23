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
  Xorshift256plus(const uint64_t sd) { seed(sd); }

  /// @brief Sets a random seed generated using system random device.
  void seed();

  /// @brief Manually set a seed
  void seed(const uint64_t);

  /// @brief Return internal state of rng.
  const uint64_t* getState() const { return s; }
  const uint64_t* getStateSimd() const { return (uint64_t*)__s; }

private:
  alignas(64) uint64_t s[4];
#ifdef __AVX512F__
  __m512i __s[4];
#else
  alignas(64) uint64_t __s[4][8];
#endif

public:
  /// @brief Returns a pseud-random 64-bits integer
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

#ifdef __AVX512F__
  // Returns a minimum of 8 uint64_t in a ptr
  inline uint64_t* simd8(const uint32_t n) noexcept {
    uint64_t* result = (uint64_t*)aligned_alloc(64, sizeof(uint64_t) * n);

    for (uint32_t i = 0; i < n - 8; i += 8) {
      const __m512i __result = _mm512_add_epi64(__s[0], __s[3]);
      _mm512_store_epi64(result + i, __result);

      const __m512i __t = _mm512_slli_epi64(__s[1], 17);

      __s[2] = _mm512_xor_epi64(__s[2], __s[0]);
      __s[3] = _mm512_xor_epi64(__s[3], __s[1]);
      __s[1] = _mm512_xor_epi64(__s[1], __s[2]);
      __s[0] = _mm512_xor_epi64(__s[0], __s[3]);

      __s[2] = _mm512_xor_epi64(__s[2], __t);

      const __m512i __temp1 = _mm512_slli_epi64(__s[3], 45);
      const __m512i __temp2 = _mm512_srli_epi64(__s[3], 64 - 45);
      __s[3] = _mm512_or_si512(__temp1, __temp2);
    }

    // Save scalar generator state
    uint64_t curr_state[4];
    memcpy(curr_state, s, 4 * sizeof(uint64_t));
    // Tail loop
    for (uint32_t i = n - 8; i < n; ++i) {
      result[i] = this->operator()();
    }
    // Restore previous state
    memcpy(s, curr_state, 4 * sizeof(uint64_t));

    // Returning a raw ptr. Passing the resposibility to free to caller
    return result;
  }
#else
  // Returns a minimum of 8 uint64_t in a ptr
  inline uint64_t* simd8(const uint32_t n) {
    uint64_t* result = (uint64_t*)aligned_alloc(64, sizeof(uint64_t) * n);

    for (uint32_t i = 0; i < n - 8; i += 8) {
      uint64_t t[8];

      for (int j = 0; j < 8; ++j) {
        result[i + j] = __s[1][j] + __s[3][j];
      }

      for (int j = 0; j < 8; ++j) {
        t[j] = __s[1][j] << 17;
      }

      for (int j = 0; j < 8; ++j) {
        __s[2][j] ^= __s[0][j];
        __s[3][j] ^= __s[1][j];
        __s[1][j] ^= __s[2][j];
        __s[0][j] ^= __s[3][j];

        __s[2][j] ^= t[j];
      }

      for (int j = 0; j < 8; ++j) {
        __s[3][j] = (__s[3][j] << 45) | (__s[3][j] >> (64 - 45));
      }
    }
    // Store scalar generator state
    uint64_t curr_state[4];
    std::memcpy(curr_state, s, 4 * sizeof(uint64_t));
    // Tail loop
    for (uint32_t i = n - 8; i < n; ++i) {
      result[i] = this->operator()();
    }
    // Restore previous state
    std::memcpy(s, curr_state, 4 * sizeof(uint64_t));
    // Returning a raw ptr. Passing the resposibility to free to caller
    return result;
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

  /// @brief Gives an uniformly distributed random
  template <typename T = double>
  inline T Rand() noexcept;
  pair<float, float> RandF2() noexcept;
  /// @brief Gives an uniform random integer
  uint32_t randInteger(const uint32_t) noexcept;

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
  return static_cast<double>(m_rng() & ((1ULL << 53) - 1)) * 0x1p-53;
}

/**
 * This method uses the internal PRNG to get a random uint64
 * then sets its sign bit to 0 and the exponent bits are set to
 * 0x3f8. By aliasing the uint to a double and subtracting 1
 * the result is a random float in range (0-1].
 */
template <>
inline float SiPMRandom::Rand<float>() noexcept {
  return static_cast<float>(m_rng() & ((1ULL << 24) - 1)) * 0x1p-24;
}
} // namespace sipm
#endif /* SIPM_RANDOM_H */
