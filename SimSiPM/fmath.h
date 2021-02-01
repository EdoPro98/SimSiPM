#include <math.h>
#include <utility>
#ifdef __AVX2__
#include <immintrin.h>
#endif
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE2__
#include <emmintrin.h>
#endif

namespace sipm {
namespace math {

inline float rsqrt(float);
inline float fsqrt(float);

inline float rsqrt(float x) {
#ifdef __SSE__
  __m128 __x = _mm_set1_ps(x);
  __x = _mm_rsqrt_ss(__x);
  return __x[0];
#else
  return 1.0f / sqrt(x);
#endif
}

inline float fsqrt(float x) {
#ifdef __SSE__
  __m128 __x = _mm_set1_ps(x);
  __x = _mm_rsqrt_ss(__x);
  __x = _mm_rcp_ss(__x);
  return __x[0];
#else
  return sqrt(x);
#endif
}

}  // namespace math
}  // namespace sipm
