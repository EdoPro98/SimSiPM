#include "SiPMDebugInfo.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;

void SiPMDebugInfoPy(py::module& m) {
  py::class_<SiPMDebugInfo> sipmdebuginfo(m, "SiPMDebugInfo");
  
  sipmdebuginfo.def_readonly("nPhotons", &SiPMDebugInfo::nPhotons)
    .def_readonly("nPhotoelectrons", &SiPMDebugInfo::nPhotoelectrons)
    .def_readonly("nDcr", &SiPMDebugInfo::nDcr)
    .def_readonly("nXt", &SiPMDebugInfo::nXt)
    .def_readonly("nDXt", &SiPMDebugInfo::nDXt)
    .def_readonly("nAp", &SiPMDebugInfo::nAp);
}
