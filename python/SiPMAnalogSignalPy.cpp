#include "SiPMAnalogSignal.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;

void SiPMAnalogSignalPy(py::module& m) {
  py::class_<SiPMAnalogSignal> sipmanalogsignal(m, "SiPMAnalogSignal");
  
  sipmanalogsignal.def("size", &SiPMAnalogSignal::size)
    .def("sampling", &SiPMAnalogSignal::sampling)
    .def("waveform", &SiPMAnalogSignal::waveform)
    .def("integral", &SiPMAnalogSignal::integral)
    .def("peak", &SiPMAnalogSignal::peak)
    .def("tot", &SiPMAnalogSignal::tot)
    .def("toa", &SiPMAnalogSignal::toa)
    .def("top", &SiPMAnalogSignal::top)
    .def("lowpass", &SiPMAnalogSignal::lowpass)
    .def("__len__", &SiPMAnalogSignal::size);
}
