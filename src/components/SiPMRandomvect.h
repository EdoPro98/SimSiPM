#include <math.h>
#include <vector>
#include <stdint.h>
#include "SiPMRandom.h"


#ifndef SIPM_RANDOMVECT_H
#define SIPM_RANDOMVECT_H

namespace sipm{

std::vector<double> Rand (const uint32_t);
std::vector<double> randGaussian (const double, const double,
 const uint32_t);
 std::vector<uint32_t> randInteger (const uint32_t max, const uint32_t n);
std::vector<uint32_t> randPoisson (const double mu, const uint32_t n);

} /* NAMESPACE_SIPM */

#endif /* SIPM_RANDOMVECT_H */
