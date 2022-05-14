#include "SiPMDebugInfo.h"
#include "nanobind/nanobind.h"

namespace nb = nanobind;
using namespace sipm;

void SiPMDebugInfoPy(nb::module_& m) {
  nb::class_<SiPMDebugInfo> sipmdebuginfo(m, "SiPMDebugInfo");
  sipmdebuginfo.def("__repr__", &SiPMDebugInfo::toString);

  sipmdebuginfo.def_readonly("nPhotons", &SiPMDebugInfo::nPhotons)
    .def_readonly("nPhotoelectrons", &SiPMDebugInfo::nPhotoelectrons)
    .def_readonly("nDcr", &SiPMDebugInfo::nDcr)
    .def_readonly("nXt", &SiPMDebugInfo::nXt)
    .def_readonly("nDXt", &SiPMDebugInfo::nDXt)
    .def_readonly("nAp", &SiPMDebugInfo::nAp);
}
