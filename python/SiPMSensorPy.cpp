#include "SiPMSensor.h"
#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"

namespace nb = nanobind;
using namespace sipm;

void SiPMSensorPy(nb::module_& m) {
  nb::class_<SiPMSensor> sipmsensor(m, "SiPMSensor");
  sipmsensor.def(nb::init<>())
    .def(nb::init<const SiPMProperties&>())
    .def("properties", static_cast<SiPMProperties& (SiPMSensor::*)()>(&SiPMSensor::properties))
    .def("properties", static_cast<const SiPMProperties& (SiPMSensor::*)() const>(&SiPMSensor::properties))
    .def("hits", &SiPMSensor::hits)
    .def("hitsGraph", &SiPMSensor::hitsGraph)
    .def("signal", &SiPMSensor::signal)
    .def("rng", static_cast<const SiPMRandom (SiPMSensor::*)() const>(&SiPMSensor::rng))
    .def("debug", &SiPMSensor::debug)
    .def("setProperty", &SiPMSensor::setProperty)
    .def("setProperties", &SiPMSensor::setProperties)
    .def("addPhoton", nb::overload_cast<>(&SiPMSensor::addPhoton))
    .def("addPhoton", nb::overload_cast<const double>(&SiPMSensor::addPhoton))
    .def("addPhoton", nb::overload_cast<const double, const double>(&SiPMSensor::addPhoton))
    .def("addPhotons", nb::overload_cast<const std::vector<double>&>(&SiPMSensor::addPhotons))
    .def("addPhotons",
         nb::overload_cast<const std::vector<double>&, const std::vector<double>&>(&SiPMSensor::addPhotons))
    .def("runEvent", &SiPMSensor::runEvent)
    .def("resetState", &SiPMSensor::resetState)
    .def("__repr__", &SiPMSensor::toString);
}
