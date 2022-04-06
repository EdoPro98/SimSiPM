#ifndef SIPM_SIPMHELPERS_H
#define SIPM_SIPMHELPERS_H
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdlib.h>
#include <vector>
#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace sipm {
namespace math {
/** @brief Custom implementation of @ref std::pair
 * This is a simple custom implementation of std::pair
 * but with no run-time checks to improve performance
 */
template <typename T, typename U = T> struct pair {
  T first, second;
  pair(T x, U y) : first(x), second(y) {}
  pair() = default;
};

#ifdef __AVX2__
/** @brief Fast reciprocal calculated directly using hardware instructions.
 * It generates the following asm code:
 * `rcpss xmm xmm`
 */
inline float rec(const float x) {
  float y;
  __m128 __x = _mm_load_ss(&x);
  __x = _mm_rcp_ss(__x);
  _mm_store_ss(&y, __x);
  return y;
}
#else
inline float rec(const float x) { return 1 / x; }
#endif

#ifdef __AVX2__
/** @brief Fast square root calculated using hardware instruction.
 * Square root is calculated using the fast reciprocal square root
 * and then fast reciprocal of the result.
 * It generates the following asm code
 * `rsqrtss xmm xmm`
 * `rcpss xmm xmm`
 */
inline float sqrt(const float x) {
  float y;
  __m128 __x = _mm_load_ss(&x);
  __x = _mm_rsqrt_ss(__x);
  __x = _mm_rcp_ss(__x);
  _mm_store_ss(&y, __x);
  return y;
}
#else
inline float sqrt(const float x) { return sqrt(x); }
#endif
} // namespace math
} // namespace sipm
#endif /* SIPM_SIPMHELPERS_H */
