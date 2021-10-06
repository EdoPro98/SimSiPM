import SiPM
from tabulate import tabulate
import matplotlib.pyplot as plt
import mplhep
import numpy as np
from iminuit.cost import ExtendedUnbinnedNLL
from iminuit import Minuit

plt.style.use(mplhep.style.ATLAS)


def exp(x, a, b):
    return a, a * np.exp(-x * b)


print("***********************************")
print("* MC truth qualification of noise *")
print("***********************************")

N = 1000000

properties = SiPM.SiPMProperties()
properties.setDXtOn()

sensor = SiPM.SiPMSensor(properties)

nDcr = 0
nXt = 0
nXtDel = 0
nAp = 0
nPe = 0
dcrDelays = []
dcrTimes = []
for i in range(N):
    sensor.resetState()
    sensor.runEvent()
    debug = sensor.debug()
    hits = sensor.hits()

    for hit in hits:
        if hit.hitType() == SiPM.SiPMHit.HitType.kDarkCount:
            dcrDelays.append(hit.time() + i * sensor.properties().signalLength())
            dcrTimes.append(hit.time())
    nPe += debug.nPhotoelectrons
    nDcr += debug.nDcr
    nXt += debug.nXt
    nXtDel += debug.nDXt
    nAp += debug.nAp

dcrDelays = np.diff(np.sort(dcrDelays)) * 1e-9

dcr = nDcr / (1e-9 * N * sensor.properties().signalLength())
xt = nXt / nPe
delXtFrac = nXtDel / (nXt + nXtDel)
ap = nAp / nPe

tableData = {
    "": ["MC", "Expected"],
    "Dcr [kHz]": [dcr / 1e3, sensor.properties().dcr() / 1e3],
    "Xt [%]": [100 * xt, 100 * sensor.properties().xt()],
    "Delayed Xt [%]": [100 * delXtFrac, 100 * sensor.properties().dxt()],
    "Ap [%]": [100 * ap, 100 * sensor.properties().ap()],
}

print(tabulate(tableData, headers="keys", floatfmt=".2f"))

cost = UnbinnedNLL(dcrDelays, exp)
fit = Minuit(cost, a=1/np.mean(dcrDelays))
fit.migrad()
fit.minos()

print(fit)

fig, ax = plt.subplots(2, 1)
h = np.histogram(dcrTimes, 300)
mplhep.histplot(h, label="Time distribution of DCR", ax=ax[0])
ax[0].set_xlabel("Time [ns]")
ax[0].legend(frameon=False)

h = np.histogram(dcrDelays, 300, density=True)
mplhep.histplot(h, label="Inter-arriving time distribution of DCR", ax=ax[1])
x=np.linspace(0,dcrDelays.max(),100)
ax[1].plot(x,exp(x,*fit.values), label="Exponential fit")
ax[1].set_xlabel("Time [s]")
ax[1].legend(frameon=False)
plt.show()

tableData = {
    "": ["MC", "Expected"],
    "Avg Dcr time [ns]": np.mean(dcrTimes),
    "Avg Dcr delay [ns]": np.mean(dcrDelays),
    "Fit Dcr delay [ns]": 0,
}
