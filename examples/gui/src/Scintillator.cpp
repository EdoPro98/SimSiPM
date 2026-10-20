#include "Scintillator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>

#include <nlohmann/json.hpp>

#include "SiPMRandom.h"

namespace sipmgui {
namespace {

constexpr double kMeC2 = 510.99895; // keV

struct ElementInfo {
  int z;
  double mass;   // g/mol
  double kAlpha; // keV, 0 = fluorescence not tracked
  double kBeta;  // keV
};

// Atomic masses and K X-ray energies (K-alpha1, K-beta1)
const std::map<std::string, ElementInfo> kElements = {
  {"H", {1, 1.008, 0, 0}},
  {"C", {6, 12.011, 0, 0}},
  {"O", {8, 15.999, 0, 0}},
  {"Na", {11, 22.990, 0, 0}},
  {"Al", {13, 26.982, 0, 0}},
  {"Si", {14, 28.085, 0, 0}},
  {"Ga", {31, 69.723, 9.252, 10.264}},
  {"Ge", {32, 72.630, 9.886, 10.982}},
  {"Br", {35, 79.904, 11.924, 13.291}},
  {"Y", {39, 88.906, 14.958, 16.738}},
  {"I", {53, 126.904, 28.612, 32.295}},
  {"Cs", {55, 132.905, 30.973, 34.987}},
  {"La", {57, 138.905, 33.442, 37.801}},
  {"Ce", {58, 140.116, 34.720, 39.258}},
  {"Gd", {64, 157.25, 42.996, 48.697}},
  {"Lu", {71, 174.967, 54.070, 61.283}},
  {"W", {74, 183.84, 59.318, 67.244}},
  {"Pb", {82, 207.2, 74.969, 84.936}},
  {"Bi", {83, 208.980, 77.108, 87.343}},
};

/// Log-spaced lookup grid for the material cross sections
constexpr double kGridMin = 1.0; // keV
constexpr double kGridMax = 1e4; // keV
constexpr int kGridN = 4000;
const double kGridStep = std::log(kGridMax / kGridMin) / (kGridN - 1);

struct XcomTable {
  std::vector<double> e, inc, pe, pair; // keV, cm2/g
  double kEdge = 0;                     // keV
  double kJump = 1;                     // photoelectric jump ratio at the K edge
};

std::string readXcom(const std::string& path, XcomTable& t) {
  std::ifstream f(path);
  if (!f) {
    return "Cannot open " + path;
  }
  std::string line;
  while (std::getline(f, line)) {
    if (line.rfind("# K-edge MeV:", 0) == 0) {
      t.kEdge = std::stod(line.substr(13)) * 1e3;
      continue;
    }
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::stringstream ss(line);
    std::array<double, 5> v{};
    char comma;
    ss >> v[0] >> comma >> v[1] >> comma >> v[2] >> comma >> v[3] >> comma >> v[4];
    t.e.push_back(v[0] * 1e3);
    t.inc.push_back(v[1]);
    t.pe.push_back(v[2]);
    t.pair.push_back(v[3] + v[4]);
  }
  for (size_t i = 0; i + 1 < t.e.size(); ++i) {
    if (t.kEdge > 0 && t.e[i] == t.kEdge && t.e[i + 1] == t.kEdge && t.pe[i] > 0) {
      t.kJump = t.pe[i + 1] / t.pe[i];
    }
  }
  return t.e.size() < 2 ? "Empty table " + path : std::string();
}

/// Log-log interpolation; at an edge (repeated energy) the value above is used
double interp(const std::vector<double>& x, const std::vector<double>& y, double e) {
  if (e <= x.front()) {
    return y.front();
  }
  size_t i = std::upper_bound(x.begin(), x.end(), e) - x.begin();
  if (i >= x.size()) {
    return y.back();
  }
  const double x0 = x[i - 1], x1 = x[i], y0 = y[i - 1], y1 = y[i];
  if (y0 <= 0 || y1 <= 0) {
    return y0 + (y1 - y0) * (e - x0) / (x1 - x0);
  }
  return std::exp(std::log(y0) + std::log(y1 / y0) * std::log(e / x0) / std::log(x1 / x0));
}

/// Linear attenuation coefficients of a material [1/mm] on the log grid
struct Attenuation {
  struct Fluo {
    double kEdge, pK, kAlpha, kBeta, fBeta; // pK: P(K shell) * fluorescence yield
  };
  std::vector<double> inc, pe, pair, total;
  std::vector<std::vector<double>> peByElement; // cumulative fractions of photoelectric per element
  std::vector<Fluo> fluo;

  static int index(double e) {
    const int i = static_cast<int>(std::log(std::max(e, kGridMin) / kGridMin) / kGridStep);
    return std::min(i, kGridN - 1);
  }
};

std::string buildAttenuation(const ScintMaterial& m, const std::string& xcomDir, Attenuation& a) {
  double molar = 0;
  for (const auto& [sym, n] : m.composition) {
    const auto it = kElements.find(sym);
    if (it == kElements.end()) {
      return "Unknown element " + sym;
    }
    molar += n * it->second.mass;
  }
  a.inc.assign(kGridN, 0);
  a.pe.assign(kGridN, 0);
  a.pair.assign(kGridN, 0);
  a.peByElement.assign(m.composition.size(), std::vector<double>(kGridN, 0));
  std::vector<std::vector<double>> pe(m.composition.size(), std::vector<double>(kGridN, 0));
  for (size_t k = 0; k < m.composition.size(); ++k) {
    const auto& [sym, n] = m.composition[k];
    const ElementInfo& el = kElements.at(sym);
    XcomTable t;
    const std::string err = readXcom(xcomDir + "/" + sym + ".csv", t);
    if (!err.empty()) {
      return err;
    }
    const double w = n * el.mass / molar * m.density / 10; // mass fraction * density, per mm
    for (int i = 0; i < kGridN; ++i) {
      const double e = kGridMin * std::exp(i * kGridStep);
      a.inc[i] += w * interp(t.e, t.inc, e);
      pe[k][i] = w * interp(t.e, t.pe, e);
      a.pe[i] += pe[k][i];
      a.pair[i] += w * interp(t.e, t.pair, e);
    }
    const double z4 = std::pow(el.z, 4.0);
    const double omega = z4 / (1.12e6 + z4); // K fluorescence yield (fit)
    const double fBeta = el.z < 40 ? 0.13 : el.z < 60 ? 0.18 : 0.22;
    a.fluo.push_back({t.kEdge, el.kAlpha > 0 ? (1 - 1 / t.kJump) * omega : 0.0, el.kAlpha, el.kBeta, fBeta});
  }
  a.total.resize(kGridN);
  for (int i = 0; i < kGridN; ++i) {
    a.total[i] = a.inc[i] + a.pe[i] + a.pair[i];
    double c = 0;
    for (size_t k = 0; k < pe.size(); ++k) {
      c += a.pe[i] > 0 ? pe[k][i] / a.pe[i] : 0;
      a.peByElement[k][i] = c;
    }
  }
  return {};
}

struct Vec {
  double x, y, z;
};

/// Crystal: box |x|,|y| <= w/2 or cylinder x^2+y^2 <= r^2, with 0 <= z <= L
struct Geometry {
  bool cylinder;
  double half, length;

  /// Distance along u from p (inside) to the surface
  double exit(const Vec& p, const Vec& u) const {
    double t = 1e30;
    if (u.z > 0) {
      t = (length - p.z) / u.z;
    } else if (u.z < 0) {
      t = -p.z / u.z;
    }
    if (cylinder) {
      const double a = u.x * u.x + u.y * u.y;
      if (a > 0) {
        const double b = p.x * u.x + p.y * u.y, c = p.x * p.x + p.y * p.y - half * half;
        t = std::min(t, (-b + std::sqrt(std::max(0.0, b * b - a * c))) / a);
      }
    } else {
      if (u.x != 0) {
        t = std::min(t, ((u.x > 0 ? half : -half) - p.x) / u.x);
      }
      if (u.y != 0) {
        t = std::min(t, ((u.y > 0 ? half : -half) - p.y) / u.y);
      }
    }
    return std::max(0.0, t);
  }

  /// Entry distance of a ray from outside, or -1 if it misses
  double enter(const Vec& p, const Vec& u) const {
    double t0 = -1e30, t1 = 1e30;
    auto slab = [&](double pp, double uu, double lo, double hi) {
      if (uu == 0) {
        return pp >= lo && pp <= hi;
      }
      double a = (lo - pp) / uu, b = (hi - pp) / uu;
      if (a > b) {
        std::swap(a, b);
      }
      t0 = std::max(t0, a);
      t1 = std::min(t1, b);
      return t0 <= t1;
    };
    if (!slab(p.z, u.z, 0, length)) {
      return -1;
    }
    if (cylinder) {
      const double a = u.x * u.x + u.y * u.y, b = p.x * u.x + p.y * u.y, c = p.x * p.x + p.y * p.y - half * half;
      if (a == 0) {
        if (c > 0) {
          return -1;
        }
      } else {
        const double d = b * b - a * c;
        if (d < 0) {
          return -1;
        }
        t0 = std::max(t0, (-b - std::sqrt(d)) / a);
        t1 = std::min(t1, (-b + std::sqrt(d)) / a);
      }
    } else if (!slab(p.x, u.x, -half, half) || !slab(p.y, u.y, -half, half)) {
      return -1;
    }
    return t0 <= t1 && t1 > 0 ? std::max(t0, 0.0) : -1;
  }
};

Vec isotropic(sipm::SiPMRandom& rng) {
  const double cz = 2 * rng.Rand() - 1, phi = 2 * M_PI * rng.Rand();
  const double s = std::sqrt(std::max(0.0, 1 - cz * cz));
  return {s * std::cos(phi), s * std::sin(phi), cz};
}

/// Rotates direction u by polar angle (cos ct) and random azimuth
Vec scatter(const Vec& u, double ct, sipm::SiPMRandom& rng) {
  const double st = std::sqrt(std::max(0.0, 1 - ct * ct)), phi = 2 * M_PI * rng.Rand();
  const double cp = std::cos(phi), sp = std::sin(phi);
  if (std::abs(u.z) > 0.99999) {
    const double sgn = u.z > 0 ? 1 : -1;
    return {st * cp, st * sp, sgn * ct};
  }
  const double r = std::sqrt(1 - u.z * u.z);
  return {u.x * ct + st * (u.x * u.z * cp - u.y * sp) / r, u.y * ct + st * (u.y * u.z * cp + u.x * sp) / r,
          u.z * ct - st * cp * r};
}

struct Photon {
  Vec p, u;
  double e;
};

/// Transports the photons on the stack, returns the energy deposited [keV]
double transport(std::vector<Photon>& stack, const Geometry& g, const Attenuation& a, sipm::SiPMRandom& rng) {
  double dep = 0;
  while (!stack.empty()) {
    Photon ph = stack.back();
    stack.pop_back();
    while (true) {
      if (ph.e < kGridMin) {
        dep += ph.e;
        break;
      }
      const int i = Attenuation::index(ph.e);
      const double s = -std::log(rng.Rand()) / a.total[i];
      if (s >= g.exit(ph.p, ph.u)) {
        break; // escapes
      }
      ph.p = {ph.p.x + s * ph.u.x, ph.p.y + s * ph.u.y, ph.p.z + s * ph.u.z};
      const double r = rng.Rand() * a.total[i];
      if (r <= a.pe[i]) {
        // Photoelectric: pick the element, possibly emit a K X-ray
        const double q = rng.Rand();
        size_t k = 0;
        while (k + 1 < a.fluo.size() && q > a.peByElement[k][i]) {
          ++k;
        }
        const auto& f = a.fluo[k];
        if (f.pK > 0 && ph.e > f.kEdge && rng.Rand() <= f.pK) {
          const double ex = rng.Rand() <= f.fBeta ? f.kBeta : f.kAlpha;
          dep += ph.e - ex;
          stack.push_back({ph.p, isotropic(rng), ex});
        } else {
          dep += ph.e;
        }
        break;
      }
      if (r <= a.pe[i] + a.inc[i]) {
        // Compton: Klein-Nishina by rejection
        const double eps = ph.e / kMeC2;
        double ct = 1, p = 1;
        for (int n = 0; n < 1000; ++n) {
          ct = 2 * rng.Rand() - 1;
          p = 1 / (1 + eps * (1 - ct));
          if (2 * rng.Rand() <= p * p * (p + 1 / p - (1 - ct * ct))) {
            break;
          }
        }
        dep += ph.e * (1 - p);
        ph.e *= p;
        ph.u = scatter(ph.u, ct, rng);
        continue;
      }
      // Pair production: electrons deposit locally, positron annihilates at rest
      dep += ph.e - 2 * kMeC2;
      const Vec d = isotropic(rng);
      stack.push_back({ph.p, d, kMeC2});
      stack.push_back({ph.p, {-d.x, -d.y, -d.z}, kMeC2});
      break;
    }
  }
  return dep;
}

} // namespace

std::string loadScintillatorDb(const std::string& path, ScintillatorDb& db) {
  std::ifstream f(path);
  if (!f) {
    return "Cannot open " + path;
  }
  const nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
  if (j.is_discarded()) {
    return "Invalid JSON in " + path;
  }
  db = {};
  const nlohmann::json sizes = j.value("sizes", nlohmann::json::array());
  const nlohmann::json materials = j.value("materials", nlohmann::json::array());
  for (const auto& s : sizes) {
    CrystalSize c;
    c.id = s.value("id", "");
    c.name = s.value("name", c.id);
    c.cylinder = s.value("shape", "box") == "cylinder";
    c.width = s.value("width", 10.0);
    c.length = s.value("length", 10.0);
    c.distance = s.value("distance", 10.0);
    db.sizes.push_back(c);
  }
  for (const auto& m : materials) {
    ScintMaterial s;
    s.id = m.value("id", "");
    s.name = m.value("name", s.id);
    s.density = m.value("density", 1.0);
    const nlohmann::json comp = m.value("composition", nlohmann::json::object());
    for (const auto& [sym, n] : comp.items()) {
      s.composition.emplace_back(sym, n.get<double>());
    }
    s.lightYield = m.value("lightYield", 1.0);
    s.resolution662 = m.value("resolution662", 0.0);
    s.riseTime = m.value("riseTime", 0.0);
    const nlohmann::json decay = m.value("decay", nlohmann::json::array());
    for (const auto& d : decay) {
      s.decay.emplace_back(d.at(0).get<double>(), d.at(1).get<double>());
    }
    const auto em = m.value("emission", nlohmann::json::object());
    s.emissionPeak = em.value("peak", 420.0);
    s.emissionFwhm = em.value("fwhm", 60.0);
    db.materials.push_back(s);
  }
  return db.materials.empty() || db.sizes.empty() ? "No materials or sizes in " + path : std::string();
}

SpectrumResult simulateSpectrum(const ScintMaterial& material, const CrystalSize& size,
                                const std::vector<GammaLine>& lines, const std::string& xcomDir, uint64_t nDetected,
                                uint64_t seed) {
  SpectrumResult res;
  Attenuation att;
  res.error = buildAttenuation(material, xcomDir, att);
  if (!res.error.empty()) {
    return res;
  }
  if (lines.empty()) {
    res.error = "No emission lines";
    return res;
  }
  const Geometry geo{size.cylinder, size.width / 2, size.length};
  const Vec source{0, 0, -size.distance};
  // Widest polar angle under which the source sees the crystal (front face rim/corner)
  const double rim = size.cylinder ? size.width / 2 : size.width / std::sqrt(2.0);
  const double cosMax = size.distance / std::hypot(size.distance, rim);
  const double sigmaK = material.resolution662 / 100 / 2.3548 * std::sqrt(661.657);
  sipm::SiPMRandom rng;
  rng.seed(seed);

  std::vector<Photon> stack;
  const uint64_t maxDecays = nDetected * 100000;
  while (res.detected < nDetected && res.decays < maxDecays) {
    ++res.decays;
    stack.clear();
    auto emit = [&](const Vec& u, double e) {
      const double t = geo.enter(source, u);
      if (t >= 0) {
        stack.push_back({{source.x + t * u.x, source.y + t * u.y, source.z + t * u.z}, u, e});
      }
    };
    // Isotropic direction with the polar cosine drawn first: most photons
    // fall outside the cone containing the crystal and are rejected cheaply
    auto direction = [&](double cz) {
      const double phi = 2 * M_PI * rng.Rand(), st = std::sqrt(std::max(0.0, 1 - cz * cz));
      return Vec{st * std::cos(phi), st * std::sin(phi), cz};
    };
    for (const GammaLine& l : lines) {
      if (std::abs(l.energy - kMeC2) < 0.5) {
        // Annihilation: back-to-back pair, intensity counts both photons
        if (rng.Rand() <= l.intensity / 200) {
          const double cz = 2 * rng.Rand() - 1;
          if (std::abs(cz) >= cosMax) {
            const Vec u = direction(cz);
            emit(u, l.energy);
            emit({-u.x, -u.y, -u.z}, l.energy);
          }
        }
      } else if (rng.Rand() <= l.intensity / 100) {
        const double cz = 2 * rng.Rand() - 1;
        if (cz >= cosMax) {
          emit(direction(cz), l.energy);
        }
      }
    }
    if (stack.empty()) {
      continue;
    }
    double dep = transport(stack, geo, att, rng);
    if (dep <= 0) {
      continue;
    }
    if (sigmaK > 0) {
      dep = std::max(0.0, dep + rng.randGaussian(0, sigmaK * std::sqrt(dep)));
    }
    const auto bin = static_cast<size_t>(dep);
    if (bin >= res.counts.size()) {
      res.counts.resize(bin + 1, 0.0);
    }
    res.counts[bin] += 1;
    ++res.detected;
  }
  if (res.detected == 0) {
    res.error = "No energy deposited";
  }
  return res;
}

std::string spectrumFileName(const std::string& materialId, const std::string& sizeId, const std::string& source) {
  return materialId + "_" + sizeId + "_" + source + ".csv";
}

} // namespace sipmgui
