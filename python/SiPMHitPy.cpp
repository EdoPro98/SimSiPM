#include "SiPMHit.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;

void SiPMHitPy(py::module& m) {
  py::class_<SiPMHit, std::shared_ptr<SiPMHit>> sipmhit(m, "SiPMHit");

  sipmhit.def("time", &SiPMHit::time)
    .def("row", &SiPMHit::row)
    .def("col", &SiPMHit::col)
    .def("amplitude", static_cast<double (SiPMHit::*)() const noexcept>(&SiPMHit::amplitude))
    .def("hitType", &SiPMHit::hitType)
    .def("__repr__", &SiPMHit::toString)
    .def("__copy__", [](const SiPMHit& self) { return SiPMHit(self); })
    .def("__deepcopy__", [](const SiPMHit& self, py::dict) { return SiPMHit(self); });

  py::enum_<SiPMHit::HitType>(sipmhit, "HitType")
    .value("kPhotoelectron", SiPMHit::HitType::kPhotoelectron)
    .value("kDarkCount", SiPMHit::HitType::kDarkCount)
    .value("kOpticalCrosstalk", SiPMHit::HitType::kOpticalCrosstalk)
    .value("kDelayedOpticalCrosstalk", SiPMHit::HitType::kDelayedOpticalCrosstalk)
    .value("kFastAfterPulse", SiPMHit::HitType::kFastAfterPulse)
    .value("kSlowAfterPulse", SiPMHit::HitType::kSlowAfterPulse);
}
