import SiPM
import numpy as np
import matplotlib.pyplot as plt
import mplhep

plt.style.use(mplhep.style.ATLAS)

rng = SiPM.SiPMRandom()
N = 100000

apMcCountsFast = np.zeros(N)
apMcCountsSlow = np.zeros(N)
apMcDelayFast = []
apMcDelaySlow = []
apMcAmplitudeFast = []
apMcAmplitudeSlow = []

properties = SiPM.SiPMProperties()
properties.setAp(0.10)
properties.setTauApFastComponent(5)
properties.setTauApFastComponent(50)
sensor = SiPM.SiPMSensor()

for i in range(N):
    sensor.resetState()
    n = rng.randPoisson(100)
    if n:
        t = rng.randGaussian(10,0.1,n)
        sensor.addPhotons(t)
    sensor.runEvent()

    hits = sensor.hits()
    hitsGraph = sensor.hitsGraph()
    nFast = 0
    nSlow = 0
    for i,h in enumerate(hits):
        if h.hitType() == SiPM.SiPMHit.HitType.kFastAfterPulse:
            nFast += 1
            parent = hits[hitsGraph[i]]
            if hitsGraph[hitsGraph[i]] == -1:
                continue
            apMcDelayFast.append(h.time() - parent.time())
            apMcAmplitudeFast.append(h.amplitude())
        if h.hitType() == SiPM.SiPMHit.HitType.kSlowAfterPulse:
            nSlow += 1
            parent = hits[hitsGraph[i]]
            if hitsGraph[hitsGraph[i]] == -1:
                continue
            apMcDelaySlow.append(h.time() - parent.time())
            apMcAmplitudeSlow.append(h.amplitude())

    apMcCountsFast[i] = nFast
    apMcCountsSlow[i] = nSlow

plt.figure()
h1 = np.histogram(apMcDelayFast,100)
h2 = np.histogram(apMcDelaySlow,100)
mplhep.histplot(h1)
mplhep.histplot(h2)
plt.figure()
plt.scatter(apMcDelayFast, apMcAmplitudeFast,s=1)
plt.scatter(apMcDelaySlow, apMcAmplitudeSlow,s=1)
plt.show()
