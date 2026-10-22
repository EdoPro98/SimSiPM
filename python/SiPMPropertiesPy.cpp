#include "SiPMProperties.h"
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;
using std::vector;

void SiPMPropertiesPy(py::module& m) {
  py::class_<SiPMProperties> sipmproperties(m, "SiPMProperties");

  sipmproperties
    .def(py::init<>())
    .def_static("readSettings", &SiPMProperties::readSettings)
    .def_static("readSettingsJson", &SiPMProperties::readSettingsJson)
    .def(py::pickle(
      [](const SiPMProperties& p) {
        py::dict d;
        d["size"]                 = p.size();
        d["pitch"]                = p.pitch();
        d["sampling"]             = p.sampling();
        d["signalLength"]         = p.signalLength();
        d["risingTime"]           = p.risingTime();
        d["fallingTimeFast"]      = p.fallingTimeFast();
        d["fallingTimeSlow"]      = p.fallingTimeSlow();
        d["slowComponentFraction"]= p.slowComponentFraction();
        d["recoveryTime"]         = p.recoveryTime();
        d["snrdB"]                = p.snrdB();
        d["dcr"]                  = p.dcr();
        d["xt"]                   = p.xt();
        d["dxt"]                  = p.dxt();
        d["dxtTau"]               = p.dxtTau();
        d["ap"]                   = p.ap();
        d["tauApFast"]            = p.tauApFast();
        d["tauApSlow"]            = p.tauApSlow();
        d["apSlowFraction"]       = p.apSlowFraction();
        d["ccgv"]                 = p.ccgv();
        d["pde"]                  = p.pde();
        d["pdeType"]              = static_cast<int>(p.pdeType());
        d["hitDistribution"]      = static_cast<int>(p.hitDistribution());
        d["hasDcr"]               = p.hasDcr();
        d["hasXt"]                = p.hasXt();
        d["hasDXt"]               = p.hasDXt();
        d["hasAp"]                = p.hasAp();
        d["hasSlowComponent"]     = p.hasSlowComponent();
        // PDE spectrum as parallel lists
        py::list wls, pdes;
        for (const auto& kv : p.pdeSpectrum()) {
          wls.append(kv.first);
          pdes.append(kv.second);
        }
        d["pdeSpectrum_wl"]  = wls;
        d["pdeSpectrum_pde"] = pdes;
        return d;
      },
      [](py::dict d) {
        SiPMProperties p;
        p.setSize(d["size"].cast<double>());
        p.setPitch(d["pitch"].cast<double>());
        p.setSampling(d["sampling"].cast<double>());
        p.setSignalLength(d["signalLength"].cast<double>());
        p.setRiseTime(d["risingTime"].cast<double>());
        p.setFallTimeFast(d["fallingTimeFast"].cast<double>());
        p.setFallTimeSlow(d["fallingTimeSlow"].cast<double>());
        p.setSlowComponentFraction(d["slowComponentFraction"].cast<double>());
        p.setRecoveryTime(d["recoveryTime"].cast<double>());
        p.setSnr(d["snrdB"].cast<double>());
        p.setDcr(d["dcr"].cast<double>());
        p.setXt(d["xt"].cast<double>());
        p.setDXt(d["dxt"].cast<double>());
        p.setDXtTau(d["dxtTau"].cast<double>());
        p.setAp(d["ap"].cast<double>());
        p.setTauApFastComponent(d["tauApFast"].cast<double>());
        p.setTauApSlowComponent(d["tauApSlow"].cast<double>());
        p.setApSlowFraction(d["apSlowFraction"].cast<double>());
        p.setCcgv(d["ccgv"].cast<float>());
        p.setPde(d["pde"].cast<double>());
        p.setPdeType(static_cast<SiPMProperties::PdeType>(d["pdeType"].cast<int>()));
        p.setHitDistribution(static_cast<SiPMProperties::HitDistribution>(d["hitDistribution"].cast<int>()));
        if (d["hasDcr"].cast<bool>()) p.setDcrOn(); else p.setDcrOff();
        if (d["hasXt"].cast<bool>())  p.setXtOn();  else p.setXtOff();
        if (d["hasDXt"].cast<bool>()) p.setDXtOn(); else p.setDXtOff();
        if (d["hasAp"].cast<bool>())  p.setApOn();  else p.setApOff();
        if (d["hasSlowComponent"].cast<bool>()) p.setSlowComponentOn(); else p.setSlowComponentOff();
        // restore PDE spectrum if present
        auto wls  = d["pdeSpectrum_wl"].cast<vector<double>>();
        auto pdes = d["pdeSpectrum_pde"].cast<vector<double>>();
        if (!wls.empty()) p.setPdeSpectrum(wls, pdes);
        return p;
      }
    ))
    .def("size", &SiPMProperties::size)
    .def("pitch", &SiPMProperties::pitch)
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
    .def("hasCustomSignalShape", &SiPMProperties::hasCustomSignalShape)
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
    .def("setApSlowFraction", &SiPMProperties::setApSlowFraction)
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
         py::overload_cast<const vector<double>&, const vector<double>&>(&SiPMProperties::setPdeSpectrum))
    // Numpy array overload — avoids list → vector conversion
    .def("setPdeSpectrum",
         [](SiPMProperties& self,
            py::array_t<double, py::array::c_style | py::array::forcecast> wavelengths,
            py::array_t<double, py::array::c_style | py::array::forcecast> pde) {
           auto wbuf = wavelengths.request();
           auto pbuf = pde.request();
           const double* wptr = static_cast<const double*>(wbuf.ptr);
           const double* pptr = static_cast<const double*>(pbuf.ptr);
           self.setPdeSpectrum(
             vector<double>(wptr, wptr + wbuf.size),
             vector<double>(pptr, pptr + pbuf.size));
         })
    .def("setHitDistribution", &SiPMProperties::setHitDistribution)
    .def("__repr__", &SiPMProperties::toString);

  py::enum_<SiPMProperties::PdeType>(sipmproperties, "PdeType")
    .value("kNoPde", SiPMProperties::PdeType::kNoPde)
    .value("kSimplePde", SiPMProperties::PdeType::kSimplePde)
    .value("kSpectrumPde", SiPMProperties::PdeType::kSpectrumPde);

  py::enum_<SiPMProperties::HitDistribution>(sipmproperties, "HitDistribution")
    .value("kUniform", SiPMProperties::HitDistribution::kUniform)
    .value("kGaussian", SiPMProperties::HitDistribution::kGaussian)
    .value("kCircle", SiPMProperties::HitDistribution::kCircle);
}
