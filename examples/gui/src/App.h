#ifndef SIPM_GUI_APP_H
#define SIPM_GUI_APP_H

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "Config.h"
#include "Pipeline.h"
#include "SipmModels.h"

namespace sipmgui {

/// Fixed-binning histogram that remembers its input (up to a cap) so it can
/// be re-binned or re-ranged without losing data.
class Histogram {
public:
  void clear();
  void add(float v);
  void setBins(int n);
  /// Values are multiples of q (e.g. the sampling time); 0 = continuous
  void setQuantum(double q);
  void autoRange();

  const std::vector<double>& stairs() const { return m_Stairs; } // counts + closing point
  double lo() const { return m_Lo; }
  double binWidth() const { return (m_Hi - m_Lo) / m_Counts.size(); }
  uint64_t entries() const { return m_Entries; }
  uint64_t belowThreshold() const { return m_Below; }
  bool ready() const { return m_Ranged; }
  bool takeRangeChanged() {
    const bool r = m_RangeChanged;
    m_RangeChanged = false;
    return r;
  }

private:
  void rebin();
  void fill(float v);

  static constexpr size_t kLearn = 1000;
  static constexpr size_t kStoreCap = 2'000'000;

  std::vector<float> m_Values;
  std::vector<double> m_Counts = std::vector<double>(256, 0.0);
  std::vector<double> m_Stairs;
  double m_Lo = 0, m_Hi = 1;
  double m_Quantum = 0;
  int m_NBins = 256;
  uint64_t m_Entries = 0, m_Below = 0;
  bool m_Ranged = false;
  bool m_RangeChanged = false;
};

/// Minimal modal file picker built on std::filesystem.
class FileBrowser {
public:
  /// Opens the picker; `extensions` filters the listed files (e.g. ".csv").
  /// In save mode the file name can be typed and the first extension is appended if missing.
  void open(const std::string& title, const std::string& current, const std::vector<std::string>& extensions,
            bool save = false);
  /// Draws the popup if open. Returns true when a file has been chosen.
  bool draw(std::string& chosen);

private:
  void refresh();

  std::string m_Title;
  std::filesystem::path m_Dir;
  std::vector<std::filesystem::path> m_Dirs, m_Files;
  std::string m_Selected;
  std::vector<std::string> m_Extensions;
  bool m_Save = false;
  bool m_RequestOpen = false;
};

class App {
public:
  App();
  ~App();

  /// Builds the whole UI for one frame.
  void frame();
  /// True when the GUI should render as fast as possible (no vsync, no cap).
  bool maxFps() const { return m_Rate == UpdateRate::kMax; }
  /// A plot to export has been drawn this frame: returns its screen rectangle
  /// (logical pixels) and destination. Call after rendering, before swapping.
  bool takeCapture(float rect[4], std::string& path);
  /// Reports the result of writing the capture (empty = success)
  void captureDone(const std::string& path, const std::string& error);

private:
  enum class UpdateRate : int { kMax, k60Hz, k5Hz, k1Hz, kOnClick };
  enum class TriggerMode : int { kAuto, kNormal, kSingle };
  enum class View : int { kPeak, kIntegral, kTot, kToa, kMatrix };
  enum class FileTarget {
    kNone,
    kPde,
    kNPhotons,
    kGamma,
    kSaveSettings,
    kLoadSettings,
    kExportPng,
    kRecordFile,
    kExportCsv
  };
  enum class PlotId { kNone, kScope, kHist, kMatrix };
  using Clock = std::chrono::steady_clock;

  void drawControls();
  void drawScope(float height);
  void drawBottom(float height);
  void drawStatusBar();

  void fileField(const char* label, const std::string& path, FileTarget target, const std::string& status, bool ok);
  void loadFile(FileTarget target);
  void loadGammaSpectrum();
  void applyModel(int i);
  std::string settingsJson(int indent) const;
  void saveSettings(const std::string& path);
  void drawRecording();
  void startRecording();
  void stopRecording(const char* reason);
  void loadSettings(const std::string& path);
  void exportButton(PlotId plot, const char* name);
  bool printing(PlotId plot) const { return m_Export.plot == plot && m_Export.wait == 0; }
  void recordCapture();
  void message(const std::string& text);
  // Histogram helpers shared by the toolbar and the parameter scan
  void freezeHistogram(int feature, const std::string& name);
  std::string writeHistogramCsv(const std::string& path, int feature, const std::string& note) const;
  std::string refName() const;
  // Parameter scan
  void drawScanWindow();
  void startScan();
  void updateScan();
  void finishScan(const char* status);
  void writeScanJson() const;
  bool matchesModel() const;

  void drainResults();
  void applyConfig();
  void restartPipeline();
  void resetDisplay();
  void updateMatrixTexture();

  SimConfig m_Cfg;
  Pipeline m_Pipeline;
  uint64_t m_Version = 0;
  bool m_Dirty = false;
  int m_NWorkers = 1, m_MaxWorkers = 1;
  std::vector<std::filesystem::path> m_Profiles; // gamma sources: data/nuclides/*.csv
  std::string m_ProfileItems;                    // combo items: "Custom file" + profiles
  int m_ProfileIdx = 0;
  double m_NphMeanBin = 0; // mean bin of the loaded photon histogram
  // PNG export: the plot is drawn once in print style, then read back by main()
  struct Export {
    PlotId plot = PlotId::kNone;
    int wait = 0; // frames to wait (file dialog closing) before drawing in print style
    bool ready = false;
    float rect[4] = {0, 0, 0, 0};
    std::string path;
  } m_Export;
  PlotId m_ExportPending = PlotId::kNone; // plot whose file name is being chosen
  bool m_TexLight = false;                // matrix texture built with the print palette
  // Waveform recording
  bool m_Recording = false;
  std::string m_RecFile = (std::filesystem::current_path() / "waveforms.sipmw").string();
  int m_RecCount = 100000;
  double m_RecLsb = 1e-3;
  std::string m_Message;
  Clock::time_point m_MessageTime{};

  // Parameter scan: axes over numeric parameters, matrix (all combinations)
  // or linked (all axes advance together, following the first one)
  struct ScanAxis {
    int param = 0;
    double start = 1, end = 10;
    int steps = 5;
    bool log = false;
  };
  struct ScanStep {
    std::vector<double> values; // one per axis
    uint64_t events = 0;
    double seconds = 0;
    std::vector<std::pair<std::string, std::string>> files; // kind, file name (relative to the scan folder)
  };
  struct Scan {
    bool window = false;
    bool linked = false;
    std::vector<ScanAxis> axes{ScanAxis{}};
    int eventsPerStep = 20000;
    bool saveHistograms = true;
    bool saveWaveforms = false;
    std::string baseDir = (std::filesystem::current_path() / "scans").string();
    // run state
    bool running = false;
    bool acquiring = false; // false: apply the next step
    std::vector<std::vector<double>> points;
    size_t step = 0;
    std::filesystem::path folder;
    std::string created, status;
    std::string baseSettings; // JSON of the settings at the start
    SimConfig base, stepConfig;
    int baseModelIdx = 0;
    std::vector<ScanStep> done;
    Clock::time_point stepStart{};
  } m_Scan;
  std::string m_CsvPath;

  // Frozen histograms shown as references (kept across parameter changes)
  struct Frozen {
    std::string name;
    int feature;
    bool perEvent; // normalised per event (independent timing) instead of per second
    double lo, binWidth;
    std::vector<double> ys;     // rate (or per event) per unit
    std::vector<double> counts; // raw counts
  };
  std::vector<Frozen> m_Frozen;
  int m_FrozenCount = 0;
  std::array<size_t, 4> m_FitRefs{}; // references per view at the last X fit
  std::vector<double> m_PlotBuf;

  // Commercial SiPM presets (index 0 = custom)
  std::vector<SipmModel> m_Models;
  std::string m_ModelItems = std::string("Custom") + '\0';
  int m_ModelIdx = 0;
  SimConfig m_ModelApplied;
  // Scintillator presets and gamma source
  ScintillatorDb m_Db;
  int m_MaterialIdx = 0, m_SizeIdx = 0;
  std::string m_MaterialItems, m_SizeItems, m_SpectrumStatus;
  std::vector<GammaLine> m_GammaLines;
  std::map<std::string, std::vector<double>> m_SpectrumCache; // custom sources

  // File loading feedback
  std::string m_PdeStatus, m_NphStatus, m_GammaStatus;
  bool m_PdeOk = false, m_NphOk = false, m_GammaOk = false;
  FileBrowser m_Browser;
  FileTarget m_BrowserTarget = FileTarget::kNone;

  // Oscilloscope
  UpdateRate m_Rate = UpdateRate::k60Hz;
  TriggerMode m_Trigger = TriggerMode::kAuto;
  double m_TriggerLevel = 1.5;
  bool m_Armed = true;
  bool m_AcquireRequested = false;
  bool m_AutoScaleY = true;
  float m_ScopeYLo = -1, m_ScopeYHi = 10;
  float m_ScopePlotPx = 400; // plot area height of the previous frame
  int m_GateDrag = -1;       // gate zone being dragged: 0 start, 1 end, 2 body
  double m_GateGrab = 0;     // body drag: mouse offset from the gate start [ns]
  int m_PersistenceIdx = 0;
  bool m_FitScopeX = true;
  bool m_DragPending = false;
  Clock::time_point m_NextShow{}, m_LastTriggered{};

  EventResult m_CandTrig, m_CandAny, m_Shown;
  bool m_HasCandTrig = false, m_HasCandAny = false, m_HasShown = false;
  double m_ShownSampling = 1, m_CurSampling = 1;
  uint32_t m_ShownNSide = 0, m_CurNSide = 0;
  std::deque<std::vector<float>> m_Persistence;

  // Histograms / matrix
  View m_View = View::kPeak;
  std::array<Histogram, 4> m_Hists;
  int m_BinsIdx = 3;
  bool m_LogY = false;
  int m_HistYMode = 1; // 0 counts, 1 rate
  unsigned int m_Texture = 0;
  int m_TexSize = 0;
  bool m_TexDirty = false;
  uint32_t m_Grid = 0;            // matrix cells per side shown in the texture
  std::vector<int8_t> m_CellType; // first hit type per cell, -1 = none
  std::vector<float> m_CellTime;  // first hit time per cell

  // Rates
  Clock::time_point m_RateTime{};
  uint64_t m_LastGen = 0, m_LastSim = 0, m_LastDrop = 0, m_LastShown = 0;
  uint64_t m_NDisplayed = 0;
  double m_RateGen = 0, m_RateSim = 0, m_RateDrop = 0, m_RateGui = 0;
};

} // namespace sipmgui
#endif
