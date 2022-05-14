#include "SiPMAnalogSignal.h"
#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include <vector>

namespace nb = nanobind;
using namespace sipm;

void SiPMAnalogSignalPy(nb::module_& m) {
  nb::class_<SiPMAnalogSignal> sipmanalogsignal(m, "SiPMAnalogSignal");

  sipmanalogsignal.def("size", &SiPMAnalogSignal::size)
    .def("sampling", &SiPMAnalogSignal::sampling)
    .def("waveform", &SiPMAnalogSignal::waveform<std::vector<float>>)
    .def("integral", &SiPMAnalogSignal::integral)
    .def("peak", &SiPMAnalogSignal::peak)
    .def("tot", &SiPMAnalogSignal::tot)
    .def("toa", &SiPMAnalogSignal::toa)
    .def("top", &SiPMAnalogSignal::top)
    .def("__len__", &SiPMAnalogSignal::size)
    .def("__repr__", &SiPMAnalogSignal::toString)
    .def("__getitem__",[](const SiPMAnalogSignal& signal, const uint32_t i){ return signal[i]; });
}
