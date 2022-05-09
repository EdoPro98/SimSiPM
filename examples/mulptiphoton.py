import SiPM
import numpy as np
import matplotlib.pyplot as plt

rng = SiPM.SiPMRandom()

sensor = SiPM.SiPMSensor()

N = 100000

integral = np.empty(N, dtype=np.float64)

for i in range(N):
    sensor.resetState()
    nphe = rng.randPoisson(5)
    if nphe > 0:
        times = rng.randGaussian(25,0.1,nphe)
        sensor.addPhotons(times)
    sensor.runEvent()

    integral[i] = sensor.signal().integral(20,250,0.0)

fig,ax = plt.subplots()
plt.hist(integral, 500, color="k")
plt.xlabel("Integral [A.U.]")
ax.set_yscale("log")
plt.show()

