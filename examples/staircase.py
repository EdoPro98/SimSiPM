import SiPM
import numpy as np
import matplotlib.pyplot as plt
import mplhep
plt.style.use(mplhep.style.ATLAS)

sensor = SiPM.SiPMSensor()
sensor.setProperty("Dcr",500e3)
sensor.setProperty("Xt",0.05)

start = 0
end = 4
step = 0.025

ntest = 10
nwindows = 1000
window = 250
threshold = np.arange(start,end,step)
nstep = threshold.size

counts = np.zeros(nstep,dtype=np.uint32)
countserr = np.zeros_like(counts)

# Loop on threshold
for i in range(nstep):
    th = threshold[i]
    # Loop on repetition
    repcounts = np.zeros(ntest)
    for j in range(ntest):
        c = 0
        for _ in range(nwindows):
            sensor.resetState()
            sensor.runEvent()

            peak = sensor.signal().peak(0,window,0)

            if peak > th:
                c += 1
        repcounts[j] = c
    counts[i] = np.mean(repcounts)
    countserr[i] = np.std(repcounts)

rate = 1e6 * counts / window
raterr = 1e6 * countserr / window

fig,ax = plt.subplots()

ax.errorbar(threshold, rate, yerr=raterr, fmt=".k")
ax.set_yscale("log")
plt.show()

