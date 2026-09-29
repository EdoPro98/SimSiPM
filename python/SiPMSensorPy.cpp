#include "SiPMSensor.h"
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;

void SiPMSensorPy(py::module& m) {
  py::class_<SiPMSensor, std::shared_ptr<SiPMSensor>> sipmsensor(m, "SiPMSensor");
  sipmsensor.def(py::init<>())
    .def(py::init<const SiPMProperties&>())
    .def("properties", static_cast<const SiPMProperties& (SiPMSensor::*)() const>(&SiPMSensor::properties),
         py::return_value_policy::reference_internal)
    .def("hits", [](const SiPMSensor& self) {
           return std::vector<SiPMHit>(self.hits());
         })
    .def("signal", &SiPMSensor::signal)
    .def("rng", static_cast<const SiPMRandom (SiPMSensor::*)() const>(&SiPMSensor::rng))
    .def("debug", &SiPMSensor::debug)
    .def("setProperty", &SiPMSensor::setProperty)
    .def("setProperties", &SiPMSensor::setProperties)
    .def("addPhoton", py::overload_cast<const double>(&SiPMSensor::addPhoton))
    .def("addPhoton", py::overload_cast<const double, const double>(&SiPMSensor::addPhoton))
    // List/iterable overloads (via pybind11/stl.h)
    .def("addPhotons", py::overload_cast<const std::vector<double>&>(&SiPMSensor::addPhotons))
    .def("addPhotons",
         py::overload_cast<const std::vector<double>&, const std::vector<double>&>(&SiPMSensor::addPhotons))
    // Numpy array overloads — avoids the Python-list → vector copy chain
    .def("addPhotons",
         [](SiPMSensor& self, py::array_t<double, py::array::c_style | py::array::forcecast> times) {
           auto buf = times.request();
           const double* ptr = static_cast<const double*>(buf.ptr);
           self.addPhotons(std::vector<double>(ptr, ptr + buf.size));
         })
    .def("addPhotons",
         [](SiPMSensor& self,
            py::array_t<double, py::array::c_style | py::array::forcecast> times,
            py::array_t<double, py::array::c_style | py::array::forcecast> wavelengths) {
           auto tbuf = times.request();
           auto wbuf = wavelengths.request();
           self.addPhotons(
             std::vector<double>(
               static_cast<const double*>(tbuf.ptr),
               static_cast<const double*>(tbuf.ptr) + tbuf.size),
             std::vector<double>(
               static_cast<const double*>(wbuf.ptr),
               static_cast<const double*>(wbuf.ptr) + wbuf.size));
         })
    .def("runEvent", &SiPMSensor::runEvent)
    .def("resetState", &SiPMSensor::resetState)
    .def("__repr__", &SiPMSensor::toString);
}
