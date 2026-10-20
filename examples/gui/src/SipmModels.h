#ifndef SIPM_GUI_SIPMMODELS_H
#define SIPM_GUI_SIPMMODELS_H

#include <string>
#include <vector>

namespace sipmgui {

/// Commercial SiPM preset (data/sipm_models.json), values at the datasheet overvoltage.
struct SipmModel {
  std::string name, maker;
  double vov = 0;                  // overvoltage of the datasheet values [V]
  double sizeMm = 1, pitchUm = 25; // square sensor
  double dcrKHz = 0, xtPct = 0, apPct = 0;
  double riseNs = 1, fallNs = 50, recoveryNs = 50;
  double gain = 0;                               // informative only
  std::vector<double> pdeWavelengths, pdeValues; // nm, fraction [0,1]
  std::vector<std::string> estimated;            // fields not given by the datasheet
};

/// Loads the presets. Returns an empty string on success.
std::string loadSipmModels(const std::string& path, std::vector<SipmModel>& models);

} // namespace sipmgui
#endif
