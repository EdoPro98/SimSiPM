#include "SiPM.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

PYBIND11_MAKE_OPAQUE(std::vector<double>);
PYBIND11_MAKE_OPAQUE(std::map<double, double>);

namespace py = pybind11;

void SiPMPropertiesPy(py::module&);
void SiPMAnalogSignalPy(py::module&);
void SiPMDigitalSignalPy(py::module&);
void SiPMDebugInfoPy(py::module&);
void SiPMSensorPy(py::module&);
void SiPMRandomPy(py::module&);

PYBIND11_MODULE(SiPM, m) {
  m.doc() = "Module for SiPM simulation";

  py::bind_vector<std::vector<double>>(m, "vectorDouble");
  py::bind_map<std::map<double, double>>(m, "mapDoubleDouble");

  SiPMPropertiesPy(m);
  SiPMAnalogSignalPy(m);
  SiPMDigitalSignalPy(m);
  SiPMDebugInfoPy(m);
  SiPMSensorPy(m);
  SiPMRandomPy(m);
}
