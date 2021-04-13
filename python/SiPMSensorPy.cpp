#include "SiPMSensor.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;
using std::vector;

void SiPMSensorPy(py::module &m) {
  py::class_<SiPMSensor> SiPMSensor(m, "SiPMSensor");
  SiPMSensor.def(py::init<>())
    .def(py::init<const SiPMProperties &>())
    .def("properties", static_cast<SiPMProperties &(SiPMSensor::*)()>(&SiPMSensor::properties))
    .def("properties", static_cast<const SiPMProperties &(SiPMSensor::*)() const>(&SiPMSensor::properties))
    .def("signal", &SiPMSensor::signal)
    .def("rng", static_cast<SiPMRandom &(SiPMSensor::*)()>(&SiPMSensor::rng))
    .def("rng", static_cast<const SiPMRandom &(SiPMSensor::*)() const>(&SiPMSensor::rng))
    .def("debug", &SiPMSensor::debug)
    .def("setProperty", &SiPMSensor::setProperty)
    .def("setProperties", &SiPMSensor::setProperties)
    .def("addPhoton", py::overload_cast<>(&SiPMSensor::addPhoton))
    .def("addPhoton", py::overload_cast<const double>(&SiPMSensor::addPhoton))
    .def("addPhoton", py::overload_cast<const double, const double>(&SiPMSensor::addPhoton))
    .def("addPhotons", py::overload_cast<const vector<double> &>(&SiPMSensor::addPhotons))
    .def("addPhotons", py::overload_cast<const vector<double> &, const vector<double> &>(&SiPMSensor::addPhotons))
    .def("runEvent", &SiPMSensor::runEvent)
    .def("resetState", &SiPMSensor::resetState);
}
