
#ifndef SIPM_SIPMHELPERS_H
#define SIPM_SIPMHELPERS_H

#include <cmath>
#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace sipm {
namespace math {
#ifdef __AVX2__
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

template <typename T> struct pair {
  T first, second;
  pair(T x, T y) : first(x), second(y) {}
  pair() = default;
};
} // namespace math
} // namespace sipm
#endif /* SIPM_SIPMHELPERS_H */
