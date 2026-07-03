import SiPM
import matplotlib.pyplot as plt

rng = SiPM.SiPMRandom()

prop10um = SiPM.SiPMProperties()
prop25um = SiPM.SiPMProperties()
prop50um = SiPM.SiPMProperties()

prop10um.setProperty("Pitch", 10)
prop25um.setProperty("Pitch", 25)
prop50um.setProperty("Pitch", 50)

sensor10um = SiPM.SiPMSensor(prop10um)
sensor25um = SiPM.SiPMSensor(prop25um)
sensor50um = SiPM.SiPMSensor(prop50um)

N = 1000

npe = [0] * N
int10um = [0.0] * N
int25um = [0.0] * N
int50um = [0.0] * N

for i in range(N):
    npe[i] = rng.randInteger(2000)
    sensor10um.resetState()
    sensor25um.resetState()
    sensor50um.resetState()

    if npe[i] > 0:
        t = rng.randGaussian(25.0, 0.01, npe[i])
        sensor10um.addPhotons(t)
        sensor25um.addPhotons(t)
        sensor50um.addPhotons(t)

    sensor10um.runEvent()
    sensor25um.runEvent()
    sensor50um.runEvent()

    int10um[i] = sensor10um.signal().integral(20, 250, 0.0)
    int25um[i] = sensor25um.signal().integral(20, 250, 0.0)
    int50um[i] = sensor50um.signal().integral(20, 250, 0.0)

filtered = [int10um[i] / npe[i] for i in range(N) if 0 < npe[i] < 100]
dpp = sum(filtered) / len(filtered)

npe_max = max(npe)
fig, ax = plt.subplots()
ax.scatter(npe, [v / dpp for v in int10um], s=1, c="r", label="10um")
ax.scatter(npe, [v / dpp for v in int25um], s=1, c="g", label="25um")
ax.scatter(npe, [v / dpp for v in int50um], s=1, c="b", label="50um")
plt.plot([0, npe_max], [0, npe_max], ":k")
plt.plot([1000, npe_max], [(1000.0 / 50.0)**2, (1000.0 / 50.0)**2], "b")
plt.text(1250, 1.1 * (1000 / 50)**2, "Expected saturation", fontsize="large", color="b")
plt.legend()
plt.xlabel("Number of photons as input")
plt.ylabel("Number of photons detected")
plt.show()
