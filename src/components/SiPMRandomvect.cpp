#include "SiPMRandomvect.h"
#include <math.h>

#include <iostream>

namespace sipm{

// Generate n random uniform values in range [0-1]
std::vector<double> Rand (const uint32_t n) {
  std::vector<double> out(n);

  for (int i = 0; i < n; ++i){
    out[i] = rng();
  }

  // Separated loop to allow for vectorization
  for (int i = 0; i < n; ++i){
    out[i] /= MT_MAX;
  }

  return out;
}


// Generate n random gaussian values with mean mu and std sigma
std::vector<double> randGaussian(const double mu, const double sigma,
 const uint32_t n) {
  std::vector<double> out(n);
  if(n==0){return out;}

  double v,u;
  #pragma omp parallel for private(u,v)
  for(uint32_t i=0; i<n-1; ++i){
    v = TWO_PI*Rand();
    u = sqrt(-2*log(Rand())) * sigma;
    out[i] = u * sin(v) + mu;
    ++i;
    out[i] = u * cos(v) + mu;
  }
  out[n-1] = sqrt(-2*log(Rand())) * sin(TWO_PI*Rand()) * sigma + mu;

  return out;
}

// Generate random integers in range [0 - max]
std::vector<uint32_t> randInteger (const uint32_t max, const uint32_t n) {
  std::vector<double> u = Rand(n);
  std::vector<uint32_t> out(n);

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
