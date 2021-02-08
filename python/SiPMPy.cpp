#include <pybind11/pybind11.h>
#include "SiPM.h"

namespace py = pybind11;

void SiPMPropertiesPy(py::module &);
void SiPMAnalogSignalPy(py::module &);
void SiPMDigitalSignalPy(py::module &);
void SiPMDebugInfoPy(py::module &);
void SiPMSensorPy(py::module &);
void SiPMRandomPy(py::module &);

PYBIND11_MODULE(SiPM, m){
  m.doc() = "";

  SiPMPropertiesPy(m);
  SiPMAnalogSignalPy(m);
  SiPMDigitalSignalPy(m);
  SiPMDebugInfoPy(m);
  SiPMSensorPy(m);
  SiPMRandomPy(m);
}
