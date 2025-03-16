import SiPM
import numpy as np
import matplotlib.pyplot as plt
import mplhep

rng = SiPM.SiPMRandom()
prop = SiPM.SiPMProperties()
prop.setSampling(1)
sensor = SiPM.SiPMSensor(prop)

N = 50000

integral = np.empty(N, dtype=np.float64)

for i in range(N):
    sensor.resetState()
    nphe = rng.randPoisson(10)
    if nphe > 0:
        times = rng.randGaussian(25,0.1,nphe)
        sensor.addPhotons(times)
    sensor.runEvent()
    integral[i] = sensor.signal().integral(20,250,0.0)

integral[integral == np.inf] = 0 

fig,ax = plt.subplots()
plt.hist(integral,int(2*N**0.5), color="k")
plt.xlabel("Integral [A.U.]")
plt.show()

