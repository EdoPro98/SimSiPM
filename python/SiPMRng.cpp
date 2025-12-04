#include "SiPMRandom.h"
#include <cstdint>
#include <pybind11/pybind11.h>
#include <sys/types.h>

namespace py = pybind11;
using namespace sipm;
using namespace SiPMRng;

void SiPMRngPy(py::module& m){
  py::class_<Xorshift256plus> xorshift256plus(m, "Xorshift256plus");
  xorshift256plus.def(py::init<>())
  .def("__call__", &Xorshift256plus::operator())
  .def("seed", static_cast<void (Xorshift256plus::*)(void)>(&Xorshift256plus::seed))
  .def("seed", py::overload_cast<uint64_t>(&Xorshift256plus::seed));
}
