#include "SiPM.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

namespace py = pybind11;

void SiPMPropertiesPy(py::module &);
void SiPMAnalogSignalPy(py::module &);
void SiPMDigitalSignalPy(py::module &);
void SiPMDebugInfoPy(py::module &);
void SiPMSensorPy(py::module &);
void SiPMRandomPy(py::module &);
void SiPMSimulatorPy(py::module &);

PYBIND11_MODULE(SiPM, m) {
  m.doc() = "Module for SiPM simulation";

  SiPMPropertiesPy(m);
  SiPMAnalogSignalPy(m);
  SiPMDigitalSignalPy(m);
  SiPMDebugInfoPy(m);
  SiPMSensorPy(m);
  SiPMRandomPy(m);
  SiPMSimulatorPy(m);
}
