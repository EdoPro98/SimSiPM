#ifndef SIPM_GUI_FILEIO_H
#define SIPM_GUI_FILEIO_H

#include <string>
#include <vector>

namespace sipmgui {

/// Reads a two-column CSV (wavelength in nm, PDE in [0,1]).
/// Lines that do not start with two numbers (e.g. headers) are skipped.
/// Returns an empty string on success, an error message otherwise.
std::string loadPdeCsv(const std::string& path, std::vector<double>& wavelengths, std::vector<double>& pde);

/// Reads a two-column CSV of gamma lines (energy in keV, emission intensity
/// in % per decay). Lines starting with '#' or not starting with two numbers
/// are skipped. Returns an empty string on success, an error message otherwise.
std::string loadLinesCsv(const std::string& path, std::vector<double>& energies, std::vector<double>& intensities);

/// Reads a comma separated histogram of relative counts; bin i is i photons.
/// Returns an empty string on success, an error message otherwise.
std::string loadHistogramCsv(const std::string& path, std::vector<double>& counts);

} // namespace sipmgui
#endif
