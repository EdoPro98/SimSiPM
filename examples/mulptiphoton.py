import SiPM
import numpy as np
import matplotlib.pyplot as plt
import mplhep

plt.style.use(mplhep.style.ATLAS)

rng = SiPM.SiPMRandom()

sensor = SiPM.SiPMSensor()
sensor.setProperty("Snr",20)
sensor.setProperty("Ccgv",0.05)

N = 100000

integral = np.empty(N, dtype=np.float64)

for i in range(N):
    sensor.resetState()
    nphe = rng.randPoisson(3)
    if nphe > 0:
        times = rng.randGaussian(25,0.1,nphe)
        sensor.addPhotons(times)
    sensor.runEvent()

    integral[i] = sensor.signal().integral(20,250,0.0)

h = np.histogram(integral, 500)
fig,ax = plt.subplots()
mplhep.histplot(h,ax=ax, color="k")
ax.set_yscale("log")
plt.show()
