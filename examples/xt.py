import SiPM
import ROOT
import numpy as np
import matplotlib.pyplot as plt

INF = -0.1
SUP = 8.0
N = 2**16
prop = SiPM.SiPMProperties()
prop.setDcr(100e3)
prop.setXt(0.3)
sensor = SiPM.SiPMSensor(prop)
hist_peak = ROOT.TH1D('h2','Peak',512,INF,SUP)

peaks = np.zeros(N)
INT_TIME = prop.signalLength()
for i in range(N):
    sensor.resetState()
    sensor.runEvent()
    peaks[i] = sensor.signal().peak(0,INT_TIME,0.0)
    if peaks[i] == np.inf:
        print(sensor.signal().waveform())
        plt.plot(sensor.signal().waveform())
        plt.show()
    hist_peak.Fill(peaks[i])

c_stair = ROOT.TCanvas()
c_stair.SetLogy()

c_peak = ROOT.TCanvas()
c_peak.SetLogy()

c_stair.cd()
x = np.sort(peaks)
print(x.min(), x.max())
y = np.linspace(1,0,peaks.size, endpoint=False)
staircase = ROOT.TGraph(x.size,x,y)
staircase.Draw('ap')

NFIT = 7
lines = []
for i in range(NFIT):
    y = prop.dcr()*(prop.xt()**i)
    line = ROOT.TLine(INF,y,SUP,y)
    line.SetLineStyle(ROOT.kDashed)
    line.SetLineColor(ROOT.kRed)
    lines.append(line)
[line.Draw() for line in lines]

c_peak.cd()
hist_peak.Draw('e')
fits = []
stringFTot = ''
params = np.ones(3*NFIT)
for i in range(NFIT):
    func = ROOT.TF1(f'g{i}','gaus',1+i-0.5,1+i+0.5)
    hist_peak.Fit(func,'LE+ 0',"",1+i-0.5,1+i+0.5)
    func.GetParameters(params[3*i:3*i+3])
    func.SetLineColor(ROOT.kGreen)
    fits.append(func)
    stringFTot += f'gaus({3*i})+'
fTot = ROOT.TF1('gtot',stringFTot[:-1],INF,SUP)
fTot.SetLineColor(ROOT.kRed)

fTot.SetParameters(params)
hist_peak.Fit(fTot,'LE0','same',0.5,NFIT+0.5)

for func in fits:
    func.Draw('same')
fTot.Draw('same')

integrals = []
for i in range(NFIT):
    integral = fTot.Integral(1+i-0.5, 1+i+0.5)
    integrals.append(integral)

for i in range(1,NFIT):
    print(integrals[i]/integrals[i-1])

c_stair.Modified()
c_stair.Update()
c_peak.Modified()
c_peak.Update()
input()
