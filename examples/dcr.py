import ROOT
import SiPM
import numpy as np
import ctypes
from uncertainties import ufloat
from uncertainties.umath import log
from tqdm import tqdm, trange

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetTextFont(42)
ROOT.gStyle.SetOptStat(10)
ROOT.gStyle.SetOptFit(111)
ROOT.gStyle.SetImageScaling(3.0)

borelFcn = ROOT.TF1(
    "fb", "[N]*TMath::Power([XT]*x,x-1)*exp(-[XT]*x)/TMath::Gamma(x+1)", 0.0, 20.0
)
gPoisFcn = ROOT.TF1(
    "fgp",
    "[#mu]*TMath::Power([#mu]+[XT]*x,x-1)*exp(-[#mu]-[XT]*x)/TMath::Gamma(x+1)",
    0.0,
    20.0,
)
poisFcn = ROOT.TF1("fp", "[N]*TMath::Poisson(x,[#mu])", 0.0, 20.0)
geomFcn = ROOT.TF1("fg", "[N]*TMath::Power([P],x-1)*(1-[P])", 0.0, 20.0)

N = 10_000_000
dcr = 1000e3
xt = 0.3

rng = SiPM.SiPMRandom()
prop = SiPM.SiPMProperties()
prop.setDcr(dcr)
prop.setXt(xt)
prop.setDXt(0)
prop.setApOff()
prop.setSignalLength(1e9 / dcr)
sensor = SiPM.SiPMSensor(prop)

xbins = np.arange(-0.5, 19.5, 1)
hist_mc = ROOT.TH1D("h1", "", xbins.size - 1, xbins)
hist_mc_light = ROOT.TH1D("h2", "", xbins.size - 1, xbins)

pbar = tqdm(total=N)
while hist_mc.GetEntries() < N:
    sensor.resetState()
    sensor.runEvent()
    nDcr = sensor.debug().nDcr
    if nDcr < 2:
        hist_mc.Fill(sensor.debug().nPhotoelectrons)
        pbar.update(1)
pbar.close()

prop.setDcrOff()
sensor = SiPM.SiPMSensor(prop)
for i in trange(N):
    sensor.resetState()
    n = rng.randPoisson(2)
    if n:
        sensor.addPhotons([10] * n)
    sensor.runEvent()
    hist_mc_light.Fill(sensor.debug().nPhotoelectrons)

N0 = ufloat(hist_mc.GetBinContent(1), hist_mc.GetBinError(1))
N1 = ufloat(hist_mc.GetBinContent(2), hist_mc.GetBinError(2))
nAbove0 = hist_mc.GetEntries() - N0
nAbove1 = hist_mc.GetEntries() - N0 - N1
xt_mc = nAbove1 / nAbove0
print(f"Xt from N2/N1: {xt_mc.n:.3f}+/-{xt_mc.s:.3f}")

hist_mc.Scale(1e9 / (hist_mc.GetEntries() * prop.signalLength()))
hist_mc_light.Scale(1 / N)

c1 = ROOT.TCanvas()
c2 = ROOT.TCanvas()
c1.SetLogy()
c2.SetLogy()

c1.cd()

mg = ROOT.TMultiGraph()
gborel = ROOT.TGraphErrors(hist_mc.GetNbinsX())
ggeom = ROOT.TGraphErrors(hist_mc.GetNbinsX())
gborel.SetMarkerColor(ROOT.kRed)
gborel.SetLineColor(ROOT.kRed)
ggeom.SetMarkerColor(ROOT.kGreen + 2)
ggeom.SetLineColor(ROOT.kGreen + 2)
ggeom.SetMarkerSize(1)
gborel.SetMarkerSize(1)
gborel.SetMarkerStyle(ROOT.kCircle)
ggeom.SetMarkerStyle(ROOT.kCircle)

borelFcn.SetLineColor(ROOT.kRed)
geomFcn.SetLineColor(ROOT.kGreen + 2)

hist_mc.Draw("e1")

borelFcn.SetParameter("N", 1e6 / 2)
borelFcn.SetParameter("XT", prop.xt())
geomFcn.SetParameter("N", 1e6 / 2)
geomFcn.SetParameter("P", prop.xt())
hist_mc.Fit(borelFcn, "0LMSE+", "", 1, 10)
for i in range(1, hist_mc.GetNbinsX()):
    x = hist_mc.GetBinCenter(i)
    gborel.SetPoint(i - 1, x, borelFcn.Eval(x))
ROOT.TVirtualFitter.GetFitter().GetConfidenceIntervals(gborel, 0.997)

hist_mc.Fit(geomFcn, "0LMSE+", "", 1, 2)
for i in range(1, hist_mc.GetNbinsX()):
    x = hist_mc.GetBinCenter(i)
    ggeom.SetPoint(i - 1, x, geomFcn.Eval(x))
ROOT.TVirtualFitter.GetFitter().GetConfidenceIntervals(ggeom, 0.997)

hist_mc.SetMarkerStyle(ROOT.kFullCircle)
hist_mc.SetLineColor(ROOT.kBlack)
mg.GetXaxis().SetTitle("Fired cells")
mg.GetYaxis().SetTitle("Count rate [Hz]")

borelFcn.DrawF1(1, xbins[-1], "c same")
geomFcn.DrawF1(1, xbins[-1], "c same")
mg.Add(gborel, "p")
mg.Add(ggeom, "p")
mg.Draw()
legend1 = ROOT.TLegend(0.15, 0.15, 0.35, 0.30)
legend1.AddEntry(hist_mc, "Simulation", "pe")
legend1.AddEntry(gborel, "Borel", "ple")
legend1.AddEntry(ggeom, "Geometric", "ple")
legend1.Draw()
hist_mc.SetMinimum(1e-2)
hist_mc.GetXaxis().SetTitle("Fired cells")
hist_mc.GetYaxis().SetTitle("Rate [Hz]")

c2.cd()

mg_light = ROOT.TMultiGraph()
ggpois = ROOT.TGraphErrors(hist_mc.GetNbinsX())
gpois = ROOT.TGraphErrors(hist_mc.GetNbinsX())
ggpois.SetMarkerColor(ROOT.kRed)
ggpois.SetLineColor(ROOT.kRed)
ggpois.SetMarkerSize(1)
ggpois.SetMarkerStyle(ROOT.kCircle)
gPoisFcn.SetLineColor(ROOT.kRed)
gpois.SetMarkerColor(ROOT.kBlue)
gpois.SetLineColor(ROOT.kBlue)
gpois.SetMarkerSize(1)
gpois.SetMarkerStyle(ROOT.kCircle)
poisFcn.SetLineColor(ROOT.kBlue)

gPoisFcn.SetParameter("#mu", hist_mc_light.GetMean())
gPoisFcn.SetParameter("XT", prop.xt())

hist_mc_light.Draw("e1")

hist_mc_light.Fit(gPoisFcn, "0MSE+", "", 0, 10)
for i in range(1, hist_mc_light.GetNbinsX()):
    x = hist_mc_light.GetBinCenter(i)
    ggpois.SetPoint(i - 1, x, gPoisFcn.Eval(x))
ROOT.TVirtualFitter.GetFitter().GetConfidenceIntervals(ggpois, 0.997)

poisFcn.SetParameter("N", 1)
poisFcn.SetParameter("#mu", gPoisFcn.GetParameter("#mu"))
hist_mc_light.Fit(poisFcn, "0MSE+", "", 0, 1)
for i in range(1, hist_mc_light.GetNbinsX()):
    x = hist_mc_light.GetBinCenter(i)
    gpois.SetPoint(i - 1, x, poisFcn.Eval(x))
ROOT.TVirtualFitter.GetFitter().GetConfidenceIntervals(gpois, 0.997)


hist_mc_light.SetMarkerStyle(ROOT.kFullCircle)
hist_mc_light.SetLineColor(ROOT.kBlack)

gPoisFcn.DrawF1(0, xbins[-1], "c same")
poisFcn.DrawF1(0, xbins[-1], "c same")
mg_light.Add(ggpois, "p")
mg_light.Add(gpois, "p")
mg_light.Draw()
legend2 = ROOT.TLegend(0.15, 0.15, 0.35, 0.30)
legend2.AddEntry(hist_mc_light, "Simulation", "pe")
legend2.AddEntry(ggpois, "Generalized Poisson", "ple")
legend2.AddEntry(gpois, "Pure Poisson", "ple")
legend2.Draw()
hist_mc_light.SetMinimum(1e-6)
hist_mc_light.GetXaxis().SetTitle("Fired cells")
hist_mc_light.GetYaxis().SetTitle("Probability")


c1.Update()
c2.Update()
c1.SaveAs("Crosstalk.pdf")
c2.SaveAs("Crosstalk2.pdf")
input()
