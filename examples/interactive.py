#!/usr/bin/env python3
"""
Interactive SiPM simulator -- real-time histograms, scatter, waveform, hit matrix.

Requirements: PyQt5, pyqtgraph, numpy, SiPM (SimSiPM installed)
Run:  python examples/interactive.py
"""

import sys, time, threading, queue
import numpy as np
import SiPM

from PyQt5 import QtWidgets, QtCore, QtGui
import pyqtgraph as pg

pg.setConfigOption("background", "#1e1e2e")
pg.setConfigOption("foreground", "#cdd6f4")

PREBATCH    = 400
MAX_HISTORY = 200_000
QUEUE_MAX   = 12

HIT_PALETTE = {
    0: ("#89b4fa", "Photoelectron"),
    1: ("#a6adc8", "Dark Count"),
    2: ("#fab387", "Crosstalk"),
    3: ("#f9e2af", "Delayed XT"),
    4: ("#f38ba8", "AP fast"),
    5: ("#cba6f7", "AP slow"),
}


# --------------------------------------------------------------------------- #
#  Ring buffer                                                                 #
# --------------------------------------------------------------------------- #

class RingBuffer:
    def __init__(self, cap, dtype=np.float32):
        self._buf  = np.empty(cap, dtype=dtype)
        self._cap  = cap
        self._ptr  = 0
        self._size = 0

    def extend(self, arr):
        n = len(arr)
        if n == 0:
            return
        if n >= self._cap:
            self._buf[:] = arr[-self._cap:]
            self._ptr = 0
            self._size = self._cap
            return
        end = self._ptr + n
        if end <= self._cap:
            self._buf[self._ptr:end] = arr
        else:
            split = self._cap - self._ptr
            self._buf[self._ptr:] = arr[:split]
            self._buf[:n - split]  = arr[split:]
        self._ptr  = end % self._cap
        self._size = min(self._size + n, self._cap)

    @property
    def data(self):
        return self._buf[:self._size]

    def clear(self):
        self._ptr = 0
        self._size = 0

    def __len__(self):
        return self._size


# --------------------------------------------------------------------------- #
#  Config snapshot                                                             #
# --------------------------------------------------------------------------- #

class Cfg:
    __slots__ = (
        "size", "pitch", "ccgv", "snr",
        "dcr", "dcr_on", "xt", "xt_on", "ap", "ap_on",
        "rise", "fall_fast", "fall_slow", "slow_frac", "slow_on",
        "signal_length", "sampling",
        "hit_dist",
        "n_photons", "gen_start", "profile", "gen_tau", "gen_sigma",
        "gate_start", "gate_len", "threshold",
    )

    def __init__(self):
        self.size = 1.0;        self.pitch = 25.0
        self.ccgv = 0.05;       self.snr   = 30.0
        self.dcr  = 200e3;      self.dcr_on = True
        self.xt   = 0.05;       self.xt_on  = True
        self.ap   = 0.03;       self.ap_on  = True
        self.rise = 1.0;        self.fall_fast = 50.0
        self.fall_slow = 100.0; self.slow_frac = 0.2; self.slow_on = False
        self.signal_length = 500.0; self.sampling = 1.0
        self.hit_dist = "uniform"
        self.n_photons = 10
        self.gen_start = 100.0; self.profile = "pulse"
        self.gen_tau = 50.0;    self.gen_sigma = 10.0
        self.gate_start = 50.0; self.gate_len = 250.0; self.threshold = 0.5

    def copy(self):
        c = Cfg()
        for k in self.__slots__:
            setattr(c, k, getattr(self, k))
        return c


# --------------------------------------------------------------------------- #
#  Simulation worker thread                                                    #
# --------------------------------------------------------------------------- #

_DIST_MAP = {
    "uniform":  "kUniform",
    "circle":   "kCircle",
    "gaussian": "kGaussian",
}


class SimWorker(threading.Thread):
    def __init__(self, out_q):
        super().__init__(daemon=True, name="SimWorker")
        self.out_q    = out_q
        self._stop    = threading.Event()
        self._lock    = threading.Lock()
        self._cfg     = Cfg()
        self._rebuild = threading.Event()
        self._rebuild.set()
        self._rng     = SiPM.SiPMRandom()
        self._sensor  = None
        self._n_side  = 0

    def update_config(self, cfg, rebuild=True):
        with self._lock:
            self._cfg = cfg.copy()
        if rebuild:
            self._rebuild.set()

    def stop(self):
        self._stop.set()

    def _make_sensor(self, c):
        prop = SiPM.SiPMProperties()
        prop.setSize(c.size);           prop.setPitch(c.pitch)
        prop.setCcgv(c.ccgv);           prop.setSnr(c.snr)
        prop.setDcr(c.dcr);             prop.setXt(c.xt);   prop.setAp(c.ap)
        if c.dcr_on: prop.setDcrOn()
        else:        prop.setDcrOff()
        if c.xt_on:  prop.setXtOn()
        else:        prop.setXtOff()
        if c.ap_on:  prop.setApOn()
        else:        prop.setApOff()
        prop.setRiseTime(c.rise)
        prop.setFallTimeFast(c.fall_fast)
        prop.setFallTimeSlow(c.fall_slow)
        prop.setSlowComponentFraction(c.slow_frac)
        if c.slow_on: prop.setSlowComponentOn()
        else:         prop.setSlowComponentOff()
        prop.setSignalLength(c.signal_length)
        prop.setSampling(c.sampling)
        dist_attr = getattr(SiPM.SiPMProperties.HitDistribution, _DIST_MAP[c.hit_dist])
        prop.setHitDistribution(dist_attr)
        sensor = SiPM.SiPMSensor(prop)
        self._n_side = sensor.properties().nSideCells()
        return sensor

    def _gen_times(self, c, n_events):
        n = max(0, self._rng.randPoisson(c.n_photons))
        if n == 0:
            return None, 0
        if c.profile == "pulse":
            return np.full(n, c.gen_start, dtype=np.float64), n
        elif c.profile == "exponential":
            all_t = self._rng.randExponential(c.gen_tau, n_events * n)
            np.add(all_t, c.gen_start, out=all_t)
            return all_t.reshape(n_events, n), n
        else:
            all_t = self._rng.randGaussian(c.gen_start, c.gen_sigma, n_events * n)
            return np.asarray(all_t, dtype=np.float32).reshape(n_events, n), n

    def run(self):
        sensor = None
        while not self._stop.is_set():
            if self._rebuild.is_set():
                self._rebuild.clear()
                with self._lock:
                    c = self._cfg.copy()
                sensor = self._make_sensor(c)
                self.out_q.put({"reset": True, "n_side": self._n_side})
                continue

            with self._lock:
                c = self._cfg.copy()

            gs, gl, th      = c.gate_start, c.gate_len, c.threshold
            times_arr, n_ph = self._gen_times(c, PREBATCH)
            pulse_mode      = (c.profile == "pulse")

            peaks     = np.empty(PREBATCH, np.float32)
            integrals = np.empty(PREBATCH, np.float32)
            toas      = np.empty(PREBATCH, np.float32)
            tots      = np.empty(PREBATCH, np.float32)
            n_pes     = np.empty(PREBATCH, np.int32)

            i = 0
            for i in range(PREBATCH):
                if self._rebuild.is_set():
                    break
                sensor.resetState()
                if n_ph > 0:
                    row = times_arr if pulse_mode else times_arr[i]
                    sensor.addPhotons(row)
                sensor.runEvent()
                sig          = sensor.signal()
                peaks[i]     = sig.peak(gs, gl, th)
                integrals[i] = sig.integral(gs, gl, th)
                toas[i]      = sig.toa(gs, gl, th)
                tots[i]      = sig.tot(gs, gl, th)
                n_pes[i]     = sensor.debug().nPhotoelectrons

            last_wf   = np.asarray(sensor.signal().waveform(), dtype=np.float32).copy()
            last_samp = sensor.properties().sampling()
            last_hits = [(h.row(), h.col(), int(h.hitType())) for h in sensor.hits()]

            n = i + 1
            msg = {
                "reset":     False,
                "peaks":     peaks[:n].copy(),
                "integrals": integrals[:n].copy(),
                "toas":      toas[:n].copy(),
                "tots":      tots[:n].copy(),
                "n_pes":     n_pes[:n].copy(),
                "last_wf":   last_wf,
                "last_samp": last_samp,
                "last_hits": last_hits,
                "n_side":    self._n_side,
            }
            try:
                self.out_q.put(msg, timeout=0.05)
            except queue.Full:
                pass  # UI is behind; discard batch and continue


# --------------------------------------------------------------------------- #
#  UI helpers                                                                  #
# --------------------------------------------------------------------------- #

STYLE_LABEL = "color:#cdd6f4;font-size:11px;font-weight:600;"
STYLE_GROUP = (
    "QGroupBox{color:#89b4fa;font-size:12px;font-weight:bold;"
    "border:1px solid #45475a;border-radius:6px;margin-top:8px;"
    "padding-top:4px;}"
    "QGroupBox::title{subcontrol-origin:margin;left:8px;padding:0 4px;}"
)
STYLE_SPIN  = (
    "QDoubleSpinBox,QSpinBox{background:#313244;color:#cdd6f4;"
    "border:1px solid #585b70;border-radius:3px;padding:1px 4px;"
    "font-size:11px;}"
)
STYLE_SLIDE = (
    "QSlider::groove:horizontal{height:4px;background:#45475a;border-radius:2px;}"
    "QSlider::handle:horizontal{background:#89b4fa;width:12px;height:12px;"
    "margin:-4px 0;border-radius:6px;}"
    "QSlider::sub-page:horizontal{background:#89b4fa;border-radius:2px;}"
)
STYLE_BTN   = (
    "QPushButton{background:#313244;color:#cdd6f4;border:1px solid #585b70;"
    "border-radius:4px;padding:3px 10px;font-size:11px;}"
    "QPushButton:hover{background:#45475a;}"
    "QPushButton:checked{background:#89b4fa;color:#1e1e2e;}"
)
STYLE_COMBO = (
    "QComboBox{background:#313244;color:#cdd6f4;border:1px solid #585b70;"
    "border-radius:3px;padding:2px 6px;font-size:11px;}"
    "QComboBox QAbstractItemView{background:#313244;color:#cdd6f4;}"
)
STYLE_CHECK = "color:#cdd6f4;font-size:11px;"


def _lbl(text):
    w = QtWidgets.QLabel(text)
    w.setStyleSheet(STYLE_LABEL)
    return w


def _hb(h, a=180):
    c = QtGui.QColor(h); c.setAlpha(a)
    return pg.mkBrush(c)


def _hp(h, w=1.0):
    return pg.mkPen(h, width=w)


def mkgroup(title, lay):
    gb = QtWidgets.QGroupBox(title)
    gb.setStyleSheet(STYLE_GROUP)
    gb.setLayout(lay)
    return gb


def mkbtn(label, checkable=False):
    b = QtWidgets.QPushButton(label)
    b.setStyleSheet(STYLE_BTN)
    b.setCheckable(checkable)
    return b


def mkcombo(items):
    c = QtWidgets.QComboBox()
    c.addItems(items)
    c.setStyleSheet(STYLE_COMBO)
    return c


def _dspin(lo, hi, val, dec, suffix=""):
    s = QtWidgets.QDoubleSpinBox()
    s.setRange(lo, hi); s.setDecimals(dec); s.setValue(val)
    s.setSuffix(suffix); s.setStyleSheet(STYLE_SPIN); s.setFixedWidth(90)
    return s


def _ispin(lo, hi, val):
    s = QtWidgets.QSpinBox()
    s.setRange(lo, hi); s.setValue(val)
    s.setStyleSheet(STYLE_SPIN); s.setFixedWidth(72)
    return s


class FloatParam(QtWidgets.QWidget):
    valueChanged = QtCore.pyqtSignal(float)

    def __init__(self, label, lo, hi, val, dec=2, steps=200, suffix="", parent=None):
        super().__init__(parent)
        self._lo = lo; self._hi = hi; self._steps = steps; self._sync = False
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0); lay.setSpacing(2)
        lay.addWidget(_lbl(label))
        row = QtWidgets.QHBoxLayout(); row.setSpacing(4)
        self.slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.slider.setRange(0, steps)
        self.slider.setStyleSheet(STYLE_SLIDE)
        self.slider.setFixedHeight(20)
        self.spin = QtWidgets.QDoubleSpinBox()
        self.spin.setRange(lo, hi); self.spin.setDecimals(dec)
        self.spin.setSuffix(suffix); self.spin.setFixedWidth(90)
        self.spin.setStyleSheet(STYLE_SPIN)
        row.addWidget(self.slider, 1); row.addWidget(self.spin)
        lay.addLayout(row)
        self.setValue(val)
        self.slider.valueChanged.connect(self._from_slider)
        self.spin.valueChanged.connect(self._from_spin)

    def _to_int(self, v):
        return round((v - self._lo) / (self._hi - self._lo) * self._steps)

    def _from_int(self, i):
        return self._lo + i / self._steps * (self._hi - self._lo)

    def _from_slider(self, i):
        if self._sync: return
        self._sync = True; self.spin.setValue(self._from_int(i)); self._sync = False
        self.valueChanged.emit(self.spin.value())

    def _from_spin(self, v):
        if self._sync: return
        self._sync = True; self.slider.setValue(self._to_int(v)); self._sync = False
        self.valueChanged.emit(v)

    def value(self): return self.spin.value()

    def setValue(self, v):
        self._sync = True
        self.spin.setValue(v); self.slider.setValue(self._to_int(v))
        self._sync = False


class NoiseRow(QtWidgets.QWidget):
    enableChanged = QtCore.pyqtSignal(bool)
    valueChanged  = QtCore.pyqtSignal(float)

    def __init__(self, label, lo, hi, val, dec, steps, suffix, enabled, parent=None):
        super().__init__(parent)
        lay = QtWidgets.QHBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0); lay.setSpacing(4)
        self.cb = QtWidgets.QCheckBox()
        self.cb.setChecked(enabled); self.cb.setStyleSheet(STYLE_CHECK)
        self.cb.setFixedWidth(18)
        self.param = FloatParam(label, lo, hi, val, dec, steps, suffix)
        lay.addWidget(self.cb); lay.addWidget(self.param, 1)
        self.cb.toggled.connect(self.enableChanged)
        self.param.valueChanged.connect(self.valueChanged)

    def value(self):   return self.param.value()
    def enabled(self): return self.cb.isChecked()


# --------------------------------------------------------------------------- #
#  Main window                                                                 #
# --------------------------------------------------------------------------- #

BIN_OPTIONS = [32, 64, 128, 256, 512]


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SimSiPM -- Interactive")
        self.resize(1480, 860)
        self.setStyleSheet("background:#1e1e2e;color:#cdd6f4;")

        self._cfg    = Cfg()
        self._q      = queue.Queue(maxsize=QUEUE_MAX)
        self._worker = SimWorker(self._q)

        self._rb_peak = RingBuffer(MAX_HISTORY)
        self._rb_int  = RingBuffer(MAX_HISTORY)
        self._rb_toa  = RingBuffer(MAX_HISTORY)
        self._rb_tot  = RingBuffer(MAX_HISTORY)
        self._rb_sc_x = RingBuffer(MAX_HISTORY, dtype=np.float32)
        self._rb_sc_y = RingBuffer(MAX_HISTORY, dtype=np.float32)

        self._last_wf   = None
        self._last_samp = 1.0
        self._last_hits = []
        self._n_side    = 0

        self._ev_count = 0
        self._rate_ev  = 0
        self._rate_t0  = time.monotonic()

        self._n_bins    = 128
        self._log_scale = [False, False, False, False]
        self._sc_y      = "integral"

        self._build_ui()
        self._worker.start()

        self._timer = QtCore.QTimer(self)
        self._timer.timeout.connect(self._on_frame)
        self._timer.start(35)

        self._rate_timer = QtCore.QTimer(self)
        self._rate_timer.timeout.connect(self._on_rate)
        self._rate_timer.start(500)

        # Debounce timers — slider drags only commit after the user pauses
        self._rebuild_timer = QtCore.QTimer(self)
        self._rebuild_timer.setSingleShot(True)
        self._rebuild_timer.setInterval(150)
        self._rebuild_timer.timeout.connect(self._do_rebuild)

        self._light_timer = QtCore.QTimer(self)
        self._light_timer.setSingleShot(True)
        self._light_timer.setInterval(80)
        self._light_timer.timeout.connect(self._do_light)

        self._gate_timer = QtCore.QTimer(self)
        self._gate_timer.setSingleShot(True)
        self._gate_timer.setInterval(80)
        self._gate_timer.timeout.connect(self._do_gate)

    # ------------------------------------------------------------------ build

    def _build_ui(self):
        c = QtWidgets.QWidget(); self.setCentralWidget(c)
        root = QtWidgets.QHBoxLayout(c)
        root.setContentsMargins(4, 4, 4, 4); root.setSpacing(4)
        sp = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        sp.setHandleWidth(4)
        sp.setStyleSheet("QSplitter::handle{background:#45475a;}")
        root.addWidget(sp)
        sp.addWidget(self._build_left())
        sp.addWidget(self._build_right())
        sp.setStretchFactor(0, 0); sp.setStretchFactor(1, 1)
        sp.setSizes([290, 1190])

    def _build_left(self):
        scroll = QtWidgets.QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        scroll.setFixedWidth(288)
        scroll.setStyleSheet(
            "QScrollArea{border:none;background:#181825;}"
            "QScrollBar:vertical{background:#181825;width:8px;}"
            "QScrollBar::handle:vertical{background:#45475a;border-radius:4px;}"
        )
        inner = QtWidgets.QWidget(); inner.setStyleSheet("background:#181825;")
        vlay  = QtWidgets.QVBoxLayout(inner)
        vlay.setContentsMargins(4, 4, 4, 4); vlay.setSpacing(6)
        for grp in (self._grp_sipm(), self._grp_noise(), self._grp_signal(),
                    self._grp_sigcfg(), self._grp_hitdist(), self._grp_light(),
                    self._grp_stats()):
            vlay.addWidget(grp)
        vlay.addStretch()
        scroll.setWidget(inner)
        return scroll

    def _grp_sipm(self):
        lay = QtWidgets.QVBoxLayout(); lay.setSpacing(5)
        self._p_size  = FloatParam("Size (mm)",  0.5, 6,   1,   1, 110, " mm")
        self._p_pitch = FloatParam("Pitch (um)", 10,  100, 25,  0, 180, " um")
        self._p_ccgv  = FloatParam("CCGV (%)",   0,   20,  5,   1, 200, " %")
        self._p_snr   = FloatParam("SNR (dB)",   10,  60,  30,  1, 200, " dB")
        for w in (self._p_size, self._p_pitch, self._p_ccgv, self._p_snr):
            lay.addWidget(w); w.valueChanged.connect(self._on_rebuild)
        return mkgroup("SiPM Properties", lay)

    def _grp_noise(self):
        lay = QtWidgets.QVBoxLayout(); lay.setSpacing(5)
        self._nr_dcr = NoiseRow("DCR (kHz)", 0, 5000, 200, 0, 200, " kHz", True)
        self._nr_xt  = NoiseRow("XT (%)",    0, 50,   5,   1, 200, " %",   True)
        self._nr_ap  = NoiseRow("AP (%)",    0, 30,   3,   1, 200, " %",   True)
        for nr in (self._nr_dcr, self._nr_xt, self._nr_ap):
            lay.addWidget(nr)
            nr.valueChanged.connect(self._on_rebuild)
            nr.enableChanged.connect(self._on_rebuild)
        return mkgroup("Noise", lay)

    def _grp_signal(self):
        lay = QtWidgets.QVBoxLayout(); lay.setSpacing(5)
        self._p_rise      = FloatParam("Rise (ns)",      0.1, 20,   1,   1, 200, " ns")
        self._p_fall_fast = FloatParam("Fall fast (ns)", 1,   300,  50,  0, 300, " ns")
        self._p_fall_slow = FloatParam("Fall slow (ns)", 10,  1000, 100, 0, 300, " ns")
        self._p_slow_frac = FloatParam("Slow frac (%)",  0,   100,  20,  0, 200, " %")
        self._cb_slow     = QtWidgets.QCheckBox("Slow component ON")
        self._cb_slow.setStyleSheet(STYLE_CHECK)
        for w in (self._p_rise, self._p_fall_fast, self._p_fall_slow, self._p_slow_frac):
            lay.addWidget(w); w.valueChanged.connect(self._on_rebuild)
        lay.addWidget(self._cb_slow)
        self._cb_slow.toggled.connect(self._on_rebuild)
        return mkgroup("Signal Shape", lay)

    def _grp_sigcfg(self):
        lay = QtWidgets.QVBoxLayout(); lay.setSpacing(5)
        self._p_siglen = FloatParam("Length (ns)",   50,  2000, 500, 0, 200, " ns")
        self._p_sampl  = FloatParam("Sampling (ns)", 0.1, 10,   1,   2, 200, " ns")
        for w in (self._p_siglen, self._p_sampl):
            lay.addWidget(w); w.valueChanged.connect(self._on_rebuild)
        return mkgroup("Signal Config", lay)

    def _grp_hitdist(self):
        lay = QtWidgets.QVBoxLayout(); lay.setSpacing(4)
        self._rg_dist = QtWidgets.QButtonGroup(self)
        row = QtWidgets.QHBoxLayout()
        for i, lbl in enumerate(["Uniform", "Circle", "Gaussian"]):
            rb = QtWidgets.QRadioButton(lbl); rb.setStyleSheet(STYLE_CHECK)
            if i == 0: rb.setChecked(True)
            self._rg_dist.addButton(rb, i); row.addWidget(rb)
        lay.addLayout(row)
        self._rg_dist.buttonToggled.connect(self._on_rebuild)
        return mkgroup("Hit Distribution", lay)

    def _grp_light(self):
        lay = QtWidgets.QVBoxLayout(); lay.setSpacing(5)
        r0  = QtWidgets.QHBoxLayout()
        r0.addWidget(_lbl("N photons"))
        self._spin_n = _ispin(0, 10000, 10)
        r0.addStretch(); r0.addWidget(self._spin_n)
        lay.addLayout(r0)
        self._spin_n.valueChanged.connect(self._on_light)

        self._p_gen_start = FloatParam("Start (ns)", 0, 490, 100, 0, 490, " ns")
        self._p_gen_start.valueChanged.connect(self._on_light)
        lay.addWidget(self._p_gen_start)

        r1 = QtWidgets.QHBoxLayout()
        r1.addWidget(_lbl("Profile"))
        self._combo_prof = mkcombo(["pulse", "exponential", "gaussian"])
        r1.addStretch(); r1.addWidget(self._combo_prof)
        lay.addLayout(r1)

        self._p_tau   = FloatParam("Tau (ns)",   1, 500, 50, 0, 200, " ns")
        self._p_sigma = FloatParam("Sigma (ns)", 1, 200, 10, 0, 200, " ns")
        self._p_tau.setVisible(False); self._p_sigma.setVisible(False)
        lay.addWidget(self._p_tau); lay.addWidget(self._p_sigma)
        self._combo_prof.currentTextChanged.connect(self._on_profile)
        self._p_tau.valueChanged.connect(self._on_light)
        self._p_sigma.valueChanged.connect(self._on_light)
        return mkgroup("Light Generation", lay)

    def _grp_stats(self):
        lay = QtWidgets.QVBoxLayout(); lay.setSpacing(3)
        S = "color:#a6e3a1;font-size:11px;font-family:monospace;"
        self._lbl_ev   = QtWidgets.QLabel("Events: 0")
        self._lbl_rate = QtWidgets.QLabel("Rate: -- ev/s")
        for l in (self._lbl_ev, self._lbl_rate):
            l.setStyleSheet(S); lay.addWidget(l)
        return mkgroup("Statistics", lay)

    # ------------------------------------------------------------------ right

    def _ax(self, p):
        for side in ("bottom", "left"):
            ax = p.getAxis(side)
            ax.setPen(_hp("#585b70")); ax.setTextPen(_hp("#a6adc8"))
        p.setMenuEnabled(False)
        if hasattr(p, "title"): p.title.setColor("#cdd6f4")

    def _build_right(self):
        w = QtWidgets.QWidget(); w.setStyleSheet("background:#1e1e2e;")
        vlay = QtWidgets.QVBoxLayout(w)
        vlay.setContentsMargins(2, 2, 2, 2); vlay.setSpacing(4)
        self._glw = pg.GraphicsLayoutWidget()
        self._glw.setBackground("#181825")
        vlay.addWidget(self._glw, 5)
        self._build_plots()
        vlay.addWidget(self._build_ctrl_bar())
        self._bot_split = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        self._bot_split.setHandleWidth(3)
        self._bot_split.setStyleSheet("QSplitter::handle{background:#45475a;}")
        self._bot_split.setVisible(False)
        vlay.addWidget(self._bot_split, 2)
        self._sig_widget = self._build_sig_view()
        self._mat_widget = self._build_mat_view()
        self._bot_split.addWidget(self._sig_widget)
        self._bot_split.addWidget(self._mat_widget)
        self._sig_widget.setVisible(False)
        self._mat_widget.setVisible(False)
        return w

    def _build_plots(self):
        specs = [
            ("Peak",     "#f38ba8", 0, 0),
            ("Integral", "#a6e3a1", 0, 1),
            ("ToA (ns)", "#fab387", 1, 0),
            ("ToT (ns)", "#89b4fa", 1, 1),
        ]
        self._hist_plots  = []
        self._hist_curves = []
        for title, color, row, col in specs:
            p = self._glw.addPlot(row=row, col=col, title=title)
            self._ax(p)
            p.setLabel("left",   "counts", color="#a6adc8", size="9pt")
            p.setLabel("bottom", title,    color="#a6adc8", size="9pt")
            p.setDownsampling(auto=True, mode='peak')
            p.setClipToView(True)
            curve = pg.PlotCurveItem(
                x=np.zeros(2), y=np.zeros(1),
                stepMode="center", fillLevel=0,
                brush=_hb(color, 160), pen=_hp(color, 1.2),
            )
            p.addItem(curve)
            self._hist_plots.append(p)
            self._hist_curves.append(curve)

        p_sc = self._glw.addPlot(row=0, col=2, rowspan=2, title="N_pe vs measurement")
        self._ax(p_sc)
        p_sc.setLabel("left",   "measurement",      color="#a6adc8", size="9pt")
        p_sc.setLabel("bottom", "N photoelectrons",  color="#a6adc8", size="9pt")
        self._scat_item = pg.ScatterPlotItem(
            size=3, pen=pg.mkPen(None), brush=_hb("#cba6f7", 110)
        )
        p_sc.addItem(self._scat_item)
        self._scat_plot = p_sc

        ci = self._glw.ci
        for col in range(3):
            ci.layout.setColumnStretchFactor(col, 1)

    def _build_ctrl_bar(self):
        outer = QtWidgets.QWidget()
        outer.setStyleSheet("background:#181825;border-radius:4px;")
        lay = QtWidgets.QHBoxLayout(outer)
        lay.setContentsMargins(6, 4, 6, 4); lay.setSpacing(10)

        gate_gb  = QtWidgets.QGroupBox("Analysis Gate"); gate_gb.setStyleSheet(STYLE_GROUP)
        gate_lay = QtWidgets.QHBoxLayout(gate_gb); gate_lay.setSpacing(6)
        self._spin_gs  = _dspin(0,   490, 50,  0, " ns")
        self._spin_gl  = _dspin(1,   500, 250, 0, " ns")
        self._spin_thr = _dspin(0,   10,  0.5, 2)
        for lbl, w in (("Start", self._spin_gs), ("Gate", self._spin_gl), ("Thr", self._spin_thr)):
            gate_lay.addWidget(_lbl(lbl)); gate_lay.addWidget(w)
        self._spin_gs.valueChanged.connect(self._on_gate)
        self._spin_gl.valueChanged.connect(self._on_gate)
        self._spin_thr.valueChanged.connect(self._on_gate)
        lay.addWidget(gate_gb)

        hist_gb  = QtWidgets.QGroupBox("Histograms"); hist_gb.setStyleSheet(STYLE_GROUP)
        hist_lay = QtWidgets.QHBoxLayout(hist_gb); hist_lay.setSpacing(6)
        hist_lay.addWidget(_lbl("Bins"))
        self._combo_bins = mkcombo([str(b) for b in BIN_OPTIONS])
        self._combo_bins.setCurrentText("128"); self._combo_bins.setFixedWidth(70)
        self._combo_bins.currentTextChanged.connect(self._on_bins)
        hist_lay.addWidget(self._combo_bins)
        hist_lay.addWidget(_lbl("Log:"))
        self._log_checks = []
        for i, lbl in enumerate(["Peak", "Int", "ToA", "ToT"]):
            cb = QtWidgets.QCheckBox(lbl); cb.setStyleSheet(STYLE_CHECK)
            cb.toggled.connect(lambda chk, idx=i: self._on_log(idx, chk))
            self._log_checks.append(cb); hist_lay.addWidget(cb)
        lay.addWidget(hist_gb)

        sc_gb  = QtWidgets.QGroupBox("Scatter Y"); sc_gb.setStyleSheet(STYLE_GROUP)
        sc_lay = QtWidgets.QHBoxLayout(sc_gb); sc_lay.setSpacing(4)
        self._rb_sc_int  = QtWidgets.QRadioButton("Integral"); self._rb_sc_int.setStyleSheet(STYLE_CHECK)
        self._rb_sc_peak = QtWidgets.QRadioButton("Peak");     self._rb_sc_peak.setStyleSheet(STYLE_CHECK)
        self._rb_sc_int.setChecked(True)
        self._rb_sc_int.toggled.connect(self._on_scatter_y)
        self._rb_sc_peak.toggled.connect(self._on_scatter_y)
        sc_lay.addWidget(self._rb_sc_int); sc_lay.addWidget(self._rb_sc_peak)
        lay.addWidget(sc_gb)

        self._btn_reset  = mkbtn("Reset")
        self._btn_sig    = mkbtn("Signal", checkable=True)
        self._btn_matrix = mkbtn("Matrix", checkable=True)
        self._btn_reset.clicked.connect(self._do_reset)
        self._btn_sig.toggled.connect(self._toggle_bot)
        self._btn_matrix.toggled.connect(self._toggle_bot)
        for b in (self._btn_reset, self._btn_sig, self._btn_matrix):
            lay.addWidget(b)
        lay.addStretch()
        return outer

    def _build_sig_view(self):
        w = pg.GraphicsLayoutWidget(); w.setBackground("#181825")
        p = w.addPlot(title="Last signal + integration gate")
        self._ax(p)
        p.setLabel("bottom", "time (ns)",      color="#a6adc8", size="9pt")
        p.setLabel("left",   "amplitude (PE)", color="#a6adc8", size="9pt")
        self._sig_curve   = p.plot(pen=_hp("#cba6f7", 1.5))
        self._gate_region = pg.LinearRegionItem(
            values=[50, 300],
            brush=pg.mkBrush(137, 180, 250, 35),
            pen=_hp("#89b4fa", 1), movable=False,
        )
        self._thr_line = pg.InfiniteLine(pos=0.5, angle=0, pen=_hp("#f38ba8", 1))
        p.addItem(self._gate_region); p.addItem(self._thr_line)
        self._sig_plot = p
        return w

    def _build_mat_view(self):
        w = pg.GraphicsLayoutWidget(); w.setBackground("#181825")
        p = w.addPlot(title="SiPM hit matrix (last event)")
        self._ax(p)
        p.setLabel("bottom", "col", color="#a6adc8", size="9pt")
        p.setLabel("left",   "row", color="#a6adc8", size="9pt")
        p.setAspectLocked(True)
        self._mat_bg = pg.ScatterPlotItem(size=4, pen=pg.mkPen(None), brush=_hb("#45475a", 60))
        p.addItem(self._mat_bg)
        self._mat_hits = {}
        legend = p.addLegend(offset=(10, 10))
        for htype, (color, label) in HIT_PALETTE.items():
            item = pg.ScatterPlotItem(size=7, pen=pg.mkPen(color, width=0.5), brush=_hb(color, 220))
            p.addItem(item); legend.addItem(item, label)
            self._mat_hits[htype] = item
        self._mat_plot = p
        return w

    # ------------------------------------------------------------------ slots

    def _collect_cfg(self):
        c = self._cfg
        c.size   = self._p_size.value()
        c.pitch  = self._p_pitch.value()
        c.ccgv   = self._p_ccgv.value() / 100.0
        c.snr    = self._p_snr.value()
        c.dcr    = self._nr_dcr.value() * 1e3;  c.dcr_on = self._nr_dcr.enabled()
        c.xt     = self._nr_xt.value() / 100.0;  c.xt_on  = self._nr_xt.enabled()
        c.ap     = self._nr_ap.value() / 100.0;  c.ap_on  = self._nr_ap.enabled()
        c.rise       = self._p_rise.value()
        c.fall_fast  = self._p_fall_fast.value()
        c.fall_slow  = self._p_fall_slow.value()
        c.slow_frac  = self._p_slow_frac.value() / 100.0
        c.slow_on    = self._cb_slow.isChecked()
        c.signal_length = self._p_siglen.value()
        c.sampling      = self._p_sampl.value()
        c.hit_dist = ["uniform", "circle", "gaussian"][self._rg_dist.checkedId()]
        c.n_photons  = self._spin_n.value()
        c.gen_start  = self._p_gen_start.value()
        c.profile    = self._combo_prof.currentText()
        c.gen_tau    = self._p_tau.value()
        c.gen_sigma  = self._p_sigma.value()
        c.gate_start = self._spin_gs.value()
        c.gate_len   = self._spin_gl.value()
        c.threshold  = self._spin_thr.value()
        return c

    def _on_rebuild(self, *_):
        self._rebuild_timer.start()  # restart: fires only after slider settles

    def _do_rebuild(self):
        self._worker.update_config(self._collect_cfg(), rebuild=True)

    def _on_light(self, *_):
        self._light_timer.start()

    def _do_light(self):
        self._worker.update_config(self._collect_cfg(), rebuild=False)
        self._do_reset()

    def _on_gate(self, *_):
        self._collect_cfg()
        self._update_gate_overlay()  # immediate visual feedback
        self._gate_timer.start()     # debounced data reset

    def _do_gate(self):
        self._worker.update_config(self._cfg, rebuild=False)
        self._do_reset()

    def _on_profile(self, text):
        self._p_tau.setVisible(text == "exponential")
        self._p_sigma.setVisible(text == "gaussian")
        self._on_light()

    def _on_bins(self, text):
        try: self._n_bins = int(text)
        except ValueError: pass

    def _on_log(self, idx, checked):
        self._log_scale[idx] = checked
        self._hist_plots[idx].setLogMode(y=checked)

    def _on_scatter_y(self, *_):
        self._sc_y = "integral" if self._rb_sc_int.isChecked() else "peak"
        self._rb_sc_x.clear(); self._rb_sc_y.clear()

    def _toggle_bot(self, *_):
        show_s = self._btn_sig.isChecked()
        show_m = self._btn_matrix.isChecked()
        self._sig_widget.setVisible(show_s)
        self._mat_widget.setVisible(show_m)
        self._bot_split.setVisible(show_s or show_m)
        if show_s: self._update_gate_overlay()

    def _do_reset(self):
        for rb in (self._rb_peak, self._rb_int, self._rb_toa, self._rb_tot,
                   self._rb_sc_x, self._rb_sc_y):
            rb.clear()
        self._ev_count = 0
        for curve in self._hist_curves:
            curve.clear()
        self._scat_item.setData([], [])

    def _update_gate_overlay(self):
        gs = self._spin_gs.value(); gl = self._spin_gl.value()
        self._gate_region.setRegion([gs, gs + gl])
        self._thr_line.setValue(self._spin_thr.value())

    def _update_mat_bg(self, n_side):
        if n_side == 0 or n_side > 200:
            self._mat_bg.setData([], []); return
        rows, cols = np.meshgrid(np.arange(n_side), np.arange(n_side), indexing="ij")
        self._mat_bg.setData(x=cols.ravel().astype(np.float32),
                             y=rows.ravel().astype(np.float32))

    # ------------------------------------------------------------------ frame

    def _on_frame(self):
        new_data = False
        last_wf = last_hits = None; last_samp = 1.0

        try:
            for _ in range(QUEUE_MAX):
                msg = self._q.get_nowait()
                if msg.get("reset"):
                    n_side = msg["n_side"]; self._n_side = n_side
                    self._do_reset(); self._update_mat_bg(n_side)
                    continue

                peaks     = msg["peaks"];     integrals = msg["integrals"]
                toas      = msg["toas"];       tots      = msg["tots"]
                n_pes     = msg["n_pes"];      n         = len(peaks)

                def filt(a): return a[np.isfinite(a)]

                self._rb_peak.extend(filt(peaks))
                self._rb_int.extend(filt(integrals))
                self._rb_toa.extend(filt(toas))
                self._rb_tot.extend(filt(tots))

                sy   = integrals if self._sc_y == "integral" else peaks
                mask = np.isfinite(sy)
                self._rb_sc_x.extend(n_pes[mask].astype(np.float32))
                self._rb_sc_y.extend(sy[mask])

                self._ev_count += n; self._rate_ev += n
                last_wf   = msg["last_wf"]; last_samp = msg["last_samp"]
                last_hits = msg["last_hits"]
                new_data  = True
        except queue.Empty:
            pass

        if not new_data:
            return

        # histograms via numpy
        rbs = (self._rb_peak, self._rb_int, self._rb_toa, self._rb_tot)
        for i, (rb, curve) in enumerate(zip(rbs, self._hist_curves)):
            if len(rb) < 2: continue
            d  = rb.data
            lo = float(d.min()); hi = float(d.max())
            if hi <= lo: hi = lo + 1.0
            counts, edges = np.histogram(d, bins=self._n_bins, range=(lo, hi))
            c_f = counts.astype(np.float64)
            if self._log_scale[i]:
                c_f = np.where(c_f > 0, c_f, 0.5)
            curve.setData(x=edges, y=c_f)

        # scatter
        if len(self._rb_sc_x) > 1:
            x = self._rb_sc_x.data; y = self._rb_sc_y.data
            if len(x) > 8000:
                idx = np.random.choice(len(x), 8000, replace=False)
                x = x[idx]; y = y[idx]
            jitter = (np.random.rand(len(x)).astype(np.float32) - 0.5) * 0.7
            self._scat_item.setData(x=(x + jitter), y=y)

        # signal
        if last_wf is not None:
            self._last_wf = last_wf; self._last_samp = last_samp
            if self._btn_sig.isChecked():
                t = np.arange(len(last_wf), dtype=np.float32) * last_samp
                self._sig_curve.setData(t, last_wf)

        # matrix
        if last_hits is not None and self._btn_matrix.isChecked():
            self._last_hits = last_hits
            by_type = {k: ([], []) for k in HIT_PALETTE}
            for row, col, htype in last_hits:
                if htype in by_type:
                    by_type[htype][0].append(col)
                    by_type[htype][1].append(row)
            for htype, item in self._mat_hits.items():
                xs, ys = by_type[htype]
                item.setData(x=xs, y=ys)

    def _on_rate(self):
        dt = time.monotonic() - self._rate_t0
        if dt > 0:
            rate = self._rate_ev / dt
            self._lbl_ev.setText(f"Events: {self._ev_count:,}")
            self._lbl_rate.setText(f"Rate: {rate:,.0f} ev/s")
        self._rate_ev = 0; self._rate_t0 = time.monotonic()

    def closeEvent(self, ev):
        self._worker.stop(); super().closeEvent(ev)


# --------------------------------------------------------------------------- #
#  Entry point                                                                 #
# --------------------------------------------------------------------------- #

def main():
    app = QtWidgets.QApplication(sys.argv)
    app.setStyle("Fusion")
    p = QtGui.QPalette()
    for role, col in (
        (QtGui.QPalette.Window,          "#1e1e2e"),
        (QtGui.QPalette.WindowText,      "#cdd6f4"),
        (QtGui.QPalette.Base,            "#181825"),
        (QtGui.QPalette.AlternateBase,   "#313244"),
        (QtGui.QPalette.Text,            "#cdd6f4"),
        (QtGui.QPalette.Button,          "#313244"),
        (QtGui.QPalette.ButtonText,      "#cdd6f4"),
        (QtGui.QPalette.Highlight,       "#89b4fa"),
        (QtGui.QPalette.HighlightedText, "#1e1e2e"),
    ):
        p.setColor(role, QtGui.QColor(col))
    app.setPalette(p)
    win = MainWindow()
    win._update_gate_overlay()
    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
