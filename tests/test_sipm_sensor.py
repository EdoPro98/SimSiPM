"""Tests for SiPMSensor Python bindings — construction, I/O, simulation correctness."""
import copy

import numpy as np
import pytest
import SiPM


def _clean(signal_length=500.0, **overrides):
    p = SiPM.SiPMProperties()
    p.setDcrOff(); p.setXtOff(); p.setApOff()
    p.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
    p.setSnr(100); p.setCcgv(0)
    p.setSignalLength(signal_length)
    for k, v in overrides.items():
        p.setProperty(k, v)
    return p


# ── Construction ───────────────────────────────────────────────────────────────

class TestConstruction:

    def test_default_constructor(self):
        assert SiPM.SiPMSensor() is not None

    def test_props_constructor(self):
        p = _clean(signal_length=200.0)
        s = SiPM.SiPMSensor(p)
        assert s.properties().signalLength() == pytest.approx(200.0)

    def test_repr_nonempty(self):
        assert len(repr(SiPM.SiPMSensor())) > 0

    def test_rng_accessible(self):
        assert isinstance(SiPM.SiPMSensor().rng(), SiPM.SiPMRandom)

    def test_debug_accessible(self):
        assert isinstance(SiPM.SiPMSensor().debug(), SiPM.SiPMDebugInfo)


# ── addPhoton / addPhotons ─────────────────────────────────────────────────────

class TestAddPhotons:

    def test_single_photon(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent()
        assert s.debug().nPhotons == 1

    def test_photon_with_wavelength(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0, 450.0); s.runEvent()
        assert s.debug().nPhotons == 1

    def test_addPhoton_accumulates(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.addPhoton(20.0); s.addPhoton(30.0)
        s.runEvent()
        assert s.debug().nPhotons == 3

    def test_addPhotons_list(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhotons([10.0, 20.0, 30.0]); s.runEvent()
        assert s.debug().nPhotons == 3

    def test_addPhotons_list_with_wavelengths(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhotons([10.0, 20.0], [400.0, 450.0]); s.runEvent()
        assert s.debug().nPhotons == 2

    def test_addPhotons_numpy(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhotons(np.array([10.0, 20.0, 30.0])); s.runEvent()
        assert s.debug().nPhotons == 3

    def test_addPhotons_numpy_with_wavelengths(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhotons(np.array([10.0, 20.0]), np.array([400.0, 450.0]))
        s.runEvent()
        assert s.debug().nPhotons == 2

    def test_addPhotons_float32_forcecast(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhotons(np.array([10.0, 20.0], dtype=np.float32)); s.runEvent()
        assert s.debug().nPhotons == 2

    def test_addPhotons_noncontiguous_forcecast(self):
        s = SiPM.SiPMSensor(_clean())
        base = np.array([10.0, 0.0, 20.0, 0.0, 30.0, 0.0])
        s.addPhotons(base[::2]); s.runEvent()  # strided slice
        assert s.debug().nPhotons == 3

    def test_addPhotons_list_numpy_equivalent(self):
        times = [10.0, 25.0, 50.0, 100.0, 200.0]
        s1 = SiPM.SiPMSensor(_clean()); s2 = SiPM.SiPMSensor(_clean())
        s1.addPhotons(times); s2.addPhotons(np.array(times))
        s1.runEvent(); s2.runEvent()
        assert s1.debug().nPhotons == s2.debug().nPhotons
        assert s1.debug().nPhotoelectrons == s2.debug().nPhotoelectrons

    def test_addPhotons_large_numpy(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhotons(np.linspace(5.0, 490.0, 1000)); s.runEvent()
        assert s.debug().nPhotons == 1000

    def test_photon_before_window_discarded(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(-1.0); s.runEvent()
        assert s.debug().nPhotoelectrons == 0

    def test_photon_after_window_discarded(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(501.0); s.runEvent()
        assert s.debug().nPhotoelectrons == 0


# ── resetState ─────────────────────────────────────────────────────────────────

class TestResetState:

    def test_clears_debug(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent(); s.resetState()
        d = s.debug()
        assert d.nPhotons == d.nPhotoelectrons == d.nDcr == d.nXt == d.nAp == 0

    def test_clears_hits(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent(); s.resetState()
        assert s.hits() == []

    def test_can_reuse_after_reset(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent(); s.resetState()
        s.addPhoton(20.0); s.runEvent()
        assert s.debug().nPhotons == 1


# ── runEvent(generateSignal) ───────────────────────────────────────────────────

class TestRunEventWithoutSignal:
    # The sensor rng cannot be seeded from Python, so use a deterministic event
    # (one photon, no noise, no cell-to-cell variation); exact same-seed
    # equivalence of both modes is checked in the C++ tests.

    @staticmethod
    def _sensor():
        s = SiPM.SiPMSensor(_clean(Snr=100))
        s.addPhoton(100.0)
        return s

    def test_positional_and_keyword_argument(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent(False)
        s.runEvent(generateSignal=False)
        s.runEvent(True)

    def test_hits_available_without_signal(self):
        s = self._sensor()
        s.runEvent(False)
        assert len(s.hits()) == 1

    def test_signal_generated_on_demand_matches_full_event(self):
        full, lazy = self._sensor(), self._sensor()
        full.runEvent()
        lazy.runEvent(False)
        wf = np.asarray(lazy.signal().waveform())
        assert wf.max() == pytest.approx(1.0, abs=0.1)  # the pulse is there
        assert np.argmax(wf) == pytest.approx(np.argmax(full.signal().waveform()), abs=3)

    def test_reset_discards_pending_signal(self):
        s = self._sensor()
        s.runEvent(False); s.resetState()
        assert s.signal().size() == 0


# ── setSignalShape ─────────────────────────────────────────────────────────────

class TestSetSignalShape:

    def test_custom_shape_is_used_verbatim(self):
        s = SiPM.SiPMSensor(_clean(Snr=100))
        assert not s.properties().hasCustomSignalShape()
        shape = [0.0, 1.0, 2.0, 1.0, 0.5]
        s.setSignalShape(shape, 2.0)
        assert s.properties().hasCustomSignalShape()
        assert s.properties().sampling() == pytest.approx(2.0)
        assert s.properties().nSignalPoints() == len(shape)
        s.addPhoton(0.0); s.runEvent()
        wf = np.asarray(s.signal().waveform())
        assert len(wf) == len(shape)
        assert np.argmax(wf) == 2 and wf[2] == pytest.approx(2.0, abs=0.1)

    def test_accepts_numpy_array(self):
        s = SiPM.SiPMSensor(_clean())
        s.setSignalShape(np.linspace(0, 1, 20), 1.0)
        assert s.properties().nSignalPoints() == 20


# ── PDE behavior ───────────────────────────────────────────────────────────────

class TestPde:

    def test_no_pde_all_detected(self):
        s = SiPM.SiPMSensor(_clean())
        for i in range(10): s.addPhoton(10.0 + i * 3.0)
        s.runEvent()
        assert s.debug().nPhotoelectrons == 10

    def test_zero_pde_none_detected(self):
        p = _clean(); p.setPde(0.0)
        s = SiPM.SiPMSensor(p)
        for i in range(10): s.addPhoton(10.0 + i * 3.0)
        s.runEvent()
        assert s.debug().nPhotoelectrons == 0

    def test_full_pde_all_detected(self):
        p = _clean(); p.setPde(1.0)
        s = SiPM.SiPMSensor(p)
        for i in range(20): s.addPhoton(10.0 + i * 2.0)
        s.runEvent()
        assert s.debug().nPhotoelectrons == 20

    def test_spectrum_pde_zero_below_cutoff(self):
        p = SiPM.SiPMProperties()
        p.setDcrOff(); p.setXtOff(); p.setApOff()
        p.setPdeSpectrum([300.0, 400.0, 500.0], [0.0, 0.0, 1.0])
        s = SiPM.SiPMSensor(p)
        for _ in range(50):
            s.resetState()
            s.addPhotons([10.0] * 5, [350.0] * 5)
            s.runEvent()
            assert s.debug().nPhotoelectrons == 0

    def test_half_pde_statistical(self):
        p = _clean(); p.setPde(0.5)
        s = SiPM.SiPMSensor(p)
        total = 0
        for _ in range(500):
            s.resetState()
            for i in range(20): s.addPhoton(10.0 + i * 2.0)
            s.runEvent()
            total += s.debug().nPhotoelectrons
        assert abs(total / 500 - 10.0) < 2.0  # 0.5 * 20 = 10


# ── Noise flags ────────────────────────────────────────────────────────────────

class TestNoiseFlags:

    def test_dcr_off_zero_dark_counts(self):
        p = SiPM.SiPMProperties()
        p.setDcrOff(); p.setXtOff(); p.setApOff(); p.setPde(0.0)
        s = SiPM.SiPMSensor(p)
        for _ in range(200):
            s.resetState(); s.runEvent()
            assert s.debug().nDcr == 0

    def test_xt_off_zero_crosstalk(self):
        p = SiPM.SiPMProperties()
        p.setDcrOff(); p.setXtOff(); p.setApOff()
        p.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
        s = SiPM.SiPMSensor(p)
        for _ in range(200):
            s.resetState(); s.addPhoton(10.0); s.runEvent()
            assert s.debug().nXt == 0

    def test_ap_off_zero_afterpulses(self):
        p = SiPM.SiPMProperties()
        p.setDcrOff(); p.setXtOff(); p.setApOff()
        p.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
        s = SiPM.SiPMSensor(p)
        for _ in range(200):
            s.resetState(); s.addPhoton(10.0); s.runEvent()
            assert s.debug().nAp == 0

    def test_dcr_produces_dark_counts(self):
        p = SiPM.SiPMProperties()
        p.setXtOff(); p.setApOff(); p.setDcr(10e6); p.setPde(0.0)
        s = SiPM.SiPMSensor(p)
        total = sum(
            (s.resetState() or s.runEvent() or s.debug().nDcr)
            for _ in range(200)
        )
        assert total > 0


# ── Signal output ──────────────────────────────────────────────────────────────

class TestSignalOutput:

    def test_signal_type(self):
        s = SiPM.SiPMSensor()
        s.runEvent()
        assert isinstance(s.signal(), SiPM.SiPMAnalogSignal)

    def test_signal_size_matches_properties(self):
        p = _clean(signal_length=200.0, Sampling=0.5)
        s = SiPM.SiPMSensor(p)
        s.addPhoton(10.0); s.runEvent()
        assert s.signal().size() == s.properties().nSignalPoints()

    def test_waveform_dtype(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent()
        assert s.signal().waveform().dtype == np.float32

    def test_peak_positive_after_photon(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(50.0); s.runEvent()
        assert s.signal().peak(0, 500, 0) > 0

    def test_no_peak_without_photons(self):
        p = _clean(); p.setPde(0.0)
        s = SiPM.SiPMSensor(p)
        s.runEvent()
        assert s.signal().peak(0, 500, 0.5) < 0


# ── Hits list ──────────────────────────────────────────────────────────────────

class TestHitsList:

    def test_hits_is_list(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent()
        assert isinstance(s.hits(), list)

    def test_hits_are_sipmhit(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent()
        assert all(isinstance(h, SiPM.SiPMHit) for h in s.hits())

    def test_one_photon_one_hit(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent()
        assert len(s.hits()) == 1

    def test_hit_type_photoelectron(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0); s.runEvent()
        assert s.hits()[0].hitType() == SiPM.SiPMHit.HitType.kPhotoelectron

    def test_hits_empty_before_run(self):
        s = SiPM.SiPMSensor(_clean())
        s.addPhoton(10.0)
        assert len(s.hits()) == 0


# ── setProperty / setProperties ───────────────────────────────────────────────

class TestSetProperty:

    def test_set_property(self):
        s = SiPM.SiPMSensor()
        s.setProperty("Dcr", 300e3)
        assert s.properties().dcr() == pytest.approx(300e3)

    def test_set_properties(self):
        s = SiPM.SiPMSensor()
        p = SiPM.SiPMProperties(); p.setSignalLength(100.0)
        s.setProperties(p)
        assert s.properties().signalLength() == pytest.approx(100.0)
