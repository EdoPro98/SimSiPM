#include "SiPMRandom.h"
#include <cstdint>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;
using namespace sipm;

// Moves a vector into a numpy array without an extra copy.
// The capsule owns the heap-allocated vector and frees it when the array is GC'd.
template <typename T>
static py::array_t<T> vec_to_array(std::vector<T> v) {
  auto* heap = new std::vector<T>(std::move(v));
  py::capsule owner(heap, [](void* p) { delete static_cast<std::vector<T>*>(p); });
  return py::array_t<T>({heap->size()}, {sizeof(T)}, heap->data(), owner);
}

void SiPMRandomPy(py::module& m) {
  py::class_<SiPMRandom> sipmrandom(m, "SiPMRandom");
  sipmrandom.def(py::init<>())
    .def("seed", static_cast<void (SiPMRandom::*)(const uint64_t)>(&SiPMRandom::seed))
    // Scalar overloads
    .def("Rand", static_cast<double (SiPMRandom::*)()>(&SiPMRandom::Rand<double>))
    .def("randInteger", static_cast<uint32_t (SiPMRandom::*)(const uint32_t)>(&SiPMRandom::randInteger))
    .def("randGaussian",
         static_cast<double (SiPMRandom::*)(const double, const double)>(&SiPMRandom::randGaussian))
    .def("randExponential", static_cast<double (SiPMRandom::*)(double)>(&SiPMRandom::randExponential))
    .def("randPoisson", &SiPMRandom::randPoisson)
    // Array overloads — return numpy arrays instead of Python lists
    .def("Rand",
         [](SiPMRandom& self, uint32_t n) { return vec_to_array(self.Rand(n)); })
    .def("randGaussian",
         [](SiPMRandom& self, double mu, double sigma, uint32_t n) {
           return vec_to_array(self.randGaussian(mu, sigma, n));
         })
    .def("randGaussianF",
         [](SiPMRandom& self, float mu, float sigma, uint32_t n) {
           return vec_to_array(self.randGaussianF(mu, sigma, n));
         })
    .def("randInteger",
         [](SiPMRandom& self, uint32_t max, uint32_t n) {
           return vec_to_array(self.randInteger(max, n));
         })
    .def("randExponential", [](SiPMRandom& self, double lambda, uint32_t n) {
      return vec_to_array(self.randExponential(lambda, n));
    });
}
