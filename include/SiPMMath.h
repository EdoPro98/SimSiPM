#ifndef SIPM_SIPMHELPERS_H
#define SIPM_SIPMHELPERS_H
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdlib.h>
#ifdef __SSE__
#include <xmmintrin.h>
#endif

namespace sipm {
namespace math {
#ifdef __SSE__
/** @brief Fast reciprocal calculated directly using hardware instructions.
 * It generates the following asm code:
 * `rcpss xmm xmm`
 */
inline float reciprocal(const float x){
  float retval;
  __m128 __x = _mm_load_ss(&x);
  __x = _mm_rcp_ss(__x);
  _mm_store_ss(&retval,__x);
  return retval;
}
#else
inline float reciprocal(const float x){ return 1/x; }
#endif

/** @brief Custom implementation of @ref std::pair
 * This is a simple custom implementation of std::pair
 * but with no run-time checks to improve performance
 */
template <typename T, typename U = T> struct pair {
  T first, second;
  pair(T x, U y) : first(x), second(y) {}
  pair() = default;
};
} // namespace math
} // namespace sipm
#endif /* SIPM_SIPMHELPERS_H */
