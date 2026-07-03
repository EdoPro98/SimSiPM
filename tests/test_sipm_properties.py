"""Tests for SiPMProperties Python bindings."""
import math

import numpy as np
import pytest
import SiPM


# ── Default values ─────────────────────────────────────────────────────────────

class TestDefaults:
    def setup_method(self):
        self.p = SiPM.SiPMProperties()

    def test_size(self):             assert self.p.size() == 1
    def test_pitch(self):            assert self.p.pitch() == 25
    def test_n_cells_default(self):  assert self.p.nCells() == self.p.nSideCells() ** 2
    def test_sampling(self):         assert self.p.sampling() == pytest.approx(1.0)
    def test_signal_length(self):    assert self.p.signalLength() == pytest.approx(500.0)
    def test_rising_time(self):      assert self.p.risingTime() == pytest.approx(1.0)
    def test_falling_time_fast(self):assert self.p.fallingTimeFast() == pytest.approx(50.0)
    def test_recovery_time(self):    assert self.p.recoveryTime() == pytest.approx(50.0)
    def test_dcr(self):              assert self.p.dcr() == pytest.approx(200e3)
    def test_xt(self):               assert self.p.xt() == pytest.approx(0.05)
    def test_ap(self):               assert self.p.ap() == pytest.approx(0.03)
    def test_tau_ap_fast(self):      assert self.p.tauApFast() == pytest.approx(10.0)
    def test_tau_ap_slow(self):      assert self.p.tauApSlow() == pytest.approx(80.0)
    def test_ap_slow_fraction(self): assert self.p.apSlowFraction() == pytest.approx(0.5)
    def test_ccgv(self):             assert self.p.ccgv() == pytest.approx(0.05, abs=1e-6)
    def test_snr_db(self):           assert self.p.snrdB() == pytest.approx(30.0)

    def test_has_dcr_true(self):         assert self.p.hasDcr() is True
    def test_has_xt_true(self):          assert self.p.hasXt() is True
    def test_has_ap_true(self):          assert self.p.hasAp() is True
    def test_has_dxt_false(self):        assert self.p.hasDXt() is False
    def test_has_slow_comp_false(self):  assert self.p.hasSlowComponent() is False
    def test_pde_type_no_pde(self):
        assert self.p.pdeType() == SiPM.SiPMProperties.PdeType.kNoPde


# ── Cell-count invariant: nCells == nSideCells² ────────────────────────────────

class TestCellCountInvariant:

    def test_default(self):
        p = SiPM.SiPMProperties()
        assert p.nCells() == p.nSideCells() ** 2

    @pytest.mark.parametrize("size", [1, 2, 3, 5, 6])
    def test_after_set_size(self, size):
        p = SiPM.SiPMProperties()
        p.setSize(size)
        assert p.nCells() == p.nSideCells() ** 2
        assert p.nSideCells() == 1000 * size // p.pitch()

    @pytest.mark.parametrize("pitch", [25, 35, 50, 75, 100])
    def test_after_set_pitch(self, pitch):
        p = SiPM.SiPMProperties()
        p.setPitch(pitch)
        assert p.nCells() == p.nSideCells() ** 2
        assert p.nSideCells() == 1000 * p.size() // pitch


# ── Signal-points invariant: nSignalPoints == signalLength / sampling ──────────

class TestSignalPointsInvariant:

    def test_default(self):
        p = SiPM.SiPMProperties()
        assert p.nSignalPoints() == int(p.signalLength() / p.sampling())

    @pytest.mark.parametrize("sampling", [0.1, 0.25, 0.5, 1.0, 2.0])
    def test_after_set_sampling(self, sampling):
        p = SiPM.SiPMProperties()
        p.setSampling(sampling)
        assert p.sampling() == pytest.approx(sampling)
        assert p.nSignalPoints() == int(p.signalLength() / sampling)

    @pytest.mark.parametrize("length", [100.0, 200.0, 500.0, 1000.0])
    def test_after_set_signal_length(self, length):
        p = SiPM.SiPMProperties()
        p.setSignalLength(length)
        assert p.signalLength() == pytest.approx(length)
        assert p.nSignalPoints() == int(length / p.sampling())


# ── SNR dB → linear conversion ─────────────────────────────────────────────────

class TestSnrConversion:

    def test_default_linear(self):
        p = SiPM.SiPMProperties()
        assert p.snrLinear() == pytest.approx(10 ** (-30.0 / 20.0), rel=1e-6)

    @pytest.mark.parametrize("db", [10.0, 20.0, 30.0, 40.0, 50.0])
    def test_set_snr(self, db):
        p = SiPM.SiPMProperties()
        p.setSnr(db)
        assert p.snrdB() == pytest.approx(db)
        assert p.snrLinear() == pytest.approx(10 ** (-db / 20.0), rel=1e-6)

    def test_set_snr_via_string(self):
        p = SiPM.SiPMProperties()
        p.setProperty("Snr", 20.0)
        assert p.snrLinear() == pytest.approx(10 ** (-20.0 / 20.0), rel=1e-6)


# ── PDE setters ────────────────────────────────────────────────────────────────

class TestPdeSetters:

    def test_set_pde_value_and_type(self):
        p = SiPM.SiPMProperties()
        p.setPde(0.3)
        assert p.pde() == pytest.approx(0.3)
        assert p.pdeType() == SiPM.SiPMProperties.PdeType.kSimplePde

    @pytest.mark.parametrize("pde_type", [
        SiPM.SiPMProperties.PdeType.kNoPde,
        SiPM.SiPMProperties.PdeType.kSimplePde,
        SiPM.SiPMProperties.PdeType.kSpectrumPde,
    ])
    def test_set_pde_type(self, pde_type):
        p = SiPM.SiPMProperties()
        p.setPdeType(pde_type)
        assert p.pdeType() == pde_type

    def test_pde_spectrum_list_switches_type(self):
        p = SiPM.SiPMProperties()
        p.setPdeSpectrum([300.0, 400.0, 500.0], [0.1, 0.3, 0.4])
        assert p.pdeType() == SiPM.SiPMProperties.PdeType.kSpectrumPde
        assert len(p.pdeSpectrum()) > 0  # interpolated internally

    def test_pde_spectrum_numpy_switches_type(self):
        p = SiPM.SiPMProperties()
        p.setPdeSpectrum(np.array([300.0, 400.0, 500.0]),
                         np.array([0.1, 0.3, 0.4]))
        assert p.pdeType() == SiPM.SiPMProperties.PdeType.kSpectrumPde
        assert len(p.pdeSpectrum()) > 0  # interpolated internally

    def test_pde_spectrum_list_and_numpy_equivalent(self):
        wav = [300.0, 400.0, 500.0, 600.0]
        pde = [0.1, 0.3, 0.4, 0.2]
        p1 = SiPM.SiPMProperties()
        p2 = SiPM.SiPMProperties()
        p1.setPdeSpectrum(wav, pde)
        p2.setPdeSpectrum(np.array(wav), np.array(pde))
        spec1 = p1.pdeSpectrum()
        spec2 = p2.pdeSpectrum()
        assert len(spec1) == len(spec2)
        for k in spec1:
            assert spec1[k] == pytest.approx(spec2[k])


# ── Noise flag toggles ─────────────────────────────────────────────────────────

class TestNoiseFlags:

    def test_dcr_toggle(self):
        p = SiPM.SiPMProperties()
        assert p.hasDcr()
        p.setDcrOff(); assert not p.hasDcr()
        p.setDcrOn();  assert p.hasDcr()

    def test_xt_toggle(self):
        p = SiPM.SiPMProperties()
        assert p.hasXt()
        p.setXtOff(); assert not p.hasXt()
        p.setXtOn();  assert p.hasXt()

    def test_ap_toggle(self):
        p = SiPM.SiPMProperties()
        assert p.hasAp()
        p.setApOff(); assert not p.hasAp()
        p.setApOn();  assert p.hasAp()

    def test_dxt_toggle(self):
        p = SiPM.SiPMProperties()
        assert not p.hasDXt()
        p.setDXtOn();  assert p.hasDXt()
        p.setDXtOff(); assert not p.hasDXt()

    def test_slow_component_toggle(self):
        p = SiPM.SiPMProperties()
        assert not p.hasSlowComponent()
        p.setSlowComponentOn();  assert p.hasSlowComponent()
        p.setSlowComponentOff(); assert not p.hasSlowComponent()

    def test_set_dcr_enables_flag(self):
        p = SiPM.SiPMProperties()
        p.setDcrOff()
        p.setDcr(500e3)
        assert p.hasDcr() and p.dcr() == pytest.approx(500e3)

    def test_set_xt_enables_flag(self):
        p = SiPM.SiPMProperties()
        p.setXtOff()
        p.setXt(0.1)
        assert p.hasXt() and p.xt() == pytest.approx(0.1)

    def test_set_ap_enables_flag(self):
        p = SiPM.SiPMProperties()
        p.setApOff()
        p.setAp(0.05)
        assert p.hasAp() and p.ap() == pytest.approx(0.05)

    def test_set_dxt_enables_flag(self):
        p = SiPM.SiPMProperties()
        p.setDXt(0.2)
        assert p.hasDXt() and p.dxt() == pytest.approx(0.2)

    def test_set_fall_time_slow_enables_slow_comp(self):
        p = SiPM.SiPMProperties()
        p.setFallTimeSlow(200.0)
        assert p.fallingTimeSlow() == pytest.approx(200.0)
        assert p.hasSlowComponent()

    def test_set_slow_fraction_enables_slow_comp(self):
        p = SiPM.SiPMProperties()
        p.setSlowComponentFraction(0.3)
        assert p.slowComponentFraction() == pytest.approx(0.3)
        assert p.hasSlowComponent()


# ── Hit distribution enum ──────────────────────────────────────────────────────

class TestHitDistribution:

    @pytest.mark.parametrize("dist", [
        SiPM.SiPMProperties.HitDistribution.kUniform,
        SiPM.SiPMProperties.HitDistribution.kGaussian,
        SiPM.SiPMProperties.HitDistribution.kCircle,
    ])
    def test_set_and_get(self, dist):
        p = SiPM.SiPMProperties()
        p.setHitDistribution(dist)
        assert p.hitDistribution() == dist


# ── AP time constants ──────────────────────────────────────────────────────────

class TestApTimeConstants:

    def test_tau_ap_fast(self):
        p = SiPM.SiPMProperties()
        p.setTauApFastComponent(5.0)
        assert p.tauApFast() == pytest.approx(5.0)

    def test_tau_ap_slow(self):
        p = SiPM.SiPMProperties()
        p.setTauApSlowComponent(120.0)
        assert p.tauApSlow() == pytest.approx(120.0)

    def test_ap_slow_fraction(self):
        p = SiPM.SiPMProperties()
        p.setApSlowFraction(0.7)
        assert p.apSlowFraction() == pytest.approx(0.7)


# ── setProperty string API ─────────────────────────────────────────────────────

class TestSetPropertyApi:

    @pytest.mark.parametrize("name,value,getter", [
        ("Dcr",         100e3, lambda p: p.dcr()),
        ("Xt",          0.15,  lambda p: p.xt()),
        ("Ap",          0.07,  lambda p: p.ap()),
        ("RecoveryTime",100.0, lambda p: p.recoveryTime()),
        ("RiseTime",    2.5,   lambda p: p.risingTime()),
        ("FallTimeFast",30.0,  lambda p: p.fallingTimeFast()),
        ("Sampling",    2.0,   lambda p: p.sampling()),
        ("SignalLength",300.0, lambda p: p.signalLength()),
    ])
    def test_set_and_get(self, name, value, getter):
        p = SiPM.SiPMProperties()
        p.setProperty(name, value)
        assert getter(p) == pytest.approx(value)

    def test_set_size(self):
        p = SiPM.SiPMProperties()
        p.setProperty("Size", 3)
        assert p.size() == 3
        assert p.nCells() == p.nSideCells() ** 2

    def test_set_pitch(self):
        p = SiPM.SiPMProperties()
        p.setProperty("Pitch", 50)
        assert p.pitch() == 50

    def test_set_pde_via_string(self):
        p = SiPM.SiPMProperties()
        p.setProperty("Pde", 0.4)
        assert p.pde() == pytest.approx(0.4)
        assert p.pdeType() == SiPM.SiPMProperties.PdeType.kSimplePde

    def test_set_ccgv(self):
        p = SiPM.SiPMProperties()
        p.setProperty("Ccgv", 0.1)
        assert p.ccgv() == pytest.approx(0.1, abs=1e-6)

    def test_set_fall_time_slow_enables_slow_comp(self):
        p = SiPM.SiPMProperties()
        p.setProperty("FallTimeSlow", 150.0)
        assert p.fallingTimeSlow() == pytest.approx(150.0)
        assert p.hasSlowComponent()


# ── repr / str ─────────────────────────────────────────────────────────────────

class TestRepr:

    def test_repr_is_nonempty_string(self):
        p = SiPM.SiPMProperties()
        assert isinstance(repr(p), str) and len(repr(p)) > 0
