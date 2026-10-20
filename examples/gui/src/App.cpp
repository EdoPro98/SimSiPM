#include "App.h"

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <system_error>
#include <thread>

#include "FileIO.h"
#include "SipmModels.h"
#include <nlohmann/json.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"
#include "misc/cpp/imgui_stdlib.h"

namespace sipmgui {
namespace {

constexpr ImVec4 kColTrace{1.00f, 0.84f, 0.04f, 1.0f};
constexpr ImVec4 kColWindow{0.30f, 0.64f, 1.00f, 1.0f};
constexpr ImVec4 kColThreshold{1.00f, 0.42f, 0.42f, 1.0f};
constexpr ImVec4 kColTrigger{0.24f, 0.86f, 0.59f, 1.0f};
constexpr ImVec4 kColHist{0.30f, 0.64f, 1.00f, 1.0f};
constexpr ImVec4 kColError{1.00f, 0.42f, 0.42f, 1.0f};

// Indexed by SiPMHit::HitType
constexpr ImVec4 kHitColors[6] = {
  {0.30f, 0.64f, 1.00f, 1.0f}, // photoelectron
  {0.60f, 0.65f, 0.70f, 1.0f}, // dark count
  {1.00f, 0.62f, 0.26f, 1.0f}, // crosstalk
  {1.00f, 0.84f, 0.04f, 1.0f}, // delayed crosstalk
  {1.00f, 0.42f, 0.42f, 1.0f}, // fast afterpulse
  {0.75f, 0.52f, 0.99f, 1.0f}, // slow afterpulse
};
constexpr const char* kHitNames[6] = {"Photoelectron", "Dark count", "Crosstalk", "Delayed XT", "AP fast", "AP slow"};
constexpr ImU32 kEmptyCell = IM_COL32(32, 37, 46, 255);
constexpr ImU32 kCellBorder = IM_COL32(20, 23, 28, 255);

constexpr int kBinChoices[] = {64, 128, 256, 512, 1024, 2048};
constexpr int kPersistChoices[] = {0, 10, 50, 200};
constexpr int kMaxTexture = 1024;

float g_LabelW = 120;

// ---------------------------------------------------------------- form helpers
// Every row is "label | widget". Numeric fields only commit on Enter.

void rowLabel(const char* text) {
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(text);
  ImGui::SameLine(g_LabelW);
  ImGui::SetNextItemWidth(-FLT_MIN);
}

bool inputDouble(const char* text, double* v, const char* fmt, double lo, double hi) {
  rowLabel(text);
  ImGui::PushID(text);
  double tmp = *v;
  bool changed = ImGui::InputDouble("##v", &tmp, 0, 0, fmt, ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::PopID();
  tmp = std::clamp(tmp, lo, hi);
  changed = changed && tmp != *v;
  *v = tmp;
  return changed;
}

bool inputInt(const char* text, int* v, int lo, int hi) {
  rowLabel(text);
  ImGui::PushID(text);
  int tmp = *v;
  bool changed = ImGui::InputInt("##v", &tmp, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::PopID();
  tmp = std::clamp(tmp, lo, hi);
  changed = changed && tmp != *v;
  *v = tmp;
  return changed;
}

/// Checkbox in the label column, numeric field greyed out while unchecked.
bool checkedDouble(const char* text, bool* on, double* v, const char* fmt, double lo, double hi) {
  ImGui::PushID(text);
  bool changed = ImGui::Checkbox(text, on);
  ImGui::SameLine(g_LabelW);
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::BeginDisabled(!*on);
  double tmp = *v;
  const bool entered = ImGui::InputDouble("##v", &tmp, 0, 0, fmt, ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::EndDisabled();
  ImGui::PopID();
  tmp = std::clamp(tmp, lo, hi);
  changed |= entered && tmp != *v;
  *v = tmp;
  return changed;
}

template <typename E>
bool combo(const char* text, E* v, const char* items) {
  rowLabel(text);
  ImGui::PushID(text);
  int i = static_cast<int>(*v);
  const bool changed = ImGui::Combo("##c", &i, items);
  ImGui::PopID();
  *v = static_cast<E>(i);
  return changed;
}

/// Small inline combo for toolbars (label on the left).
bool toolCombo(const char* text, int* v, const char* items, float width) {
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(text);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(width);
  ImGui::PushID(text);
  const bool changed = ImGui::Combo("##c", v, items);
  ImGui::PopID();
  return changed;
}

std::string formatRate(double r) {
  char buf[32];
  if (r >= 1e6) {
    std::snprintf(buf, sizeof(buf), "%.2f M/s", r / 1e6);
  } else if (r >= 1e3) {
    std::snprintf(buf, sizeof(buf), "%.1f k/s", r / 1e3);
  } else {
    std::snprintf(buf, sizeof(buf), "%.0f /s", r);
  }
  return buf;
}

float featureOf(const EventFeatures& f, int i) {
  switch (i) {
    case 0:
      return f.peak;
    case 1:
      return f.integral;
    case 2:
      return f.tot;
    default:
      return f.toa;
  }
}

/// Rate of acquired windows [Hz]: source rate reduced by the dead time of the
/// trigger (one window length, non-paralyzable). 0 for independent events.
double windowRateHz(const SimConfig& c) {
  if (c.timing == TimingMode::kIndependent || c.rateKHz <= 0) {
    return 0;
  }
  const double r = c.rateKHz * 1e3, dead = c.signalLength * 1e-9;
  if (c.timing == TimingMode::kPoisson) {
    return r / (1 + r * dead);
  }
  // Periodic: a window can only open on a source tick after the dead time
  return r / std::max(1.0, std::ceil(dead * r - 1e-9));
}

/// White background, dark axes: used for one frame while exporting a plot
void pushPrintStyle() {
  const ImVec4 white(1, 1, 1, 1), black(0, 0, 0, 1), grey(0.55f, 0.55f, 0.55f, 1);
  ImPlot::PushStyleColor(ImPlotCol_FrameBg, white);
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, white);
  ImPlot::PushStyleColor(ImPlotCol_PlotBorder, grey);
  ImPlot::PushStyleColor(ImPlotCol_LegendBg, white);
  ImPlot::PushStyleColor(ImPlotCol_LegendBorder, grey);
  ImPlot::PushStyleColor(ImPlotCol_LegendText, black);
  ImPlot::PushStyleColor(ImPlotCol_TitleText, black);
  ImPlot::PushStyleColor(ImPlotCol_InlayText, black);
  ImPlot::PushStyleColor(ImPlotCol_AxisText, black);
  ImPlot::PushStyleColor(ImPlotCol_AxisGrid, ImVec4(0, 0, 0, 0.12f));
  ImPlot::PushStyleColor(ImPlotCol_AxisTick, ImVec4(0, 0, 0, 0.5f));
}
void popPrintStyle() { ImPlot::PopStyleColor(11); }

// Colors of the frozen reference histograms
constexpr ImVec4 kRefColors[] = {{1.00f, 0.62f, 0.26f, 1},
                                 {0.24f, 0.86f, 0.59f, 1},
                                 {0.75f, 0.52f, 0.99f, 1},
                                 {1.00f, 0.42f, 0.42f, 1},
                                 {0.60f, 0.65f, 0.70f, 1}};

/// Numeric parameters that can be scanned
struct ScanParam {
  const char* key;
  const char* label;
  double SimConfig::* member;
};
const ScanParam kScanParams[] = {
  {"dcrKHz", "DCR [kHz]", &SimConfig::dcrKHz},
  {"xtPct", "XT [%]", &SimConfig::xtPct},
  {"dxtPct", "Delayed XT [%]", &SimConfig::dxtPct},
  {"apPct", "AP [%]", &SimConfig::apPct},
  {"ccgvPct", "CCGV [%]", &SimConfig::ccgvPct},
  {"snrDb", "SNR [dB]", &SimConfig::snrDb},
  {"sizeMm", "Size [mm]", &SimConfig::sizeMm},
  {"pitchUm", "Cell pitch [um]", &SimConfig::pitchUm},
  {"riseTime", "Rise tau [ns]", &SimConfig::riseTime},
  {"fallTimeFast", "Fall tau [ns]", &SimConfig::fallTimeFast},
  {"fallTimeSlow", "Slow tau [ns]", &SimConfig::fallTimeSlow},
  {"recoveryTime", "Recovery tau [ns]", &SimConfig::recoveryTime},
  {"pdePct", "PDE [%]", &SimConfig::pdePct},
  {"rateKHz", "Rate [kHz]", &SimConfig::rateKHz},
  {"nphMu", "Mean photons", &SimConfig::nphMu},
  {"nphScale", "Histogram scale [ph/bin]", &SimConfig::nphScale},
  {"lightYieldScale", "Light yield scale", &SimConfig::lightYieldScale},
  {"timeOffset", "Time offset [ns]", &SimConfig::timeOffset},
  {"timeSigma", "Time sigma [ns]", &SimConfig::timeSigma},
  {"timeTau", "Time tau [ns]", &SimConfig::timeTau},
  {"intStart", "Int. start [ns]", &SimConfig::intStart},
  {"intWindow", "Int. window [ns]", &SimConfig::intWindow},
  {"threshold", "Threshold [p.e.]", &SimConfig::threshold},
};
constexpr int kNumScanParams = sizeof(kScanParams) / sizeof(kScanParams[0]);
constexpr const char* kFeatureNames[] = {"peak", "integral", "tot", "toa"};

/// Value of step i of n between start and end, linear or logarithmic
double scanValue(double start, double end, int i, int n, bool log) {
  if (n <= 1) {
    return start;
  }
  const double f = static_cast<double>(i) / (n - 1);
  return log ? start * std::pow(end / start, f) : start + (end - start) * f;
}

/// Detection efficiency recorded in the header of a precomputed spectrum
std::string readEfficiency(const std::string& path) {
  std::ifstream f(path);
  std::string line;
  while (std::getline(f, line)) {
    const auto p = line.find("(efficiency ");
    if (line.rfind("# decays", 0) == 0 && p != std::string::npos) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%.3g%%", 100 * std::atof(line.c_str() + p + 12));
      return buf;
    }
  }
  return "n/a";
}

std::filesystem::path dataDir() {
  std::error_code ec;
  const std::filesystem::path p(SIPM_GUI_DATA_DIR);
  return std::filesystem::is_directory(p, ec) ? p : std::filesystem::current_path(ec);
}

} // namespace

// ================================================================= Histogram

void Histogram::clear() {
  m_Values.clear();
  m_Stairs.assign(m_Counts.size() + 1, 0.0);
  std::fill(m_Counts.begin(), m_Counts.end(), 0.0);
  m_Entries = 0;
  m_Below = 0;
  m_Ranged = false;
}

void Histogram::add(float v) {
  if (v < 0) { // library sentinel: nothing above threshold
    ++m_Below;
    return;
  }
  ++m_Entries;
  if (m_Values.size() < kStoreCap) {
    m_Values.push_back(v);
  }
  if (!m_Ranged) {
    if (m_Values.size() >= kLearn) {
      autoRange();
    }
    return;
  }
  fill(v);
}

void Histogram::fill(float v) {
  const int n = static_cast<int>(m_Counts.size());
  const int idx = static_cast<int>(std::floor((v - m_Lo) / (m_Hi - m_Lo) * n));
  if (idx < 0 || idx >= n) {
    return;
  }
  m_Counts[idx] += 1;
  m_Stairs[idx] = m_Counts[idx];
  if (idx == n - 1) {
    m_Stairs[n] = m_Counts[idx];
  }
}

void Histogram::setBins(int n) {
  m_NBins = n;
  m_Counts.assign(n, 0.0);
  m_Stairs.assign(n + 1, 0.0);
  if (m_Ranged) {
    autoRange();
  }
}

void Histogram::setQuantum(double q) {
  m_Quantum = q;
  if (m_Ranged) {
    autoRange();
  }
}

void Histogram::autoRange() {
  if (m_Values.empty()) {
    return;
  }
  // Robust range: ignore the most extreme 0.1% on each side
  std::vector<float> tmp(m_Values);
  const size_t iLo = tmp.size() / 1000, iHi = tmp.size() - 1 - tmp.size() / 1000;
  std::nth_element(tmp.begin(), tmp.begin() + iLo, tmp.end());
  const double mn = tmp[iLo];
  std::nth_element(tmp.begin(), tmp.begin() + iHi, tmp.end());
  const double mx = tmp[iHi];

  double span = mx - mn;
  if (span <= 0) {
    span = std::max(1.0, std::abs(mn));
  }
  m_Lo = mn - 0.05 * span;
  m_Hi = mx + 0.05 * span;
  if (m_Quantum > 0) {
    // Values are multiples of the sampling time: use an integer number of
    // samples per bin, with bins centred on the samples (avoids aliasing combs)
    // (bin count may shrink below the requested one for narrow distributions)
    const double bw = std::max(1.0, std::ceil((m_Hi - m_Lo) / m_NBins / m_Quantum)) * m_Quantum;
    const double hi = m_Hi;
    m_Lo = std::floor(m_Lo / m_Quantum) * m_Quantum - 0.5 * m_Quantum; // edges at half samples
    const int n = std::clamp(static_cast<int>(std::ceil((hi - m_Lo) / bw)), 1, m_NBins);
    m_Hi = m_Lo + bw * n;
    m_Counts.assign(n, 0.0);
  } else {
    m_Counts.assign(m_NBins, 0.0);
  }
  m_Ranged = true;
  m_RangeChanged = true;
  rebin();
}

void Histogram::rebin() {
  std::fill(m_Counts.begin(), m_Counts.end(), 0.0);
  m_Stairs.assign(m_Counts.size() + 1, 0.0);
  for (const float v : m_Values) {
    fill(v);
  }
}

// =============================================================== FileBrowser

void FileBrowser::open(const std::string& title, const std::string& current, const std::vector<std::string>& extensions,
                       bool save) {
  m_Title = title;
  m_Extensions = extensions;
  m_Save = save;
  std::error_code ec;
  const std::filesystem::path cur(current);
  if (!current.empty() && std::filesystem::is_directory(cur.parent_path(), ec)) {
    m_Dir = std::filesystem::absolute(cur.parent_path(), ec);
  } else {
    m_Dir = save ? std::filesystem::current_path(ec) : dataDir();
  }
  m_Selected =
    current.empty() || std::filesystem::path(current).has_parent_path() ? current : (m_Dir / current).string();
  m_RequestOpen = true;
  refresh();
}

void FileBrowser::refresh() {
  m_Dirs.clear();
  m_Files.clear();
  std::error_code ec;
  for (std::filesystem::directory_iterator it(m_Dir, ec), end; !ec && it != end; it.increment(ec)) {
    const auto& p = it->path();
    if (p.filename().string().rfind('.', 0) == 0) {
      continue;
    } // hidden
    if (it->is_directory(ec)) {
      m_Dirs.push_back(p);
    } else {
      const std::string ext = p.extension().string();
      if (std::find(m_Extensions.begin(), m_Extensions.end(), ext) != m_Extensions.end()) {
        m_Files.push_back(p);
      }
    }
  }
  std::sort(m_Dirs.begin(), m_Dirs.end());
  std::sort(m_Files.begin(), m_Files.end());
}

bool FileBrowser::draw(std::string& chosen) {
  if (m_RequestOpen) {
    ImGui::OpenPopup(m_Title.c_str());
    m_RequestOpen = false;
  }
  const float fs = ImGui::GetFontSize();
  ImGui::SetNextWindowSize(ImVec2(42 * fs, 30 * fs), ImGuiCond_Appearing);
  bool done = false;
  if (ImGui::BeginPopupModal(m_Title.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings)) {
    if (ImGui::Button("Up")) {
      m_Dir = m_Dir.parent_path();
      refresh();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", m_Dir.string().c_str());

    const float footer = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    ImGui::BeginChild("##list", ImVec2(0, -footer), ImGuiChildFlags_Borders);
    std::filesystem::path enter;
    for (const auto& d : m_Dirs) {
      if (ImGui::Selectable((d.filename().string() + "/").c_str())) {
        enter = d;
      }
    }
    for (const auto& f : m_Files) {
      const std::string full = f.string();
      if (ImGui::Selectable(f.filename().string().c_str(), m_Selected == full, ImGuiSelectableFlags_AllowDoubleClick)) {
        m_Selected = full;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          done = true;
        }
      }
    }
    ImGui::EndChild();
    if (!enter.empty()) {
      m_Dir = enter;
      refresh();
    }

    const char* action = m_Save ? "Save" : "Open";
    const float buttons = ImGui::CalcTextSize(action).x + ImGui::CalcTextSize("Cancel").x + 6 * fs;
    ImGui::SetNextItemWidth(-buttons);
    if (ImGui::InputText("##path", &m_Selected, ImGuiInputTextFlags_EnterReturnsTrue)) {
      done = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(action)) {
      done = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      ImGui::CloseCurrentPopup();
    }
    std::error_code ec;
    if (done && std::filesystem::is_directory(m_Selected, ec)) {
      m_Dir = std::filesystem::absolute(m_Selected, ec);
      refresh();
      done = false;
    }
    if (done && m_Save && !m_Extensions.empty() && std::filesystem::path(m_Selected).extension().empty()) {
      m_Selected += m_Extensions.front();
    }
    if (done) {
      chosen = m_Selected;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
  return done;
}

// ======================================================================= App

App::App() {
  const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
  // One thread generates events, one runs the GUI, the rest simulate
  m_MaxWorkers = static_cast<int>(std::max(1u, hw > 2 ? hw - 2 : 1u));
  m_NWorkers = m_MaxWorkers;

  // Ready-made photon number profiles (nuclide energy spectra)
  std::error_code ec;
  for (std::filesystem::directory_iterator it(dataDir() / "nuclides", ec), end; !ec && it != end; it.increment(ec)) {
    if (it->path().extension() == ".csv") {
      m_Profiles.push_back(it->path());
    }
  }
  std::sort(m_Profiles.begin(), m_Profiles.end());
  m_ProfileItems = std::string("Custom file") + '\0';
  for (const auto& p : m_Profiles) {
    m_ProfileItems += p.stem().string() + '\0';
  }

  // Preload the example files so switching mode works out of the box
  const std::filesystem::path data = dataDir();
  m_Cfg.pdeFile = (data / "pde_example.csv").string();
  m_Cfg.nphFile = (data / "nphotons_example.csv").string();
  loadFile(FileTarget::kPde);
  loadFile(FileTarget::kNPhotons);
  // Commercial SiPM presets
  if (loadSipmModels((data / "sipm_models.json").string(), m_Models).empty()) {
    m_ModelItems = std::string("Custom") + '\0';
    for (const auto& m : m_Models) {
      m_ModelItems += m.name + '\0';
    }
  }

  // Scintillator presets
  if (loadScintillatorDb((data / "scintillators.json").string(), m_Db).empty()) {
    for (size_t i = 0; i < m_Db.materials.size(); ++i) {
      m_MaterialItems += m_Db.materials[i].name + '\0';
      if (m_Db.materials[i].id == "LYSO") {
        m_MaterialIdx = static_cast<int>(i);
      }
    }
    for (const auto& s : m_Db.sizes) {
      m_SizeItems += s.name + '\0';
    }
  }
  m_Cfg.gammaFile = (data / "nuclides" / "Cs-137.csv").string();
  loadFile(FileTarget::kGamma);

  glGenTextures(1, &m_Texture);
  glBindTexture(GL_TEXTURE_2D, m_Texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  for (auto& h : m_Hists) {
    h.setBins(kBinChoices[m_BinsIdx]);
  }
  m_Hists[2].setQuantum(m_Cfg.sampling);
  m_Hists[3].setQuantum(m_Cfg.sampling);
  m_Version = m_Pipeline.setConfig(m_Cfg);
  m_Pipeline.start(m_NWorkers);
  m_RateTime = Clock::now();
}

App::~App() {
  stopRecording("Recording stopped");
  m_Pipeline.stop();
  if (m_Texture != 0) {
    glDeleteTextures(1, &m_Texture);
  }
}

void App::frame() {
  if (m_Recording && m_Pipeline.recorder().finished()) {
    stopRecording("Recording complete");
  }
  if (m_Export.plot != PlotId::kNone && m_Export.wait > 0) {
    --m_Export.wait;
  }
  drainResults();
  updateScan();

  const auto now = Clock::now();
  const double dt = std::chrono::duration<double>(now - m_RateTime).count();
  if (dt >= 0.5) {
    const uint64_t gen = m_Pipeline.nGenerated, sim = m_Pipeline.nSimulated, drop = m_Pipeline.nDropped;
    m_RateGen = (gen - m_LastGen) / dt;
    m_RateSim = (sim - m_LastSim) / dt;
    m_RateDrop = (drop - m_LastDrop) / dt;
    m_RateGui = (m_NDisplayed - m_LastShown) / dt;
    m_LastGen = gen;
    m_LastSim = sim;
    m_LastDrop = drop;
    m_LastShown = m_NDisplayed;
    m_RateTime = now;
  }

  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::Begin("##main", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

  const float fs = ImGui::GetFontSize();
  g_LabelW = 9.0f * fs;

  ImGui::BeginChild("##controls", ImVec2(21 * fs, 0), ImGuiChildFlags_Borders);
  ImGui::BeginDisabled(m_Scan.running); // the scan owns the settings while it runs
  drawControls();
  ImGui::EndDisabled();
  ImGui::EndChild();

  ImGui::SameLine();
  ImGui::BeginChild("##right", ImVec2(0, 0));
  const float status = ImGui::GetFrameHeightWithSpacing();
  const float avail = ImGui::GetContentRegionAvail().y - status - ImGui::GetStyle().ItemSpacing.y;
  drawScope(std::floor(avail * 0.56f));
  drawBottom(std::floor(avail * 0.44f));
  drawStatusBar();
  ImGui::EndChild();

  std::string chosen;
  if (m_Browser.draw(chosen) && m_BrowserTarget == FileTarget::kSaveSettings) {
    saveSettings(chosen);
  } else if (!chosen.empty() && m_BrowserTarget == FileTarget::kLoadSettings) {
    loadSettings(chosen);
  } else if (!chosen.empty() && m_BrowserTarget == FileTarget::kExportCsv) {
    const std::string err = writeHistogramCsv(chosen, static_cast<int>(m_View), "");
    message(err.empty() ? "Saved " + chosen : err);
  } else if (!chosen.empty() && m_BrowserTarget == FileTarget::kRecordFile) {
    m_RecFile = chosen;
  } else if (!chosen.empty() && m_BrowserTarget == FileTarget::kExportPng) {
    m_Export = {};
    m_Export.plot = m_ExportPending;
    m_Export.wait = 2; // let the dialog disappear first
    m_Export.path = chosen;
  } else if (!chosen.empty()) {
    if (m_BrowserTarget == FileTarget::kPde) {
      m_Cfg.pdeFile = chosen;
    }
    if (m_BrowserTarget == FileTarget::kGamma) {
      m_Cfg.gammaFile = chosen;
    }
    if (m_BrowserTarget == FileTarget::kNPhotons) {
      m_Cfg.nphFile = chosen;
    }
    loadFile(m_BrowserTarget);
    m_Dirty = true;
  }

  ImGui::End();
  drawScanWindow();

  if (m_Dirty && m_Scan.running) {
    // Settings are owned by the scan: undo edits made on the plots (gate, threshold)
    m_Cfg = m_Scan.stepConfig;
    m_Dirty = false;
    message("Settings are locked while a scan is running");
  }
  if (m_Dirty) {
    applyConfig();
  }
}

// ------------------------------------------------------------------ controls

void App::drawControls() {
  SimConfig& c = m_Cfg;
  bool d = false;
  const float fs = ImGui::GetFontSize();

  // Run / workers
  const bool running = m_Pipeline.isRunning();
  ImGui::PushStyleColor(ImGuiCol_Button, running ? ImVec4(0.55f, 0.20f, 0.22f, 1) : ImVec4(0.16f, 0.45f, 0.30f, 1));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        running ? ImVec4(0.66f, 0.25f, 0.27f, 1) : ImVec4(0.20f, 0.55f, 0.36f, 1));
  if (ImGui::Button(running ? "Stop" : "Run", ImVec2(-FLT_MIN, 1.8f * fs))) {
    m_Pipeline.setRunning(!running);
  }
  ImGui::PopStyleColor(2);
  {
    const float w = 0.5f * (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::Button("Save settings", ImVec2(w, 0))) {
      m_BrowserTarget = FileTarget::kSaveSettings;
      m_Browser.open("Save settings", "sipm_settings.json", {".json"}, true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load settings", ImVec2(w, 0))) {
      m_BrowserTarget = FileTarget::kLoadSettings;
      m_Browser.open("Load settings", "", {".json"});
    }
    if (ImGui::Button("Parameter scan...", ImVec2(-FLT_MIN, 0))) {
      m_Scan.window = true;
    }
  }
  const bool workers = inputInt("Worker threads", &m_NWorkers, 1, m_MaxWorkers);
  ImGui::SetItemTooltip("Simulation threads (max %d): one more thread generates the events and one runs the GUI",
                        m_MaxWorkers);
  if (workers) {
    restartPipeline();
  }

  ImGui::SeparatorText("Sensor");
  rowLabel("Model");
  if (ImGui::Combo("##model", &m_ModelIdx, m_ModelItems.c_str()) && m_ModelIdx > 0) {
    applyModel(m_ModelIdx - 1);
    d = true;
  }
  if (m_ModelIdx > 0) {
    const SipmModel& m = m_Models[m_ModelIdx - 1];
    const auto peak = std::max_element(m.pdeValues.begin(), m.pdeValues.end()) - m.pdeValues.begin();
    std::string est;
    for (const auto& e : m.estimated) {
      est += (est.empty() ? "" : ", ") + e;
    }
    ImGui::SetCursorPosX(g_LabelW);
    ImGui::PushTextWrapPos(0);
    ImGui::TextDisabled("%s, datasheet at Vov %g V, gain %.2g, PDE %.0f%% @ %.0f nm.%s%s", m.maker.c_str(), m.vov,
                        m.gain, 100 * m.pdeValues[peak], m.pdeWavelengths[peak],
                        est.empty() ? "" : " Estimated: ", est.c_str());
    ImGui::PopTextWrapPos();
  }
  d |= inputDouble("Size [mm]", &c.sizeMm, "%.3g", 0.1, 50);
  d |= inputDouble("Cell pitch [µm]", &c.pitchUm, "%.3g", 1, 500);
  {
    const uint32_t side = static_cast<uint32_t>(1000 * c.sizeMm / c.pitchUm);
    ImGui::SetCursorPosX(g_LabelW);
    ImGui::TextDisabled("%u × %u = %u cells", side, side, side * side);
  }
  d |= combo("Light profile", &c.lightProfile, "Uniform\0Circle\0Gaussian\0");

  ImGui::SeparatorText("Noise");
  d |= checkedDouble("DCR [kHz]", &c.dcrOn, &c.dcrKHz, "%.4g", 0, 1e6);
  d |= checkedDouble("XT [%]", &c.xtOn, &c.xtPct, "%.3g", 0, 100);
  ImGui::BeginDisabled(!c.xtOn);
  d |= checkedDouble("Delayed XT [%]", &c.dxtOn, &c.dxtPct, "%.3g", 0, 100);
  ImGui::EndDisabled();
  d |= checkedDouble("AP [%]", &c.apOn, &c.apPct, "%.3g", 0, 100);
  ImGui::BeginDisabled(!c.apOn);
  d |= inputDouble("AP τ fast [ns]", &c.apTauFast, "%.3g", 0.01, 1e5);
  d |= inputDouble("AP τ slow [ns]", &c.apTauSlow, "%.3g", 0.01, 1e5);
  ImGui::EndDisabled();
  d |= inputDouble("CCGV [%]", &c.ccgvPct, "%.3g", 0, 100);
  d |= inputDouble("SNR [dB]", &c.snrDb, "%.3g", -20, 200);

  ImGui::SeparatorText("Signal");
  d |= inputDouble("Length [ns]", &c.signalLength, "%.4g", 10, 1e6);
  d |= inputDouble("Sampling [ns]", &c.sampling, "%.3g", 0.01, 100);
  d |= inputDouble("Rise τ [ns]", &c.riseTime, "%.3g", 0.01, 1e5);
  d |= inputDouble("Fall τ [ns]", &c.fallTimeFast, "%.3g", 0.01, 1e5);
  d |= checkedDouble("Slow τ [ns]", &c.slowOn, &c.fallTimeSlow, "%.3g", 0.01, 1e5);
  ImGui::BeginDisabled(!c.slowOn);
  d |= inputDouble("Slow fraction [%]", &c.slowPct, "%.3g", 0, 100);
  ImGui::EndDisabled();
  d |= inputDouble("Recovery τ [ns]", &c.recoveryTime, "%.3g", 0.01, 1e5);

  ImGui::SeparatorText("Detection efficiency");
  d |= combo("PDE", &c.pdeMode, "No PDE\0Constant\0Spectrum\0");
  if (c.pdeMode == PdeMode::kConstant) {
    d |= inputDouble("PDE [%]", &c.pdePct, "%.3g", 0, 100);
  }
  if (c.pdeMode == PdeMode::kSpectrum) {
    fileField("PDE file", c.pdeFile, FileTarget::kPde, m_PdeStatus, m_PdeOk);
  }

  ImGui::SeparatorText("Light source");
  d |= combo("Event timing", &c.timing, "Independent\0Poisson\0Periodic\0");
  ImGui::SetItemTooltip("Independent: one event per window, no pile-up.\n"
                        "Poisson / Periodic: stream of events at the given rate. Each window is triggered\n"
                        "by an event (time offset = pre-trigger) and includes any other event overlapping it.");
  {
    const bool stream = c.timing != TimingMode::kIndependent;
    ImGui::BeginDisabled(!stream);
    d |= inputDouble("Rate [kHz]", &c.rateKHz, "%.4g", 1e-3, 1e7);
    ImGui::EndDisabled();
    if (stream) {
      ImGui::SetCursorPosX(g_LabelW);
      ImGui::TextDisabled("mean %.3g events / window", c.rateKHz * 1e-6 * c.signalLength);
    }
  }
  d |= combo("Photons", &c.nphMode, "Fixed\0Poisson\0Exponential\0Histogram file\0Gamma source\0");
  switch (c.nphMode) {
    case NPhotonMode::kFixed:
      d |= inputInt("N photons", &c.nphFixed, 0, 1000000);
      break;
    case NPhotonMode::kPoisson:
    case NPhotonMode::kExponential:
      d |= inputDouble("Mean µ", &c.nphMu, "%.4g", 0, 1e6);
      break;
    case NPhotonMode::kFile:
      {
        std::string status = m_NphStatus;
        if (m_NphOk) {
          char buf[96];
          std::snprintf(buf, sizeof(buf), "%zu bins, mean %.4g ph", c.nphHistogram.size(), m_NphMeanBin * c.nphScale);
          status = buf;
        }
        fileField("Histogram", c.nphFile, FileTarget::kNPhotons, status, m_NphOk);
        d |= inputDouble("Scale [ph/bin]", &c.nphScale, "%.4g", 0, 1e6);
        ImGui::SetItemTooltip("Photons generated per histogram bin");
        break;
      }
    case NPhotonMode::kGamma:
      {
        rowLabel("Source");
        if (ImGui::Combo("##profile", &m_ProfileIdx, m_ProfileItems.c_str())) {
          if (m_ProfileIdx > 0) {
            c.gammaFile = m_Profiles[m_ProfileIdx - 1].string();
            loadFile(FileTarget::kGamma);
            d = true;
          } else {
            m_BrowserTarget = FileTarget::kGamma;
            m_Browser.open("Gamma lines", c.gammaFile, {".csv", ".txt", ".dat"});
          }
        }
        fileField("Lines", c.gammaFile, FileTarget::kGamma, m_GammaStatus, m_GammaOk);
        rowLabel("Crystal");
        if (ImGui::Combo("##material", &m_MaterialIdx, m_MaterialItems.c_str())) {
          loadGammaSpectrum();
          d = true;
        }
        rowLabel("Size");
        if (ImGui::Combo("##size", &m_SizeIdx, m_SizeItems.c_str())) {
          loadGammaSpectrum();
          d = true;
        }
        d |= inputDouble("Light yield scale", &c.lightYieldScale, "%.4g", 0, 100);
        ImGui::SetItemTooltip("Multiplies the material light yield: use it for light collection\n"
                              "and coupling efficiency onto the SiPM");
        if (!m_Db.materials.empty()) {
          const ScintMaterial& m = c.scint;
          std::string decay;
          for (const auto& [tau, frac] : m.decay) {
            char b[32];
            std::snprintf(b, sizeof(b), "%s%g ns", decay.empty() ? "" : " + ", tau);
            decay += b;
          }
          ImGui::SetCursorPosX(g_LabelW);
          ImGui::PushTextWrapPos(0);
          ImGui::TextDisabled("%.3g ph/keV (%.3g x %g), τ %s, %g nm, FWHM %g%% @662 keV. %s",
                              m.lightYield * c.lightYieldScale, c.lightYieldScale, m.lightYield, decay.c_str(),
                              m.emissionPeak, m.resolution662, m_SpectrumStatus.c_str());
          ImGui::PopTextWrapPos();
        }
        ImGui::Spacing();
        break;
      }
  }
  if (c.nphMode == NPhotonMode::kGamma) {
    // Timing and emission spectrum come from the crystal
    d |= inputDouble("Time offset [ns]", &c.timeOffset, "%.4g", -1e6, 1e6);
  } else {
    d |= combo("Time profile", &c.timeMode, "Fixed\0Uniform\0Gaussian\0Exponential\0Scintillator\0");
    d |= inputDouble("Time offset [ns]", &c.timeOffset, "%.4g", -1e6, 1e6);
    switch (c.timeMode) {
      case TimeMode::kFixed:
        break;
      case TimeMode::kUniform:
        d |= inputDouble("Width [ns]", &c.timeWidth, "%.4g", 0, 1e6);
        break;
      case TimeMode::kGaussian:
        d |= inputDouble("σ [ns]", &c.timeSigma, "%.4g", 0, 1e6);
        break;
      case TimeMode::kExponential:
        d |= inputDouble("τ [ns]", &c.timeTau, "%.4g", 0, 1e6);
        break;
      case TimeMode::kScintillator:
        d |= inputDouble("Rise τ [ns] ", &c.timeRise, "%.4g", 0, 1e6);
        d |= inputDouble("Decay τ [ns]", &c.timeTau, "%.4g", 0, 1e6);
        break;
    }
    if (c.pdeMode == PdeMode::kSpectrum) {
      d |= combo("Wavelength", &c.wlMode, "Fixed\0Uniform\0Gaussian\0");
      switch (c.wlMode) {
        case WavelengthMode::kFixed:
          d |= inputDouble("λ [nm]", &c.wlFixed, "%.4g", 100, 2000);
          break;
        case WavelengthMode::kUniform:
          d |= inputDouble("λ min [nm]", &c.wlMin, "%.4g", 100, 2000);
          d |= inputDouble("λ max [nm]", &c.wlMax, "%.4g", 100, 2000);
          break;
        case WavelengthMode::kGaussian:
          d |= inputDouble("λ mean [nm]", &c.wlMean, "%.4g", 100, 2000);
          d |= inputDouble("λ σ [nm]", &c.wlSigma, "%.4g", 0, 1000);
          break;
      }
    }
  }

  ImGui::SeparatorText("Analysis");
  d |= inputDouble("Int. start [ns]", &c.intStart, "%.4g", 0, 1e6);
  d |= inputDouble("Int. window [ns]", &c.intWindow, "%.4g", 0.01, 1e6);
  d |= inputDouble("Threshold [p.e.]", &c.threshold, "%.3g", -1e3, 1e6);

  // Any change to a field set by the model makes it a custom sensor
  if (m_ModelIdx > 0 && !matchesModel()) {
    m_ModelIdx = 0;
  }
  m_Dirty |= d;

  drawRecording();
}

void App::applyModel(int i) {
  const SipmModel& m = m_Models[i];
  SimConfig& c = m_Cfg;
  c.sizeMm = m.sizeMm;
  c.pitchUm = m.pitchUm;
  c.dcrOn = true;
  c.dcrKHz = m.dcrKHz;
  c.xtOn = true;
  c.xtPct = m.xtPct;
  c.dxtOn = false;
  c.apOn = true;
  c.apPct = m.apPct;
  c.riseTime = m.riseNs;
  c.fallTimeFast = m.fallNs;
  c.slowOn = false;
  c.recoveryTime = m.recoveryNs;
  c.pdeMode = PdeMode::kSpectrum;
  c.pdeWavelengths = m.pdeWavelengths;
  c.pdeValues = m.pdeValues;
  c.pdeFile.clear();
  m_PdeOk = true;
  m_PdeStatus = "Datasheet curve of " + m.name;
  m_ModelApplied = c;
}

bool App::matchesModel() const {
  const SimConfig &a = m_Cfg, &b = m_ModelApplied;
  return a.sizeMm == b.sizeMm && a.pitchUm == b.pitchUm && a.dcrOn == b.dcrOn && a.dcrKHz == b.dcrKHz &&
         a.xtOn == b.xtOn && a.xtPct == b.xtPct && a.dxtOn == b.dxtOn && a.apOn == b.apOn && a.apPct == b.apPct &&
         a.riseTime == b.riseTime && a.fallTimeFast == b.fallTimeFast && a.slowOn == b.slowOn &&
         a.recoveryTime == b.recoveryTime && a.pdeMode == b.pdeMode && a.pdeValues == b.pdeValues &&
         a.pdeWavelengths == b.pdeWavelengths;
}

void App::fileField(const char* label, const std::string& path, FileTarget target, const std::string& status, bool ok) {
  const float button = ImGui::CalcTextSize("...").x + 2 * ImGui::GetStyle().FramePadding.x;
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::SameLine(g_LabelW);
  ImGui::SetNextItemWidth(-(button + ImGui::GetStyle().ItemSpacing.x));
  ImGui::PushID(label);
  // Only the file name fits the panel; the full path is in the tooltip and is
  // editable in the browser.
  std::string name = std::filesystem::path(path).filename().string();
  ImGui::InputText("##path", &name, ImGuiInputTextFlags_ReadOnly);
  const bool clicked = ImGui::IsItemClicked();
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
    ImGui::SetTooltip("%s", path.c_str());
  }
  ImGui::SameLine();
  if (ImGui::Button("...") || clicked) {
    m_BrowserTarget = target;
    m_Browser.open(label, path, {".csv", ".txt", ".dat"});
  }
  ImGui::PopID();
  ImGui::SetCursorPosX(g_LabelW);
  ImGui::PushTextWrapPos(0);
  if (ok) {
    ImGui::TextDisabled("%s", status.c_str());
  } else {
    ImGui::TextColored(kColError, "%s", status.c_str());
  }
  ImGui::PopTextWrapPos();
}

void App::loadFile(FileTarget target) {
  char buf[128];
  if (target == FileTarget::kPde) {
    std::vector<double> wl, pde;
    const std::string err = loadPdeCsv(m_Cfg.pdeFile, wl, pde);
    m_PdeOk = err.empty();
    if (m_PdeOk) {
      const double peak = *std::max_element(pde.begin(), pde.end());
      std::snprintf(buf, sizeof(buf), "%.0f-%.0f nm, max %.0f%%", wl.front(), wl.back(), 100 * peak);
      m_PdeStatus = buf;
      m_Cfg.pdeWavelengths = std::move(wl);
      m_Cfg.pdeValues = std::move(pde);
    } else {
      m_PdeStatus = err + " (PDE disabled)";
      m_Cfg.pdeWavelengths.clear();
      m_Cfg.pdeValues.clear();
    }
  } else if (target == FileTarget::kGamma) {
    m_ProfileIdx = 0;
    for (size_t i = 0; i < m_Profiles.size(); ++i) {
      std::error_code ec;
      if (std::filesystem::equivalent(m_Profiles[i], m_Cfg.gammaFile, ec)) {
        m_ProfileIdx = static_cast<int>(i) + 1;
      }
    }
    std::vector<double> e, in;
    const std::string err = loadLinesCsv(m_Cfg.gammaFile, e, in);
    m_GammaOk = err.empty();
    m_GammaLines.clear();
    if (m_GammaOk) {
      double perDecay = 0;
      for (size_t i = 0; i < e.size(); ++i) {
        perDecay += in[i] / 100;
        m_GammaLines.push_back({e[i], in[i]});
      }
      std::snprintf(buf, sizeof(buf), "%zu lines, %.3g γ/decay", e.size(), perDecay);
      m_GammaStatus = buf;
    } else {
      m_GammaStatus = err + " (0 photons)";
    }
    loadGammaSpectrum();
  } else if (target == FileTarget::kNPhotons) {
    std::vector<double> counts;
    const std::string err = loadHistogramCsv(m_Cfg.nphFile, counts);
    m_NphOk = err.empty();
    if (m_NphOk) {
      double sum = 0, mean = 0;
      for (size_t i = 0; i < counts.size(); ++i) {
        sum += counts[i];
        mean += i * counts[i];
      }
      m_NphMeanBin = mean / sum;
      m_NphStatus.clear();
      m_Cfg.nphHistogram = std::move(counts);
    } else {
      m_NphStatus = err + " (0 photons)";
      m_Cfg.nphHistogram.clear();
    }
  }
}

void App::loadGammaSpectrum() {
  m_Cfg.gammaSpectrum.clear();
  m_SpectrumStatus.clear();
  if (m_Db.materials.empty()) {
    m_SpectrumStatus = "No scintillator data";
    return;
  }
  const ScintMaterial& mat = m_Db.materials[m_MaterialIdx];
  const CrystalSize& size = m_Db.sizes[m_SizeIdx];
  m_Cfg.scint = mat;
  if (m_GammaLines.empty()) {
    return;
  }
  // Preset sources: spectrum precomputed at build time
  const std::string source = std::filesystem::path(m_Cfg.gammaFile).stem().string();
  if (m_ProfileIdx > 0) {
    const std::filesystem::path file =
      std::filesystem::path(SIPM_GUI_SPECTRA_DIR) / spectrumFileName(mat.id, size.id, source);
    if (loadHistogramCsv(file.string(), m_Cfg.gammaSpectrum).empty()) {
      m_SpectrumStatus = "Detection efficiency " + readEfficiency(file.string()) + ".";
      return;
    }
  }
  // Custom source (or missing preset): run the same Monte Carlo once and cache it
  const std::string key = m_Cfg.gammaFile + "|" + mat.id + "|" + size.id;
  auto it = m_SpectrumCache.find(key);
  if (it == m_SpectrumCache.end()) {
    const SpectrumResult r = simulateSpectrum(mat, size, m_GammaLines, (dataDir() / "xcom").string(), 50000, 12345);
    if (!r.error.empty()) {
      m_SpectrumStatus = r.error;
      return;
    }
    it = m_SpectrumCache.emplace(key, r.counts).first;
  }
  m_Cfg.gammaSpectrum = it->second;
  m_SpectrumStatus = "Spectrum computed at load.";
}

// --------------------------------------------------------------------- scope

void App::drawScope(float height) {
  const float fs = ImGui::GetFontSize();
  const float y0 = ImGui::GetCursorPosY();

  // Toolbar
  int trig = static_cast<int>(m_Trigger);
  if (toolCombo("Trigger", &trig, "Auto\0Normal\0Single\0", 6 * fs)) {
    m_Trigger = static_cast<TriggerMode>(trig);
    m_Armed = true;
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(4.5f * fs);
  ImGui::InputDouble("##level", &m_TriggerLevel, 0, 0, "%.3g p.e.", ImGuiInputTextFlags_EnterReturnsTrue);
  if (m_Trigger == TriggerMode::kSingle) {
    ImGui::SameLine();
    ImGui::BeginDisabled(m_Armed);
    if (ImGui::Button("Arm")) {
      m_Armed = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextColored(m_Armed ? kColTrigger : ImVec4(0.6f, 0.6f, 0.6f, 1), m_Armed ? "Armed" : "Stopped");
  }

  ImGui::SameLine(0, 2 * fs);
  int rate = static_cast<int>(m_Rate);
  if (toolCombo("Update", &rate,
                "Max\0"
                "60 Hz\0"
                "5 Hz\0"
                "1 Hz\0"
                "On click\0",
                6 * fs)) {
    m_Rate = static_cast<UpdateRate>(rate);
    m_AcquireRequested = false;
  }
  if (m_Rate == UpdateRate::kOnClick) {
    ImGui::SameLine();
    if (ImGui::Button("Acquire")) {
      m_AcquireRequested = true;
    }
  }

  ImGui::SameLine(0, 2 * fs);
  if (toolCombo("Persistence", &m_PersistenceIdx,
                "Off\0"
                "10\0"
                "50\0"
                "200\0",
                4.5f * fs)) {
    m_Persistence.clear();
  }
  ImGui::SameLine(0, 2 * fs);
  ImGui::Checkbox("Auto Y", &m_AutoScaleY);
  exportButton(PlotId::kScope, "scope.png");

  // Event summary
  if (m_HasShown) {
    const EventResult& r = m_Shown;
    const auto& t = r.nByType;
    ImGui::TextDisabled("Event %llu", static_cast<unsigned long long>(r.id));
    ImGui::SameLine();
    ImGui::Text("sources %u   photons %u   pe %u  dcr %u  xt %u  dxt %u  ap %u", r.nSources, r.nPhotons, t[0], t[1],
                t[2], t[3], t[4] + t[5]);
    ImGui::SameLine(0, 2 * fs);
    ImGui::Text("peak %.2f   integral %.1f   ToT %.1f ns   ToA %.1f ns", r.f.peak, r.f.integral, r.f.tot, r.f.toa);
  } else {
    ImGui::TextDisabled("Waiting for events...");
  }

  const float plotH = std::max(5 * fs, height - (ImGui::GetCursorPosY() - y0));
  const bool print = printing(PlotId::kScope);
  if (print) {
    pushPrintStyle();
  }
  if (!ImPlot::BeginPlot("##scope", ImVec2(-1, plotH), ImPlotFlags_NoLegend)) {
    if (print) {
      popPrintStyle();
    }
    return;
  }
  const ImVec4 traceColor = print ? ImVec4(0.05f, 0.20f, 0.55f, 1) : kColTrace;

  // Integration gate drawn as a logic trace in a band below the signal
  const float gateH = 0.9f * fs, gatePad = 0.5f * fs;
  const float gateBand = gateH + 2 * gatePad;

  ImPlot::SetupAxes("Time [ns]", "Amplitude [p.e.]");
  ImPlot::SetupAxisLimits(ImAxis_X1, 0, m_Cfg.signalLength, m_FitScopeX ? ImPlotCond_Always : ImPlotCond_Once);
  ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0, m_Cfg.signalLength);
  ImPlot::SetupAxisZoomConstraints(ImAxis_X1, 4 * m_Cfg.sampling, m_Cfg.signalLength);
  m_FitScopeX = false;
  if (m_AutoScaleY) {
    // Range follows the shown trace: expands at once, relaxes smoothly
    float lo = std::min(0.0f, float(std::min(m_Cfg.threshold, m_TriggerLevel)));
    float hi = std::max(1.0f, float(std::max(m_Cfg.threshold, m_TriggerLevel)));
    if (m_HasShown && !m_Shown.waveform.empty()) {
      const auto [mn, mx] = std::minmax_element(m_Shown.waveform.begin(), m_Shown.waveform.end());
      lo = std::min(lo, *mn);
      hi = std::max(hi, *mx);
    }
    const float pad = 0.08f * (hi - lo);
    lo -= pad;
    hi += pad;
    // Leave room at the bottom for the gate band
    lo -= (hi - lo) * gateBand / std::max(1.0f, m_ScopePlotPx - gateBand);
    const float k = std::min(1.0f, 3.0f * ImGui::GetIO().DeltaTime);
    m_ScopeYLo = lo < m_ScopeYLo ? lo : m_ScopeYLo + (lo - m_ScopeYLo) * k;
    m_ScopeYHi = hi > m_ScopeYHi ? hi : m_ScopeYHi + (hi - m_ScopeYHi) * k;
    ImPlot::SetupAxisLimits(ImAxis_Y1, m_ScopeYLo, m_ScopeYHi, ImPlotCond_Always);
  } else {
    ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 10, ImPlotCond_Once);
  }

  if (m_HasShown) {
    ImPlotSpec ghost;
    ghost.LineColor = ImVec4(traceColor.x, traceColor.y, traceColor.z, 0.12f);
    ghost.Flags = ImPlotItemFlags_NoFit;
    for (const auto& w : m_Persistence) {
      ImPlot::PlotLine("##ghost", w.data(), static_cast<int>(w.size()), m_ShownSampling, 0, ghost);
    }
    ImPlotSpec trace;
    trace.LineColor = traceColor;
    trace.LineWeight = 1.5f;
    ImPlot::PlotLine("signal", m_Shown.waveform.data(), static_cast<int>(m_Shown.waveform.size()), m_ShownSampling, 0,
                     trace);
  }

  // Draggable markers: gate, analysis threshold, trigger level.
  // Analysis parameters are applied when the drag is released.
  const ImPlotDragToolFlags dflags = ImPlotDragToolFlags_NoFit;
  bool held = false, h = false;

  // Gate: edges resize it, the body moves it. Same mechanism as ImPlot's
  // drag tools (ButtonBehavior on screen rects inside the plot).
  const ImVec2 pp = ImPlot::GetPlotPos(), ps = ImPlot::GetPlotSize();
  m_ScopePlotPx = ps.y;
  const float yLow = pp.y + ps.y - gatePad, yHigh = yLow - gateH;
  const double sigLen = m_Cfg.signalLength, minWidth = m_Cfg.sampling;
  float xs = ImPlot::PlotToPixels(m_Cfg.intStart, 0).x;
  float xe = ImPlot::PlotToPixels(m_Cfg.intStart + m_Cfg.intWindow, 0).x;
  const float grab = 0.35f * fs;
  const ImRect zones[3] = {
    ImRect(xs - grab, yHigh - grab, xs + grab, yLow + grab),                     // start edge
    ImRect(xe - grab, yHigh - grab, xe + grab, yLow + grab),                     // end edge
    ImRect(xs + grab, yHigh - grab, std::max(xs + grab, xe - grab), yLow + grab) // body
  };
  bool gateHovered = false, gateHeld = false;
  for (int z = 0; z < 3; ++z) {
    const ImGuiID id = ImGui::GetID(z == 0 ? "gate_start" : z == 1 ? "gate_end" : "gate_body");
    ImGui::KeepAliveID(id);
    bool zh = false, zd = false;
    ImGui::ButtonBehavior(zones[z], id, &zh, &zd);
    if (zh || zd) {
      ImGui::SetMouseCursor(z == 2 ? ImGuiMouseCursor_ResizeAll : ImGuiMouseCursor_ResizeEW);
    }
    if (zd && m_GateDrag < 0) {
      // Grab: remember where the body was taken so it does not jump
      m_GateDrag = z;
      m_GateGrab = ImPlot::GetPlotMousePos().x - m_Cfg.intStart;
    }
    if (zd && m_GateDrag == z && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      const auto snap = [&](double t) { return std::round(t / minWidth) * minWidth; }; // to sampling grid
      const double mx = snap(ImPlot::GetPlotMousePos().x);
      const double end = m_Cfg.intStart + m_Cfg.intWindow;
      if (z == 0) {
        m_Cfg.intStart = std::clamp(mx, 0.0, end - minWidth);
        m_Cfg.intWindow = end - m_Cfg.intStart;
      } else if (z == 1) {
        m_Cfg.intWindow = std::clamp(mx, m_Cfg.intStart + minWidth, sigLen) - m_Cfg.intStart;
      } else {
        m_Cfg.intStart = std::clamp(snap(mx - m_GateGrab), 0.0, std::max(0.0, sigLen - m_Cfg.intWindow));
      }
      m_DragPending = true;
    }
    gateHovered |= zh;
    gateHeld |= zd;
  }
  if (!gateHeld) {
    m_GateDrag = -1;
  }
  held |= gateHeld;
  const bool gateActive = gateHovered || gateHeld;
  xs = ImPlot::PlotToPixels(m_Cfg.intStart, 0).x;
  xe = ImPlot::PlotToPixels(m_Cfg.intStart + m_Cfg.intWindow, 0).x;
  {
    ImDrawList* dl = ImPlot::GetPlotDrawList();
    const ImU32 line = ImGui::GetColorU32(kColWindow);
    const ImU32 fill = ImGui::GetColorU32(ImVec4(kColWindow.x, kColWindow.y, kColWindow.z, gateActive ? 0.35f : 0.18f));
    ImPlot::PushPlotClipRect();
    dl->AddRectFilled(ImVec2(xs, yHigh), ImVec2(xe, yLow), fill);
    const ImVec2 pts[] = {{pp.x, yLow}, {xs, yLow}, {xs, yHigh}, {xe, yHigh}, {xe, yLow}, {pp.x + ps.x, yLow}};
    dl->AddPolyline(pts, 6, line, ImDrawFlags_None, gateActive ? 2.0f : 1.5f);
    ImPlot::PopPlotClipRect();
  }
  ImPlot::TagY(ImPlot::PixelsToPlot(ImVec2(pp.x, 0.5f * (yLow + yHigh))).y, kColWindow, "Gate");
  if (gateActive) {
    ImPlot::TagX(m_Cfg.intStart, kColWindow, "%.4g", m_Cfg.intStart);
    ImPlot::TagX(m_Cfg.intStart + m_Cfg.intWindow, kColWindow, "%.4g", m_Cfg.intStart + m_Cfg.intWindow);
  }

  double thr = m_Cfg.threshold;
  if (ImPlot::DragLineY(3, &thr, kColThreshold, 1, dflags, nullptr, nullptr, &h)) {
    m_Cfg.threshold = thr;
    m_DragPending = true;
  }
  held |= h;
  ImPlot::DragLineY(4, &m_TriggerLevel, kColTrigger, 1, dflags);
  ImPlot::TagY(m_Cfg.threshold, kColThreshold, "Thr");
  ImPlot::TagY(m_TriggerLevel, kColTrigger, "Trig");
  if (m_DragPending && !held) {
    m_DragPending = false;
    m_Dirty = true;
  }

  ImPlot::EndPlot();
  if (print) {
    popPrintStyle();
    recordCapture();
  }
}

// ------------------------------------------------------- histograms / matrix

void App::drawBottom(float height) {
  const float fs = ImGui::GetFontSize();
  const float y0 = ImGui::GetCursorPosY();

  int view = static_cast<int>(m_View);
  if (toolCombo("View", &view, "Peak\0Integral\0Time over threshold\0Time of arrival\0Cell matrix\0", 10 * fs)) {
    m_View = static_cast<View>(view);
    m_TexDirty = true;
  }

  if (m_View != View::kMatrix) {
    Histogram& hist = m_Hists[static_cast<int>(m_View)];
    ImGui::SameLine(0, 2 * fs);
    if (toolCombo("Bins", &m_BinsIdx,
                  "64\0"
                  "128\0"
                  "256\0"
                  "512\0"
                  "1024\0"
                  "2048\0",
                  4.5f * fs)) {
      for (auto& hh : m_Hists) {
        hh.setBins(kBinChoices[m_BinsIdx]);
      }
    }
    ImGui::SameLine();
    ImGui::Checkbox("Log Y", &m_LogY);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(5.5f * fs);
    ImGui::Combo("##ymode", &m_HistYMode, "Counts\0Rate\0");
    ImGui::SetItemTooltip("Counts: raw bin contents. Rate: normalised per second (or per event) and per unit,\n"
                          "comparable between runs with different statistics and binning.");
    ImGui::SameLine();
    if (ImGui::Button("Auto range")) {
      hist.autoRange();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
      for (auto& hh : m_Hists) {
        hh.clear();
      }
    }
    // Rates: counts / received events x window rate / bin width, so runs with
    // different statistics, binning or timing can be compared
    const int feature = static_cast<int>(m_View);
    const uint64_t below = hist.belowThreshold(), total = hist.entries() + below;
    const double rate = windowRateHz(m_Cfg);
    const double bw = hist.ready() ? hist.binWidth() : 1;
    const double rateScale = total > 0 ? (rate > 0 ? rate : 1.0) / total / bw : 0;
    const bool asRate = m_HistYMode == 1;
    const double scale = asRate ? rateScale : 1.0;
    ImGui::SameLine();
    ImGui::BeginDisabled(!hist.ready());
    if (ImGui::Button("Freeze")) {
      freezeHistogram(feature, "Ref " + std::to_string(++m_FrozenCount) + ": " + refName());
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Keep a copy of this histogram as a reference (kept when parameters change)");
    const bool anyFrozen =
      std::any_of(m_Frozen.begin(), m_Frozen.end(), [&](const Frozen& f) { return f.feature == feature; });
    if (anyFrozen) {
      ImGui::SameLine();
      if (ImGui::Button("Clear refs")) {
        m_Frozen.erase(
          std::remove_if(m_Frozen.begin(), m_Frozen.end(), [&](const Frozen& f) { return f.feature == feature; }),
          m_Frozen.end());
      }
    }
    exportButton(PlotId::kHist, "histogram.png");
    ImGui::SameLine();
    ImGui::BeginDisabled(!hist.ready());
    if (ImGui::Button("CSV")) {
      m_BrowserTarget = FileTarget::kExportCsv;
      m_Browser.open("Export histogram", std::string("histogram_") + kFeatureNames[feature] + ".csv", {".csv"}, true);
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Save this histogram as CSV (bin edges, counts, rate) with a descriptive header");
    ImGui::SameLine(0, 2 * fs);
    // Events with no sample above the analysis threshold inside the gate are
    // not histogrammed: show them as a fraction of all received events
    ImGui::TextDisabled("entries %llu   below threshold %.2f%%", static_cast<unsigned long long>(hist.entries()),
                        total > 0 ? 100.0 * below / total : 0.0);
    ImGui::SetItemTooltip("%llu of %llu events have no sample above the analysis threshold in the gate\n"
                          "and are not in the histogram",
                          static_cast<unsigned long long>(below), static_cast<unsigned long long>(total));

    static constexpr const char* kXLabels[] = {"Peak [p.e.]", "Integral [p.e. × ns]", "Time over threshold [ns]",
                                               "Time of arrival (from int. start) [ns]"};
    static constexpr const char* kUnits[] = {"p.e.", "p.e. × ns", "ns", "ns"};
    char yLabel[64];
    std::snprintf(yLabel, sizeof(yLabel),
                  !asRate    ? "Counts"
                  : rate > 0 ? "Rate [Hz / %s]"
                             : "Events / %s (fraction)",
                  kUnits[feature]);
    const float plotH = std::max(5 * fs, height - (ImGui::GetCursorPosY() - y0));
    const bool print = printing(PlotId::kHist);
    if (print) {
      pushPrintStyle();
    }
    if (ImPlot::BeginPlot("##hist", ImVec2(-1, plotH), anyFrozen ? 0 : ImPlotFlags_NoLegend)) {
      ImPlot::SetupAxes(kXLabels[feature], yLabel, 0, ImPlotAxisFlags_AutoFit);
      ImPlot::SetupLegend(ImPlotLocation_NorthEast);
      if (m_LogY) {
        ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
      }
      if (hist.ready()) {
        const auto& st = hist.stairs();
        // X range: live histogram plus the references of this view; refit when either changes
        double xLo = hist.lo(), xHi = hist.lo() + bw * (st.size() - 1);
        size_t nRefs = 0;
        for (const Frozen& f : m_Frozen) {
          if (f.feature == feature) {
            xLo = std::min(xLo, f.lo);
            xHi = std::max(xHi, f.lo + f.binWidth * (f.ys.size() - 1));
            ++nRefs;
          }
        }
        const bool refsChanged = nRefs != m_FitRefs[feature];
        m_FitRefs[feature] = nRefs;
        ImPlot::SetupAxisLimits(ImAxis_X1, xLo, xHi,
                                hist.takeRangeChanged() || refsChanged ? ImPlotCond_Always : ImPlotCond_Once);
        m_PlotBuf.resize(st.size());
        for (size_t i = 0; i < st.size(); ++i) {
          m_PlotBuf[i] = st[i] * scale;
        }
        ImPlotSpec spec;
        spec.LineColor = kColHist;
        spec.FillColor = kColHist;
        spec.FillAlpha = 0.25f;
        if (!m_LogY) {
          spec.Flags = ImPlotStairsFlags_Shaded;
        }
        ImPlot::PlotStairs("Live", m_PlotBuf.data(), static_cast<int>(m_PlotBuf.size()), bw, hist.lo(), spec);
      }
      // Frozen references: semi-transparent outlines, click the legend to hide
      int k = 0;
      for (const Frozen& f : m_Frozen) {
        if (f.feature != feature) {
          continue;
        }
        ImPlotSpec spec;
        const ImVec4 c = kRefColors[k++ % 5];
        spec.LineColor = ImVec4(c.x, c.y, c.z, 0.65f);
        spec.LineWeight = 1.5f;
        const std::string label = f.perEvent == (rate <= 0) ? f.name : f.name + (f.perEvent ? " (per event)" : " (Hz)");
        const std::vector<double>& ys = asRate ? f.ys : f.counts;
        ImPlot::PlotStairs(label.c_str(), ys.data(), static_cast<int>(ys.size()), f.binWidth, f.lo, spec);
      }
      ImPlot::EndPlot();
    }
    if (print) {
      popPrintStyle();
      recordCapture();
    }
    return;
  }

  // Cell matrix legend (counts refer to the event shown on the scope)
  for (int i = 0; i < 6; ++i) {
    ImGui::SameLine(0, i == 0 ? 2 * fs : 1.2f * fs);
    ImGui::ColorButton(kHitNames[i], kHitColors[i], ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                       ImVec2(0.8f * fs, 0.8f * fs));
    ImGui::SameLine(0, 0.3f * fs);
    ImGui::Text("%s %u", kHitNames[i], m_HasShown ? m_Shown.nByType[i] : 0u);
  }

  exportButton(PlotId::kMatrix, "matrix.png");
  const bool print = printing(PlotId::kMatrix);
  if (print != m_TexLight) {
    m_TexLight = print;
    m_TexDirty = true;
  }
  if (m_TexDirty) {
    updateMatrixTexture();
  }
  // Plain image of the sensor, centred and square; hover a cell for details
  float side = std::max(4 * fs, std::min(ImGui::GetContentRegionAvail().x,
                                         height - (ImGui::GetCursorPosY() - y0) - ImGui::GetStyle().ItemSpacing.y));
  if (m_TexSize == 0) {
    return;
  }
  // Whole pixels per cell when borders are drawn, so lines and texels align
  const bool borders = side / m_Grid >= 6;
  if (borders) {
    side = std::floor(side / m_Grid) * m_Grid;
  }
  ImGui::SetCursorPosX(std::floor(ImGui::GetCursorPosX() + 0.5f * (ImGui::GetContentRegionAvail().x - side)));
  const float margin = 0; // the white cell borders frame the exported image
  if (print) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x - margin, p.y - margin),
                                              ImVec2(p.x + side + margin, p.y + side + margin), IM_COL32_WHITE);
  }
  ImGui::Image(static_cast<ImTextureID>(m_Texture), ImVec2(side, side));
  const ImU32 border = print ? IM_COL32_WHITE : kCellBorder;
  if (print) {
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    m_Export.rect[0] = a.x - margin;
    m_Export.rect[1] = a.y - margin;
    m_Export.rect[2] = b.x + margin;
    m_Export.rect[3] = b.y + margin;
    m_Export.ready = true;
  }
  if (borders) {
    const float cellPx = side / m_Grid;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 o(std::floor(ImGui::GetItemRectMin().x), std::floor(ImGui::GetItemRectMin().y));
    for (uint32_t i = 0; i <= m_Grid; ++i) {
      // 1 px filled rects rather than lines: no anti-aliasing bleed into cells
      const float d = i * cellPx;
      dl->AddRectFilled(ImVec2(o.x + d, o.y), ImVec2(o.x + d + 1, o.y + side), border);
      dl->AddRectFilled(ImVec2(o.x, o.y + d), ImVec2(o.x + side, o.y + d + 1), border);
    }
  }
  if (ImGui::IsItemHovered() && m_Grid > 0) {
    const ImVec2 m = ImGui::GetMousePos(), o = ImGui::GetItemRectMin();
    const ImVec2 rel((m.x - o.x) / side, (m.y - o.y) / side);
    const uint32_t r = std::min<uint32_t>(m_Grid - 1, static_cast<uint32_t>(rel.y * m_Grid));
    const uint32_t c = std::min<uint32_t>(m_Grid - 1, static_cast<uint32_t>(rel.x * m_Grid));
    const uint32_t nSide = m_ShownNSide;
    ImGui::BeginTooltip();
    if (m_Grid == nSide) {
      ImGui::Text("Row %u, column %u", r, c);
    } else {
      ImGui::Text("Rows %u-%u, columns %u-%u", r * nSide / m_Grid, (r + 1) * nSide / m_Grid - 1, c * nSide / m_Grid,
                  (c + 1) * nSide / m_Grid - 1);
    }
    const int8_t t = m_CellType[size_t(r) * m_Grid + c];
    if (t >= 0 && t < 6) {
      ImGui::TextColored(kHitColors[t], "%s at %.1f ns", kHitNames[t], m_CellTime[size_t(r) * m_Grid + c]);
    } else {
      ImGui::TextDisabled("No hit");
    }
    ImGui::EndTooltip();
  }
}

void App::updateMatrixTexture() {
  m_TexDirty = false;
  const uint32_t nSide = m_ShownNSide;
  if (!m_HasShown || nSide == 0) {
    return;
  }

  // One texel per cell; large sensors are downsampled to at most
  // kMaxTexture cells per side. Cell borders are drawn on top when shown.
  const uint32_t grid = std::min<uint32_t>(nSide, kMaxTexture);
  const int size = static_cast<int>(grid);

  m_Grid = grid;
  std::vector<int8_t>& type = m_CellType;
  std::vector<float>& first = m_CellTime;
  type.assign(size_t(grid) * grid, -1);
  first.assign(size_t(grid) * grid, FLT_MAX);
  for (const HitLite& h : m_Shown.hits) {
    const size_t r = size_t(h.row) * grid / nSide, c = size_t(h.col) * grid / nSide;
    const size_t idx = r * grid + c;
    if (h.time < first[idx]) {
      first[idx] = h.time;
      type[idx] = static_cast<int8_t>(h.type);
    }
  }

  std::vector<ImU32> px(size_t(size) * size);
  for (size_t i = 0; i < px.size(); ++i) {
    const int8_t t = type[i];
    px[i] = t >= 0 && t < 6 ? ImGui::GetColorU32(kHitColors[t])
            : m_TexLight    ? IM_COL32(222, 226, 232, 255)
                            : kEmptyCell;
  }

  glBindTexture(GL_TEXTURE_2D, m_Texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
  m_TexSize = size;
}

void App::drawStatusBar() {
  const size_t inFill = m_Pipeline.inQueueSize(), outFill = m_Pipeline.results().size();
  ImGui::Separator();
  ImGui::TextDisabled(
    "Generated %s   Simulated %s   Displayed %s   Dropped %s   FIFO %zu/%zu  %zu/%zu   %d workers   %.0f FPS",
    formatRate(m_RateGen).c_str(), formatRate(m_RateSim).c_str(), formatRate(m_RateGui).c_str(),
    formatRate(m_RateDrop).c_str(), inFill, m_Pipeline.inQueueCapacity(), outFill, m_Pipeline.results().capacity(),
    m_Pipeline.nWorkers(), ImGui::GetIO().Framerate);
  if (m_Recording) {
    ImGui::SameLine(0, 2 * ImGui::GetFontSize());
    ImGui::TextColored(kColThreshold, "REC %llu",
                       static_cast<unsigned long long>(m_Pipeline.recorder().stats().events));
  }
  if (!m_Message.empty() && Clock::now() - m_MessageTime < std::chrono::seconds(6)) {
    ImGui::SameLine(0, 2 * ImGui::GetFontSize());
    ImGui::TextColored(kColTrigger, "%s", m_Message.c_str());
  }
}

// ------------------------------------------------------------- export

void App::exportButton(PlotId plot, const char* name) {
  ImGui::SameLine();
  ImGui::PushID(name);
  if (ImGui::Button("PNG")) {
    m_ExportPending = plot;
    m_BrowserTarget = FileTarget::kExportPng;
    m_Browser.open("Export PNG", name, {".png"}, true);
  }
  ImGui::SetItemTooltip("Save this plot as a PNG with a white background");
  ImGui::PopID();
}

void App::recordCapture() {
  const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
  m_Export.rect[0] = a.x;
  m_Export.rect[1] = a.y;
  m_Export.rect[2] = b.x;
  m_Export.rect[3] = b.y;
  m_Export.ready = true;
}

bool App::takeCapture(float rect[4], std::string& path) {
  if (!m_Export.ready) {
    return false;
  }
  std::copy(m_Export.rect, m_Export.rect + 4, rect);
  path = m_Export.path;
  m_Export = {};
  return true;
}

void App::captureDone(const std::string& path, const std::string& error) {
  message(error.empty() ? "Saved " + path : error);
}

void App::message(const std::string& text) {
  m_Message = text;
  m_MessageTime = Clock::now();
}

// ------------------------------------------------------------ settings

// Every scalar simulation parameter, by name (pdeFile/nphFile/gammaFile are
// file references reloaded on load)
#define SIPM_GUI_FIELDS(X)                                                                                             \
  X(sizeMm)                                                                                                            \
  X(pitchUm)                                                                                                           \
  X(lightProfile)                                                                                                      \
  X(dcrOn)                                                                                                             \
  X(dcrKHz)                                                                                                            \
  X(xtOn) X(xtPct) X(dxtOn) X(dxtPct) X(dxtTau) X(apOn) X(apPct) X(apTauFast) X(apTauSlow) X(apSlowPct) X(ccgvPct)     \
    X(snrDb) X(signalLength) X(sampling) X(riseTime) X(fallTimeFast) X(slowOn) X(fallTimeSlow) X(slowPct)              \
      X(recoveryTime) X(pdePct) X(pdeFile) X(rateKHz) X(nphFixed) X(nphMu) X(nphFile) X(nphScale) X(gammaFile)         \
        X(lightYieldScale) X(timeOffset) X(timeWidth) X(timeSigma) X(timeTau) X(timeRise) X(wlFixed) X(wlMin) X(wlMax) \
          X(wlMean) X(wlSigma) X(intStart) X(intWindow) X(threshold)
#define SIPM_GUI_ENUMS(X) X(pdeMode) X(timing) X(nphMode) X(timeMode) X(wlMode)

std::string App::settingsJson(int indent) const {
  nlohmann::json sim;
  const SimConfig& c = m_Cfg;
#define X(f) sim[#f] = c.f;
  SIPM_GUI_FIELDS(X)
#undef X
#define X(f) sim[#f] = static_cast<int>(c.f);
  SIPM_GUI_ENUMS(X)
#undef X
  nlohmann::json pde = nlohmann::json::array();
  for (size_t i = 0; i < c.pdeWavelengths.size(); ++i) {
    pde.push_back({c.pdeWavelengths[i], c.pdeValues[i]});
  }
  sim["pdeCurve"] = pde;
  sim["sipmModel"] = m_ModelIdx > 0 ? m_Models[m_ModelIdx - 1].name : "";
  if (!m_Db.materials.empty()) {
    sim["crystal"] = m_Db.materials[m_MaterialIdx].id;
    sim["crystalSize"] = m_Db.sizes[m_SizeIdx].id;
  }
  nlohmann::json gui = {{"workers", m_NWorkers},
                        {"trigger", static_cast<int>(m_Trigger)},
                        {"triggerLevel", m_TriggerLevel},
                        {"updateRate", static_cast<int>(m_Rate)},
                        {"persistence", m_PersistenceIdx},
                        {"autoY", m_AutoScaleY},
                        {"view", static_cast<int>(m_View)},
                        {"bins", m_BinsIdx},
                        {"logY", m_LogY},
                        {"histYMode", m_HistYMode}};
  return nlohmann::json{{"format", "SimSiPM GUI settings"}, {"version", 1}, {"simulation", sim}, {"gui", gui}}.dump(
    indent);
}

void App::saveSettings(const std::string& path) {
  std::ofstream f(path);
  f << settingsJson(2) << "\n";
  message(f ? "Settings saved to " + path : "Cannot write " + path);
}

// ----------------------------------------------------------- recording

void App::startRecording() {
  const SimConfig& c = m_Cfg;
  const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  char created[32];
  std::strftime(created, sizeof(created), "%Y-%m-%dT%H:%M:%S", std::localtime(&now));
  const nlohmann::json header = {
    {"format", "SimSiPM waveform dump"},
    {"version", 1},
    {"created", created},
    {"settings", nlohmann::json::parse(settingsJson(-1))},
    {"run",
     {{"sampling_ns", c.sampling},
      {"samples_per_waveform", static_cast<uint64_t>(std::lround(c.signalLength / c.sampling))},
      {"signal_length_ns", c.signalLength},
      {"amplitude_unit", "p.e. (single photoelectron peak = 1)"},
      {"event_timing", c.timing == TimingMode::kIndependent ? "independent"
                       : c.timing == TimingMode::kPoisson   ? "poisson"
                                                            : "periodic"},
      {"source_rate_kHz", c.rateKHz},
      {"window_rate_Hz", windowRateHz(c)},
      {"sipm_model", m_ModelIdx > 0 ? m_Models[m_ModelIdx - 1].name : "custom"},
      {"max_waveforms", m_RecCount}}},
    {"encoding",
     {{"magic", "SIPMWAV1"},
      {"byte_order", "little endian"},
      {"lsb_pe", m_RecLsb},
      {"file_layout", "magic (8 bytes), uint32 header size, JSON header, blocks..., end marker"},
      {"block", "uint32 nEvents (> 0), uint32 rawSize, uint32 dataSize, dataSize bytes of zlib data (RFC 1950)"},
      {"end_marker", "uint32 0, uint32 summarySize, summary JSON {events, seconds}"},
      {"compression", "zlib deflate level 1 (fastest); inflating gives rawSize bytes"},
      {"event", "varint id, varint nSources, varint nSamples, then nSamples zigzag varints of q[0] and "
                "q[i] - q[i-1]; sample[i] = q[i] * lsb_pe"},
      {"varint", "unsigned LEB128 (7 bits per byte, low bits first, high bit = continuation)"},
      {"zigzag", "u = (d << 1) ^ (d >> 63); d = (u >> 1) ^ -(u & 1)"},
      {"order", "blocks are written in completion order: sort waveforms by id if needed"},
      {"reader", "examples/gui/tools/read_waveforms.py"}}}};
  const std::string err =
    m_Pipeline.recorder().start(m_RecFile, header.dump(), m_RecLsb, static_cast<uint64_t>(std::max(0, m_RecCount)));
  if (!err.empty()) {
    message(err);
    return;
  }
  m_Recording = true;
  message("Recording to " + m_RecFile);
}

void App::stopRecording(const char* reason) {
  if (!m_Recording) {
    return;
  }
  Recorder& rec = m_Pipeline.recorder();
  rec.stop();
  m_Recording = false;
  const Recorder::Stats s = rec.stats();
  char buf[256];
  std::snprintf(buf, sizeof(buf), "%s: %llu waveforms, %.1f MB in %s", reason,
                static_cast<unsigned long long>(s.events), s.fileBytes / 1e6, s.path.c_str());
  message(buf);
}

void App::drawRecording() {
  ImGui::SeparatorText("Waveform recording");
  Recorder& rec = m_Pipeline.recorder();
  ImGui::BeginDisabled(m_Recording);
  rowLabel("File");
  {
    const float button = ImGui::CalcTextSize("...").x + 2 * ImGui::GetStyle().FramePadding.x;
    ImGui::SetNextItemWidth(-(button + ImGui::GetStyle().ItemSpacing.x));
    std::string name = std::filesystem::path(m_RecFile).filename().string();
    ImGui::InputText("##recfile", &name, ImGuiInputTextFlags_ReadOnly);
    ImGui::SetItemTooltip("%s", m_RecFile.c_str());
    ImGui::SameLine();
    if (ImGui::Button("...##rec")) {
      m_BrowserTarget = FileTarget::kRecordFile;
      m_Browser.open("Waveform file", m_RecFile, {".sipmw"}, true);
    }
  }
  inputInt("Waveforms", &m_RecCount, 0, 2000000000);
  ImGui::SetItemTooltip("Number of waveforms to record, 0 = until Stop");
  inputDouble("Resolution [p.e.]", &m_RecLsb, "%.3g", 1e-6, 10);
  ImGui::SetItemTooltip("Quantisation step of the stored samples (max error = half of it).\n"
                        "Smaller steps keep more of the noise and give bigger files.");
  ImGui::EndDisabled();

  const float fs = ImGui::GetFontSize();
  ImGui::PushStyleColor(ImGuiCol_Button, m_Recording ? ImVec4(0.55f, 0.20f, 0.22f, 1) : ImVec4(0.17f, 0.19f, 0.23f, 1));
  if (ImGui::Button(m_Recording ? "Stop recording" : "Start recording", ImVec2(-FLT_MIN, 1.6f * fs))) {
    if (m_Recording) {
      stopRecording("Recording stopped");
    } else {
      startRecording();
    }
  }
  ImGui::PopStyleColor();
  const Recorder::Stats s = rec.stats();
  if (m_Recording || s.events > 0) {
    ImGui::PushTextWrapPos(0);
    char target[48] = "";
    if (s.target > 0) {
      std::snprintf(target, sizeof(target), " / %llu", static_cast<unsigned long long>(s.target));
    }
    ImGui::TextDisabled("%llu%s waveforms, %.1f MB (%.1fx smaller than float32), %s",
                        static_cast<unsigned long long>(s.events), target, s.fileBytes / 1e6,
                        s.fileBytes > 0 ? double(s.floatBytes) / s.fileBytes : 0.0,
                        formatRate(s.seconds > 0 ? s.events / s.seconds : 0).c_str());
    ImGui::PopTextWrapPos();
  }
}

void App::loadSettings(const std::string& path) {
  std::ifstream f(path);
  const nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
  if (!f || j.is_discarded() || !j.contains("simulation")) {
    message("Not a settings file: " + path);
    return;
  }
  const nlohmann::json& sim = j["simulation"];
  SimConfig c = m_Cfg;
#define X(f) c.f = sim.value(#f, c.f);
  SIPM_GUI_FIELDS(X)
#undef X
#define X(f) c.f = static_cast<decltype(c.f)>(sim.value(#f, static_cast<int>(c.f)));
  SIPM_GUI_ENUMS(X)
#undef X
  if (sim.contains("pdeCurve")) {
    c.pdeWavelengths.clear();
    c.pdeValues.clear();
    for (const auto& p : sim["pdeCurve"]) {
      c.pdeWavelengths.push_back(p.at(0).get<double>());
      c.pdeValues.push_back(p.at(1).get<double>());
    }
  }
  // SiPM model: apply it, then the saved values (identical unless they were edited)
  m_ModelIdx = 0;
  const std::string model = sim.value("sipmModel", "");
  for (size_t i = 0; i < m_Models.size(); ++i) {
    if (m_Models[i].name == model) {
      applyModel(static_cast<int>(i));
      m_ModelIdx = static_cast<int>(i) + 1;
    }
  }
  m_Cfg = c;
  for (size_t i = 0; i < m_Db.materials.size(); ++i) {
    if (m_Db.materials[i].id == sim.value("crystal", "")) {
      m_MaterialIdx = static_cast<int>(i);
    }
  }
  for (size_t i = 0; i < m_Db.sizes.size(); ++i) {
    if (m_Db.sizes[i].id == sim.value("crystalSize", "")) {
      m_SizeIdx = static_cast<int>(i);
    }
  }
  // Reload referenced files (a PDE file replaces the stored curve)
  if (!m_Cfg.pdeFile.empty()) {
    loadFile(FileTarget::kPde);
  } else {
    m_PdeOk = !m_Cfg.pdeValues.empty();
    m_PdeStatus = m_PdeOk ? "Curve from settings file" : "No PDE curve";
  }
  loadFile(FileTarget::kNPhotons);
  loadFile(FileTarget::kGamma);

  const nlohmann::json gui = j.value("gui", nlohmann::json::object());
  m_Trigger = static_cast<TriggerMode>(gui.value("trigger", static_cast<int>(m_Trigger)));
  m_TriggerLevel = gui.value("triggerLevel", m_TriggerLevel);
  m_Rate = static_cast<UpdateRate>(gui.value("updateRate", static_cast<int>(m_Rate)));
  m_PersistenceIdx = gui.value("persistence", m_PersistenceIdx);
  m_AutoScaleY = gui.value("autoY", m_AutoScaleY);
  m_View = static_cast<View>(gui.value("view", static_cast<int>(m_View)));
  m_LogY = gui.value("logY", m_LogY);
  m_HistYMode = gui.value("histYMode", m_HistYMode);
  const int bins = gui.value("bins", m_BinsIdx);
  if (bins != m_BinsIdx) {
    m_BinsIdx = bins;
    for (auto& h : m_Hists) {
      h.setBins(kBinChoices[m_BinsIdx]);
    }
  }
  const int workers = std::clamp(gui.value("workers", m_NWorkers), 1, m_MaxWorkers);
  if (workers != m_NWorkers) {
    m_NWorkers = workers;
    restartPipeline();
  }
  m_Armed = true;
  m_TexDirty = true;
  m_Dirty = true;
  message("Settings loaded from " + path);
}

// ------------------------------------------------------- histograms I/O

std::string App::refName() const {
  std::string name = m_ModelIdx > 0 ? m_Models[m_ModelIdx - 1].name : "custom SiPM";
  if (m_Cfg.nphMode == NPhotonMode::kGamma) {
    name += ", " + std::filesystem::path(m_Cfg.gammaFile).stem().string() + " " + m_Cfg.scint.name;
  }
  return name;
}

void App::freezeHistogram(int feature, const std::string& name) {
  const Histogram& hist = m_Hists[feature];
  if (!hist.ready()) {
    return;
  }
  const uint64_t total = hist.entries() + hist.belowThreshold();
  const double rate = windowRateHz(m_Cfg);
  const double bw = hist.binWidth();
  const double rateScale = total > 0 ? (rate > 0 ? rate : 1.0) / total / bw : 0;
  Frozen f;
  f.name = name;
  f.feature = feature;
  f.perEvent = rate <= 0;
  f.lo = hist.lo();
  f.binWidth = bw;
  for (const double v : hist.stairs()) {
    f.ys.push_back(v * rateScale);
    f.counts.push_back(v);
  }
  m_Frozen.push_back(std::move(f));
}

std::string App::writeHistogramCsv(const std::string& path, int feature, const std::string& note) const {
  static constexpr const char* kQuantity[] = {"Peak [p.e.]", "Integral [p.e. x ns]", "Time over threshold [ns]",
                                              "Time of arrival from int. start [ns]"};
  static constexpr const char* kUnit[] = {"p.e.", "p.e. x ns", "ns", "ns"};
  const Histogram& hist = m_Hists[feature];
  if (!hist.ready()) {
    return "Histogram not ready";
  }
  std::ofstream f(path);
  if (!f) {
    return "Cannot write " + path;
  }
  const uint64_t below = hist.belowThreshold(), total = hist.entries() + below;
  const double rate = windowRateHz(m_Cfg), bw = hist.binWidth();
  const double rateScale = total > 0 ? (rate > 0 ? rate : 1.0) / total / bw : 0;
  f << "# SimSiPM histogram\n";
  f << "# quantity: " << kQuantity[feature] << "\n";
  f << "# sipm: " << refName() << "\n";
  if (!note.empty()) {
    f << "# " << note << "\n";
  }
  f << "# entries: " << hist.entries() << ", below threshold: " << below << "\n";
  f << "# window rate [Hz]: " << rate << " (0 = independent events)\n";
  f << "# bin width: " << bw << " " << kUnit[feature] << "\n";
  f << "# rate = counts / (entries + below threshold) x window rate / bin width [" << (rate > 0 ? "Hz" : "events")
    << " / " << kUnit[feature] << "]" << (rate > 0 ? "" : ", per event") << "\n";
  f << "bin_low,bin_high,counts,rate\n";
  const auto& st = hist.stairs();
  for (size_t i = 0; i + 1 < st.size(); ++i) {
    const double lo = hist.lo() + i * bw;
    f << lo << "," << lo + bw << "," << st[i] << "," << st[i] * rateScale << "\n";
  }
  return f ? std::string() : "Write error " + path;
}

// --------------------------------------------------------- parameter scan

void App::drawScanWindow() {
  if (!m_Scan.window) {
    return;
  }
  const float fs = ImGui::GetFontSize();
  ImGui::SetNextWindowSize(ImVec2(46 * fs, 26 * fs), ImGuiCond_FirstUseEver);
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - 47 * fs, vp->WorkPos.y + 3 * fs),
                          ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Parameter scan", &m_Scan.window, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }
  Scan& sc = m_Scan;
  ImGui::BeginDisabled(sc.running);
  int mode = sc.linked ? 1 : 0;
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("Mode");
  ImGui::SameLine(6 * fs);
  ImGui::SetNextItemWidth(12 * fs);
  if (ImGui::Combo("##mode", &mode, "Matrix (all combinations)\0Linked (follow first axis)\0")) {
    sc.linked = mode == 1;
  }
  ImGui::SetItemTooltip("Matrix: every combination of the axis values.\n"
                        "Linked: all axes advance together; the first one sets the number of steps and\n"
                        "the others follow it over their own range.");

  static std::string items;
  if (items.empty()) {
    for (const auto& p : kScanParams) {
      items += std::string(p.label) + '\0';
    }
  }
  const ImGuiTableFlags tflags =
    ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
  int remove = -1;
  if (ImGui::BeginTable("##axes", 6, tflags)) {
    ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch, 3);
    ImGui::TableSetupColumn("Start", ImGuiTableColumnFlags_WidthStretch, 1.5f);
    ImGui::TableSetupColumn("End", ImGuiTableColumnFlags_WidthStretch, 1.5f);
    ImGui::TableSetupColumn("Steps", ImGuiTableColumnFlags_WidthStretch, 1);
    ImGui::TableSetupColumn("Scale", ImGuiTableColumnFlags_WidthStretch, 1.3f);
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 1.6f * fs);
    ImGui::TableHeadersRow();
    for (size_t a = 0; a < sc.axes.size(); ++a) {
      ScanAxis& ax = sc.axes[a];
      ImGui::PushID(static_cast<int>(a));
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (ImGui::Combo("##p", &ax.param, items.c_str())) {
        // Start from the current value of the parameter
        ax.start = m_Cfg.*kScanParams[ax.param].member;
        ax.end = ax.start * 2;
      }
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-FLT_MIN);
      ImGui::InputDouble("##s", &ax.start, 0, 0, "%.4g");
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-FLT_MIN);
      ImGui::InputDouble("##e", &ax.end, 0, 0, "%.4g");
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (sc.linked && a > 0) {
        ImGui::BeginDisabled();
        int n = sc.axes[0].steps;
        ImGui::InputInt("##n", &n, 0, 0);
        ImGui::EndDisabled();
      } else {
        ImGui::InputInt("##n", &ax.steps, 0, 0);
        ax.steps = std::clamp(ax.steps, 1, 1000);
      }
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-FLT_MIN);
      int lg = ax.log ? 1 : 0;
      if (ImGui::Combo("##l", &lg, "Linear\0Log\0")) {
        ax.log = lg == 1;
      }
      ImGui::TableNextColumn();
      ImGui::BeginDisabled(sc.axes.size() == 1);
      if (ImGui::Button("x")) {
        remove = static_cast<int>(a);
      }
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  if (remove >= 0) {
    sc.axes.erase(sc.axes.begin() + remove);
  }
  if (ImGui::Button("Add parameter")) {
    ScanAxis ax;
    ax.param = std::min(static_cast<int>(sc.axes.size()), kNumScanParams - 1);
    ax.start = m_Cfg.*kScanParams[ax.param].member;
    ax.end = ax.start * 2;
    ax.steps = sc.axes.empty() ? 5 : sc.axes[0].steps;
    sc.axes.push_back(ax);
  }
  size_t total = sc.linked ? static_cast<size_t>(sc.axes[0].steps) : 1;
  if (!sc.linked) {
    for (const auto& ax : sc.axes) {
      total *= static_cast<size_t>(ax.steps);
    }
  }
  ImGui::SameLine(0, 2 * fs);
  ImGui::TextDisabled("%zu steps", total);

  ImGui::Separator();
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("Events per step");
  ImGui::SameLine(10 * fs);
  ImGui::SetNextItemWidth(8 * fs);
  ImGui::InputInt("##ev", &sc.eventsPerStep, 0, 0);
  sc.eventsPerStep = std::max(sc.eventsPerStep, 1000); // histograms auto-range after 1000 events
  ImGui::Checkbox("Save histograms (CSV)", &sc.saveHistograms);
  ImGui::SameLine(0, 2 * fs);
  ImGui::Checkbox("Save waveforms (.sipmw)", &sc.saveWaveforms);
  ImGui::SetItemTooltip("Records the events of each step with the resolution set in Waveform recording");
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("Output folder");
  ImGui::SameLine(10 * fs);
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::InputText("##dir", &sc.baseDir);
  ImGui::SetItemTooltip("A new scan_<date>_<time> folder is created here for every scan");
  ImGui::EndDisabled();

  ImGui::Spacing();
  ImGui::PushStyleColor(ImGuiCol_Button, sc.running ? ImVec4(0.55f, 0.20f, 0.22f, 1) : ImVec4(0.16f, 0.45f, 0.30f, 1));
  if (ImGui::Button(sc.running ? "Abort scan" : "Start scan", ImVec2(10 * fs, 1.6f * fs))) {
    if (sc.running) {
      finishScan("aborted");
    } else {
      startScan();
    }
  }
  ImGui::PopStyleColor();
  if (sc.running && !sc.points.empty()) {
    ImGui::SameLine();
    const uint64_t ev = m_Hists[0].entries() + m_Hists[0].belowThreshold();
    const double frac = (sc.step + (sc.acquiring ? std::min(1.0, double(ev) / sc.eventsPerStep) : 0.0)) /
                        static_cast<double>(sc.points.size());
    std::string label = "Step " + std::to_string(sc.step + 1) + "/" + std::to_string(sc.points.size());
    for (size_t a = 0; a < sc.axes.size() && sc.step < sc.points.size(); ++a) {
      char b[96];
      std::snprintf(b, sizeof(b), "%s %s = %.4g", a == 0 ? ":" : ",", kScanParams[sc.axes[a].param].label,
                    sc.points[sc.step][a]);
      label += b;
    }
    ImGui::ProgressBar(static_cast<float>(frac), ImVec2(-FLT_MIN, 1.6f * fs), label.c_str());
  } else if (!sc.status.empty()) {
    ImGui::SameLine();
    ImGui::TextDisabled("Last scan %s: %s", sc.status.c_str(), sc.folder.string().c_str());
  }
  ImGui::End();
}

void App::startScan() {
  Scan& sc = m_Scan;
  for (const auto& ax : sc.axes) {
    if (ax.log && (ax.start <= 0 || ax.end <= 0)) {
      message(std::string("Log scale needs positive values: ") + kScanParams[ax.param].label);
      return;
    }
  }
  // Steps: linked = all axes at the same index; matrix = cartesian product, last axis fastest
  sc.points.clear();
  if (sc.linked) {
    const int n = sc.axes[0].steps;
    for (int i = 0; i < n; ++i) {
      std::vector<double> p;
      for (const auto& ax : sc.axes) {
        p.push_back(scanValue(ax.start, ax.end, i, n, ax.log));
      }
      sc.points.push_back(p);
    }
  } else {
    std::vector<int> idx(sc.axes.size(), 0);
    while (true) {
      std::vector<double> p;
      for (size_t a = 0; a < sc.axes.size(); ++a) {
        p.push_back(scanValue(sc.axes[a].start, sc.axes[a].end, idx[a], sc.axes[a].steps, sc.axes[a].log));
      }
      sc.points.push_back(p);
      int a = static_cast<int>(sc.axes.size()) - 1;
      while (a >= 0 && ++idx[a] == sc.axes[a].steps) {
        idx[a--] = 0;
      }
      if (a < 0) {
        break;
      }
    }
  }
  const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  char stamp[32], created[32];
  std::strftime(stamp, sizeof(stamp), "scan_%Y%m%d_%H%M%S", std::localtime(&now));
  std::strftime(created, sizeof(created), "%Y-%m-%dT%H:%M:%S", std::localtime(&now));
  std::error_code ec;
  sc.folder = std::filesystem::path(sc.baseDir) / stamp;
  std::filesystem::create_directories(sc.folder, ec);
  if (ec) {
    message("Cannot create " + sc.folder.string());
    return;
  }
  stopRecording("Recording stopped (scan)");
  sc.created = created;
  sc.status = "running";
  sc.base = m_Cfg;
  sc.baseModelIdx = m_ModelIdx;
  sc.baseSettings = settingsJson(-1);
  sc.done.clear();
  sc.step = 0;
  sc.acquiring = false;
  sc.running = true;
  m_Pipeline.setRunning(true);
  writeScanJson();
  message("Scan started: " + sc.folder.string());
}

void App::updateScan() {
  Scan& sc = m_Scan;
  if (!sc.running) {
    return;
  }
  char prefix[32];
  std::snprintf(prefix, sizeof(prefix), "step_%04zu", sc.step);
  if (!sc.acquiring) {
    // Apply the step: base settings + scanned values
    m_Cfg = sc.base;
    m_ModelIdx = sc.baseModelIdx;
    for (size_t a = 0; a < sc.axes.size(); ++a) {
      m_Cfg.*kScanParams[sc.axes[a].param].member = sc.points[sc.step][a];
    }
    applyConfig();
    sc.stepConfig = m_Cfg;
    if (sc.saveWaveforms) {
      m_RecFile = (sc.folder / (std::string(prefix) + ".sipmw")).string();
      m_RecCount = sc.eventsPerStep;
      startRecording();
    }
    sc.acquiring = true;
    sc.stepStart = Clock::now();
    return;
  }
  const uint64_t events = m_Hists[0].entries() + m_Hists[0].belowThreshold();
  if (events < static_cast<uint64_t>(sc.eventsPerStep) || m_Recording) {
    return; // still acquiring (or the recording is still being written)
  }
  // Step complete: freeze, save, record
  ScanStep st;
  st.values = sc.points[sc.step];
  st.events = events;
  st.seconds = std::chrono::duration<double>(Clock::now() - sc.stepStart).count();
  std::string label;
  for (size_t a = 0; a < sc.axes.size(); ++a) {
    char b[96];
    std::snprintf(b, sizeof(b), "%s%s=%.4g", a ? ", " : "", kScanParams[sc.axes[a].param].label, st.values[a]);
    label += b;
  }
  for (int f = 0; f < 4; ++f) {
    freezeHistogram(f, "S" + std::to_string(sc.step + 1) + ": " + label);
    if (sc.saveHistograms && m_Hists[f].ready()) {
      const std::string file = std::string(prefix) + "_" + kFeatureNames[f] + ".csv";
      if (writeHistogramCsv((sc.folder / file).string(), f, "scan step " + std::to_string(sc.step) + ": " + label)
            .empty()) {
        st.files.emplace_back(kFeatureNames[f], file);
      }
    }
  }
  if (sc.saveWaveforms) {
    st.files.emplace_back("waveforms", std::string(prefix) + ".sipmw");
  }
  const std::string settingsFile = std::string(prefix) + "_settings.json";
  std::ofstream(sc.folder / settingsFile) << settingsJson(2) << "\n";
  st.files.emplace_back("settings", settingsFile);
  sc.done.push_back(std::move(st));
  ++sc.step;
  sc.acquiring = false;
  writeScanJson();
  if (sc.step == sc.points.size()) {
    finishScan("complete");
  }
}

void App::finishScan(const char* status) {
  Scan& sc = m_Scan;
  if (!sc.running) {
    return;
  }
  stopRecording("Recording stopped (scan)");
  sc.running = false;
  sc.status = status;
  writeScanJson();
  // Back to the settings the scan started from
  m_Cfg = sc.base;
  m_ModelIdx = sc.baseModelIdx;
  applyConfig();
  message("Scan " + sc.status + ": " + std::to_string(sc.done.size()) + " steps in " + sc.folder.string());
}

void App::writeScanJson() const {
  const Scan& sc = m_Scan;
  nlohmann::json axes = nlohmann::json::array();
  for (const auto& ax : sc.axes) {
    axes.push_back({{"parameter", kScanParams[ax.param].key},
                    {"label", kScanParams[ax.param].label},
                    {"start", ax.start},
                    {"end", ax.end},
                    {"steps", sc.linked ? sc.axes[0].steps : ax.steps},
                    {"scale", ax.log ? "log" : "linear"}});
  }
  nlohmann::json steps = nlohmann::json::array();
  for (size_t i = 0; i < sc.done.size(); ++i) {
    const ScanStep& st = sc.done[i];
    nlohmann::json values, files;
    for (size_t a = 0; a < sc.axes.size(); ++a) {
      values[kScanParams[sc.axes[a].param].key] = st.values[a];
    }
    for (const auto& [kind, file] : st.files) {
      files[kind] = file;
    }
    steps.push_back(
      {{"index", i}, {"values", values}, {"events", st.events}, {"seconds", st.seconds}, {"files", files}});
  }
  const nlohmann::json doc = {
    {"format", "SimSiPM parameter scan"},
    {"version", 1},
    {"created", sc.created},
    {"status", sc.status},
    {"mode", sc.linked ? "linked" : "matrix"},
    {"events_per_step", sc.eventsPerStep},
    {"total_steps", sc.points.size()},
    {"axes", axes},
    {"files_note", "file names are relative to this folder; histograms are CSV with a commented header, "
                   "waveforms use the .sipmw format (see tools/read_waveforms.py), settings can be loaded in the GUI"},
    {"base_settings", nlohmann::json::parse(sc.baseSettings, nullptr, false)},
    {"steps", steps}};
  std::ofstream(sc.folder / "scan.json") << doc.dump(2) << "\n";
}

// ------------------------------------------------------------- data flow

void App::drainResults() {
  // Results are small (waveforms only for scope candidates), so the GUI keeps
  // up with the workers; the time budget only protects responsiveness.
  m_Pipeline.setTriggerLevel(m_TriggerLevel);
  const auto deadline = Clock::now() + std::chrono::milliseconds(4);
  ResultBatch rb;
  while (Clock::now() < deadline && m_Pipeline.results().tryPop(rb)) {
    if (rb.version != m_Version || rb.features.empty()) {
      continue;
    }
    for (const EventFeatures& f : rb.features) {
      for (int i = 0; i < 4; ++i) {
        m_Hists[i].add(featureOf(f, i));
      }
    }
    m_CurSampling = rb.sampling;
    m_CurNSide = rb.nSideCells;

    // Keep only the newest triggered and the newest event as candidates.
    // (the level may have changed since the worker picked them: re-check)
    if (rb.lastIsTriggered && rb.last.f.maxAmp > m_TriggerLevel) {
      m_CandTrig = std::move(rb.last);
      m_HasCandTrig = true;
      continue;
    }
    if (rb.hasTriggered && !rb.lastIsTriggered && rb.lastTriggered.f.maxAmp > m_TriggerLevel) {
      m_CandTrig = std::move(rb.lastTriggered);
      m_HasCandTrig = true;
    }
    if (!rb.last.waveform.empty()) {
      m_CandAny = std::move(rb.last);
      m_HasCandAny = true;
    }
  }

  const auto now = Clock::now();
  Clock::duration period{0};
  switch (m_Rate) {
    case UpdateRate::k60Hz:
      period = std::chrono::microseconds(16667);
      break;
    case UpdateRate::k5Hz:
      period = std::chrono::milliseconds(200);
      break;
    case UpdateRate::k1Hz:
      period = std::chrono::seconds(1);
      break;
    default:
      break;
  }
  const bool due = m_Rate == UpdateRate::kMax || (m_Rate == UpdateRate::kOnClick && m_AcquireRequested) ||
                   (period.count() > 0 && now >= m_NextShow);
  if (!due) {
    return;
  }

  EventResult* pick = nullptr;
  switch (m_Trigger) {
    case TriggerMode::kAuto:
      // Free-run if nothing crossed the level for a while
      if (m_HasCandTrig) {
        pick = &m_CandTrig;
      } else if (m_HasCandAny && now - m_LastTriggered > std::chrono::milliseconds(200)) {
        pick = &m_CandAny;
      }
      break;
    case TriggerMode::kNormal:
      if (m_HasCandTrig) {
        pick = &m_CandTrig;
      }
      break;
    case TriggerMode::kSingle:
      if (m_Armed && m_HasCandTrig) {
        pick = &m_CandTrig;
        m_Armed = false;
      }
      break;
  }
  if (pick == nullptr) {
    return;
  }
  if (pick == &m_CandTrig) {
    m_LastTriggered = now;
  }

  const size_t persist = kPersistChoices[m_PersistenceIdx];
  if (m_HasShown && persist > 0) {
    m_Persistence.push_back(std::move(m_Shown.waveform));
    while (m_Persistence.size() > persist) {
      m_Persistence.pop_front();
    }
  }
  m_Shown = std::move(*pick);
  m_HasShown = true;
  ++m_NDisplayed; // only events actually shown on the scope count as displayed
  m_ShownSampling = m_CurSampling;
  m_ShownNSide = m_CurNSide;
  m_HasCandTrig = m_HasCandAny = false;
  m_AcquireRequested = false;
  // Fixed-rate schedule (not "period since last show") so frame jitter does
  // not skip updates; resynchronise after a stall
  m_NextShow = (period.count() > 0 && m_NextShow + 2 * period > now) ? m_NextShow + period : now + period;
  m_TexDirty = m_View == View::kMatrix;
}

void App::applyConfig() {
  m_Dirty = false;
  // The file header describes one configuration: a change ends the recording
  stopRecording("Recording stopped (settings changed)");
  SimConfig& c = m_Cfg;
  // Cross-field sanity
  c.pitchUm = std::min(c.pitchUm, 1000 * c.sizeMm);
  c.signalLength = std::clamp(c.signalLength, 2 * c.sampling, 2e6 * c.sampling);
  if (c.wlMax < c.wlMin) {
    std::swap(c.wlMin, c.wlMax);
  }

  m_Version = m_Pipeline.setConfig(c);
  resetDisplay();
  m_Hists[2].setQuantum(c.sampling);
  m_Hists[3].setQuantum(c.sampling);
}

void App::restartPipeline() {
  stopRecording("Recording stopped");
  m_Pipeline.start(m_NWorkers);
  resetDisplay();
}

void App::resetDisplay() {
  for (auto& h : m_Hists) {
    h.clear();
  }
  m_Persistence.clear();
  m_HasCandTrig = m_HasCandAny = false;
  m_FitScopeX = true;
  m_Armed = true;
}

} // namespace sipmgui
