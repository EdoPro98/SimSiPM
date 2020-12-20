#include <math.h>
#include <stdint.h>
#include <random>


#ifndef SIPM_RANDOM_H
#define SIPM_RANDOM_H

namespace sipm{
static std::random_device rd;
static std::mt19937_64 rng(rd());
static constexpr double MT_MAX = rng.max();
static constexpr double TWO_PI = 2 * M_PI;

double randExponential(const double);
double randGaussian(const double, const double);
uint32_t randInteger(const uint32_t);
uint32_t randPoisson(const double);

//Uniform random in [0,1)
inline double Rand(){
  return rng() / MT_MAX;
}

} /* NAMESPACE_SIPM */
#endif /* SIPM_RANDOM_H */
