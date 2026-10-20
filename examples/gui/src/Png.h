#ifndef SIPM_GUI_PNG_H
#define SIPM_GUI_PNG_H

#include <string>
#include <vector>

namespace sipmgui {

/// Writes an 8-bit RGB image (rows top to bottom) as PNG. Dependency free:
/// the data is stored uncompressed (deflate "stored" blocks).
/// Returns an empty string on success, an error message otherwise.
std::string writePng(const std::string& path, int width, int height, const std::vector<unsigned char>& rgb);

} // namespace sipmgui
#endif
