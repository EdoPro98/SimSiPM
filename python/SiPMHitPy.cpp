#include "SiPMHit.h"
#include "nanobind/nanobind.h"

namespace nb = nanobind;
using namespace sipm;

void SiPMHitPy(nb::module_& m) {
  nb::class_<SiPMHit> sipmhit(m, "SiPMHit");

  sipmhit.def("time", &SiPMHit::time)
    .def("row", &SiPMHit::row)
    .def("col", &SiPMHit::col)
    .def("amplitude", static_cast<double (SiPMHit::*)() const>(&SiPMHit::amplitude))
    .def("hitType", &SiPMHit::hitType)
    .def("__repr__", &SiPMHit::toString)
    .def("__copy__", [](const SiPMHit& self) { return SiPMHit(self); })
    .def("__deepcopy__", [](const SiPMHit& self, nb::dict) { return SiPMHit(self); });

  nb::enum_<SiPMHit::HitType>(sipmhit, "HitType")
    .value("kPhotoelectron", SiPMHit::HitType::kPhotoelectron)
    .value("kDarkCount", SiPMHit::HitType::kDarkCount)
    .value("kOpticalCrosstalk", SiPMHit::HitType::kOpticalCrosstalk)
    .value("kDelayedOpticalCrosstalk", SiPMHit::HitType::kDelayedOpticalCrosstalk)
    .value("kFastAfterPulse", SiPMHit::HitType::kFastAfterPulse)
    .value("kSlowAfterPulse", SiPMHit::HitType::kSlowAfterPulse);
}
