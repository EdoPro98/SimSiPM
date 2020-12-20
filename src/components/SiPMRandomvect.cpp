#include "SiPMRandomvect.h"

#include <math.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif

#include <iostream>


namespace sipm{

// Generate n random uniform values in range [0-1]
std::vector<double> Rand (const uint32_t n) {
  std::vector<double> out(n);

  #pragma GCC ivdep
  for (int i = 0; i < n; ++i){
    out[i] = rng();
  }

  // Separated loop to allow for vectorization
  #pragma GCC ivdep
  for (int i = 0; i < n; ++i){
    out[i] /= MT_MAX;
  }

  return out;
}


// Generate n random gaussian values with mean mu and std sigma
std::vector<double> randGaussian (const double mu, const double sigma,
 const uint32_t n) {

  // Output vector
  std::vector<double> out(n);

  #pragma GCC ivdep
  for(uint32_t i=0; i<n; ++i){
    out[i] = randGaussian(mu, sigma);
  }

  return out;
}


// Generate random exponential value with mean mu
std::vector<double> randExponential (const double mu, const uint32_t n) {
  std::vector<double> u = Rand(n);

  #pragma GCC ivdep
  for (int i = 0; i < n; ++i){
    u[i] = log(u[i]);
  }

  #pragma GCC ivdep
  for (uint32_t i=0; i< n; ++i){
    u[i] *= -mu;
  }
  return u;
}


// Generate random integers in range [0 - max]
std::vector<uint32_t> randInteger (const uint32_t max, const uint32_t n) {
  std::vector<double> u = Rand(n);
  std::vector<uint32_t> out(n);

  #pragma GCC ivdep
  for (int i = 0; i < n; ++i){
    out[i] = static_cast<uint32_t>(u[i] * (max + 1));
  }
  return out;
}


std::vector<uint32_t> randPoisson (const double mu, const uint32_t n){

  std::vector<uint32_t> out(n);

  for(uint32_t i=0; i<n; ++i){
    out[i] = randPoisson(mu);
  }

  return out;
}

} /* NAMESPACE_SIPM */
