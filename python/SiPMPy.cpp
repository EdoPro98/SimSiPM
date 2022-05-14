#include "SiPM.h"
#include "nanobind/nanobind.h"

namespace nb = nanobind;

void SiPMPropertiesPy(nb::module_&);
void SiPMAnalogSignalPy(nb::module_&);
void SiPMDebugInfoPy(nb::module_&);
void SiPMHitPy(nb::module_&);
void SiPMSensorPy(nb::module_&);
void SiPMRandomPy(nb::module_&);

NB_MODULE(SiPM, m) {
  // m.raw_doc() = "Module for SiPM simulation";
  m.attr("__version__") = SIPM_VERSION;
  SiPMPropertiesPy(m);
  SiPMAnalogSignalPy(m);
  SiPMDebugInfoPy(m);
  SiPMHitPy(m);
  SiPMSensorPy(m);
  SiPMRandomPy(m);
}
