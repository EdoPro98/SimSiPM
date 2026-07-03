#include "SiPMAnalogSignal.h"
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;
using namespace pybind11::literals;
using namespace sipm;

void SiPMAnalogSignalPy(py::module& m) {
  py::class_<SiPMAnalogSignal>(m, "SiPMAnalogSignal", py::buffer_protocol())
    // Buffer protocol: allows np.asarray(signal) with zero copy
    .def_buffer([](SiPMAnalogSignal& s) -> py::buffer_info {
      return py::buffer_info(
        s.data(),
        sizeof(float),
        py::format_descriptor<float>::format(),
        1,
        {static_cast<py::ssize_t>(s.size())},
        {sizeof(float)});
    })
    .def("size", &SiPMAnalogSignal::size)
    .def("sampling", &SiPMAnalogSignal::sampling)
    // Returns numpy array; the array's base keeps this SiPMAnalogSignal alive (zero copy)
    .def("waveform",
         [](py::object self_obj) -> py::array_t<float> {
           auto& self = self_obj.cast<SiPMAnalogSignal&>();
           return py::array_t<float>(
             {static_cast<py::ssize_t>(self.size())},
             {sizeof(float)},
             self.data(),
             self_obj);
         })
    .def("integral", &SiPMAnalogSignal::integral)
    .def("peak", &SiPMAnalogSignal::peak)
    .def("tot", &SiPMAnalogSignal::tot)
    .def("toa", &SiPMAnalogSignal::toa)
    .def("top", &SiPMAnalogSignal::top)
    .def("__len__", &SiPMAnalogSignal::size)
    .def("__repr__", &SiPMAnalogSignal::toString)
    // __array__ lets numpy pick up the buffer automatically when dtype is requested
    .def(
      "__array__",
      [](py::object self_obj, py::object dtype) -> py::array {
        auto& self = self_obj.cast<SiPMAnalogSignal&>();
        py::array_t<float> arr(
          {static_cast<py::ssize_t>(self.size())},
          {sizeof(float)},
          self.data(),
          self_obj);
        if (!dtype.is_none()) {
          return arr.attr("astype")(dtype, "copy"_a = false);
        }
        return arr;
      },
      py::arg("dtype") = py::none());
}
