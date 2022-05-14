#include "SiPMRandom.h"
#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include <vector>

namespace nb = nanobind;
using namespace sipm;

void SiPMRandomPy(nb::module_& m) {
  nb::class_<SiPMRandom> SiPMRandom(m, "SiPMRandom");
  SiPMRandom.def(nb::init<>())
    .def("Rand", static_cast<double (SiPMRandom::*)(void)>(&SiPMRandom::Rand))
    .def("randInteger", static_cast<uint32_t (SiPMRandom::*)(const uint32_t)>(&SiPMRandom::randInteger))
    .def("randGaussian", static_cast<double (SiPMRandom::*)(const double, const double)>(&SiPMRandom::randGaussian))
    .def("randExponential", static_cast<double (SiPMRandom::*)(double)>(&SiPMRandom::randExponential))
    .def("randPoisson", &SiPMRandom::randPoisson)
    .def("Rand", static_cast<std::vector<double> (SiPMRandom::*)(const uint32_t)>(&SiPMRandom::Rand))
    .def("randGaussian", static_cast<std::vector<double> (SiPMRandom::*)(const double, const double, const uint32_t)>(
                           &SiPMRandom::randGaussian))
    .def("randInteger", static_cast<std::vector<uint32_t>(SiPMRandom::*)(const uint32_t, const uint32_t)>(&SiPMRandom::randInteger))
    .def("randExponential",
         static_cast<std::vector<double> (SiPMRandom::*)(const double, const uint32_t)>(&SiPMRandom::randExponential));
}
