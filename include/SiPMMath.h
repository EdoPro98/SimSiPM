
#ifndef SIPM_SIPMHELPERS_H
#define SIPM_SIPMHELPERS_H

#include <cmath>
#ifdef __AVX2__
  #include <immintrin.h>
#endif

namespace sipm {
namespace math{
#ifdef __AVX2__
inline float rec(float x){
  float r;
  __m128 __x = _mm_load_ss(&x);
  __x = _mm_rcp_ss(__x);
  _mm_store_ss(&r,__x);
  return r;
}
#else
inline float rec(float x) {
  union { float f; int i; } v;
  float w, sx;

  sx = (x < 0) ? -1:1;
  x = sx * x;
  v.i = (int)(0x7EF127EA - *(uint32_t *)&x);
  w = x * v.f;
  v.f = v.f * (2 - w);     // Single iteration, Err = -3.36e-3 * 2^(-flr(log2(x)))
  v.f = v.f * ( 4 + w * (-6 + w * (4 - w)));  // Second iteration, Err = -1.13e-5 * 2^(-flr(log2(x)))
  v.f = v.f * (8 + w * (-28 + w * (56 + w * (-70 + w *(56 + w * (-28 + w * (8 - w)))))));  // Third Iteration, Err = +-6.8e-8 *  2^(-flr(log2(x)))
  return v.f * sx;
}
#endif

template<typename T>
struct pair{
  T first,second;
  pair(T x, T y) : first(x), second(y) {}
};
} // namespace math
} // namespace sipm
#endif /* SIPM_SIPMHELPERS_H */
