import SiPM
import numpy as np
import matplotlib.pyplot as plt
import mplhep
from scipy.optimize import curve_fit as fit

plt.style.use(mplhep.style.ATLAS)


def line(x, m, q):
    return x * m + q


def occupancy(npe, ncell):
    return npe * np.exp(-npe / ncell)


N = 10000
properties = SiPM.SiPMProperties()
properties.dumpSettings()
properties.setDcr(300e3)
properties.setXt(0.05)
sensor = SiPM.SiPMSensor(properties)
simulator = SiPM.SiPMSimulator(sensor)
intgate = 250
simulator.setIntegration(10, intgate, 0.0)
imgdir = "Results/"

print("************************")
print("***> Estimating DPP <***")
print("************************")
print()

events = [[20] * np.random.randint(5, 25) for i in range(1000)]

simulator.clear()
simulator.addEvents(events)
simulator.runSimulation()
results = np.array(simulator.getResults())
integral = np.array([x.Integral for x in results])
npe = np.array([x.Debug.nPhotoelectrons for x in results])
avg_integral = []
std_integral = []
avg_npe = []
for n in range(np.max(npe)):
    if np.any(npe == n):
        avg_npe.append(n)
        avg_integral.append(np.mean(integral[npe == n]))
        std_integral.append(np.std(integral[npe == n]) / integral[npe == n].size ** 0.5)

fit_dpp, cov_dpp = fit(line, avg_npe, avg_integral, sigma=np.array(std_integral), absolute_sigma=True)
print(f"Dpp = {fit_dpp[0]:.2f} +/- {cov_dpp[0,0]**0.5:.2f}")
DPP_INTEGRAL = fit_dpp[0]

plt.figure()
plt.title("Calibration of DPP")
plt.errorbar(avg_npe, avg_integral, std_integral, fmt=".k", capsize=2, label="Data")
plt.plot(avg_npe, line(np.array(avg_npe), *fit_dpp), "r", label="Fit")
plt.xlabel("Number of photoelectrons")
plt.ylabel("Integral [A.U.]")
plt.legend()
plt.savefig(imgdir + "dpp.png")

print("*****************************")
print("***> Start test on noise <***")
print("*****************************")
print()
print(f"***> Generating {N:d} empty events <***")
events = [[]] * N
simulator.clear()
simulator.addEvents(events)
simulator.runSimulation()
results = np.array(simulator.getResults())
integral = np.array([x.Integral for x in results])
ndcr = np.array([x.Debug.nDcr for x in results])
nxt = np.array([x.Debug.nXt for x in results])
pe = integral / DPP_INTEGRAL
threshold = np.arange(0, pe.max(), 0.01)
stair = np.empty_like(threshold)
for i, t in enumerate(threshold):
    stair[i] = np.count_nonzero(pe > t) / pe.size

dcr = 1e6 * ndcr.mean() / properties.signalLength()
dcr_sigma = 1e6 * ndcr.mean() ** 0.5 / ndcr.size / properties.signalLength()  # TEMP:
xt = nxt.sum() / ndcr.sum()
xt_sigma = ((ndcr.std() / ndcr.sum()) ** 2 + (nxt.std() / nxt.sum()) ** 2) ** 0.5  # TEMP:

print(f"DCR = {dcr:.2f} +/- {dcr_sigma*100:.2f}kHz")
print(f"Xt = {100*xt:.2f} +/- {100*xt_sigma:.2f}")

plt.figure()
plt.subplot(211)
plt.hist(pe, 300, color="k", label="Integral")
plt.xlim(-0.3, pe.max())
plt.yscale("log")
plt.xlabel("Number of photoelectrons")
plt.legend()
plt.subplot(212)
plt.plot(threshold, 1e6 * stair / intgate, ".k", ms=4, label="Staircase")
plt.hlines(properties.dcr() * 1e-3, 0, pe.max() / 3, "r", label="Expected DCR")
plt.hlines(properties.dcr() * properties.xt() * 1e-3, 0, 2 * pe.max() / 3, "r", label="Expected Xt")
plt.yscale("log")
plt.xlim(-0.3, pe.max())
plt.xlabel("Threshold [pe]")
plt.ylabel("Counts over threshold [kHz]")
plt.legend()
plt.savefig(imgdir + "noise.png")


print("*********************************")
print("***> Start test on linearity <***")
print("*********************************")
print()

events = []
for i in range(N):
    events.append([20] * np.random.randint(1, properties.nCells()))

simulator.clear()
simulator.addEvents(events)
simulator.runSimulation()
results = simulator.getResults()

integral = np.empty(len(results))
npe = np.empty_like(integral)
for i, r in enumerate(results):
    integral[i] = r.Integral / DPP_INTEGRAL
    npe[i] = r.Debug.nPhotons

fit_occ, cov_occ = fit(occupancy, npe, integral)
print(f"Ncell = {fit_occ[0]:.2f} +/- {cov_occ[0,0]**0.5:.2f}")

plt.figure()
plt.scatter(npe, integral, s=2, c="k")
plt.plot(np.sort(npe), occupancy(np.sort(npe), *fit_occ), "r")
plt.xlabel("Occupancy")
plt.ylabel("Relative response")
plt.show()
