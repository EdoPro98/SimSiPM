"""Tests for SiPMAnalogSignal Python bindings."""
import gc

import numpy as np
import pytest
import SiPM


def _make_signal(n_pe=1, t=50.0, snr=100, ccgv=0, signal_length=500.0):
    p = SiPM.SiPMProperties()
    p.setDcrOff(); p.setXtOff(); p.setApOff()
    p.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
    p.setSnr(snr); p.setCcgv(ccgv)
    p.setSignalLength(signal_length)
    s = SiPM.SiPMSensor(p)
    for i in range(n_pe):
        s.addPhoton(t + i * 5.0)
    s.runEvent()
    return s.signal(), p


# ── Basics ─────────────────────────────────────────────────────────────────────

class TestBasics:

    def test_signal_returns_analog_signal(self):
        sig, _ = _make_signal()
        assert isinstance(sig, SiPM.SiPMAnalogSignal)

    def test_size_matches_n_signal_points(self):
        sig, props = _make_signal()
        assert sig.size() == props.nSignalPoints()

    def test_len_equals_size(self):
        sig, _ = _make_signal()
        assert len(sig) == sig.size()

    def test_sampling_matches_properties(self):
        sig, props = _make_signal()
        assert sig.sampling() == pytest.approx(props.sampling())

    def test_repr_nonempty(self):
        sig, _ = _make_signal()
        s = repr(sig)
        assert isinstance(s, str) and len(s) > 0


# ── waveform() numpy output ────────────────────────────────────────────────────

class TestWaveform:

    def test_returns_ndarray(self):
        sig, _ = _make_signal()
        assert isinstance(sig.waveform(), np.ndarray)

    def test_dtype_float32(self):
        sig, _ = _make_signal()
        assert sig.waveform().dtype == np.float32

    def test_shape(self):
        sig, props = _make_signal()
        assert sig.waveform().shape == (props.nSignalPoints(),)

    def test_no_nan_or_inf(self):
        sig, _ = _make_signal()
        assert np.all(np.isfinite(sig.waveform()))

    def test_peak_positive_with_photon(self):
        sig, _ = _make_signal(n_pe=1)
        assert sig.waveform().max() > 0.0

    def test_peak_increases_with_pe_count(self):
        sig1, _ = _make_signal(n_pe=1)
        sig5, _ = _make_signal(n_pe=5)
        assert sig5.waveform().max() > sig1.waveform().max()

    def test_equals_asarray(self):
        sig, _ = _make_signal()
        np.testing.assert_array_equal(sig.waveform(), np.asarray(sig))


# ── Buffer protocol ────────────────────────────────────────────────────────────

class TestBufferProtocol:

    def test_asarray_works(self):
        sig, _ = _make_signal()
        assert isinstance(np.asarray(sig), np.ndarray)

    def test_asarray_dtype_float32(self):
        sig, _ = _make_signal()
        assert np.asarray(sig).dtype == np.float32

    def test_asarray_shape(self):
        sig, props = _make_signal()
        assert np.asarray(sig).shape == (props.nSignalPoints(),)

    def test_array_protocol(self):
        sig, _ = _make_signal()
        assert isinstance(np.array(sig), np.ndarray)

    def test_dtype_conversion(self):
        sig, _ = _make_signal()
        assert np.asarray(sig, dtype=np.float64).dtype == np.float64

    def test_memoryview_format(self):
        sig, _ = _make_signal()
        mv = memoryview(sig)
        assert mv.format == 'f'
        assert mv.itemsize == 4

    def test_buffer_is_writable(self):
        sig, _ = _make_signal()
        arr = np.asarray(sig)
        arr[0] = 999.0  # must not raise
        assert arr[0] == pytest.approx(999.0)


# ── Zero-copy / memory sharing ─────────────────────────────────────────────────

class TestZeroCopy:

    def test_waveform_and_asarray_share_memory(self):
        sig, _ = _make_signal()
        assert np.shares_memory(sig.waveform(), np.asarray(sig))

    def test_two_waveform_calls_share_memory(self):
        sig, _ = _make_signal()
        assert np.shares_memory(sig.waveform(), sig.waveform())

    def test_write_through_buffer(self):
        sig, _ = _make_signal()
        arr = np.asarray(sig)
        arr[0] = 12345.0
        assert np.asarray(sig)[0] == pytest.approx(12345.0)

    def test_write_through_waveform(self):
        sig, _ = _make_signal()
        arr = sig.waveform()
        arr[0] = 67890.0
        assert sig.waveform()[0] == pytest.approx(67890.0)

    def test_signal_kept_alive_by_waveform_array(self):
        def get_arr():
            sig, _ = _make_signal()
            return sig.waveform()

        arr = get_arr()
        gc.collect()
        assert len(arr) > 0 and not np.any(np.isnan(arr))

    def test_signal_kept_alive_by_asarray(self):
        def get_arr():
            sig, _ = _make_signal()
            return np.asarray(sig)

        arr = get_arr()
        gc.collect()
        assert len(arr) > 0 and not np.any(np.isnan(arr))


# ── Feature extraction ─────────────────────────────────────────────────────────

class TestFeatureExtraction:

    def test_peak_above_zero_with_photon(self):
        sig, props = _make_signal(n_pe=1, t=50.0)
        assert sig.peak(0, props.signalLength(), 0.0) > 0

    def test_peak_minus_one_above_threshold(self):
        sig, props = _make_signal(n_pe=1)
        assert sig.peak(0, props.signalLength(), 1e6) == pytest.approx(-1.0, abs=0.01)

    def test_integral_positive_with_photon(self):
        sig, props = _make_signal(n_pe=1, t=50.0)
        assert sig.integral(0, props.signalLength(), 0.0) > 0

    def test_integral_minus_one_above_threshold(self):
        sig, props = _make_signal(n_pe=0)
        assert sig.integral(0, props.signalLength(), 1e6) == pytest.approx(-1.0, abs=0.1)

    def test_tot_positive_with_photon(self):
        sig, props = _make_signal(n_pe=1, t=50.0)
        assert sig.tot(0, props.signalLength(), 0.0) > 0

    def test_toa_in_window(self):
        sig, props = _make_signal(n_pe=1, t=50.0)
        toa = sig.toa(0, props.signalLength(), 0.0)
        assert 0.0 <= toa < props.signalLength()

    def test_top_in_window(self):
        sig, props = _make_signal(n_pe=1, t=50.0)
        top = sig.top(0, props.signalLength(), -1e9)
        assert 0.0 <= top < props.signalLength()

    def test_c_contiguous(self):
        sig, _ = _make_signal()
        assert sig.waveform().flags['C_CONTIGUOUS']
