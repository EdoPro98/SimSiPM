#include "SiPMSensor.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/iostream.h>

namespace py = pybind11;
using namespace sipm;
using std::vector;

void SiPMSensorPy(py::module& m) {
  py::class_<SiPMSensor, std::shared_ptr<SiPMSensor>> sipmsensor(m, "SiPMSensor");
  sipmsensor.def(py::init<>())
    .def(py::init<const SiPMProperties&>())
    .def("properties", static_cast<SiPMProperties& (SiPMSensor::*)()>(&SiPMSensor::properties))
    .def("properties", static_cast<const SiPMProperties& (SiPMSensor::*)() const>(&SiPMSensor::properties))
    .def("hits",&SiPMSensor::hits)
    .def("signal", &SiPMSensor::signal)
    .def("rng", static_cast<SiPMRandom& (SiPMSensor::*)()>(&SiPMSensor::rng))
    .def("rng", static_cast<const SiPMRandom& (SiPMSensor::*)() const>(&SiPMSensor::rng))
    .def("debug", &SiPMSensor::debug)
    .def("setProperty", &SiPMSensor::setProperty)
    .def("setProperties", &SiPMSensor::setProperties)
    .def("addPhoton", py::overload_cast<>(&SiPMSensor::addPhoton))
    .def("addPhoton", py::overload_cast<const double>(&SiPMSensor::addPhoton))
    .def("addPhoton", py::overload_cast<const double, const double>(&SiPMSensor::addPhoton))
    .def("addPhotons", py::overload_cast<const vector<double>&>(&SiPMSensor::addPhotons))
    .def("addPhotons", py::overload_cast<const vector<double>&, const vector<double>&>(&SiPMSensor::addPhotons))
    .def("runEvent", &SiPMSensor::runEvent)
    .def("dumpHits", &SiPMSensor::dumpHits)
    .def("resetState", &SiPMSensor::resetState);
}
