"""Tests for SiPMHit and SiPMDebugInfo Python bindings."""
import copy

import pytest
import SiPM


# ── Helpers ────────────────────────────────────────────────────────────────────

def _sensor_clean():
    p = SiPM.SiPMProperties()
    p.setDcrOff(); p.setXtOff(); p.setApOff()
    p.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
    return SiPM.SiPMSensor(p)


def _one_hit():
    s = _sensor_clean()
    s.addPhoton(10.0); s.runEvent()
    return s.hits()[0], s


# ── SiPMHit.HitType enum ───────────────────────────────────────────────────────

class TestHitTypeEnum:

    @pytest.mark.parametrize("name", [
        "kPhotoelectron", "kDarkCount", "kOpticalCrosstalk",
        "kDelayedOpticalCrosstalk", "kFastAfterPulse", "kSlowAfterPulse",
    ])
    def test_value_exists(self, name):
        assert hasattr(SiPM.SiPMHit.HitType, name)

    def test_all_values_distinct(self):
        vals = [
            SiPM.SiPMHit.HitType.kPhotoelectron,
            SiPM.SiPMHit.HitType.kDarkCount,
            SiPM.SiPMHit.HitType.kOpticalCrosstalk,
            SiPM.SiPMHit.HitType.kDelayedOpticalCrosstalk,
            SiPM.SiPMHit.HitType.kFastAfterPulse,
            SiPM.SiPMHit.HitType.kSlowAfterPulse,
        ]
        assert len(set(vals)) == len(vals)


# ── SiPMHit accessors ─────────────────────────────────────────────────────────

class TestHitAccessors:

    def test_time_is_float(self):
        h, s = _one_hit()
        assert isinstance(h.time(), float)

    def test_time_in_window(self):
        h, s = _one_hit()
        assert 0.0 <= h.time() <= s.properties().signalLength()

    def test_row_is_int(self):
        h, s = _one_hit()
        assert isinstance(h.row(), int)

    def test_col_is_int(self):
        h, s = _one_hit()
        assert isinstance(h.col(), int)

    def test_row_in_sensor(self):
        h, s = _one_hit()
        assert 0 <= h.row() < s.properties().nSideCells()

    def test_col_in_sensor(self):
        h, s = _one_hit()
        assert 0 <= h.col() < s.properties().nSideCells()

    def test_amplitude_positive_float(self):
        h, _ = _one_hit()
        assert isinstance(h.amplitude(), float)
        assert h.amplitude() > 0.0

    def test_hit_type_is_enum(self):
        h, _ = _one_hit()
        assert isinstance(h.hitType(), SiPM.SiPMHit.HitType)

    def test_photoelectron_type(self):
        h, _ = _one_hit()
        assert h.hitType() == SiPM.SiPMHit.HitType.kPhotoelectron

    def test_photoelectron_has_no_parent(self):
        h, _ = _one_hit()
        assert h.parentIdx() == -1

    def test_repr_nonempty(self):
        h, _ = _one_hit()
        assert isinstance(repr(h), str) and len(repr(h)) > 0


# ── deepcopy ──────────────────────────────────────────────────────────────────

class TestDeepCopy:

    def test_deepcopy_creates_new(self):
        h, _ = _one_hit()
        hc = copy.deepcopy(h)
        assert hc is not h

    def test_deepcopy_same_time(self):
        h, _ = _one_hit()
        hc = copy.deepcopy(h)
        assert hc.time() == h.time()

    def test_deepcopy_same_type(self):
        h, _ = _one_hit()
        hc = copy.deepcopy(h)
        assert hc.hitType() == h.hitType()

    def test_deepcopy_survives_sensor_reset(self):
        h, s = _one_hit()
        hc = copy.deepcopy(h)
        s.resetState()
        assert hc.time() >= 0.0  # accessible after sensor reset


# ── Correlated-noise hit properties ───────────────────────────────────────────

class TestXtHits:

    def test_xt_hits_have_parent_and_neighbor_cell(self):
        p = SiPM.SiPMProperties()
        p.setDcrOff(); p.setApOff(); p.setXt(0.5)
        p.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
        s = SiPM.SiPMSensor(p)
        found = False
        for _ in range(200):
            s.resetState(); s.addPhoton(10.0); s.runEvent()
            hits = s.hits()
            for h in hits:
                if h.hitType() == SiPM.SiPMHit.HitType.kOpticalCrosstalk:
                    assert 0 <= h.parentIdx() < len(hits)
                    parent = hits[h.parentIdx()]
                    assert not (h.row() == parent.row() and h.col() == parent.col())
                    found = True
        assert found


class TestApHits:

    def test_ap_hits_after_parent(self):
        p = SiPM.SiPMProperties()
        p.setDcrOff(); p.setXtOff(); p.setAp(0.3)
        p.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
        s = SiPM.SiPMSensor(p)
        found = False
        for _ in range(200):
            s.resetState(); s.addPhoton(10.0); s.runEvent()
            hits = s.hits()
            for h in hits:
                if h.hitType() in (SiPM.SiPMHit.HitType.kFastAfterPulse,
                                   SiPM.SiPMHit.HitType.kSlowAfterPulse):
                    assert 0 <= h.parentIdx() < len(hits)
                    parent = hits[h.parentIdx()]
                    assert h.time() > parent.time()
                    assert h.row() == parent.row()
                    assert h.col() == parent.col()
                    found = True
        assert found


# ── SiPMDebugInfo ──────────────────────────────────────────────────────────────

class TestDebugInfo:

    def test_returns_debug_info(self):
        assert isinstance(SiPM.SiPMSensor().debug(), SiPM.SiPMDebugInfo)

    @pytest.mark.parametrize("field", [
        "nPhotons", "nPhotoelectrons", "nDcr", "nXt", "nDXt", "nAp"
    ])
    def test_field_accessible(self, field):
        dbg = SiPM.SiPMSensor().debug()
        assert hasattr(dbg, field)

    def test_fields_are_ints(self):
        s = _sensor_clean(); s.addPhoton(10.0); s.runEvent()
        dbg = s.debug()
        for f in ("nPhotons", "nPhotoelectrons", "nDcr", "nXt", "nDXt", "nAp"):
            assert isinstance(getattr(dbg, f), int), f"field {f} not int"

    def test_n_photons_correct(self):
        s = _sensor_clean()
        s.addPhoton(10.0); s.addPhoton(20.0); s.runEvent()
        assert s.debug().nPhotons == 2

    def test_n_pe_equals_photons_with_no_pde(self):
        s = _sensor_clean()
        s.addPhotons([10.0, 20.0, 30.0]); s.runEvent()
        assert s.debug().nPhotoelectrons == 3

    def test_noise_zero_when_off(self):
        s = _sensor_clean(); s.addPhoton(10.0); s.runEvent()
        dbg = s.debug()
        assert dbg.nDcr == 0 and dbg.nXt == 0 and dbg.nAp == 0

    def test_all_zero_after_reset(self):
        s = _sensor_clean(); s.addPhoton(10.0); s.runEvent(); s.resetState()
        dbg = s.debug()
        for f in ("nPhotons", "nPhotoelectrons", "nDcr", "nXt", "nAp"):
            assert getattr(dbg, f) == 0, f"field {f} not 0 after reset"

    def test_repr_nonempty(self):
        dbg = SiPM.SiPMSensor().debug()
        assert isinstance(repr(dbg), str) and len(repr(dbg)) > 0
