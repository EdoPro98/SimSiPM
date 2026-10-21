import matplotlib.pyplot as plt
import mplhep
import numpy as np
from matplotlib.colors import LogNorm

import SiPM

plt.style.use(mplhep.style.ATLAS)

sensor = SiPM.SiPMSensor()
# Windows are laid end to end as one continuous dark-noise stream
sensor.setProperty("SignalLength", 1e6)  # 1 ms: hits near a window edge (~tau/window) are negligible
sensor.setProperty("Sampling", 10)  # the waveform is not used here
sensor.setProperty("Dcr", 200e3)
sensor.setProperty("Xt", 0.15)
sensor.setProperty("DXt", 0.3)
sensor.setProperty("Ap", 0.1)
sensor.setProperty("RecoveryTime", 50)

N = 500  # 0.5 s of dark noise
THRESHOLD = 0.25  # p.e.; like a real trigger, smaller pulses are not seen and do not restart the delay

HT = SiPM.SiPMHit.HitType
LABELS = {
    HT.kDarkCount: "Primary DCR",
    HT.kDelayedOpticalCrosstalk: "DeCT",
    HT.kFastAfterPulse: "Fast AP",
    HT.kSlowAfterPulse: "Slow AP",
}
COLORS = {"Primary DCR": "tab:blue", "DiCT": "gold", "DeCT": "tab:green", "Fast AP": "tab:red", "Slow AP": "tab:purple"}

window = sensor.properties().signalLength()
delays, amplitudes, kinds = [], [], []
last = None
for i in range(N):
    sensor.resetState()
    sensor.runEvent()
    hits = sorted(sensor.hits(), key=lambda h: h.time())
    j = 0
    while j < len(hits):
        # Hits at the same instant form one pulse: direct crosstalk fires with its parent
        k = j
        while k < len(hits) and hits[k].time() == hits[j].time():
            k += 1
        pulse = hits[j:k]
        j = k
        amplitude = sum(h.amplitude() for h in pulse)
        if amplitude < THRESHOLD:
            continue
        t = i * window + pulse[0].time()
        if last is not None:
            delays.append(t - last)
            amplitudes.append(amplitude)
            direct_xt = any(h.hitType() == HT.kOpticalCrosstalk for h in pulse)
            kinds.append("DiCT" if direct_xt else LABELS[pulse[0].hitType()])
        last = t

delays = np.array(delays)
amplitudes = np.array(amplitudes)
kinds = np.array(kinds)

fig, ax = plt.subplots(figsize=(10, 7))
for kind, color in COLORS.items():
    sel = kinds == kind
    ax.scatter(delays[sel], amplitudes[sel], s=1, c=color, label=f"{kind} ({sel.sum()})", rasterized=True)
ax.set_xscale("log")
ax.set_ylim(0, None)
ax.set_xlabel("Inter-arrival time [ns]")
ax.set_ylabel("Amplitude [p.e.]")
ax.legend(markerscale=10)
fig.savefig("delays_scatter.png", dpi=150)

xbins = np.logspace(np.log10(delays.min()), np.log10(delays.max()), 200)
ybins = np.linspace(0, amplitudes.max(), 200)
fig, ax = plt.subplots(figsize=(10, 7))
*_, image = ax.hist2d(delays, amplitudes, bins=[xbins, ybins], cmap="jet", norm=LogNorm())
fig.colorbar(image, ax=ax, label="Counts")
ax.set_xscale("log")
ax.set_xlabel("Inter-arrival time [ns]")
ax.set_ylabel("Amplitude [p.e.]")
fig.savefig("delays_hist2d.png", dpi=150)

plt.show()
