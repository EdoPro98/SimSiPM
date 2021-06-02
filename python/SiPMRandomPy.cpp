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
    .def("Rand", py::overload_cast<>(&SiPMRandom::Rand))
    .def("randInteger", py::overload_cast<const uint32_t>(&SiPMRandom::randInteger))
    .def("randGaussian", py::overload_cast<const double, const double>(&SiPMRandom::randGaussian))
    .def("randExponential", py::overload_cast<const double>(&SiPMRandom::randExponential))
    .def("randPoisson", py::overload_cast<const double>(&SiPMRandom::randPoisson))
    .def("Rand", py::overload_cast<const uint32_t>(&SiPMRandom::Rand))
    .def("randGaussian", py::overload_cast<const double, const double, const uint32_t>(&SiPMRandom::randGaussian))
    .def("randInteger", py::overload_cast<const uint32_t, const uint32_t>(&SiPMRandom::randInteger))
    .def("randExponential", py::overload_cast<const double, const uint32_t>(&SiPMRandom::randExponential));
}
