import SiPM
import numpy as np
import matplotlib.pyplot as plt
import mplhep
from scipy.special import erf
from scipy.optimize import curve_fit as fit
from scipy.signal import savgol_filter
plt.style.use(mplhep.style.ATLAS)

dcr = 300
xt = 0.10
sensor = SiPM.SiPMSensor()
sensor.setProperty("Dcr",dcr*1000)
sensor.setProperty("Xt",xt)
sensor.setProperty("SignalLength",300)

def fitFun(x,
        a1,a2,a3,
        b1,b2,b3,
        mu1,mu2,mu3,
        s1,s2,s3):
    y1 = a1*(1-erf((x-mu1)/s1))/2 + b1
    y2 = a2*(1-erf((x-mu2)/s2))/2 + b2
    y3 = a3*(1-erf((x-mu3)/s3))/2 + b3
    return y1+y2+y3

start = 0
end = 4.0
step = 0.05
window = 250
N = 25000

threshold = np.arange(start,end,step)
nstep = threshold.size
counts = np.zeros((nstep,25))

for j in range(25):
    peaks = np.zeros(N)
    for i in range(N):
        sensor.resetState()
        sensor.runEvent()
        peaks[i] = sensor.signal().peak(0,250,0)

    for i,th in enumerate(threshold):
        counts[i,j] = np.count_nonzero(peaks > th)

countsErr = np.std(counts,axis=1)
counts = np.mean(counts,axis=1)
rate = 1e6 * counts / (window*N)
raterr = 1e6 * countsErr / (window*N)

par, cov = fit(fitFun, threshold[(threshold > 0.5) & (threshold < 4)], rate[(threshold > 0.5) & (threshold < 4)], p0=[
    dcr,dcr*xt,dcr*xt*xt,
    0,0,0,
    1.0,2.0,3.0,
    0.05,0.05,0.1])

fig,ax = plt.subplots()
ax.plot(threshold, rate, ".k")
diffRate = -savgol_filter(rate,5,2,1)
ax.plot(threshold, diffRate)
ax.fill_between(threshold, rate+raterr, rate-raterr, color="r",alpha=0.25)
ax.plot(np.linspace(0,4.0,1000), fitFun(np.linspace(0,4.0,1000),*par), "--g")
ax.set_yscale("log")
ax.hlines(dcr,0,1,'r')
ax.hlines(dcr*xt,1,2,'r')
ax.hlines(dcr*xt*xt,2,3,'r')
ax.hlines(dcr*xt*xt*xt,3,4,'r')
plt.show()

