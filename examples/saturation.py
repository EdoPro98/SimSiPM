import SiPM
import numpy as np
import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetPadRightMargin(0.05)
ROOT.gStyle.SetPadTopMargin(0.05)
ROOT.gStyle.SetPadBottomMargin(0.1)
ROOT.gStyle.SetPadLeftMargin(0.1)

ROOT.gStyle.SetMarkerSize(0.2)

rng = SiPM.SiPMRandom()

prop10um = SiPM.SiPMProperties()
prop25um = SiPM.SiPMProperties()
prop50um = SiPM.SiPMProperties()

prop10um.setDcrOff()
prop25um.setDcrOff()
prop50um.setDcrOff()
prop10um.setXtOff()
prop25um.setXtOff()
prop50um.setXtOff()

prop10um.setProperty("Pitch", 25)
prop25um.setProperty("Pitch", 50)
prop50um.setProperty("Pitch", 100)

prop10um.setSignalLength(200)
prop25um.setSignalLength(200)
prop50um.setSignalLength(200)

sensor10um = SiPM.SiPMSensor(prop10um)
sensor25um = SiPM.SiPMSensor(prop25um)
sensor50um = SiPM.SiPMSensor(prop50um)

ncell10um = prop10um.nCells()
ncell25um = prop25um.nCells()
ncell50um = prop50um.nCells()

MAX_NPE = 5000
N = 10_000

npe = np.zeros(N, dtype=np.int32)
int10um = np.zeros(N, dtype=np.float64)
int25um = np.zeros(N, dtype=np.float64)
int50um = np.zeros(N, dtype=np.float64)

for i in range(N):
    if i < N / 4:
        npe[i] = rng.randInteger(100)
    else:
        npe[i] = rng.randInteger(MAX_NPE)
    sensor10um.resetState()
    sensor25um.resetState()
    sensor50um.resetState()

    if npe[i] > 0:
        t = np.array(rng.randExponential(1, npe[i])) + 25
        sensor10um.addPhotons(t)
        sensor25um.addPhotons(t)
        sensor50um.addPhotons(t)

    sensor10um.runEvent()
    sensor25um.runEvent()
    sensor50um.runEvent()

    int10um[i] = sensor10um.signal().peak(20, 100, 0.0)
    int25um[i] = sensor25um.signal().peak(20, 100, 0.0)
    int50um[i] = sensor50um.signal().peak(20, 100, 0.0)

npe = np.float64(npe)

fitFun = ROOT.TF1("saturation", "[Ncell]*(1-exp(-([k]*x/[Ncell])))+[y0]", 0, MAX_NPE)
fitFun.SetParameter("k", 1)
fitFun.SetParameter("y0", 0)

canvas = ROOT.TCanvas()
mg = ROOT.TMultiGraph()

xbins = np.geomspace(1, MAX_NPE, 65)

prof1 = ROOT.TProfile("prof1", "10um", xbins.size - 1, xbins, "s")
prof2 = ROOT.TProfile("prof2", "25um", xbins.size - 1, xbins, "s")
prof3 = ROOT.TProfile("prof3", "50um", xbins.size - 1, xbins, "s")
[prof1.Fill(x, y) for x, y in zip(npe, int10um)]
[prof2.Fill(x, y) for x, y in zip(npe, int25um)]
[prof3.Fill(x, y) for x, y in zip(npe, int50um)]
prof1.Approximate()
prof2.Approximate()
prof3.Approximate()

fitFun.SetParameter(0, ncell10um)
prof1.Fit(fitFun, "LEG0", "", 0, 2 * ncell10um)

fitFun.SetParameter(0, ncell25um)
prof2.Fit(fitFun, "LEG0", "", 0, 2 * ncell25um)

fitFun.SetParameter(0, ncell50um)
prof3.Fit(fitFun, "LEG0", "", 0, 2 * ncell50um)

g1 = ROOT.TGraph(N, npe, int10um)
mg.Add(g1, "P")
g2 = ROOT.TGraph(N, npe, int25um)
mg.Add(g2, "P")
g3 = ROOT.TGraph(N, npe, int50um)
mg.Add(g3, "P")

g1.SetMarkerColor(ROOT.kOrange + 1)
g1.SetMarkerStyle(ROOT.kFullCircle)
g2.SetMarkerColor(ROOT.kGreen + 1)
g2.SetMarkerStyle(ROOT.kFullCircle)
g3.SetMarkerColor(ROOT.kCyan + 2)
g3.SetMarkerStyle(ROOT.kFullCircle)

prof1.SetLineColor(ROOT.kBlack)
prof2.SetLineColor(ROOT.kBlack)
prof3.SetLineColor(ROOT.kBlack)

mg.Draw("ap")

prof1.Draw("e same")
prof2.Draw("e same")
prof3.Draw("e same")

line1 = prof1.GetFunction("saturation")
line2 = prof2.GetFunction("saturation")
line3 = prof3.GetFunction("saturation")
line1.DrawF1(0, MAX_NPE, "L SAME")
line2.DrawF1(0, MAX_NPE, "L SAME")
line3.DrawF1(0, MAX_NPE, "L SAME")

fitFun.SetLineStyle(2)
fitFun.SetLineColor(ROOT.kBlack)
fitFun.FixParameter(0, ncell10um)
fitFun.DrawCopy("L SAME")
fitFun.FixParameter(0, ncell25um)
fitFun.DrawCopy("L SAME")
fitFun.FixParameter(0, ncell50um)
fitFun.DrawCopy("L SAME")

mg.GetXaxis().SetTitle("N_{#gamma}")
mg.GetYaxis().SetTitle("N_{G}")

legend = ROOT.TLegend(0.15, 0.9, 0.30, 0.75)
legend.AddEntry(g1, "Data 25#mum", "p")
legend.AddEntry(g2, "Data 50#mum", "p")
legend.AddEntry(g3, "Data 100#mum", "p")
legend.AddEntry(fitFun, "Ideal Equation", "l")
legend.AddEntry(line1, "Fit", "l")
legend.Draw()

canvas.Modified()
canvas.Update()
canvas.SaveAs("saturation.pdf")
input()
