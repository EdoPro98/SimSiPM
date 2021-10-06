#include "SiPMDigitalSignal.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;

void SiPMDigitalSignalPy(py::module& m) {
  py::class_<SiPMDigitalSignal> sipmdigitalsignal(m, "SiPMDigitalSignal");
  
  sipmdigitalsignal.def("size", &SiPMDigitalSignal::size)
    .def("sampling", &SiPMDigitalSignal::sampling)
    .def("waveform", &SiPMDigitalSignal::waveform)
    .def("integral", &SiPMDigitalSignal::integral)
    .def("peak", &SiPMDigitalSignal::peak)
    .def("tot", &SiPMDigitalSignal::tot)
    .def("toa", &SiPMDigitalSignal::toa)
    .def("top", &SiPMDigitalSignal::top)
    .def("__len__", &SiPMDigitalSignal::size);
}
