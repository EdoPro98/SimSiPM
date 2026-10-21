/** @class sipm::SiPMRandom SiPMRandom.h
 *
 * @brief Class for random number generation.
 *
 * Class used for random number generation. The simulation needs very fast
 * pseudo-random number generation, xoshiro256+ algorithm is used as it is one
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
#if defined(__AVX512F__) || defined(__AVX2__)
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
  /// It creates an instance of Xorshift256plus and sets the seed using
  /// a 64 bit LCG and a random value from the system random device
  Xorshift256plus() { seed(); }

  /// @brief Constructor with seed for Xorshift256plus
  /// It creates an instance of Xorshift256plus and sets the seed
  /// using a 64 bit LCG and the seed value provided
  /// @param sd uint64_t User provided seed. Must not be 0!
  explicit Xorshift256plus(const uint64_t sd) { seed(sd); }

  /// @brief Sets a random seed generated using system random device.
  void seed();

  /// @brief Manually set a seed
  /// @param sd Seed, must not be 0
  void seed(const uint64_t sd);

private:
  // Generator state
  alignas(64) uint64_t s[4];  ///< Generator state
  // Size of the buffer
  static constexpr uint32_t N = 1 << 12;  ///< Size of the buffer
  // Buffer for random values
  alignas(64) uint64_t buffer[N];  ///< Buffer of random values
  uint32_t index = N;  ///< Next unused value of the buffer
#ifdef __AVX512F__
  __m512i __sA[4];
  __m512i __sB[4];
#elif defined(__AVX2__)
  __m256i __sA[4];
  __m256i __sB[4];
#endif

public:
  /// @brief Returns a pseudo-random 64-bits integer
  /// Values come from a buffer that is refilled (vectorized when AVX2/AVX512 are available) when empty.
  /// @return A uniformly distributed 64-bits integer
  inline uint64_t operator()() noexcept {
    if (index == N) {
      getRand(buffer, N);
      index = 0;
    }
    return buffer[index++];
  }

#ifdef __AVX512F__
  // Overload for uint64_t — two independent states, 16 values per iteration
  /// @brief Fills an array with 64-bits pseudo-random integers
  /// @param array Destination, any alignment
  /// @param n Number of values to generate
  inline void getRand(uint64_t* __restrict array, const uint32_t n) noexcept {
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
      const __m512i __rA = _mm512_add_epi64(__sA[0], __sA[3]);
      const __m512i __rB = _mm512_add_epi64(__sB[0], __sB[3]);
      // array is caller-supplied (e.g. std::vector::data()) and not guaranteed
      // 64-byte aligned, so an unaligned store is required here.
      _mm512_storeu_si512(array + i,     __rA);
      _mm512_storeu_si512(array + i + 8, __rB);

      const __m512i __tA = _mm512_slli_epi64(__sA[1], 17);
      const __m512i __tB = _mm512_slli_epi64(__sB[1], 17);

      __sA[2] = _mm512_xor_si512(__sA[2], __sA[0]);
      __sB[2] = _mm512_xor_si512(__sB[2], __sB[0]);
      __sA[3] = _mm512_xor_si512(__sA[3], __sA[1]);
      __sB[3] = _mm512_xor_si512(__sB[3], __sB[1]);
      __sA[1] = _mm512_xor_si512(__sA[1], __sA[2]);
      __sB[1] = _mm512_xor_si512(__sB[1], __sB[2]);
      __sA[0] = _mm512_xor_si512(__sA[0], __sA[3]);
      __sB[0] = _mm512_xor_si512(__sB[0], __sB[3]);
      __sA[2] = _mm512_xor_si512(__sA[2], __tA);
      __sB[2] = _mm512_xor_si512(__sB[2], __tB);
      __sA[3] = _mm512_rol_epi64(__sA[3], 45);
      __sB[3] = _mm512_rol_epi64(__sB[3], 45);
    }
    if (i < n) {
      alignas(64) uint64_t buf[16];
      const __m512i __rA = _mm512_add_epi64(__sA[0], __sA[3]);
      const __m512i __rB = _mm512_add_epi64(__sB[0], __sB[3]);
      _mm512_store_si512(buf,     __rA);
      _mm512_store_si512(buf + 8, __rB);

      const __m512i __tA = _mm512_slli_epi64(__sA[1], 17);
      const __m512i __tB = _mm512_slli_epi64(__sB[1], 17);

      __sA[2] = _mm512_xor_si512(__sA[2], __sA[0]);
      __sB[2] = _mm512_xor_si512(__sB[2], __sB[0]);
      __sA[3] = _mm512_xor_si512(__sA[3], __sA[1]);
      __sB[3] = _mm512_xor_si512(__sB[3], __sB[1]);
      __sA[1] = _mm512_xor_si512(__sA[1], __sA[2]);
      __sB[1] = _mm512_xor_si512(__sB[1], __sB[2]);
      __sA[0] = _mm512_xor_si512(__sA[0], __sA[3]);
      __sB[0] = _mm512_xor_si512(__sB[0], __sB[3]);
      __sA[2] = _mm512_xor_si512(__sA[2], __tA);
      __sB[2] = _mm512_xor_si512(__sB[2], __tB);
      __sA[3] = _mm512_rol_epi64(__sA[3], 45);
      __sB[3] = _mm512_rol_epi64(__sB[3], 45);

      for (size_t j = 0; j < n - i; ++j) array[i + j] = buf[j];
    }
  }

  // Overload for uint32_t — two independent states, 32 values per iteration
  /// @brief Fills an array with 32-bits pseudo-random integers
  /// @param array Destination, any alignment
  /// @param n Number of values to generate
  inline void getRand(uint32_t* array, const uint32_t n) noexcept {
    size_t i = 0;
    for (; i + 32 <= n; i += 32) {
      const __m512i __rA = _mm512_add_epi64(__sA[0], __sA[3]);
      const __m512i __rB = _mm512_add_epi64(__sB[0], __sB[3]);
      // array is caller-supplied (e.g. std::vector::data()) and not guaranteed
      // 64-byte aligned, so an unaligned store is required here.
      _mm512_storeu_si512(array + i,      __rA);
      _mm512_storeu_si512(array + i + 16, __rB);

      const __m512i __tA = _mm512_slli_epi64(__sA[1], 17);
      const __m512i __tB = _mm512_slli_epi64(__sB[1], 17);

      __sA[2] = _mm512_xor_si512(__sA[2], __sA[0]);
      __sB[2] = _mm512_xor_si512(__sB[2], __sB[0]);
      __sA[3] = _mm512_xor_si512(__sA[3], __sA[1]);
      __sB[3] = _mm512_xor_si512(__sB[3], __sB[1]);
      __sA[1] = _mm512_xor_si512(__sA[1], __sA[2]);
      __sB[1] = _mm512_xor_si512(__sB[1], __sB[2]);
      __sA[0] = _mm512_xor_si512(__sA[0], __sA[3]);
      __sB[0] = _mm512_xor_si512(__sB[0], __sB[3]);
      __sA[2] = _mm512_xor_si512(__sA[2], __tA);
      __sB[2] = _mm512_xor_si512(__sB[2], __tB);
      __sA[3] = _mm512_rol_epi64(__sA[3], 45);
      __sB[3] = _mm512_rol_epi64(__sB[3], 45);
    }
    if (i < n) {
      alignas(64) uint32_t buf[32];
      const __m512i __rA = _mm512_add_epi64(__sA[0], __sA[3]);
      const __m512i __rB = _mm512_add_epi64(__sB[0], __sB[3]);
      _mm512_store_si512(buf,      __rA);
      _mm512_store_si512(buf + 16, __rB);

      const __m512i __tA = _mm512_slli_epi64(__sA[1], 17);
      const __m512i __tB = _mm512_slli_epi64(__sB[1], 17);

      __sA[2] = _mm512_xor_si512(__sA[2], __sA[0]);
      __sB[2] = _mm512_xor_si512(__sB[2], __sB[0]);
      __sA[3] = _mm512_xor_si512(__sA[3], __sA[1]);
      __sB[3] = _mm512_xor_si512(__sB[3], __sB[1]);
      __sA[1] = _mm512_xor_si512(__sA[1], __sA[2]);
      __sB[1] = _mm512_xor_si512(__sB[1], __sB[2]);
      __sA[0] = _mm512_xor_si512(__sA[0], __sA[3]);
      __sB[0] = _mm512_xor_si512(__sB[0], __sB[3]);
      __sA[2] = _mm512_xor_si512(__sA[2], __tA);
      __sB[2] = _mm512_xor_si512(__sB[2], __tB);
      __sA[3] = _mm512_rol_epi64(__sA[3], 45);
      __sB[3] = _mm512_rol_epi64(__sB[3], 45);

      for (size_t j = 0; j < n - i; ++j) array[i + j] = buf[j];
    }
  }
#elif defined(__AVX2__)

  // Overload for uint64_t — two independent states, 8 values per iteration
  /// @brief Fills an array with 64-bits pseudo-random integers
  /// @param array Destination, any alignment
  /// @param n Number of values to generate
  inline void getRand(uint64_t* __restrict array, const uint32_t n) noexcept {
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
      const __m256i __rA = _mm256_add_epi64(__sA[0], __sA[3]);
      const __m256i __rB = _mm256_add_epi64(__sB[0], __sB[3]);
      _mm256_storeu_si256((__m256i*)(array + i),     __rA);
      _mm256_storeu_si256((__m256i*)(array + i + 4), __rB);

      const __m256i __tA = _mm256_slli_epi64(__sA[1], 17);
      const __m256i __tB = _mm256_slli_epi64(__sB[1], 17);

      __sA[2] = _mm256_xor_si256(__sA[2], __sA[0]);
      __sB[2] = _mm256_xor_si256(__sB[2], __sB[0]);
      __sA[3] = _mm256_xor_si256(__sA[3], __sA[1]);
      __sB[3] = _mm256_xor_si256(__sB[3], __sB[1]);
      __sA[1] = _mm256_xor_si256(__sA[1], __sA[2]);
      __sB[1] = _mm256_xor_si256(__sB[1], __sB[2]);
      __sA[0] = _mm256_xor_si256(__sA[0], __sA[3]);
      __sB[0] = _mm256_xor_si256(__sB[0], __sB[3]);
      __sA[2] = _mm256_xor_si256(__sA[2], __tA);
      __sB[2] = _mm256_xor_si256(__sB[2], __tB);
      __sA[3] = _mm256_or_si256(_mm256_slli_epi64(__sA[3], 45), _mm256_srli_epi64(__sA[3], 19));
      __sB[3] = _mm256_or_si256(_mm256_slli_epi64(__sB[3], 45), _mm256_srli_epi64(__sB[3], 19));
    }
    if (i < n) {
      alignas(32) uint64_t buf[8];
      const __m256i __rA = _mm256_add_epi64(__sA[0], __sA[3]);
      const __m256i __rB = _mm256_add_epi64(__sB[0], __sB[3]);
      _mm256_store_si256((__m256i*)buf,     __rA);
      _mm256_store_si256((__m256i*)(buf+4), __rB);

      const __m256i __tA = _mm256_slli_epi64(__sA[1], 17);
      const __m256i __tB = _mm256_slli_epi64(__sB[1], 17);

      __sA[2] = _mm256_xor_si256(__sA[2], __sA[0]);
      __sB[2] = _mm256_xor_si256(__sB[2], __sB[0]);
      __sA[3] = _mm256_xor_si256(__sA[3], __sA[1]);
      __sB[3] = _mm256_xor_si256(__sB[3], __sB[1]);
      __sA[1] = _mm256_xor_si256(__sA[1], __sA[2]);
      __sB[1] = _mm256_xor_si256(__sB[1], __sB[2]);
      __sA[0] = _mm256_xor_si256(__sA[0], __sA[3]);
      __sB[0] = _mm256_xor_si256(__sB[0], __sB[3]);
      __sA[2] = _mm256_xor_si256(__sA[2], __tA);
      __sB[2] = _mm256_xor_si256(__sB[2], __tB);
      __sA[3] = _mm256_or_si256(_mm256_slli_epi64(__sA[3], 45), _mm256_srli_epi64(__sA[3], 19));
      __sB[3] = _mm256_or_si256(_mm256_slli_epi64(__sB[3], 45), _mm256_srli_epi64(__sB[3], 19));

      for (size_t j = 0; j < n - i; ++j) array[i + j] = buf[j];
    }
  }

  // Overload for uint32_t — two independent states, 16 values per iteration
  /// @brief Fills an array with 32-bits pseudo-random integers
  /// @param array Destination, any alignment
  /// @param n Number of values to generate
  inline void getRand(uint32_t* array, const uint32_t n) noexcept {
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
      const __m256i __rA = _mm256_add_epi64(__sA[0], __sA[3]);
      const __m256i __rB = _mm256_add_epi64(__sB[0], __sB[3]);
      _mm256_storeu_si256((__m256i*)(array + i),     __rA);
      _mm256_storeu_si256((__m256i*)(array + i + 8), __rB);

      const __m256i __tA = _mm256_slli_epi64(__sA[1], 17);
      const __m256i __tB = _mm256_slli_epi64(__sB[1], 17);

      __sA[2] = _mm256_xor_si256(__sA[2], __sA[0]);
      __sB[2] = _mm256_xor_si256(__sB[2], __sB[0]);
      __sA[3] = _mm256_xor_si256(__sA[3], __sA[1]);
      __sB[3] = _mm256_xor_si256(__sB[3], __sB[1]);
      __sA[1] = _mm256_xor_si256(__sA[1], __sA[2]);
      __sB[1] = _mm256_xor_si256(__sB[1], __sB[2]);
      __sA[0] = _mm256_xor_si256(__sA[0], __sA[3]);
      __sB[0] = _mm256_xor_si256(__sB[0], __sB[3]);
      __sA[2] = _mm256_xor_si256(__sA[2], __tA);
      __sB[2] = _mm256_xor_si256(__sB[2], __tB);
      __sA[3] = _mm256_or_si256(_mm256_slli_epi64(__sA[3], 45), _mm256_srli_epi64(__sA[3], 19));
      __sB[3] = _mm256_or_si256(_mm256_slli_epi64(__sB[3], 45), _mm256_srli_epi64(__sB[3], 19));
    }
    if (i < n) {
      alignas(32) uint32_t buf[16];
      const __m256i __rA = _mm256_add_epi64(__sA[0], __sA[3]);
      const __m256i __rB = _mm256_add_epi64(__sB[0], __sB[3]);
      _mm256_store_si256((__m256i*)buf,      __rA);
      _mm256_store_si256((__m256i*)(buf+8),  __rB);

      const __m256i __tA = _mm256_slli_epi64(__sA[1], 17);
      const __m256i __tB = _mm256_slli_epi64(__sB[1], 17);

      __sA[2] = _mm256_xor_si256(__sA[2], __sA[0]);
      __sB[2] = _mm256_xor_si256(__sB[2], __sB[0]);
      __sA[3] = _mm256_xor_si256(__sA[3], __sA[1]);
      __sB[3] = _mm256_xor_si256(__sB[3], __sB[1]);
      __sA[1] = _mm256_xor_si256(__sA[1], __sA[2]);
      __sB[1] = _mm256_xor_si256(__sB[1], __sB[2]);
      __sA[0] = _mm256_xor_si256(__sA[0], __sA[3]);
      __sB[0] = _mm256_xor_si256(__sB[0], __sB[3]);
      __sA[2] = _mm256_xor_si256(__sA[2], __tA);
      __sB[2] = _mm256_xor_si256(__sB[2], __tB);
      __sA[3] = _mm256_or_si256(_mm256_slli_epi64(__sA[3], 45), _mm256_srli_epi64(__sA[3], 19));
      __sB[3] = _mm256_or_si256(_mm256_slli_epi64(__sB[3], 45), _mm256_srli_epi64(__sB[3], 19));

      for (size_t j = 0; j < n - i; ++j) array[i + j] = buf[j];
    }
  }
#else

  // Overload for uint64_t (most of the times this one is used)
  /// @brief Fills an array with 64-bits pseudo-random integers
  /// @param array Destination, any alignment
  /// @param n Number of values to generate
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
  /// @brief Fills an array with 32-bits pseudo-random integers
  /// @param array Destination, any alignment
  /// @param n Number of values to generate
  inline void getRand(uint32_t* __restrict array, const uint32_t n) {
    const uint32_t pairs = n >> 1;
    for (uint32_t i = 0; i < pairs; ++i) {
      const uint64_t randVal = s[0] + s[3];
      const uint64_t t = s[1] << 17;
      s[2] ^= s[0];
      s[3] ^= s[1];
      s[1] ^= s[2];
      s[0] ^= s[3];
      s[2] ^= t;
      s[3] = (s[3] << 45) | (s[3] >> (64 - 45));
      array[2 * i]     = static_cast<uint32_t>(randVal);
      array[2 * i + 1] = static_cast<uint32_t>(randVal >> 32);
    }
    if (n & 1u) {
      const uint64_t randVal = s[0] + s[3];
      const uint64_t t = s[1] << 17;
      s[2] ^= s[0];
      s[3] ^= s[1];
      s[1] ^= s[2];
      s[0] ^= s[3];
      s[2] ^= t;
      s[3] = (s[3] << 45) | (s[3] >> (64 - 45));
      array[n - 1] = static_cast<uint32_t>(randVal);
    }
  }
#endif
};
} // namespace SiPMRng

/// @brief Converts 64 random bits to a double uniformly distributed in (0, 1]
/** The top 53 bits k are mapped to (k + 1) * 2^-53. The range excludes 0
 * (and includes 1) so that the value can be passed to log() without
 * producing infinities, as needed by exponential and gaussian sampling.
 * @param u Random bits
 * @return Value in (0, 1]
 */
constexpr double unitDouble(const uint64_t u) noexcept { return ((u >> 11) + 1) * 0x1p-53; }

/// @brief Converts 32 random bits to a float uniformly distributed in (0, 1]
/** The top 24 bits k are mapped to (k + 1) * 2^-24, see unitDouble().
 * @param u Random bits
 * @return Value in (0, 1]
 */
constexpr float unitFloat(const uint32_t u) noexcept { return ((u >> 8) + 1) * 0x1p-24f; }

class SiPMRandom {
public:
  SiPMRandom() = default;

  /// @brief Get a reference to the underlying PRNG
  /// Use this method to seed the PRNG and to get the status
  /// @return Reference to the Xorshift256plus generator
  SiPMRng::Xorshift256plus& rng() { return m_rng; }

  /// @brief Seeds the underlying PRNG
  /// @param x Seed, must not be 0
  void seed(const uint64_t x) { m_rng.seed(x); }

  /// @brief Gives an uniformly distributed random number in (0, 1]
  /// @tparam T Type of the number: double (default) or float
  /// @return Random number in (0, 1]
  template <typename T = double>
  inline T Rand() noexcept;
  /// @brief Gives two uniformly distributed random floats in (0, 1] from a single 64-bits draw
  /// @return The two numbers
  pair<float, float> RandF2() noexcept;
  /// @brief Gives an uniform random integer
  /// @param max Upper limit, excluded
  /// @return Integer in [0, max)
  uint32_t randInteger(const uint32_t max) noexcept;
  /// @brief Gives two uniform random integers from a single draw
  /// @param max Upper limit, excluded
  /// @return Two integers in [0, max)
  pair<uint32_t> randInteger2(const uint32_t max) noexcept;

  /// @brief Gives random value with gaussian distribution
  /// @param mu Mean value
  /// @param sigma Standard deviation
  /// @return Gaussian distributed value
  double randGaussian(const double mu, const double sigma) noexcept;
  /// @brief Gives random float with gaussian distribution
  /// @param mu Mean value
  /// @param sigma Standard deviation
  /// @return Gaussian distributed value
  float randGaussianF(const float mu, const float sigma) noexcept;
  /// @brief Gives random value with exponential distribution
  /// @param mu Mean value
  /// @return Exponentially distributed value
  double randExponential(const double mu) noexcept;
  /// @brief Gives random float with exponential distribution
  /// @param mu Mean value
  /// @return Exponentially distributed value
  float randExponentialF(const float mu) noexcept;
  /// @brief Gives random value with poisson distribution
  /// Knuth's product method; for mu above 50 the gaussian approximation N(mu, sqrt(mu)) is used.
  /// @param mu Mean value
  /// @return Poisson distributed integer
  uint32_t randPoisson(const double mu) noexcept;

  /// @brief Vector version of Rand()
  std::vector<double> Rand(const uint32_t n);
  /// @brief Vector of floats in (0, 1], float version of Rand(n)
  std::vector<float> RandF(const uint32_t n);
  /// @brief Vector version of randGaussian(), generated with Box-Muller
  std::vector<double> randGaussian(const double mu, const double sigma, const uint32_t n);
  /// @brief Vector version of randGaussianF(), generated with a vectorized Box-Muller
  std::vector<float> randGaussianF(const float mu, const float sigma, const uint32_t n);
  /// @brief Vector version of randInteger()
  std::vector<uint32_t> randInteger(const uint32_t max, const uint32_t n);
  /// @brief Vector version of randExponential()
  std::vector<double> randExponential(const double mu, const uint32_t n);
  /// @brief Vector version of randExponentialF()
  std::vector<float> randExponentialF(const float mu, const uint32_t n);

private:
  SiPMRng::Xorshift256plus m_rng;  ///< Underlying generator
};

/**
 * Returns a random double uniformly distributed in (0, 1].
 * Zero is excluded so the value can be safely passed to log(),
 * see unitDouble().
 * @return Random number in (0, 1]
 */
template <>
inline double SiPMRandom::Rand<double>() noexcept {
  return unitDouble(m_rng());
}

/**
 * Returns a random float uniformly distributed in (0, 1].
 * Zero is excluded so the value can be safely passed to log(),
 * see unitFloat().
 * @return Random number in (0, 1]
 */
template <>
inline float SiPMRandom::Rand<float>() noexcept {
  return unitFloat(static_cast<uint32_t>(m_rng() >> 32));
}
} // namespace sipm
#endif /* SIPM_RANDOM_H */
