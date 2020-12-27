#include <SiPMRandom.h>


namespace sipm{

//Exponential random with mean value "mu"
double randExponential(const double mu){
  return -log(Rand()) * mu;
}


//Integer random in range [0,max]
uint32_t randInteger(const uint32_t max){
  return static_cast<uint32_t>(Rand() * (max + 1));
}


//Poisson random with mean value "mu"
uint32_t randPoisson(const double mu){
  if (mu == 0){return 0;}
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
