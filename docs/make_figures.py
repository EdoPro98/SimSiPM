"""Generates the figures of the README into images/ using the SiPM Python module.

Usage:  PYTHONPATH=build python docs/make_figures.py [name ...]     (default: all)
The benchmark figures read docs/data/bench_*.json (produced by bench/events).
Events are random: the figures differ slightly from run to run.

Style (mplhep ATLAS, like a ROOT figure of a publication): data are black markers with the standard
deviation as error bar and are never joined by lines, models and fits are red lines, one histogram is
drawn with black filled bins, several histograms with saturated steps and a transparent fill.
"""
import json
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import mplhep as hep
import numpy as np
from matplotlib.colors import LogNorm
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

import SiPM

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "images")
HT = SiPM.SiPMHit.HitType

plt.style.use(hep.style.ATLAS)
plt.rcParams.update({"savefig.dpi": 160, "savefig.bbox": "tight", "savefig.facecolor": "white", "figure.facecolor": "white",
                     "svg.fonttype": "path", "legend.frameon": False, "axes.unicode_minus": True})

INK, GRAY = "#222222", "#7A7A7A"
BLUE, ORANGE, GREEN, PURPLE, GOLD, RED = "#1F4FD8", "#F28E00", "#1B9E4B", "#7B2FBE", "#C9A100", "#E41A1C"
FULL, HITS = BLUE, ORANGE  # event + signal, hits only
TYPES = [  # (hit type, label, colour)
    (HT.kPhotoelectron, "Photoelectron", BLUE),
    (HT.kDarkCount, "Dark count", GRAY),
    (HT.kOpticalCrosstalk, "Crosstalk", ORANGE),
    (HT.kDelayedOpticalCrosstalk, "Delayed crosstalk", GOLD),
    (HT.kFastAfterPulse, "Fast afterpulse", GREEN),
    (HT.kSlowAfterPulse, "Slow afterpulse", PURPLE),
]
COLOR = {t: c for t, _, c in TYPES}
LABEL = {t: l for t, l, _ in TYPES}
NAME = {HT.kPhotoelectron: "photoelectron", HT.kDarkCount: "dark count", HT.kOpticalCrosstalk: "crosstalk",
        HT.kDelayedOpticalCrosstalk: "delayed crosstalk", HT.kFastAfterPulse: "fast afterpulse", HT.kSlowAfterPulse: "slow afterpulse"}


def save(fig, name):
    path = os.path.join(OUT, name)
    fig.savefig(path)
    plt.close(fig)
    print("wrote", os.path.relpath(path))


def tag(ax, note=None):
    """Experiment tag above the frame: 'SimSiPM Simulation', and an optional note on the right."""
    hep.label.exp_text(exp="SimSiPM", text="Simulation", loc=0, ax=ax)
    if note:
        ax.text(1.0, 1.015, note, transform=ax.transAxes, ha="right", va="bottom", fontsize=12)


def data(ax, x, y, yerr=None, color="black", label=None, ms=6, **kw):
    """Data: markers (never joined by lines), with the standard deviation as error bar when given."""
    return ax.errorbar(x, y, yerr=yerr, fmt="o", color=color, ms=ms, elinewidth=1.4, capsize=0, label=label, **kw)


def clean(**kw):
    """Properties with every noise source off (a pure, noise-free pulse)."""
    p = SiPM.SiPMProperties()
    p.setDcrOff(); p.setXtOff(); p.setApOff(); p.setCcgv(0); p.setSnr(120)
    for k, v in kw.items():
        p.setProperty(k, v)
    return p


def scint_times(rng, n, t0=20.0, tau=30.0):
    return t0 + np.asarray(rng.randExponential(tau, n)) if n > 1 else [t0 + rng.randExponential(tau)]


def fig_signals():
    """Hero figure: the same sensor at three light levels."""
    rng = SiPM.SiPMRandom()
    sensor = SiPM.SiPMSensor()
    sensor.setProperty("SignalLength", 400)
    fig, axs = plt.subplots(3, 1, figsize=(9, 8.5), sharex=True, gridspec_kw={"hspace": 0.12})
    for ax, n in zip(axs, (1, 10, 100)):
        sensor.resetState()
        sensor.addPhotons(list(scint_times(rng, n)))
        sensor.runEvent()
        wf = np.asarray(sensor.signal().waveform())
        ax.plot(np.arange(len(wf)) * sensor.properties().sampling(), wf, color="black", lw=1.3)
        ax.text(0.97, 0.88, f"{n} photon{'s' if n > 1 else ''}", transform=ax.transAxes, ha="right", va="top", fontsize=14)
        ax.set_ylabel("Amplitude [p.e.]")
        top = ax.get_ylim()[1] * 1.15
        ax.set_ylim(-0.06 * top, top)
    axs[-1].set_xlabel("Time [ns]"); axs[-1].set_xlim(0, 400)
    tag(axs[0], "Default sensor")
    save(fig, "signals.png")


def fig_pulse():
    """Anatomy of the single photoelectron pulse."""
    tr, tf, ts, frac, t0 = 2.0, 30.0, 150.0, 0.25, 10.0
    p = clean(Sampling=0.5, SignalLength=400, RiseTime=tr, FallTimeFast=tf, FallTimeSlow=ts, SlowComponentFraction=frac)
    s = SiPM.SiPMSensor(p)
    s.addPhoton(t0); s.runEvent()
    wf = np.asarray(s.signal().waveform())
    t = np.arange(len(wf)) * 0.5 - t0
    k = t >= 0
    fast, slow, rise = (1 - frac) * np.exp(-t / tf), frac * np.exp(-t / ts), -np.exp(-t / tr)
    norm = 1 / (fast + slow + rise)[k].max()

    fig, ax = plt.subplots(figsize=(9, 6.2))
    ax.plot(t[k], norm * fast[k], "--", color=GREEN, lw=2, label=r"fast decay  $(1-f)\,e^{-t/\tau_f}$")
    ax.plot(t[k], norm * slow[k], "--", color=PURPLE, lw=2, label=r"slow decay  $f\,e^{-t/\tau_s}$")
    ax.plot(t[k], norm * rise[k], "--", color=ORANGE, lw=2, label=r"rise  $-e^{-t/\tau_r}$")
    ax.plot(t[k], wf[k], color=RED, lw=3.2, label="pulse (sum), peak = 1 p.e.")
    ax.axhline(0, color=GRAY, lw=0.8)
    ax.set_xlim(0, 300); ax.set_ylim(-1.5, 1.35)
    ax.set_xlabel("Time after the photon arrival [ns]"); ax.set_ylabel("Amplitude [p.e.]")
    ax.legend(loc="lower right", fontsize=13)
    ax.text(0.97, 0.95, r"$\tau_r$ = 2 ns,  $\tau_f$ = 30 ns,  $\tau_s$ = 150 ns,  $f$ = 0.25", transform=ax.transAxes, ha="right", va="top", fontsize=13)
    tag(ax)
    save(fig, "pulse_shape.png")


def depth(hits, i):
    d = 0
    while hits[i].parentIdx() >= 0:
        i, d = hits[i].parentIdx(), d + 1
    return d


def find_chain_event(sensor, rng, need, max_hits=14, tries=300000):
    """An event with every needed hit type and a readable chain of 4-5 hits (e.g. photon -> crosstalk -> afterpulse -> ...)."""
    best, best_score = None, -1
    for _ in range(tries):
        sensor.resetState()
        sensor.addPhotons(list(scint_times(rng, 3, 20, 25)))
        sensor.runEvent(False)
        hits = sensor.hits()
        if len(hits) > max_hits or not need <= {h.hitType() for h in hits} or max(h.time() for h in hits) > 230:
            continue
        d = max(depth(hits, i) for i in range(len(hits)))
        if d > 5:
            continue
        score = 100 * d + 10 * len({h.hitType() for h in hits}) - len(hits)
        if score > best_score:
            best, best_score = list(hits), score
            if d >= 4 and score >= 400 + 50 - max_hits:  # chain of 4 links with 5 hit types
                break
    return best


def fig_hits():
    """One event with chains of hits: every hit can generate new hits."""
    p = SiPM.SiPMProperties()
    p.setSignalLength(300); p.setSnr(35); p.setDcr(4e6)
    p.setXt(0.45); p.setDXt(0.5); p.setDXtTau(12); p.setAp(0.4); p.setTauApFastComponent(8); p.setTauApSlowComponent(60)
    sensor = SiPM.SiPMSensor(p)
    rng = SiPM.SiPMRandom()
    hits = find_chain_event(sensor, rng, {HT.kPhotoelectron, HT.kDarkCount, HT.kOpticalCrosstalk, HT.kDelayedOpticalCrosstalk, HT.kFastAfterPulse})

    # waveform of exactly these hits: the pulse of one photoelectron placed at the time of each hit, plus the noise
    t_axis = np.arange(0, 300, 0.5)
    ref = SiPM.SiPMSensor(clean(SignalLength=300, Sampling=0.5))
    ref.addPhoton(0.0); ref.runEvent()
    pulse = np.asarray(ref.signal().waveform())
    wf = np.zeros(len(t_axis))
    for h in hits:
        i = int(h.time() / 0.5)
        wf[i:] += h.amplitude() * pulse[: len(wf) - i]
    wf += np.asarray(rng.randGaussian(0, p.snrLinear(), len(wf)))

    lane = {t: i for i, (t, _, _) in enumerate(reversed(TYPES))}
    leaf = max(range(len(hits)), key=lambda i: depth(hits, i))
    longest = [leaf]
    while hits[longest[-1]].parentIdx() >= 0:
        longest.append(hits[longest[-1]].parentIdx())
    longest = longest[::-1]

    # hits of the same type at the same time (a crosstalk of a crosstalk) are drawn one above the other
    ypos = []
    for i, h in enumerate(hits):
        k = sum(1 for j in range(i) if hits[j].hitType() == h.hitType() and abs(hits[j].time() - h.time()) < 0.2)
        ypos.append(lane[h.hitType()] + 0.2 * k)
    fig, (a1, a2) = plt.subplots(2, 1, figsize=(10.5, 8.6), sharex=True, gridspec_kw={"height_ratios": [1.7, 1], "hspace": 0.06})
    for i, h in enumerate(hits):
        if h.parentIdx() >= 0:
            par = hits[h.parentIdx()]
            hot = i in longest and h.parentIdx() in longest
            a1.add_patch(FancyArrowPatch((par.time(), ypos[h.parentIdx()]), (h.time(), ypos[i]), arrowstyle="-|>",
                                         mutation_scale=12 if hot else 9, color="black" if hot else "#B5B5B5", lw=2.0 if hot else 1.1,
                                         connectionstyle="arc3,rad=0.25", shrinkA=7, shrinkB=7, zorder=2 if hot else 1))
    for i, h in enumerate(hits):
        a1.scatter(h.time(), ypos[i], s=90 + 110 * h.amplitude(), color=COLOR[h.hitType()], edgecolor="white", lw=1.2, zorder=3)
    a1.set_yticks(list(lane.values())); a1.set_yticklabels([LABEL[t] for t in lane])
    a1.set_ylim(-0.95, len(TYPES) - 0.3); a1.set_xlim(0, 300)
    a1.grid(axis="y", color="#DDDDDD", lw=0.8); a1.tick_params(axis="y", which="minor", left=False, right=False)
    chain = [NAME[hits[i].hitType()] for i in longest]
    half = (len(chain) + 1) // 2
    a1.text(0.99, 0.03, "longest chain (black arrows):\n" + " → ".join(chain[:half]) + " →\n" + " → ".join(chain[half:]),
            transform=a1.transAxes, ha="right", va="bottom", fontsize=11.5, linespacing=1.4)
    a2.plot(t_axis, wf, color="black", lw=1.3)
    a2.set_ylim(-0.5, wf.max() * 1.3)
    a2.set_xlabel("Time [ns]"); a2.set_ylabel("Amplitude [p.e.]")
    a2.text(0.98, 0.9, "resulting signal", transform=a2.transAxes, ha="right", va="top", fontsize=13)
    tag(a1, "arrows: parent → child")
    save(fig, "hits.png")


def fig_recovery():
    """Cell recovery: a second hit in the same cell gives a smaller pulse."""
    tau = 50.0
    one_cell = lambda: clean(Size=1, Pitch=1000, SignalLength=500, Sampling=0.5, RecoveryTime=tau)
    delays = np.logspace(0, np.log10(500 - 20), 28)
    amps = []
    for d in delays:
        s = SiPM.SiPMSensor(one_cell())
        s.addPhotons([10.0, 10.0 + d]); s.runEvent()
        amps.append(max(h.amplitude() for h in s.hits() if h.time() > 10.0))
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(14, 6), gridspec_kw={"width_ratios": [1.1, 1], "wspace": 0.25})
    for d, c in zip((20, 60, 200), (ORANGE, GREEN, BLUE)):
        s = SiPM.SiPMSensor(one_cell())
        s.addPhotons([10.0, 10.0 + d]); s.runEvent()
        wf = np.asarray(s.signal().waveform())
        a1.plot(np.arange(len(wf)) * 0.5, wf, color=c, lw=2, label=f"delay {d} ns")
    a1.set_xlim(0, 330); a1.set_ylim(0, 1.45); a1.set_xlabel("Time [ns]"); a1.set_ylabel("Amplitude [p.e.]")
    a1.legend(loc="upper right")
    x = np.logspace(0, np.log10(480), 200)
    a2.plot(x, 1 - np.exp(-x / tau), color=RED, lw=2.5, label=r"$1-e^{-\Delta t/\tau_{rec}}$", zorder=1)
    data(a2, delays, amps, label="simulation", ms=7, zorder=3)
    a2.set_xscale("log"); a2.set_ylim(0, 1.45)
    a2.set_xlabel(r"Delay between the two hits $\Delta t$ [ns]"); a2.set_ylabel("Amplitude of the second pulse [p.e.]")
    a2.legend(loc="upper left")
    a2.text(0.97, 0.06, r"$\tau_{rec}$ = 50 ns", transform=a2.transAxes, ha="right", fontsize=13)
    tag(a1, "two photons in the same cell")
    save(fig, "recovery.png")


def fig_dark_noise():
    """Dark-noise response: delay from the previous pulse versus amplitude."""
    s = SiPM.SiPMSensor()
    s.setProperty("SignalLength", 1e6); s.setProperty("Sampling", 10)
    s.setProperty("Dcr", 200e3); s.setProperty("Xt", 0.15); s.setProperty("DXt", 0.3); s.setProperty("Ap", 0.1)
    s.setProperty("RecoveryTime", 50)
    window, threshold = s.properties().signalLength(), 0.25
    names = {HT.kDarkCount: "Primary dark count", HT.kDelayedOpticalCrosstalk: "Delayed crosstalk",
             HT.kFastAfterPulse: "Afterpulse (fast)", HT.kSlowAfterPulse: "Afterpulse (slow)"}
    colors = {"Primary dark count": BLUE, "Direct crosstalk": ORANGE, "Delayed crosstalk": GOLD,
              "Afterpulse (fast)": GREEN, "Afterpulse (slow)": PURPLE}
    delays, amps, kinds, last = [], [], [], None
    for i in range(400):
        s.resetState(); s.runEvent(False)
        hits = sorted(s.hits(), key=lambda h: h.time())
        j = 0
        while j < len(hits):
            k = j
            while k < len(hits) and hits[k].time() == hits[j].time():
                k += 1
            pulse, j = hits[j:k], k  # simultaneous hits are one pulse: direct crosstalk fires with its parent
            amp = sum(h.amplitude() for h in pulse)
            if amp < threshold:
                continue
            t = i * window + pulse[0].time()
            if last is not None:
                delays.append(t - last); amps.append(amp)
                kinds.append("Direct crosstalk" if any(h.hitType() == HT.kOpticalCrosstalk for h in pulse) else names[pulse[0].hitType()])
            last = t
    delays, amps, kinds = map(np.array, (delays, amps, kinds))
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(15, 6.2), gridspec_kw={"wspace": 0.28})
    for kind, c in colors.items():
        m = kinds == kind
        a1.scatter(delays[m], amps[m], s=3, color=c, label=kind, rasterized=True)
    a1.set_xscale("log"); a1.set_ylim(0, 6.5)
    a1.set_xlabel("Time since the previous pulse [ns]"); a1.set_ylabel("Pulse amplitude [p.e.]")
    a1.legend(markerscale=5, loc="upper left", fontsize=12)
    xb = np.logspace(np.log10(delays.min()), np.log10(delays.max()), 160)
    *_, im = a2.hist2d(delays, amps, bins=[xb, np.linspace(0, 6.5, 130)], cmap="jet", norm=LogNorm(), cmin=1)
    a2.set_xscale("log")
    a2.set_xlabel("Time since the previous pulse [ns]"); a2.set_ylabel("Pulse amplitude [p.e.]")
    fig.colorbar(im, ax=a2, label="Pulses", pad=0.02)
    tag(a1, "0.4 s of dark noise")
    save(fig, "dark_noise.png")


def fig_pde():
    """Spectral PDE: the measured efficiency of the simulation against the curve of the model."""
    wl = [800, 750, 700, 650, 600, 550, 500, 450, 400, 350, 300]
    pde = [0.22, 0.30, 0.40, 0.45, 0.50, 0.50, 0.45, 0.35, 0.25, 0.15, 0.05]
    p = clean(); p.setPdeSpectrum(wl, pde)
    curve = sorted(p.pdeSpectrum().items())
    cx, cy = np.array([c[0] for c in curve]), np.array([c[1] for c in curve])
    s = SiPM.SiPMSensor(p)
    rng = SiPM.SiPMRandom()
    edges = np.arange(300, 801, 20)
    n_per_bin = 6000
    eff, err, centers, model = [], [], 0.5 * (edges[1:] + edges[:-1]), []
    for lo, hi in zip(edges[:-1], edges[1:]):
        w = lo + (hi - lo) * np.asarray(rng.Rand(n_per_bin))  # photons uniformly spread in the bin
        s.resetState(); s.addPhotons([10.0] * n_per_bin, list(w)); s.runEvent(False)
        k = s.debug().nPhotoelectrons
        eff.append(k / n_per_bin); err.append(np.sqrt(max(k, 1) * (1 - k / n_per_bin)) / n_per_bin)
        model.append(np.interp(np.linspace(lo, hi, 50), cx, cy).mean())
    eff, err, model = map(np.array, (eff, err, model))

    fig, (a1, a2) = plt.subplots(2, 1, figsize=(9.5, 8), sharex=True, gridspec_kw={"height_ratios": [3, 1], "hspace": 0.06})
    a1.plot(cx, cy, color=RED, lw=2.5, label="model: 32 points curve, log interpolation")
    a1.plot(wl, pde, "s", mfc="white", mec=RED, mew=2, ms=8, label="input points")
    data(a1, centers, eff, err, label=f"simulation, {n_per_bin} photons per point", ms=6)
    a1.set_ylabel("Photon detection efficiency"); a1.set_ylim(0, 0.68); a1.legend(loc="upper left", fontsize=12.5)
    data(a2, centers, eff / model, err / model, ms=5)
    a2.axhline(1, color=RED, lw=2)
    a2.set_ylim(0.9, 1.1); a2.set_ylabel("Sim. / model"); a2.set_xlabel("Photon wavelength [nm]"); a2.set_xlim(290, 810)
    tag(a1)
    save(fig, "pde.png")


def fig_hit_distribution():
    """Where the photons land on the sensor for the three hit distributions."""
    D = SiPM.SiPMProperties.HitDistribution
    fig, axs = plt.subplots(1, 3, figsize=(15, 5.6), gridspec_kw={"wspace": 0.12})
    for ax, (dist, title) in zip(axs, ((D.kUniform, "kUniform"), (D.kCircle, "kCircle"), (D.kGaussian, "kGaussian"))):
        p = clean(); p.setHitDistribution(dist)
        s = SiPM.SiPMSensor(p)
        s.addPhotons([10.0] * 60000); s.runEvent(False)
        n = p.nSideCells()
        grid = np.zeros((n, n))
        for h in s.hits():
            grid[h.row(), h.col()] += 1
        ax.imshow(grid, cmap="jet", origin="lower", extent=(0, 1, 0, 1))
        ax.text(0.5, -0.07, title, transform=ax.transAxes, ha="center", va="top", fontsize=15)
        ax.set_xticks([]); ax.set_yticks([])
        ax.tick_params(which="both", length=0)
    tag(axs[0])
    axs[2].text(1.0, 1.015, "60000 photons on 40 × 40 cells", transform=axs[2].transAxes, ha="right", va="bottom", fontsize=12)
    save(fig, "hit_distribution.png")


def fig_spectrum():
    """Charge spectrum with a pulsed light source: the photoelectron peaks."""
    p = SiPM.SiPMProperties(); p.setSignalLength(300); p.setSnr(28)
    s = SiPM.SiPMSensor(p)
    rng = SiPM.SiPMRandom()
    ref = SiPM.SiPMSensor(clean(SignalLength=300)); ref.addPhoton(30.0); ref.runEvent()
    one_pe = ref.signal().integral(10, 250, -100)
    q = []
    for _ in range(40000):
        s.resetState()
        n = rng.randPoisson(2.0)
        if n:
            s.addPhotons(list(30.0 + np.asarray(rng.randGaussian(0, 1.0, n))))
        s.runEvent()
        q.append(s.signal().integral(10, 250, -100) / one_pe)
    counts, edges = np.histogram(q, bins=np.linspace(-0.6, 8.5, 230))
    fig, ax = plt.subplots(figsize=(9.5, 6.2))
    hep.histplot(counts, edges, ax=ax, histtype="fill", color="black", edgecolor="black", lw=0.6)
    ax.set_yscale("log"); ax.set_xlim(-0.6, 8.5); ax.set_ylim(5, 4e4)
    ax.set_xlabel("Integral [p.e.]"); ax.set_ylabel("Events")
    for k in range(7):
        ax.text(k, 2.2e4, f"{k}", ha="center", color=GRAY, fontsize=13)
    ax.text(0.97, 0.84, "Poisson(2) photons per event\nDCR 200 kHz, XT 5 %, AP 3 %", transform=ax.transAxes, ha="right", va="top", fontsize=12.5)
    tag(ax, "40000 events")
    save(fig, "spectrum.png")


def fig_saturation():
    """Response versus the number of photons for different cell sizes."""
    rng = SiPM.SiPMRandom()
    n = np.unique(np.logspace(0, np.log10(30000), 20).astype(int))
    fig, ax = plt.subplots(figsize=(9.5, 7))
    for pitch, c in ((10, GREEN), (25, BLUE), (50, ORANGE)):
        p = SiPM.SiPMProperties(); p.setPitch(pitch); p.setSignalLength(400)
        s = SiPM.SiPMSensor(p)
        ref = SiPM.SiPMSensor(clean(Pitch=pitch, SignalLength=400)); ref.addPhoton(25.0); ref.runEvent()
        one_pe = ref.signal().integral(10, 350, -100)
        mean, std = [], []
        for k in n:
            v = []
            for _ in range(10):
                s.resetState(); s.addPhotons(list(25.0 + np.asarray(rng.randGaussian(0, 0.2, int(k))))); s.runEvent()
                v.append(s.signal().integral(10, 350, -100) / one_pe)
            mean.append(np.mean(v)); std.append(np.std(v))
        data(ax, n, mean, std, color=c, label=f"{pitch} µm cells ({p.nCells()})", ms=6)
        cells = p.nCells()
        ax.plot(n, cells * (1 - np.exp(-n / cells)), color=RED, lw=1.8, zorder=1)
    ax.plot([], [], color=RED, lw=2, label=r"model $N_{cells}\,(1-e^{-N/N_{cells}})$")
    ax.set_xscale("log"); ax.set_yscale("log"); ax.set_xlim(0.8, 4e4); ax.set_ylim(0.8, 1e5)
    ax.set_xlabel("Photons on the sensor (all detected)"); ax.set_ylabel("Signal integral [p.e.]")
    ax.legend(loc="upper left", fontsize=12.5)
    tag(ax, "10 events per point")
    save(fig, "saturation.png")


def load_bench(kind):
    with open(os.path.join(HERE, "data", f"bench_{kind}.json")) as f:
        return json.load(f)["results"]


def fig_benchmarks():
    """Time of an event versus the number of photons, with and without signal generation."""
    r = load_bench("default")
    ns = np.array([1 << i for i in range(15)])
    lo, hi = 16, 256  # range where the time is linear in the number of hit cells
    fig, (a1, a2) = plt.subplots(2, 1, figsize=(9.5, 9), sharex=True, gridspec_kw={"height_ratios": [2.6, 1], "hspace": 0.06})
    note = []
    for mode, color, label in (("Full", FULL, "runEvent()  (hits + signal)"), ("HitsOnly", HITS, "runEvent(false)  (hits only)")):
        t = np.array([r[f"Scan/{mode}/{n}"]["mean"] for n in ns]) / 1000   # µs
        e = np.array([r[f"Scan/{mode}/{n}"]["std"] for n in ns]) / 1000
        pe = np.array([r[f"Scan/{mode}/{n}"]["pe"] for n in ns])   # hit cells per event
        data(a1, ns, t, e, color=color, label=label, ms=7)
        data(a2, ns, t / pe * 1000, e / pe * 1000, color=color, ms=7)
        sel = (ns >= lo) & (ns <= hi)
        c, a = np.polyfit(pe[sel], t[sel], 1, w=1 / t[sel])   # T = a + c * n_pe
        xs = np.logspace(0, np.log10(ns[-1]), 200)
        ys = a + c * np.interp(xs, ns, pe)
        inside = (xs >= lo) & (xs <= hi)
        a1.plot(xs[inside], ys[inside], color=RED, lw=2.5, zorder=1)
        a1.plot(xs[xs >= hi], ys[xs >= hi], color=RED, lw=1.5, ls="--", zorder=1)
        a2.plot([lo, hi], [c * 1000] * 2, color=RED, lw=2.5, zorder=1)
        a2.plot([hi, ns[-1]], [c * 1000] * 2, color=RED, lw=1.5, ls="--", zorder=1)
        note.append(f"{label.split('  ')[0]}:  c = {c * 1000:.1f} ns per p.e.,  a = {max(a, 0):.2f} µs")
    a1.plot([], [], color=RED, lw=2.5, label=rf"fit $T = a + c\,n_{{pe}}$ for ${lo} \leq N \leq {hi}$ (dashed: extrapolation)")
    a1.set_xscale("log"); a1.set_yscale("log"); a1.set_ylabel("Time per event [µs]"); a1.set_ylim(0.04, 5000)
    a1.legend(loc="upper left", fontsize=12.5)
    a1.text(0.97, 0.06, "\n".join(note), transform=a1.transAxes, ha="right", va="bottom", fontsize=12.5)
    a2.set_xscale("log"); a2.set_yscale("log"); a2.set_ylim(8, 3000)
    a2.set_ylabel("Time per p.e. [ns]"); a2.set_xlabel("Photons per event")
    a2.set_xlim(0.7, 2.4e4)
    tag(a1, "default sensor, i5-14400F")
    save(fig, "benchmarks.png")


def fig_pde_impact():
    """Event time for no PDE, a flat PDE and a spectral PDE."""
    r = load_bench("default")
    us = lambda key: r[key]["mean"] / 1000
    fig, axs = plt.subplots(1, 2, figsize=(14, 6), gridspec_kw={"wspace": 0.25})
    for ax, size, title in ((axs[0], "Photons25", "25 photons"), (axs[1], "Photons1000", "1000 photons")):
        names = ["None", "Flat", "Spectral"]; ks = ["NoPde", "FlatPde", "SpectralPde"]
        x = np.arange(3)
        f = [us(f"PdeBench/{size}/{k}/Full") for k in ks]; h = [us(f"PdeBench/{size}/{k}/HitsOnly") for k in ks]
        ef = [r[f"PdeBench/{size}/{k}/Full"]["std"] / 1000 for k in ks]; eh = [r[f"PdeBench/{size}/{k}/HitsOnly"]["std"] / 1000 for k in ks]
        ax.bar(x - 0.19, f, 0.36, color=FULL, yerr=ef, error_kw={"elinewidth": 1.3, "ecolor": "black"}, label="runEvent()")
        ax.bar(x + 0.19, h, 0.36, color=HITS, yerr=eh, error_kw={"elinewidth": 1.3, "ecolor": "black"}, label="runEvent(false)")
        for xx, v in zip(x - 0.19, f): ax.text(xx, v * 1.04, f"{v:.1f}" if v < 10 else f"{v:.0f}", ha="center", va="bottom", fontsize=12)
        for xx, v in zip(x + 0.19, h): ax.text(xx, v * 1.04, f"{v:.1f}" if v < 10 else f"{v:.0f}", ha="center", va="bottom", fontsize=12)
        ax.set_xticks(x); ax.set_xticklabels(names); ax.set_xlabel("PDE"); ax.set_ylabel("Time per event [µs]")
        ax.set_ylim(0, max(f + h) * 1.35)
        ax.text(0.04, 0.95, title, transform=ax.transAxes, ha="left", va="top", fontsize=15)
        ax.tick_params(axis="x", which="minor", bottom=False, top=False)
    axs[0].legend(loc="upper right", fontsize=12.5)
    tag(axs[0], "PDE impact")
    save(fig, "pde_impact.png")


def box(ax, x, y, w, h, text, fc, ec, fs=10, bold=False):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02,rounding_size=0.08", fc=fc, ec=ec, lw=1.4))
    ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs, color=INK, fontweight="bold" if bold else "normal")


def arrow(ax, a, b, color=GRAY, rad=0.0, lw=1.6):
    ax.add_patch(FancyArrowPatch(a, b, arrowstyle="-|>", mutation_scale=13, color=color, lw=lw, connectionstyle=f"arc3,rad={rad}"))


def fig_pipeline():
    """What runEvent() does."""
    fig, ax = plt.subplots(figsize=(11.5, 5.6))
    ax.set_xlim(0, 11.5); ax.set_ylim(0, 5.6); ax.axis("off")
    ax.add_patch(FancyBboxPatch((0.25, 2.95), 11.0, 2.35, boxstyle="round,pad=0.02,rounding_size=0.15", fc="#F3F7FC", ec="#BFD3EA", lw=1.2))
    ax.text(0.45, 5.05, "Hit simulation", fontsize=12.5, fontweight="bold", color=BLUE, va="top")
    ax.text(2.6, 5.05, "runEvent(false)", fontsize=10.5, color=GRAY, va="top", family="DejaVu Sans Mono")
    ax.text(11.05, 5.05, "results: hits(), debug()", fontsize=10.5, color=BLUE, va="top", ha="right", family="DejaVu Sans Mono")
    ax.add_patch(FancyBboxPatch((0.25, 0.3), 11.0, 1.95, boxstyle="round,pad=0.02,rounding_size=0.15", fc="#FDF5EE", ec="#F0CFB3", lw=1.2))
    ax.text(0.45, 2.05, "Signal generation", fontsize=12.5, fontweight="bold", color=ORANGE, va="top")
    ax.text(3.0, 2.05, "runEvent()  or the first call to signal()", fontsize=10.5, color=GRAY, va="top", family="DejaVu Sans Mono")
    ax.text(11.05, 2.05, "result: signal()", fontsize=10.5, color=ORANGE, va="top", ha="right", family="DejaVu Sans Mono")
    W, H, Y = 1.85, 1.1, 3.3
    xs = [0.5 + i * 2.1 for i in range(5)]
    items = ["Input\nphoton times\n(+ wavelengths)", "PDE\nkeeps or drops\neach photon", "Hit position\nuniform / circle /\ngaussian",
             "Dark counts,\ncrosstalk,\nafterpulses", "Cell recovery and\ngain variation set\nthe amplitudes"]
    for x, t in zip(xs, items):
        box(ax, x, Y, W, H, t, "white", "#9CB9DB", fs=10)
    for a_, b_ in zip(xs[:-1], xs[1:]):
        arrow(ax, (a_ + W + 0.02, Y + H / 2), (b_ - 0.02, Y + H / 2))
    y2, h2 = 0.55, 1.1
    sig = [(0.5, 2.9, "Electronic noise\nbaseline (SNR)"), (3.75, 3.3, "Pulse of every hit added:\ndirect or recursive\nconvolution"), (7.9, 2.9, "SiPMAnalogSignal:\nintegral, peak, ToA,\nToT, time of peak")]
    for x, w, t in sig:
        box(ax, x, y2, w, h2, t, "white", "#EBBB93", fs=10)
    arrow(ax, (3.42, y2 + h2 / 2), (3.73, y2 + h2 / 2)); arrow(ax, (7.07, y2 + h2 / 2), (7.88, y2 + h2 / 2))
    x0 = xs[4] + W / 2
    ax.plot([x0, x0], [Y, 2.6], color=GRAY, lw=1.4, solid_capstyle="butt")
    ax.plot([x0, 1.95], [2.6, 2.6], color=GRAY, lw=1.4)
    ax.plot([1.95, 1.95], [2.6, y2 + h2 + 0.02], color=GRAY, lw=1.4)
    ax.annotate("", xy=(1.95, y2 + h2 + 0.02), xytext=(1.95, y2 + h2 + 0.35), arrowprops=dict(arrowstyle="-|>", color=GRAY, lw=1.4, mutation_scale=13))
    save(fig, "pipeline.svg")


FIGURES = {n[4:]: f for n, f in globals().items() if n.startswith("fig_")}

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for name in sys.argv[1:] or FIGURES:
        FIGURES[name]()
