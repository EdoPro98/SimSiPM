#include "SiPMProperties.h"
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;
using std::map;
using std::vector;

void SiPMPropertiesPy(py::module& m) {
  py::class_<SiPMProperties> sipmproperties(m, "SiPMProperties");

  sipmproperties.def(py::init<>())
    // .def("readSettings",&SiPMProperties::readSettings)
    .def("nCells", &SiPMProperties::nCells)
    .def("nSideCells", &SiPMProperties::nSideCells)
    .def("nSignalPoints", &SiPMProperties::nSignalPoints)
    .def("hitDistribution", &SiPMProperties::hitDistribution)
    .def("signalLength", &SiPMProperties::signalLength)
    .def("sampling", &SiPMProperties::sampling)
    .def("risingTime", &SiPMProperties::risingTime)
    .def("fallingTimeFast", &SiPMProperties::fallingTimeFast)
    .def("fallingTimeSlow", &SiPMProperties::fallingTimeSlow)
    .def("slowComponentFraction", &SiPMProperties::slowComponentFraction)
    .def("recoveryTime", &SiPMProperties::recoveryTime)
    .def("dcr", &SiPMProperties::dcr)
    .def("xt", &SiPMProperties::xt)
    .def("dxt", &SiPMProperties::dxt)
    .def("dxtTau", &SiPMProperties::dxtTau)
    .def("ap", &SiPMProperties::ap)
    .def("tauApFast", &SiPMProperties::tauApFast)
    .def("tauApSlow", &SiPMProperties::tauApSlow)
    .def("apSlowFraction", &SiPMProperties::apSlowFraction)
    .def("ccgv", &SiPMProperties::ccgv)
    .def("snrdB", &SiPMProperties::snrdB)
    .def("snrLinear", &SiPMProperties::snrLinear)
    .def("pde", &SiPMProperties::pde)
    .def("pdeSpectrum", &SiPMProperties::pdeSpectrum)
    .def("pdeType", &SiPMProperties::pdeType)
    .def("hasDcr", &SiPMProperties::hasDcr)
    .def("hasXt", &SiPMProperties::hasXt)
    .def("hasDXt", &SiPMProperties::hasDXt)
    .def("hasAp", &SiPMProperties::hasAp)
    .def("hasSlowComponent", &SiPMProperties::hasSlowComponent)
    .def("setProperty", &SiPMProperties::setProperty)
    .def("setSize", &SiPMProperties::setSize)
    .def("setPitch", &SiPMProperties::setPitch)
    .def("setSampling", &SiPMProperties::setSampling)
    .def("setSignalLength", &SiPMProperties::setSignalLength)
    .def("setRiseTime", &SiPMProperties::setRiseTime)
    .def("setFallTimeFast", &SiPMProperties::setFallTimeFast)
    .def("setFallTimeSlow", &SiPMProperties::setFallTimeSlow)
    .def("setSlowComponentFraction", &SiPMProperties::setSlowComponentFraction)
    .def("setRecoveryTime", &SiPMProperties::setRecoveryTime)
    .def("setSnr", &SiPMProperties::setSnr)
    .def("setTauApFastComponent", &SiPMProperties::setTauApFastComponent)
    .def("setTauApSlowComponent", &SiPMProperties::setTauApSlowComponent)
    .def("setTauApSlowFraction", &SiPMProperties::setTauApSlowFraction)
    .def("setCcgv", &SiPMProperties::setCcgv)
    .def("setPde", &SiPMProperties::setPde)
    .def("setDcr", &SiPMProperties::setDcr)
    .def("setXt", &SiPMProperties::setXt)
    .def("setDXt", &SiPMProperties::setDXt)
    .def("setDXtTau", &SiPMProperties::setDXtTau)
    .def("setAp", &SiPMProperties::setAp)
    .def("setDcrOff", &SiPMProperties::setDcrOff)
    .def("setXtOff", &SiPMProperties::setXtOff)
    .def("setDXtOff", &SiPMProperties::setDXtOff)
    .def("setApOff", &SiPMProperties::setApOff)
    .def("setSlowComponentOff", &SiPMProperties::setSlowComponentOff)
    .def("setDcrOn", &SiPMProperties::setDcrOn)
    .def("setXtOn", &SiPMProperties::setXtOn)
    .def("setDXtOn", &SiPMProperties::setDXtOn)
    .def("setApOn", &SiPMProperties::setApOn)
    .def("setSlowComponentOn", &SiPMProperties::setSlowComponentOn)
    .def("setPdeType", &SiPMProperties::setPdeType)
    .def("setPdeSpectrum",
         py::overload_cast<const map<double, double>&>(&SiPMProperties::setPdeSpectrum))
    .def("setPdeSpectrum", py::overload_cast<const vector<double>&, const vector<double>&>(&SiPMProperties::setPdeSpectrum))
    .def("setHitDistribution", &SiPMProperties::setHitDistribution);

  py::enum_<SiPMProperties::PdeType>(sipmproperties, "PdeType")
    .value("kNoPde", SiPMProperties::PdeType::kNoPde)
    .value("kSimplePde", SiPMProperties::PdeType::kSimplePde)
    .value("kSpectrumPde", SiPMProperties::PdeType::kSpectrumPde);

  py::enum_<SiPMProperties::HitDistribution>(sipmproperties, "HitDistribution")
    .value("kUniform", SiPMProperties::HitDistribution::kUniform)
    .value("kGaussian", SiPMProperties::HitDistribution::kGaussian)
    .value("kCircle", SiPMProperties::HitDistribution::kCircle);
}
