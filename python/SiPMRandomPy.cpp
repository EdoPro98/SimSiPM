#include "SiPMRandom.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;

void SiPMRandomPy(py::module& m) {
  py::class_<SiPMRandom> SiPMRandom(m, "SiPMRandom");
  SiPMRandom.def(py::init<>())
    .def(py::init<const uint64_t>())
    .def("seed", py::overload_cast<>(&SiPMRandom::seed))
    .def("seed", py::overload_cast<uint64_t>(&SiPMRandom::seed))
    .def("jump", &SiPMRandom::jump)
    .def("Rand", static_cast<double(SiPMRandom::*)(void)>(&SiPMRandom::Rand))
    .def("randInteger", py::overload_cast<const uint32_t>(&SiPMRandom::randInteger))
    .def("randGaussian", static_cast<double(SiPMRandom::*)(const double, const double)>(&SiPMRandom::randGaussian))
    .def("randExponential", static_cast<double(SiPMRandom::*)(double)>(&SiPMRandom::randExponential))
    .def("randPoisson", py::overload_cast<const double>(&SiPMRandom::randPoisson))
    .def("Rand", static_cast<std::vector<double>(SiPMRandom::*)(const uint32_t)>(&SiPMRandom::Rand))
    .def("randGaussian", static_cast<std::vector<double>(SiPMRandom::*)(const double, const double, const uint32_t)>(&SiPMRandom::randGaussian))
    .def("randInteger", py::overload_cast<const uint32_t, const uint32_t>(&SiPMRandom::randInteger))
    .def("randExponential", static_cast<std::vector<double>(SiPMRandom::*)(const double, const uint32_t)>(&SiPMRandom::randExponential));
}
