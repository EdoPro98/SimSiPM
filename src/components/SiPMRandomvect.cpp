#include "SiPMRandomvect.h"
#include <math.h>

#ifdef __AVX2__
#include <immintrin.h>
#endif


namespace sipm{

// Generate n random uniform values in range [0-1]
std::vector<double> Rand(const uint32_t n) {
  std::vector<double> out(n);

  for (int i = 0; i < n; ++i){
    out[i] = xorshift32() / UINT32_MAXD;
  }
  return out;
}


// Generate n random gaussian values with mean mu and std sigma
std::vector<double> randGaussian(const double mu, const double sigma,
 const uint32_t n) {
  if(n==0){return {};}

  std::vector<double> out(n);

  for(uint32_t i=0; i<n-1; ++i){
    double v = TWO_PI*Rand();
    double w = sqrt(-2*log(Rand()));
    out[i] = w * sin(v);
    ++i;
    out[i] = w * cos(v);
  }
  out[n-1] = sqrt(-2*log(Rand())) * sin(TWO_PI*Rand());

#ifdef __AVX2__

  const uint32_t last = n - n%4;
  const __m256d __mu = _mm256_set1_pd(mu);
  const __m256d __sigma = _mm256_set1_pd(sigma);
  __m256d __out;

  for(uint32_t i=0;i<last;i+=4){
    __out = _mm256_loadu_pd(&out[i]);
    __out = _mm256_fmadd_pd(__out,__sigma,__mu);
    _mm256_storeu_pd(&out[i],__out);
  }
  for(uint32_t i=last;i<n;++i){out[i] = out[i] * sigma + mu;}

#else

  for(uint32_t i=0;i<n;++i){out[i] = out[i] * sigma + mu;}
#endif

  return out;
}

// Generate random integers in range [0 - max]
std::vector<uint32_t> randInteger(const uint32_t max, const uint32_t n) {
  std::vector<uint32_t> out(n);
  const std::vector<double> u = Rand(n);

  for (int i = 0; i < n; ++i){
    out[i] = xorshift64() % max;
  }
  return out;
}


std::vector<uint32_t> randPoisson(const double mu, const uint32_t n){
  std::vector<uint32_t> out(n);

  for(uint32_t i=0; i<n; ++i){
    out[i] = randPoisson(mu);
  }

  return out;
}

} /* NAMESPACE_SIPM */
