#include "SipmModels.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace sipmgui {

std::string loadSipmModels(const std::string& path, std::vector<SipmModel>& models) {
  std::ifstream f(path);
  if (!f) {
    return "Cannot open " + path;
  }
  const nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
  if (j.is_discarded()) {
    return "Invalid JSON in " + path;
  }
  const nlohmann::json curves = j.value("pdeCurves", nlohmann::json::object());
  const nlohmann::json list = j.value("models", nlohmann::json::array());
  models.clear();
  for (const auto& m : list) {
    SipmModel s;
    s.name = m.value("name", "");
    s.maker = m.value("maker", "");
    s.vov = m.value("vov", 0.0);
    s.sizeMm = m.value("size", 1.0);
    s.pitchUm = m.value("pitch", 25.0);
    s.dcrKHz = m.value("dcr", 0.0);
    s.xtPct = m.value("xt", 0.0);
    s.apPct = m.value("ap", 0.0);
    s.riseNs = m.value("rise", 1.0);
    s.fallNs = m.value("fall", 50.0);
    s.recoveryNs = m.value("recovery", s.fallNs);
    s.gain = m.value("gain", 0.0);
    const double scale = m.value("pdeScale", 1.0);
    const std::string curve = m.value("pde", "");
    if (curves.contains(curve)) {
      for (const auto& p : curves.at(curve)) {
        s.pdeWavelengths.push_back(p.at(0).get<double>());
        s.pdeValues.push_back(p.at(1).get<double>() * scale / 100);
      }
    }
    const nlohmann::json est = m.value("estimated", nlohmann::json::array());
    for (const auto& e : est) {
      s.estimated.push_back(e.get<std::string>());
    }
    models.push_back(std::move(s));
  }
  return models.empty() ? "No models in " + path : std::string();
}

} // namespace sipmgui
