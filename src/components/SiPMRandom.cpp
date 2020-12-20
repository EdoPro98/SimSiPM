#include <SiPMRandom.h>


namespace sipm{

//Exponential random with mean value "mu"
double randExponential(const double mu){
  return -log(Rand()) * mu;
}


//Gaussian random value with mean "mu" and sigma "sigma"
//Using Box-Muller transform
double randGaussian(const double mu, const double sigma){
  static bool isSine;
  static double sine;
  static double sqrtlog;

  if (isSine) {
    isSine = false;
    return sqrtlog * sine * sigma + mu;
  } else {
    const double sqrtlog = sqrt(-2 * log(Rand()));
    const double angle = TWO_PI * Rand();
    sine = sin(angle);
    isSine = true;
    return sqrtlog * cos(angle) * sigma + mu;
  }
}


//Integer random in range [0,max]
uint32_t randInteger(const uint32_t max){
  return static_cast<uint32_t>(Rand() * (max + 1));
}


//Poisson random with mean value "mu"
uint32_t randPoisson(const double mu){
  if (mu == 0){return 0;}
  if (mu > 50){return static_cast<uint32_t>(randGaussian(mu, sqrt(mu)));}
  const double q = exp(-mu);
  double p = 1;
  int out = -1;
  while (p > q) {
    ++out;
    p *= Rand();
  }
  return out;
}

} /* NAMESPACE_SIPM */
