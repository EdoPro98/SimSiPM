#include "SiPM.h"
#include <gtest/gtest.h>
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace sipm;

struct TestSiPMProperties : public ::testing::Test {
  SiPMProperties sut;
};

// ── Default values ────────────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, DefaultValues) {
  EXPECT_EQ(sut.size(), 1u);
  EXPECT_EQ(sut.pitch(), 25u);
  EXPECT_DOUBLE_EQ(sut.sampling(), 1.0);
  EXPECT_DOUBLE_EQ(sut.signalLength(), 500.0);
  EXPECT_DOUBLE_EQ(sut.risingTime(), 1.0);
  EXPECT_DOUBLE_EQ(sut.fallingTimeFast(), 50.0);
  EXPECT_DOUBLE_EQ(sut.recoveryTime(), 50.0);
  EXPECT_DOUBLE_EQ(sut.dcr(), 200e3);
  EXPECT_DOUBLE_EQ(sut.xt(), 0.05);
  EXPECT_DOUBLE_EQ(sut.ap(), 0.03);
  EXPECT_DOUBLE_EQ(sut.tauApFast(), 10.0);
  EXPECT_DOUBLE_EQ(sut.tauApSlow(), 80.0);
  EXPECT_DOUBLE_EQ(sut.apSlowFraction(), 0.5);
  EXPECT_FLOAT_EQ(sut.ccgv(), 0.05f);
  EXPECT_DOUBLE_EQ(sut.snrdB(), 30.0);
  EXPECT_DOUBLE_EQ(sut.gain(), 1.0);
}

TEST_F(TestSiPMProperties, DefaultFlagsCorrect) {
  EXPECT_TRUE(sut.hasDcr());
  EXPECT_TRUE(sut.hasXt());
  EXPECT_TRUE(sut.hasAp());
  EXPECT_FALSE(sut.hasDXt());
  EXPECT_FALSE(sut.hasSlowComponent());
  EXPECT_EQ(sut.pdeType(), SiPMProperties::PdeType::kNoPde);
}

// ── Cell count invariants ─────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, NCellsEqualsNSideCellsSquared) {
  EXPECT_EQ(sut.nCells(), sut.nSideCells() * sut.nSideCells());
}

TEST_F(TestSiPMProperties, NCellsEqualsNSideCellsSquaredAfterSetSize) {
  sut.setSize(3);
  EXPECT_EQ(sut.nCells(), sut.nSideCells() * sut.nSideCells());
}

TEST_F(TestSiPMProperties, NCellsEqualsNSideCellsSquaredAfterSetPitch) {
  sut.setPitch(50);
  EXPECT_EQ(sut.nCells(), sut.nSideCells() * sut.nSideCells());
}

TEST_F(TestSiPMProperties, SetSizeUpdatesCellCount) {
  // nSideCells = 1000 * size / pitch
  sut.setSize(2);
  const uint32_t expected_side = 1000 * 2 / sut.pitch();
  EXPECT_EQ(sut.nSideCells(), expected_side);
  EXPECT_EQ(sut.nCells(), expected_side * expected_side);
}

TEST_F(TestSiPMProperties, SetPitchUpdatesCellCount) {
  sut.setPitch(50);
  const uint32_t expected_side = 1000 * sut.size() / 50;
  EXPECT_EQ(sut.nSideCells(), expected_side);
  EXPECT_EQ(sut.nCells(), expected_side * expected_side);
}

// ── Signal points invariant ───────────────────────────────────────────────────

TEST_F(TestSiPMProperties, NSignalPointsEqualsLengthOverSampling) {
  EXPECT_EQ(sut.nSignalPoints(), (uint32_t)(sut.signalLength() / sut.sampling()));
}

TEST_F(TestSiPMProperties, SetSamplingUpdatesSignalPoints) {
  sut.setSampling(0.5);
  EXPECT_DOUBLE_EQ(sut.sampling(), 0.5);
  EXPECT_EQ(sut.nSignalPoints(), (uint32_t)(sut.signalLength() / 0.5));
}

TEST_F(TestSiPMProperties, SetSignalLengthUpdatesSignalPoints) {
  sut.setSignalLength(200);
  EXPECT_DOUBLE_EQ(sut.signalLength(), 200.0);
  EXPECT_EQ(sut.nSignalPoints(), (uint32_t)(200.0 / sut.sampling()));
}

// ── SNR conversion ────────────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, SnrLinearMatchesDatabaseConversion) {
  const double dB = 30.0;
  const double expected_linear = std::pow(10.0, -dB / 20.0);
  EXPECT_NEAR(sut.snrLinear(), expected_linear, 1e-10);
}

TEST_F(TestSiPMProperties, SetSnrUpdatesLinear) {
  sut.setSnr(40.0);
  EXPECT_DOUBLE_EQ(sut.snrdB(), 40.0);
  const double expected = std::pow(10.0, -40.0 / 20.0);
  EXPECT_NEAR(sut.snrLinear(), expected, 1e-10);
}

TEST_F(TestSiPMProperties, SetSnrViaStringUpdatesLinear) {
  sut.setProperty("snr", 20.0);
  const double expected = std::pow(10.0, -20.0 / 20.0);
  EXPECT_NEAR(sut.snrLinear(), expected, 1e-10);
}

// ── PDE setters ───────────────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, SetPdeSwitchesToSimplePde) {
  sut.setPde(0.3);
  EXPECT_DOUBLE_EQ(sut.pde(), 0.3);
  EXPECT_EQ(sut.pdeType(), SiPMProperties::PdeType::kSimplePde);
}

TEST_F(TestSiPMProperties, SetPdeTypeSwitchesMode) {
  sut.setPdeType(SiPMProperties::PdeType::kNoPde);
  EXPECT_EQ(sut.pdeType(), SiPMProperties::PdeType::kNoPde);
  sut.setPdeType(SiPMProperties::PdeType::kSpectrumPde);
  EXPECT_EQ(sut.pdeType(), SiPMProperties::PdeType::kSpectrumPde);
  sut.setPdeType(SiPMProperties::PdeType::kSimplePde);
  EXPECT_EQ(sut.pdeType(), SiPMProperties::PdeType::kSimplePde);
}

TEST_F(TestSiPMProperties, SetPdeSpectrumSwitchesToSpectrumPde) {
  std::vector<double> wav = {300, 400, 500, 600};
  std::vector<double> pde = {0.1, 0.3, 0.4, 0.2};
  sut.setPdeSpectrum(wav, pde);
  EXPECT_EQ(sut.pdeType(), SiPMProperties::PdeType::kSpectrumPde);
  EXPECT_FALSE(sut.pdeSpectrum().empty());
}

// ── Slow component ────────────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, SetFallTimeSlowEnablesSlowComponent) {
  EXPECT_FALSE(sut.hasSlowComponent());
  sut.setFallTimeSlow(200);
  EXPECT_DOUBLE_EQ(sut.fallingTimeSlow(), 200.0);
  EXPECT_TRUE(sut.hasSlowComponent());
}

TEST_F(TestSiPMProperties, SetSlowComponentFractionEnablesSlowComponent) {
  EXPECT_FALSE(sut.hasSlowComponent());
  sut.setSlowComponentFraction(0.3);
  EXPECT_DOUBLE_EQ(sut.slowComponentFraction(), 0.3);
  EXPECT_TRUE(sut.hasSlowComponent());
}

TEST_F(TestSiPMProperties, SetSlowComponentOffDisablesIt) {
  sut.setFallTimeSlow(200);
  EXPECT_TRUE(sut.hasSlowComponent());
  sut.setSlowComponentOff();
  EXPECT_FALSE(sut.hasSlowComponent());
}

// ── Noise toggle flags ────────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, DcrToggle) {
  EXPECT_TRUE(sut.hasDcr());
  sut.setDcrOff();
  EXPECT_FALSE(sut.hasDcr());
  sut.setDcrOn();
  EXPECT_TRUE(sut.hasDcr());
}

TEST_F(TestSiPMProperties, XtToggle) {
  EXPECT_TRUE(sut.hasXt());
  sut.setXtOff();
  EXPECT_FALSE(sut.hasXt());
  sut.setXtOn();
  EXPECT_TRUE(sut.hasXt());
}

TEST_F(TestSiPMProperties, ApToggle) {
  EXPECT_TRUE(sut.hasAp());
  sut.setApOff();
  EXPECT_FALSE(sut.hasAp());
  sut.setApOn();
  EXPECT_TRUE(sut.hasAp());
}

TEST_F(TestSiPMProperties, DxtToggle) {
  EXPECT_FALSE(sut.hasDXt());
  sut.setDXtOn();
  EXPECT_TRUE(sut.hasDXt());
  sut.setDXtOff();
  EXPECT_FALSE(sut.hasDXt());
}

TEST_F(TestSiPMProperties, SetDcrEnablesFlag) {
  sut.setDcrOff();
  EXPECT_FALSE(sut.hasDcr());
  sut.setDcr(500e3);
  EXPECT_TRUE(sut.hasDcr());
  EXPECT_DOUBLE_EQ(sut.dcr(), 500e3);
}

TEST_F(TestSiPMProperties, SetXtEnablesFlag) {
  sut.setXtOff();
  EXPECT_FALSE(sut.hasXt());
  sut.setXt(0.1);
  EXPECT_TRUE(sut.hasXt());
  EXPECT_DOUBLE_EQ(sut.xt(), 0.1);
}

TEST_F(TestSiPMProperties, SetApEnablesFlag) {
  sut.setApOff();
  EXPECT_FALSE(sut.hasAp());
  sut.setAp(0.05);
  EXPECT_TRUE(sut.hasAp());
  EXPECT_DOUBLE_EQ(sut.ap(), 0.05);
}

TEST_F(TestSiPMProperties, SetDXtEnablesFlag) {
  EXPECT_FALSE(sut.hasDXt());
  sut.setDXt(0.2);
  EXPECT_TRUE(sut.hasDXt());
  EXPECT_DOUBLE_EQ(sut.dxt(), 0.2);
}

// ── setProperty string API ────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, SetPropertyByNameDcr) {
  sut.setProperty("Dcr", 100e3);
  EXPECT_DOUBLE_EQ(sut.dcr(), 100e3);
}

TEST_F(TestSiPMProperties, SetPropertyByNameXt) {
  sut.setProperty("Xt", 0.15);
  EXPECT_DOUBLE_EQ(sut.xt(), 0.15);
}

TEST_F(TestSiPMProperties, SetPropertyByNameAp) {
  sut.setProperty("Ap", 0.07);
  EXPECT_DOUBLE_EQ(sut.ap(), 0.07);
}

TEST_F(TestSiPMProperties, SetPropertyCaseInsensitive) {
  sut.setProperty("DCR", 50e3);
  EXPECT_DOUBLE_EQ(sut.dcr(), 50e3);
  sut.setProperty("dcr", 75e3);
  EXPECT_DOUBLE_EQ(sut.dcr(), 75e3);
}

TEST_F(TestSiPMProperties, SetPropertySize) {
  sut.setProperty("Size", 3);
  EXPECT_EQ(sut.size(), 3u);
  EXPECT_EQ(sut.nCells(), sut.nSideCells() * sut.nSideCells());
}

TEST_F(TestSiPMProperties, SetPropertyPitch) {
  sut.setProperty("Pitch", 50);
  EXPECT_EQ(sut.pitch(), 50u);
}

TEST_F(TestSiPMProperties, SetPropertySampling) {
  sut.setProperty("Sampling", 2.0);
  EXPECT_DOUBLE_EQ(sut.sampling(), 2.0);
  EXPECT_EQ(sut.nSignalPoints(), (uint32_t)(sut.signalLength() / 2.0));
}

TEST_F(TestSiPMProperties, SetPropertySignalLength) {
  sut.setProperty("SignalLength", 300.0);
  EXPECT_DOUBLE_EQ(sut.signalLength(), 300.0);
}

TEST_F(TestSiPMProperties, SetPropertyRiseTime) {
  sut.setProperty("RiseTime", 2.5);
  EXPECT_DOUBLE_EQ(sut.risingTime(), 2.5);
}

TEST_F(TestSiPMProperties, SetPropertyFallTimeFast) {
  sut.setProperty("FallTimeFast", 30.0);
  EXPECT_DOUBLE_EQ(sut.fallingTimeFast(), 30.0);
}

TEST_F(TestSiPMProperties, SetPropertyFallTimeSlow) {
  sut.setProperty("FallTimeSlow", 150.0);
  EXPECT_DOUBLE_EQ(sut.fallingTimeSlow(), 150.0);
  EXPECT_TRUE(sut.hasSlowComponent());
}

TEST_F(TestSiPMProperties, SetPropertyRecoveryTime) {
  sut.setProperty("RecoveryTime", 100.0);
  EXPECT_DOUBLE_EQ(sut.recoveryTime(), 100.0);
}

TEST_F(TestSiPMProperties, SetPropertyCcgv) {
  sut.setProperty("Ccgv", 0.1);
  EXPECT_FLOAT_EQ(sut.ccgv(), 0.1f);
}

TEST_F(TestSiPMProperties, SetPropertyPde) {
  sut.setProperty("Pde", 0.4);
  EXPECT_DOUBLE_EQ(sut.pde(), 0.4);
  EXPECT_EQ(sut.pdeType(), SiPMProperties::PdeType::kSimplePde);
}

// ── Hit distribution ──────────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, SetHitDistribution) {
  sut.setHitDistribution(SiPMProperties::HitDistribution::kUniform);
  EXPECT_EQ(sut.hitDistribution(), SiPMProperties::HitDistribution::kUniform);
  sut.setHitDistribution(SiPMProperties::HitDistribution::kGaussian);
  EXPECT_EQ(sut.hitDistribution(), SiPMProperties::HitDistribution::kGaussian);
  sut.setHitDistribution(SiPMProperties::HitDistribution::kCircle);
  EXPECT_EQ(sut.hitDistribution(), SiPMProperties::HitDistribution::kCircle);
}

// ── AP time constants ─────────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, TauApComponents) {
  sut.setTauApFastComponent(5.0);
  EXPECT_DOUBLE_EQ(sut.tauApFast(), 5.0);
  sut.setTauApSlowComponent(120.0);
  EXPECT_DOUBLE_EQ(sut.tauApSlow(), 120.0);
  sut.setApSlowFraction(0.7);
  EXPECT_DOUBLE_EQ(sut.apSlowFraction(), 0.7);
}

// ── toString / ostream ────────────────────────────────────────────────────────

TEST_F(TestSiPMProperties, ToStringDoesNotCrash) {
  const std::string s = sut.toString();
  EXPECT_FALSE(s.empty());
}

// ── readSettingsJson ──────────────────────────────────────────────────────────

static const std::string kTmpJson = "/tmp/sipm_props_json_test.json";

struct TestSiPMPropertiesJson : public ::testing::Test {
  void TearDown() override { std::filesystem::remove(kTmpJson); }

  SiPMProperties load(const std::string& json) {
    { std::ofstream f(kTmpJson); f << json; }  // close before reading
    return SiPMProperties::readSettingsJson(kTmpJson);
  }
};

TEST_F(TestSiPMPropertiesJson, EmptyObjectReturnsDefaults) {
  const auto p = load("{}");
  EXPECT_EQ(p.size(), 1u);
  EXPECT_EQ(p.pitch(), 25u);
  EXPECT_DOUBLE_EQ(p.sampling(), 1.0);
  EXPECT_DOUBLE_EQ(p.signalLength(), 500.0);
}

TEST_F(TestSiPMPropertiesJson, LoadsScalarGeometry) {
  const auto p = load(R"({"size": 3, "pitch": 35})");
  EXPECT_EQ(p.size(), 3u);
  EXPECT_EQ(p.pitch(), 35u);
}

TEST_F(TestSiPMPropertiesJson, LoadsSignalTiming) {
  const auto p = load(R"({
    "sampling": 0.25,
    "signalLength": 400,
    "risingTime": 1.5,
    "fallingTimeFast": 45,
    "fallingTimeSlow": 200,
    "slowComponentFraction": 0.3,
    "recoveryTime": 60
  })");
  EXPECT_DOUBLE_EQ(p.sampling(), 0.25);
  EXPECT_DOUBLE_EQ(p.signalLength(), 400.0);
  EXPECT_DOUBLE_EQ(p.risingTime(), 1.5);
  EXPECT_DOUBLE_EQ(p.fallingTimeFast(), 45.0);
  EXPECT_DOUBLE_EQ(p.fallingTimeSlow(), 200.0);
  EXPECT_DOUBLE_EQ(p.slowComponentFraction(), 0.3);
  EXPECT_DOUBLE_EQ(p.recoveryTime(), 60.0);
}

TEST_F(TestSiPMPropertiesJson, LoadsNoiseParameters) {
  const auto p = load(R"({
    "snr": 35,
    "dcr": 100000,
    "xt": 0.08,
    "dxt": 0.04,
    "dxtTau": 20,
    "ap": 0.02,
    "tauApFast": 5,
    "tauApSlow": 90,
    "apSlowFraction": 0.6,
    "ccgv": 0.1
  })");
  EXPECT_DOUBLE_EQ(p.snrdB(), 35.0);
  EXPECT_DOUBLE_EQ(p.dcr(), 100000.0);
  EXPECT_DOUBLE_EQ(p.xt(), 0.08);
  EXPECT_DOUBLE_EQ(p.dxt(), 0.04);
  EXPECT_DOUBLE_EQ(p.dxtTau(), 20.0);
  EXPECT_DOUBLE_EQ(p.ap(), 0.02);
  EXPECT_DOUBLE_EQ(p.tauApFast(), 5.0);
  EXPECT_DOUBLE_EQ(p.tauApSlow(), 90.0);
  EXPECT_DOUBLE_EQ(p.apSlowFraction(), 0.6);
  EXPECT_FLOAT_EQ(p.ccgv(), 0.1f);
}

TEST_F(TestSiPMPropertiesJson, LoadsSimplePdeAsString) {
  const auto p = load(R"({"pde": 0.35, "pdeType": "kSimplePde"})");
  EXPECT_DOUBLE_EQ(p.pde(), 0.35);
  EXPECT_EQ(p.pdeType(), SiPMProperties::PdeType::kSimplePde);
}

TEST_F(TestSiPMPropertiesJson, LoadsNoPdeAsString) {
  const auto p = load(R"({"pdeType": "kNoPde"})");
  EXPECT_EQ(p.pdeType(), SiPMProperties::PdeType::kNoPde);
}

TEST_F(TestSiPMPropertiesJson, LoadsSpectrumPdeAsString) {
  const auto p = load(R"({"pdeType": "kSpectrumPde"})");
  EXPECT_EQ(p.pdeType(), SiPMProperties::PdeType::kSpectrumPde);
}

TEST_F(TestSiPMPropertiesJson, LoadsPdeTypeAsInt) {
  // kNoPde=0, kSimplePde=1, kSpectrumPde=2
  const auto p0 = load(R"({"pdeType": 0})");
  EXPECT_EQ(p0.pdeType(), SiPMProperties::PdeType::kNoPde);
  const auto p1 = load(R"({"pdeType": 1})");
  EXPECT_EQ(p1.pdeType(), SiPMProperties::PdeType::kSimplePde);
  const auto p2 = load(R"({"pdeType": 2})");
  EXPECT_EQ(p2.pdeType(), SiPMProperties::PdeType::kSpectrumPde);
}

TEST_F(TestSiPMPropertiesJson, LoadsHitDistributionAsString) {
  const auto pu = load(R"({"hitDistribution": "kUniform"})");
  EXPECT_EQ(pu.hitDistribution(), SiPMProperties::HitDistribution::kUniform);
  const auto pc = load(R"({"hitDistribution": "kCircle"})");
  EXPECT_EQ(pc.hitDistribution(), SiPMProperties::HitDistribution::kCircle);
  const auto pg = load(R"({"hitDistribution": "kGaussian"})");
  EXPECT_EQ(pg.hitDistribution(), SiPMProperties::HitDistribution::kGaussian);
}

TEST_F(TestSiPMPropertiesJson, LoadsHitDistributionAsInt) {
  const auto p0 = load(R"({"hitDistribution": 0})");
  EXPECT_EQ(p0.hitDistribution(), SiPMProperties::HitDistribution::kUniform);
  const auto p1 = load(R"({"hitDistribution": 1})");
  EXPECT_EQ(p1.hitDistribution(), SiPMProperties::HitDistribution::kCircle);
  const auto p2 = load(R"({"hitDistribution": 2})");
  EXPECT_EQ(p2.hitDistribution(), SiPMProperties::HitDistribution::kGaussian);
}

TEST_F(TestSiPMPropertiesJson, LoadsBooleanFlagsOff) {
  const auto p = load(R"({
    "hasDcr": false,
    "hasXt": false,
    "hasDXt": false,
    "hasAp": false,
    "hasSlowComponent": false
  })");
  EXPECT_FALSE(p.hasDcr());
  EXPECT_FALSE(p.hasXt());
  EXPECT_FALSE(p.hasDXt());
  EXPECT_FALSE(p.hasAp());
  EXPECT_FALSE(p.hasSlowComponent());
}

TEST_F(TestSiPMPropertiesJson, LoadsBooleanFlagsOn) {
  const auto p = load(R"({
    "hasDcr": true,
    "hasXt": true,
    "hasDXt": true,
    "hasAp": true,
    "hasSlowComponent": true
  })");
  EXPECT_TRUE(p.hasDcr());
  EXPECT_TRUE(p.hasXt());
  EXPECT_TRUE(p.hasDXt());
  EXPECT_TRUE(p.hasAp());
  EXPECT_TRUE(p.hasSlowComponent());
}

TEST_F(TestSiPMPropertiesJson, ExplicitFalseOverridesImplicitEnable) {
  // Setting dcr value would normally enable hasDcr; hasDcr:false overrides it.
  const auto p = load(R"({"dcr": 500000, "hasDcr": false})");
  EXPECT_DOUBLE_EQ(p.dcr(), 500000.0);
  EXPECT_FALSE(p.hasDcr());
}

TEST_F(TestSiPMPropertiesJson, LoadsPdeSpectrum) {
  const auto p = load(R"({
    "pdeSpectrum": {
      "wavelengths": [300, 400, 500, 600, 700],
      "pde":         [0.1, 0.2, 0.3, 0.25, 0.1]
    }
  })");
  EXPECT_EQ(p.pdeType(), SiPMProperties::PdeType::kSpectrumPde);
  EXPECT_FALSE(p.pdeSpectrum().empty());
}

TEST_F(TestSiPMPropertiesJson, DerivedCellCountUpdated) {
  const auto p = load(R"({"size": 3, "pitch": 35})");
  const uint32_t expected_side = 1000u * 3u / 35u;
  EXPECT_EQ(p.nSideCells(), expected_side);
  EXPECT_EQ(p.nCells(), expected_side * expected_side);
}

TEST_F(TestSiPMPropertiesJson, DerivedSignalPointsUpdated) {
  const auto p = load(R"({"sampling": 0.25, "signalLength": 400})");
  EXPECT_EQ(p.nSignalPoints(), static_cast<uint32_t>(400.0 / 0.25));
}

TEST_F(TestSiPMPropertiesJson, DerivedSnrLinearUpdated) {
  const auto p = load(R"({"snr": 40})");
  const double expected = std::pow(10.0, -40.0 / 20.0);
  EXPECT_NEAR(p.snrLinear(), expected, 1e-10);
}

TEST_F(TestSiPMPropertiesJson, MissingFileReturnsDefaults) {
  const auto p = SiPMProperties::readSettingsJson("/tmp/does_not_exist_sipm.json");
  SiPMProperties def;
  EXPECT_EQ(p.size(), def.size());
  EXPECT_EQ(p.pitch(), def.pitch());
  EXPECT_DOUBLE_EQ(p.dcr(), def.dcr());
}

TEST_F(TestSiPMPropertiesJson, MalformedJsonReturnsDefaults) {
  const auto p = load("{ this is not valid json !!!");
  SiPMProperties def;
  EXPECT_EQ(p.size(), def.size());
  EXPECT_EQ(p.pitch(), def.pitch());
  EXPECT_DOUBLE_EQ(p.dcr(), def.dcr());
}

TEST_F(TestSiPMPropertiesJson, PartialJsonLeavesOtherValuesAtDefault) {
  const auto p = load(R"({"size": 5})");
  EXPECT_EQ(p.size(), 5u);
  // Everything not in the JSON stays at construction defaults
  EXPECT_EQ(p.pitch(), 25u);
  EXPECT_DOUBLE_EQ(p.sampling(), 1.0);
  EXPECT_DOUBLE_EQ(p.dcr(), 200e3);
  EXPECT_TRUE(p.hasDcr());
  EXPECT_EQ(p.pdeType(), SiPMProperties::PdeType::kNoPde);
}
