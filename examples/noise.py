import SiPM
import numpy as np
import matplotlib.pyplot as plt

rng = SiPM.SiPMRandom()

sensor = SiPM.SiPMSensor()

sensor.setProperty("SignalLength",5000)
sensor.setProperty("FallTimeFast",20)
sensor.setProperty("Dcr",5e6)
sensor.setProperty("Xt",0.5)
sensor.setProperty("DXt",0.5)
sensor.setProperty("Ap",0.5)
sensor.setProperty("Snr",40)
sensor.setProperty("Sampling",0.1)

sensor.runEvent()

signal = np.array(sensor.signal().waveform())
hits = sensor.hits()

x = np.arange(0,sensor.properties().signalLength(),sensor.properties().sampling())

fig,ax = plt.subplots()
ax.plot(x,signal,"k",lw=1)

for h in hits:
    if h.hitType() == SiPM.SiPMHit.HitType.kDarkCount:
        plt.plot(h.time(), h.amplitude(), "or", ms=10, label="Dcr")
    if h.hitType() == SiPM.SiPMHit.HitType.kPhotoelectron:
        plt.plot(h.time(), h.amplitude(), ".k", ms=10)
    if (h.hitType() == SiPM.SiPMHit.HitType.kDelayedOpticalCrosstalk) | (h.hitType() == SiPM.SiPMHit.HitType.kOpticalCrosstalk):
        plt.plot(h.time(), h.amplitude(), "vg", ms=10, label="Xt")
    if (h.hitType() == SiPM.SiPMHit.HitType.kFastAfterPulse) | (h.hitType() == SiPM.SiPMHit.HitType.kSlowAfterPulse):
        plt.plot(h.time(), h.amplitude(), "^b", ms=10, label="Ap")
ax.grid()
plt.legend()
plt.show()
