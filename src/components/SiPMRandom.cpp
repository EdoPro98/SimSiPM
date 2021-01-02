#include "SiPMRandom.h"

namespace sipm{
//Poisson random with mean value "mu"
uint32_t randPoisson(const double mu){
  if (mu == 0){return 0;}
  const double q = exp(-mu);
  double p = 1;
  int32_t out = -1;

  while (p > q) {
    ++out;
    p *= Rand();
  }
  return out;
}


//Exponential random with mean value "mu"
double randExponential(const double mu){
  return -log(Rand()) * mu;
}


// Gaussian random value with mean "mu" and sigma "sigma"
// Using Box-Muller transform
double randGaussian(const double mu, const double sigma){
  static bool isSine;
  static double angle;
  static double sqrtlog;

  if (isSine) {
    isSine = false;
    return sqrtlog * sin(angle) * sigma + mu;
  } else {
    sqrtlog = sqrt(-2 * log(Rand()));
    angle = TWO_PI * Rand();
    isSine = true;
    return sqrtlog * cos(angle) * sigma + mu;
  }
}

} /* NAMESPACE_SIPM */
