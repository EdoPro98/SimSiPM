import ROOT
import SiPM
import numpy as np
from tqdm import trange

rng = SiPM.SiPMRandom()

ROOT.gStyle.SetLineScalePS(1)
ROOT.gStyle.SetOptFit(10)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetOptTitle(0)
ROOT.gStyle.SetTextFont(42)
ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetImageScaling(3.0)

wlen = np.array([280,  300,  350,  370,  400,  450,  500,  550,  600,  650,  700,  750,  800,  850,  900],dtype=np.float32)
pde =  np.array([0.03, 0.11, 0.20, 0.21, 0.27, 0.32, 0.31, 0.27, 0.23, 0.18, 0.14, 0.11, 0.07, 0.05, 0.03],dtype=np.float32)


prop = SiPM.SiPMProperties()
prop.setPitch(5)
prop.setDcrOff()
prop.setXtOff()
prop.setSignalLength(300)
sensor = SiPM.SiPMSensor(prop)

pdeGraph = ROOT.TGraph(wlen.size, wlen, pde)

prof = ROOT.TProfile("p1", "Pde - Wlen", 64, 250, 1000, 0, 0.5, 's')
hist = ROOT.TH2D("h1", "Pde - Wlen", 128, 250, 1000, 128, 0, 0.4)

xbins = np.arange(-0.5, 25+0.5, 1)
profDpp = ROOT.TProfile("p2", "Dpp", xbins.size - 1, xbins, 0, 2000, "s")

for i in range(10000):
    sensor.resetState()
    npe = rng.randInteger(25)
    if npe:
        sensor.addPhotons([10] * npe)
    sensor.runEvent()

    peak = sensor.signal().peak(5, 250, 0.0)
    profDpp.Fill(npe, peak)

profDpp.Fit("pol1", "0E", "", 0, 25)
pede = profDpp.GetFunction('pol1').GetParameter(0)
dpp = profDpp.GetFunction('pol1').GetParameter(1)

N = 2**16
prop.setPdeSpectrum(wlen, pde)

sensor = SiPM.SiPMSensor(prop)
for i in trange(N):
    sensor.resetState()
    n = 1000
    if n == 0:
        continue
    t = [10] * n
    w = np.ones(n) * np.random.uniform(250, 1000)
    sensor.addPhotons(t, w)
    sensor.runEvent()

    npe = sensor.debug().nPhotoelectrons
    peak = sensor.signal().peak(5, 250, 0.0)
    peak = (peak - pede) / dpp
    prof.Fill(w[0], peak / n)
    hist.Fill(w[0], peak / n)

c1 = ROOT.TCanvas()
c1.SetLogz()
hist.Draw('col')
pdeGraph.Draw('l same')
prof.Draw('same e')
hist.GetXaxis().SetTitle('Wavelength [nm]')
hist.GetYaxis().SetTitle('PDE')
hist.GetXaxis().SetNdivisions(515)
pdeGraph.SetLineColor(ROOT.kBlack)
pdeGraph.SetLineWidth(10)
prof.SetLineColor(ROOT.kRed)
prof.SetLineWidth(5)
legend = ROOT.TLegend(0.65,0.65,0.85,0.85)
legend.AddEntry(pdeGraph, 'Input PDE')
legend.AddEntry(prof, 'Measured PDE','le')
legend.Draw()

c1.Modified()
c1.Update()

c1.SaveAs('pde.pdf')
input()
