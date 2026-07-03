import SiPM
import matplotlib.pyplot as plt
import mplhep

plt.style.use(mplhep.style.ATLAS)

sensor = SiPM.SiPMSensor()

sensor.setProperty("SignalLength", 500)
sensor.setProperty("Dcr", 500e3)
sensor.setProperty("Xt", 0.05)
sensor.setProperty("DXt", 0.5)
sensor.setProperty("Ap", 0.05)
sensor.setProperty("Sampling", 10)
sensor.setProperty("RecoveryTime", 10)
sensor.setProperty("Pitch", 10)

N = 1000000

dcrAmpl = []
dcrDel = []
xtAmpl = []
xtDel = []
dxtAmpl = []
dxtDel = []
fapAmpl = []
fapDel = []
sapAmpl = []
sapDel = []

for i in range(N):
    sensor.resetState()
    sensor.runEvent()

    hits = sensor.hits()
    for hit in hits:
        if hit.hitType() == SiPM.SiPMHit.HitType.kDarkCount:
            dcrAmpl.append(hit.amplitude())
            dcrDel.append(hit.time() + i * sensor.properties().signalLength())
        else:
            parent = hit.parent()
            if parent is None:
                continue
            # Only process hits whose parent is a root hit
            if parent.parent() is not None:
                continue
            cumulativeAmplitude = 0.0
            p = parent
            while p is not None:
                cumulativeAmplitude += p.amplitude()
                p = p.parent()
            match hit.hitType():
                case SiPM.SiPMHit.HitType.kOpticalCrosstalk:
                    xtAmpl.append(hit.amplitude() + cumulativeAmplitude)
                    xtDel.append(hit.time() + i * sensor.properties().signalLength())
                case SiPM.SiPMHit.HitType.kDelayedOpticalCrosstalk:
                    dxtAmpl.append(hit.amplitude())
                    dxtDel.append(hit.time() - parent.time())
                case SiPM.SiPMHit.HitType.kFastAfterPulse:
                    fapAmpl.append(hit.amplitude())
                    fapDel.append(hit.time() - parent.time())
                case SiPM.SiPMHit.HitType.kSlowAfterPulse:
                    sapAmpl.append(hit.amplitude())
                    sapDel.append(hit.time() - parent.time())


def diff_prepend(lst, prepend=0):
    if not lst:
        return []
    return [lst[0] - prepend] + [lst[i] - lst[i - 1] for i in range(1, len(lst))]


fig, ax = plt.subplots()
ax.scatter(diff_prepend(dcrDel), dcrAmpl, s=1, c="g", label="Dcr")
ax.scatter(diff_prepend(xtDel), xtAmpl, s=1, c="r", label="Xt")
ax.scatter(dxtDel, dxtAmpl, s=1, c="orange", label="DelayedXt")
ax.scatter(sapDel, sapAmpl, s=1, c="cyan", label="SlowAp")
ax.scatter(fapDel, fapAmpl, s=1, c="b", label="FastAp")
ax.set_xscale("log")
ax.set_xlabel("Delay [ns]")
ax.set_ylabel("Amplitude")
ax.legend(markerscale=10)
plt.show()
