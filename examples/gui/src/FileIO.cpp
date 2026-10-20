#include "FileIO.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace sipmgui {
namespace {

/// Splits a line on commas, semicolons and whitespace and parses each
/// token as a number. Stops at the first token that is not a number.
std::vector<double> parseNumbers(const std::string& line) {
  std::vector<double> out;
  std::string token;
  auto flush = [&]() -> bool {
    if (token.empty()) {
      return true;
    }
    char* end = nullptr;
    const double v = std::strtod(token.c_str(), &end);
    const bool ok = end != token.c_str() && *end == '\0';
    if (ok) {
      out.push_back(v);
    }
    token.clear();
    return ok;
  };
  for (const char ch : line) {
    if (ch == ',' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\r') {
      if (!flush()) {
        return out;
      }
    } else {
      token.push_back(ch);
    }
  }
  flush();
  return out;
}

} // namespace

std::string loadPdeCsv(const std::string& path, std::vector<double>& wavelengths, std::vector<double>& pde) {
  std::ifstream file(path);
  if (!file) {
    return "Cannot open file";
  }
  std::vector<double> wl, val;
  std::string line;
  while (std::getline(file, line)) {
    const std::vector<double> nums = parseNumbers(line);
    if (nums.size() < 2) {
      continue;
    }
    wl.push_back(nums[0]);
    val.push_back(nums[1]);
  }
  if (wl.size() < 2) {
    return "Need at least two (wavelength, pde) rows";
  }
  for (double& v : val) {
    v = std::clamp(v, 0.0, 1.0);
  }
  wavelengths = std::move(wl);
  pde = std::move(val);
  return {};
}

std::string loadLinesCsv(const std::string& path, std::vector<double>& energies, std::vector<double>& intensities) {
  std::ifstream file(path);
  if (!file) {
    return "Cannot open file";
  }
  std::vector<double> e, in;
  std::string line;
  while (std::getline(file, line)) {
    const std::vector<double> nums = parseNumbers(line);
    if (nums.size() < 2 || nums[0] <= 0 || nums[1] < 0) {
      continue;
    }
    e.push_back(nums[0]);
    in.push_back(nums[1]);
  }
  if (e.empty()) {
    return "No (energy, intensity) rows found";
  }
  energies = std::move(e);
  intensities = std::move(in);
  return {};
}

std::string loadHistogramCsv(const std::string& path, std::vector<double>& counts) {
  std::ifstream file(path);
  if (!file) {
    return "Cannot open file";
  }
  std::vector<double> bins;
  std::string line;
  while (std::getline(file, line)) {
    const std::vector<double> nums = parseNumbers(line);
    bins.insert(bins.end(), nums.begin(), nums.end());
  }
  if (bins.empty()) {
    return "No values found";
  }
  double sum = 0;
  for (const double v : bins) {
    if (v < 0) {
      return "Negative counts are not allowed";
    }
    sum += v;
  }
  if (sum <= 0) {
    return "Histogram is empty";
  }
  counts = std::move(bins);
  return {};
}

} // namespace sipmgui
