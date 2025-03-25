import ROOT
import SiPM
import numpy as np
from tqdm import trange
from uncertainties import ufloat

rng = SiPM.SiPMRandom()

ROOT.gStyle.SetLineScalePS(1)
ROOT.gStyle.SetOptFit(10)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetOptTitle(0)
ROOT.gStyle.SetTextFont(42)
ROOT.gROOT.SetBatch(True)

ROOT.gStyle.SetImageScaling(3.0)

SIGLEN = 300
INTTIME = 250
MAX_NPE = 25
prop = SiPM.SiPMProperties()
prop.setPitch(10)
prop.setSignalLength(SIGLEN)
prop.setDcr(1000e3)
prop.setXt(0.3)
prop.setApOff()

xbins = np.arange(-0.5, MAX_NPE+0.5, 1)
prof = ROOT.TProfile("p1", "Dpp", xbins.size - 1, xbins, 0, MAX_NPE, "s")

prop.setDcrOff()
prop.setXtOff()
sensor = SiPM.SiPMSensor(prop)
for i in range(10000):
    sensor.resetState()
    npe = rng.randInteger(MAX_NPE)
    if npe:
        sensor.addPhotons([10] * npe)
    sensor.runEvent()

    peak = sensor.signal().peak(5, 50, 0.0)
    prof.Fill(npe, peak)

prof.Fit("pol1", "0E", "", 0, 25)
pede = prof.GetFunction('pol1').GetParameter(0)
dpp = prof.GetFunction('pol1').GetParameter(1)

prop.setDcrOn()
prop.setXtOn()
sensor = SiPM.SiPMSensor(prop)
N = 1024
N_REPEAT = 2**14

adc = np.zeros(N)
staircase = ROOT.TH1D("h1", "Staircase",N,-0.1,5)

for i in trange(staircase.GetNbinsX(), position=1):
    adc = np.zeros(N_REPEAT)
    n_over = 0
    for j in trange(N_REPEAT, leave=False, position=0):
        sensor.resetState()
        sensor.runEvent()
        adc[j] = sensor.signal().peak(SIGLEN - INTTIME, INTTIME, 0.0)

    adc = (adc - pede)/dpp
    threshold = staircase.GetBinCenter(i)
    n_over = np.sum(adc > threshold)
    staircase.SetBinContent(i, n_over)

staircase.Sumw2()
staircase.Scale(1 / (N_REPEAT * INTTIME * 1e-9))

c1 = ROOT.TCanvas()
c2 = ROOT.TCanvas()
c1.SetLogy()

c1.cd()
staircase.Draw()
staircase.GetXaxis().SetTitle('Threshold [pe]')
staircase.GetYaxis().SetTitle('Rate [Hz]')
staircase.SetMaximum(1e7)
rates = []
for i in range(4):
    x_center = i+0.5
    x_low = x_center-0.25
    x_hi = x_center+0.25
    bin_center = staircase.FindBin(x_center)
    bin_low = staircase.FindBin(x_low)
    bin_hi = staircase.FindBin(x_hi)
    binContent = np.zeros(bin_hi-bin_low)
    binError = np.zeros_like(binContent)
    for j, b in enumerate(range(bin_low, bin_hi)):
        binContent[j] = staircase.GetBinContent(b)
        binError[j] = staircase.GetBinError(b)
    bin_weights = 1/binError**2
    mean_integral = np.dot(binContent,bin_weights) / np.sum(bin_weights)
    err_integral = (np.dot(bin_weights,(binContent - mean_integral)**2)/np.sum(bin_weights))**0.5
    rate = ufloat(mean_integral,err_integral)
    rates.append(rate)
    line = ROOT.TLine(x_center,staircase.GetMinimum(), x_center, rate.n)
    line.SetLineWidth(5)
    line_low = ROOT.TLine(x_low,staircase.GetMinimum(), x_low, rate.n)
    line_hi = ROOT.TLine(x_hi,staircase.GetMinimum(), x_hi, rate.n)
    line_low.SetLineWidth(3)
    line_hi.SetLineWidth(3)
    line_low.SetLineStyle(ROOT.kDotted)
    line_hi.SetLineStyle(ROOT.kDotted)
    line.DrawClone('same')
    line_hi.DrawClone('same')
    line_low.DrawClone('same')
    line = ROOT.TLine(0,rate.n,x_center,rate.n)
    line.SetLineColor(ROOT.kRed)
    line.SetLineWidth(5)
    line.SetLineStyle(ROOT.kDashed)
    line.DrawClone('same')

xt1 = rates[1]/rates[0]
xt2 = rates[2]/rates[1]
xt3 = rates[3]/rates[2]
string0 = f'Dcr = {rates[0].n/1000:.2f}#pm{rates[0].s/1000:.2f} kHz'
string1 = f"#color[2]{{#frac{{f_{{1.5}}}}{{f_{{0.5}}}}={xt1.n:.2f}#pm{xt1.s:.2f}}}"
string2 = f"#frac{{f_{{2.5}}}}{{f_{{1.5}}}}={xt2.n:.2f}#pm{xt2.s:.2f}"
string3 = f"#frac{{f_{{3.5}}}}{{f_{{2.5}}}}={xt3.n:.2f}#pm{xt3.s:.2f}"
text0 = ROOT.TLatex(0.2,2e6,string0)
text1 = ROOT.TLatex(3.5,3e6,string1)
text2 = ROOT.TLatex(3.5,3e6/5,string2)
text3 = ROOT.TLatex(3.5,3e6/25,string3)
text0.Draw()
text1.Draw()
text2.Draw()
text3.Draw()


c2.cd()
prof.Draw('e1')
prof.GetFunction('pol1').DrawF1(0,25,'same')


c1.Modified()
c1.Update()
c1.SaveAs('Staircase.pdf')
input()
