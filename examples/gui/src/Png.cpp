#include "Png.h"

#include <array>
#include <cstdint>
#include <fstream>

namespace sipmgui {
namespace {

uint32_t crc32(const unsigned char* data, size_t n, uint32_t crc = 0) {
  static const std::array<uint32_t, 256> table = [] {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (int k = 0; k < 8; ++k) {
        c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
      }
      t[i] = c;
    }
    return t;
  }();
  crc = ~crc;
  for (size_t i = 0; i < n; ++i) {
    crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  }
  return ~crc;
}

void put32(std::vector<unsigned char>& v, uint32_t x) {
  for (int s = 24; s >= 0; s -= 8) {
    v.push_back(static_cast<unsigned char>(x >> s));
  }
}

void chunk(std::ofstream& f, const char* type, const std::vector<unsigned char>& data) {
  std::vector<unsigned char> buf;
  put32(buf, static_cast<uint32_t>(data.size()));
  buf.insert(buf.end(), type, type + 4);
  buf.insert(buf.end(), data.begin(), data.end());
  put32(buf, crc32(buf.data() + 4, buf.size() - 4));
  f.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

} // namespace

std::string writePng(const std::string& path, int width, int height, const std::vector<unsigned char>& rgb) {
  if (width <= 0 || height <= 0 || rgb.size() < size_t(width) * height * 3) {
    return "Invalid image";
  }
  std::ofstream f(path, std::ios::binary);
  if (!f) {
    return "Cannot write " + path;
  }
  static const unsigned char sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  f.write(reinterpret_cast<const char*>(sig), 8);

  std::vector<unsigned char> ihdr;
  put32(ihdr, width);
  put32(ihdr, height);
  ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0}); // 8 bit, RGB, deflate, no filter, no interlace
  chunk(f, "IHDR", ihdr);

  // Raw scanlines with filter byte 0
  std::vector<unsigned char> raw;
  raw.reserve(size_t(height) * (width * 3 + 1));
  for (int y = 0; y < height; ++y) {
    raw.push_back(0);
    raw.insert(raw.end(), rgb.begin() + size_t(y) * width * 3, rgb.begin() + size_t(y + 1) * width * 3);
  }
  // zlib stream of stored blocks (max 65535 bytes each)
  std::vector<unsigned char> z = {0x78, 0x01};
  for (size_t pos = 0; pos < raw.size() || raw.empty(); pos += 65535) {
    const size_t n = std::min<size_t>(65535, raw.size() - pos);
    z.push_back(pos + n >= raw.size() ? 1 : 0);
    z.push_back(n & 0xFF);
    z.push_back(n >> 8);
    z.push_back(~n & 0xFF);
    z.push_back((~n >> 8) & 0xFF);
    z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
    if (raw.empty()) {
      break;
    }
  }
  uint32_t a = 1, b = 0;
  for (const unsigned char c : raw) {
    a = (a + c) % 65521;
    b = (b + a) % 65521;
  }
  put32(z, (b << 16) | a);
  chunk(f, "IDAT", z);
  chunk(f, "IEND", {});
  return f ? std::string() : "Write error " + path;
}

} // namespace sipmgui
