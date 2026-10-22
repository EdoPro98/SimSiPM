"""Generates the figures of the README into images/ using the SiPM Python module.

Usage:  PYTHONPATH=build python docs/make_figures.py [name ...]     (default: all)
The benchmark figure reads docs/data/bench_*.json (produced by bench/events).
Events are random: the figures differ slightly from run to run.
"""
import json
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LogNorm
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

import SiPM

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "images")
HT = SiPM.SiPMHit.HitType

INK = "#1F2937"
GRID = "#E5E7EB"
BLUE, GRAY, ORANGE, GOLD, GREEN, PURPLE, RED = "#2E6FBA", "#6B7280", "#E8803A", "#C9A227", "#3BA372", "#8A64C0", "#D6504B"
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

plt.rcParams.update({
    "figure.facecolor": "white", "axes.facecolor": "white", "savefig.facecolor": "white",
    "font.size": 11, "text.color": INK, "axes.labelcolor": INK, "xtick.color": INK, "ytick.color": INK,
    "axes.edgecolor": "#9CA3AF", "axes.spines.top": False, "axes.spines.right": False,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8, "axes.axisbelow": True,
    "legend.frameon": False, "axes.titlesize": 12, "axes.titleweight": "bold", "axes.titlelocation": "left",
    "savefig.dpi": 170, "savefig.bbox": "tight", "svg.fonttype": "path",
})


def save(fig, name):
    path = os.path.join(OUT, name)
    fig.savefig(path)
    plt.close(fig)
    print("wrote", os.path.relpath(path))


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
    fig, axs = plt.subplots(3, 1, figsize=(8, 6.2), sharex=True, gridspec_kw={"hspace": 0.4})
    for ax, n, color in zip(axs, (1, 10, 100), (BLUE, ORANGE, GREEN)):
        sensor.resetState()
        sensor.addPhotons(list(scint_times(rng, n)))
        sensor.runEvent()
        wf = np.asarray(sensor.signal().waveform())
        ax.plot(np.arange(len(wf)) * sensor.properties().sampling(), wf, color=color, lw=1.4)
        ax.set_title(f"{n} photon{'s' if n > 1 else ''}", fontsize=11)
        ax.set_ylabel("Amplitude [p.e.]")
    axs[-1].set_xlabel("Time [ns]")
    axs[-1].set_xlim(0, 400)
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

    fig, ax = plt.subplots(figsize=(8, 4.2))
    ax.plot(t[k], norm * fast[k], "--", color=GREEN, lw=1.3, label=r"fast decay  $(1-f)\,e^{-t/\tau_f}$")
    ax.plot(t[k], norm * slow[k], "--", color=PURPLE, lw=1.3, label=r"slow decay  $f\,e^{-t/\tau_s}$")
    ax.plot(t[k], norm * rise[k], "--", color=ORANGE, lw=1.3, label=r"rise  $-e^{-t/\tau_r}$")
    ax.plot(t[k], wf[k], color=BLUE, lw=2.6, label="pulse (sum, peak = 1 p.e.)")
    ax.axhline(0, color="#9CA3AF", lw=0.8)
    ax.set_xlim(0, 300); ax.set_ylim(-1.45, 1.15)
    ax.set_xlabel("Time after the photon arrival [ns]"); ax.set_ylabel("Amplitude [p.e.]")
    ax.legend(loc="lower right", fontsize=10)
    ax.annotate(r"$\tau_r$ = 2 ns", xy=(2, -0.5), xytext=(30, -1.2), color=ORANGE, arrowprops=dict(arrowstyle="-", color=ORANGE))
    ax.annotate(r"$\tau_f$ = 30 ns,  $\tau_s$ = 150 ns,  $f$ = 0.25", xy=(60, 0.5), xytext=(75, 0.9), color=INK, fontsize=10)
    save(fig, "pulse_shape.png")


def find_event(sensor, rng, need, max_hits, tries=20000):
    for _ in range(tries):
        sensor.resetState()
        sensor.addPhotons(list(scint_times(rng, 5, 20, 25)))
        sensor.runEvent()
        hits = sensor.hits()
        kinds = {h.hitType() for h in hits}
        if need <= kinds and len(hits) <= max_hits:
            return hits
    raise RuntimeError("no event with every hit type found")


def fig_hits():
    """One event: what produces the hits, and the waveform they make."""
    p = SiPM.SiPMProperties()
    p.setSignalLength(300); p.setSnr(35); p.setDcr(4e6)
    p.setXt(0.3); p.setDXt(0.4); p.setDXtTau(12); p.setAp(0.2); p.setTauApFastComponent(8); p.setTauApSlowComponent(70)
    p.setPitch(25)
    sensor = SiPM.SiPMSensor(p)
    rng = SiPM.SiPMRandom()
    need = {HT.kPhotoelectron, HT.kDarkCount, HT.kOpticalCrosstalk, HT.kDelayedOpticalCrosstalk, HT.kFastAfterPulse}
    hits = find_event(sensor, rng, need, 14)
    wf = np.asarray(sensor.signal().waveform())

    lane = {t: i for i, (t, _, _) in enumerate(reversed(TYPES))}
    fig, (a1, a2) = plt.subplots(2, 1, figsize=(9, 6.2), sharex=True, gridspec_kw={"height_ratios": [1.5, 1], "hspace": 0.3})
    for i, h in enumerate(hits):
        if h.parentIdx() >= 0:
            par = hits[h.parentIdx()]
            a1.add_patch(FancyArrowPatch((par.time(), lane[par.hitType()]), (h.time(), lane[h.hitType()]), arrowstyle="-|>", mutation_scale=9,
                                         color="#9CA3AF", lw=1.1, connectionstyle="arc3,rad=0.25", shrinkA=6, shrinkB=6, zorder=1))
    for h in hits:
        a1.scatter(h.time(), lane[h.hitType()], s=60 + 90 * h.amplitude(), color=COLOR[h.hitType()], edgecolor="white", lw=1, zorder=3)
    a1.set_yticks(list(lane.values()))
    a1.set_yticklabels([LABEL[t] for t in lane])
    a1.set_ylim(-0.6, len(TYPES) - 0.4)
    a1.grid(axis="x", alpha=0)
    a1.set_title("Hits of one event (arrows: parent → child, marker size: amplitude)")
    t = np.arange(len(wf)) * sensor.properties().sampling()
    a2.plot(t, wf, color=INK, lw=1.4)
    a2.set_xlim(0, 300)
    a2.set_xlabel("Time [ns]"); a2.set_ylabel("Amplitude [p.e.]")
    a2.set_title("Resulting signal")
    save(fig, "hits.png")


def fig_recovery():
    """Cell recovery: a second hit in the same cell gives a smaller pulse."""
    tau = 50.0
    one_cell = lambda: clean(Size=1, Pitch=1000, SignalLength=500, Sampling=0.5, RecoveryTime=tau)
    delays = np.logspace(0, np.log10(500 - 20), 60)
    amps = []
    for d in delays:
        s = SiPM.SiPMSensor(one_cell())
        s.addPhotons([10.0, 10.0 + d]); s.runEvent()
        amps.append(max(h.amplitude() for h in s.hits() if h.time() > 10.0))
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(10, 3.9), gridspec_kw={"width_ratios": [1.15, 1]})
    for d, c in zip((20, 60, 200), (RED, ORANGE, GREEN)):
        s = SiPM.SiPMSensor(one_cell())
        s.addPhotons([10.0, 10.0 + d]); s.runEvent()
        wf = np.asarray(s.signal().waveform())
        a1.plot(np.arange(len(wf)) * 0.5, wf, color=c, lw=1.6, label=f"delay {d} ns")
    a1.set_xlim(0, 330); a1.set_xlabel("Time [ns]"); a1.set_ylabel("Amplitude [p.e.]")
    a1.set_title("Two photons in the same cell"); a1.legend()
    x = np.logspace(0, np.log10(480), 200)
    a2.plot(x, 1 - np.exp(-x / tau), color=BLUE, lw=2, label=r"$1-e^{-\Delta t/\tau_{rec}}$")
    a2.plot(delays, amps, "o", color=INK, ms=4, label="simulation")
    a2.set_xscale("log"); a2.set_ylim(0, 1.08)
    a2.set_xlabel(r"Delay between the hits $\Delta t$ [ns]"); a2.set_ylabel("Amplitude of the second pulse [p.e.]")
    a2.set_title(r"Recovery ($\tau_{rec}$ = 50 ns)"); a2.legend(loc="lower right")
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
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(11, 4.4), gridspec_kw={"wspace": 0.25})
    for kind, c in colors.items():
        m = kinds == kind
        a1.scatter(delays[m], amps[m], s=2, color=c, label=kind, rasterized=True)
    a1.set_xscale("log"); a1.set_ylim(0, 6)
    a1.set_xlabel("Time since the previous pulse [ns]"); a1.set_ylabel("Pulse amplitude [p.e.]")
    a1.legend(markerscale=6, loc="upper left", fontsize=9.5)
    xb = np.logspace(np.log10(delays.min()), np.log10(delays.max()), 160)
    *_, im = a2.hist2d(delays, amps, bins=[xb, np.linspace(0, 6, 120)], cmap="jet", norm=LogNorm(), cmin=1)
    a2.set_xscale("log"); a2.grid(False)
    a2.set_xlabel("Time since the previous pulse [ns]"); a2.set_ylabel("Pulse amplitude [p.e.]")
    fig.colorbar(im, ax=a2, label="Counts", pad=0.02)
    save(fig, "dark_noise.png")


def fig_pde():
    """Spectral PDE: input points, the curve used, and the simulated detection efficiency."""
    wl = [800, 750, 700, 650, 600, 550, 500, 450, 400, 350, 300]
    pde = [0.22, 0.30, 0.40, 0.45, 0.50, 0.50, 0.45, 0.35, 0.25, 0.15, 0.05]
    p = clean(); p.setPdeSpectrum(wl, pde)
    curve = sorted(p.pdeSpectrum().items())
    s = SiPM.SiPMSensor(p)
    rng = SiPM.SiPMRandom()
    edges = np.arange(300, 801, 25)
    # one batch of events per wavelength bin, so the bin of every detected photon is known
    seen = np.zeros(len(edges) - 1)
    for i in range(len(edges) - 1):
        for _ in range(40):
            w = edges[i] + 25 * np.asarray(rng.Rand(2000))
            s.resetState(); s.addPhotons([10.0] * 2000, list(w)); s.runEvent(False)
            seen[i] += s.debug().nPhotoelectrons
    frac = seen / (40 * 2000)
    fig, ax = plt.subplots(figsize=(8, 4.2))
    ax.plot(*zip(*curve), color=BLUE, lw=2, label="curve used by the simulation (32 points)")
    ax.plot(wl, pde, "o", color=INK, ms=6, label="input points")
    ax.bar(edges[:-1] + 12.5, frac, width=22, color=ORANGE, alpha=0.55, label="simulated: detected / incident (25 nm bins)")
    ax.set_xlabel("Photon wavelength [nm]"); ax.set_ylabel("Photon detection efficiency")
    ax.set_xlim(290, 810); ax.set_ylim(0, 0.68); ax.legend(loc="upper left", fontsize=9.5)
    save(fig, "pde.png")


def fig_hit_distribution():
    """Where the photons land on the sensor for the three hit distributions."""
    D = SiPM.SiPMProperties.HitDistribution
    fig, axs = plt.subplots(1, 3, figsize=(10.5, 3.7), gridspec_kw={"wspace": 0.12})
    for ax, (dist, title) in zip(axs, ((D.kUniform, "kUniform"), (D.kCircle, "kCircle"), (D.kGaussian, "kGaussian"))):
        p = clean(); p.setHitDistribution(dist)
        s = SiPM.SiPMSensor(p)
        s.addPhotons([10.0] * 40000); s.runEvent(False)
        n = p.nSideCells()
        grid = np.zeros((n, n))
        for h in s.hits():
            grid[h.row(), h.col()] += 1
        ax.imshow(grid, cmap="viridis", origin="lower", extent=(0, 1, 0, 1))
        ax.set_title(title, fontsize=11); ax.grid(False)
        ax.set_xticks([]); ax.set_yticks([])
        for sp in ax.spines.values():
            sp.set_visible(True); sp.set_color("#9CA3AF")
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
    fig, ax = plt.subplots(figsize=(8, 4))
    ax.hist(q, bins=np.linspace(-0.6, 8.5, 260), color=BLUE, alpha=0.85)
    ax.set_yscale("log"); ax.set_xlim(-0.6, 8.5); ax.set_ylim(bottom=5)
    ax.set_xlabel("Integral [p.e.]"); ax.set_ylabel("Events")
    for k in range(7):
        ax.text(k, ax.get_ylim()[1] * 0.7, f"{k}", ha="center", color=GRAY, fontsize=10)
    ax.text(0.99, 0.93, "Poisson(2) photons, DCR 200 kHz,\nXT 5 %, AP 3 %, SNR 28 dB", transform=ax.transAxes, ha="right", va="top", fontsize=9.5, color=GRAY)
    save(fig, "spectrum.png")


def fig_saturation():
    """Response versus the number of photons for different cell sizes."""
    rng = SiPM.SiPMRandom()
    n = np.unique(np.logspace(0, np.log10(30000), 22).astype(int))
    fig, ax = plt.subplots(figsize=(7, 4.6))
    for pitch, c in ((10, GREEN), (25, BLUE), (50, ORANGE)):
        p = SiPM.SiPMProperties(); p.setPitch(pitch); p.setSignalLength(400)
        s = SiPM.SiPMSensor(p)
        ref = SiPM.SiPMSensor(clean(Pitch=pitch, SignalLength=400)); ref.addPhoton(25.0); ref.runEvent()
        one_pe = ref.signal().integral(10, 350, -100)
        mean = []
        for k in n:
            v = []
            for _ in range(8):
                s.resetState(); s.addPhotons(list(25.0 + np.asarray(rng.randGaussian(0, 0.2, int(k))))); s.runEvent()
                v.append(s.signal().integral(10, 350, -100) / one_pe)
            mean.append(np.mean(v))
        ax.plot(n, mean, "o-", ms=3.5, lw=1.5, color=c, label=f"{pitch} µm cells ({p.nCells()} cells)")
    ax.plot(n, n, color="#9CA3AF", lw=1.2, ls="--", label="linear response")
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xlabel("Photons on the sensor (all detected)"); ax.set_ylabel("Signal integral [p.e.]")
    ax.legend(fontsize=9.5)
    save(fig, "saturation.png")


def load_bench(kind):
    with open(os.path.join(HERE, "data", f"bench_{kind}.json")) as f:
        return json.load(f)["results"]


def fig_benchmarks():
    """Event cost for typical sizes, with and without signal generation, and the PDE impact."""
    r = load_bench("default")
    us = lambda key: r[key]["ns"] / 1000

    fig, axs = plt.subplots(1, 3, figsize=(13, 4.0), gridspec_kw={"width_ratios": [1.15, 1, 1], "wspace": 0.32})
    # A: event size
    ax = axs[0]
    labels, keys = ["Empty", "25 photons", "1000 photons"], ["Empty", "Photons25", "Photons1000"]
    y = np.arange(len(keys))[::-1]
    full = [us(f"Events/{k}/Full") for k in keys]; hits = [us(f"Events/{k}/HitsOnly") for k in keys]
    ax.barh(y + 0.19, full, 0.36, color=BLUE)
    ax.barh(y - 0.19, hits, 0.36, color=ORANGE)
    for yy, v in zip(y + 0.19, full): ax.text(v * 1.12, yy, f"{v:.2f} µs" if v < 10 else f"{v:.1f} µs", va="center", fontsize=9.5)
    for yy, v in zip(y - 0.19, hits): ax.text(v * 1.12, yy, f"{v:.2f} µs" if v < 10 else f"{v:.1f} µs", va="center", fontsize=9.5)
    ax.set_yticks(y); ax.set_yticklabels(labels)
    ax.set_xscale("log"); ax.set_xlim(0.03, 200); ax.set_xlabel("Time per event [µs]")
    ax.set_title("Event size")
    ax.grid(axis="y", alpha=0)
    # B, C: PDE
    for ax, size, title in ((axs[1], "Photons25", "PDE impact, 25 photons"), (axs[2], "Photons1000", "PDE impact, 1000 photons")):
        names = ["None", "Flat", "Spectral"]; ks = ["NoPde", "FlatPde", "SpectralPde"]
        x = np.arange(3)
        f = [us(f"PdeBench/{size}/{k}/Full") for k in ks]; h = [us(f"PdeBench/{size}/{k}/HitsOnly") for k in ks]
        ax.bar(x - 0.19, f, 0.36, color=BLUE); ax.bar(x + 0.19, h, 0.36, color=ORANGE)
        for xx, v in zip(x - 0.19, f): ax.text(xx, v * 1.02, f"{v:.1f}" if v < 10 else f"{v:.0f}", ha="center", va="bottom", fontsize=9)
        for xx, v in zip(x + 0.19, h): ax.text(xx, v * 1.02, f"{v:.1f}" if v < 10 else f"{v:.0f}", ha="center", va="bottom", fontsize=9)
        ax.set_xticks(x); ax.set_xticklabels(names); ax.set_xlabel("PDE"); ax.set_ylabel("Time per event [µs]")
        ax.set_ylim(0, max(f + h) * 1.18); ax.set_title(title); ax.grid(axis="x", alpha=0)
    from matplotlib.patches import Patch
    fig.legend(handles=[Patch(color=BLUE, label="runEvent()   hits + signal"), Patch(color=ORANGE, label="runEvent(false)   hits only")],
               loc="upper center", ncol=2, bbox_to_anchor=(0.5, 1.07), fontsize=10.5)
    save(fig, "benchmarks.png")


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
    ax.text(0.45, 5.05, "Hit simulation", fontsize=11.5, fontweight="bold", color=BLUE, va="top")
    ax.text(2.45, 5.05, "runEvent(false)", fontsize=10, color=GRAY, va="top", family="monospace")
    ax.text(11.05, 5.05, "results: hits(), debug()", fontsize=10, color=BLUE, va="top", ha="right", family="monospace")
    ax.add_patch(FancyBboxPatch((0.25, 0.3), 11.0, 1.95, boxstyle="round,pad=0.02,rounding_size=0.15", fc="#FDF5EE", ec="#F0CFB3", lw=1.2))
    ax.text(0.45, 2.05, "Signal generation", fontsize=11.5, fontweight="bold", color=ORANGE, va="top")
    ax.text(2.85, 2.05, "runEvent()  or the first call to signal()", fontsize=10, color=GRAY, va="top", family="monospace")
    ax.text(11.05, 2.05, "result: signal()", fontsize=10, color=ORANGE, va="top", ha="right", family="monospace")
    W, H, Y = 1.85, 1.1, 3.3
    xs = [0.5 + i * 2.1 for i in range(5)]
    items = ["Photons\nPhoton times\n(+ wavelengths)", "PDE\nkeeps or drops\neach photon", "Hit position\nuniform / circle /\ngaussian",
             "Dark counts,\ncrosstalk,\nafterpulses", "Cell recovery and\ngain variation set\nthe amplitudes"]
    items[0] = "Input\nphoton times\n(+ wavelengths)"
    for x, t in zip(xs, items):
        box(ax, x, Y, W, H, t, "white", "#9CB9DB", fs=9.5)
    for a_, b_ in zip(xs[:-1], xs[1:]):
        arrow(ax, (a_ + W + 0.02, Y + H / 2), (b_ - 0.02, Y + H / 2))
    y2, h2 = 0.55, 1.1
    sig = [(0.5, 2.9, "Electronic noise\nbaseline (SNR)"), (3.75, 3.3, "Pulse of every hit added:\ndirect or recursive\nconvolution"), (7.9, 2.9, "SiPMAnalogSignal:\nintegral, peak, ToA,\nToT, time of peak")]
    for x, w, t in sig:
        box(ax, x, y2, w, h2, t, "white", "#EBBB93", fs=9.5)
    arrow(ax, (3.42, y2 + h2 / 2), (3.73, y2 + h2 / 2)); arrow(ax, (7.07, y2 + h2 / 2), (7.88, y2 + h2 / 2))
    # elbow connector: hits -> signal generation
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
