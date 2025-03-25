import SiPM
import ROOT
from tqdm import trange
import numpy as np
from uncertainties import ufloat

ROOT.gStyle.SetLineScalePS(1)
ROOT.gStyle.SetOptFit(1)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetOptTitle(0)
ROOT.gStyle.SetTextFont(42)
ROOT.gROOT.SetBatch(True)

ROOT.gStyle.SetImageScaling(3.0)

ROOT.EnableImplicitMT()

fitBkg = ROOT.TF1("f1", "[p0]*x*exp(-[DCR]*x)", 0, 1e6)
fitSig = ROOT.TF1("f2", "[p1]*x*exp(-x/[#tau])", 0, 1e6)
fitTot = ROOT.TF1("f3", "[p0]*x*exp(-[DCR]*x)+[p1]*x*exp(-x/[#tau])", 0, 1e6)
fitRec = ROOT.TF1("f4", "[p0]*(1-exp(-x/[#tau_{r}]))+[y0]", 0, 1e6)
fitAp = ROOT.TF1("f5", "[p0]*x*(exp(-x/[#tau_{ap}]))", 0, 1e6)

prop = SiPM.SiPMProperties()
prop.setDcr(100e3)
prop.setSignalLength(5000)
prop.setAp(0.10)
prop.setXtOff()
prop.setTauApFastComponent(10)
prop.setTauApSlowComponent(10)
prop.setRecoveryTime(25)
sensor = SiPM.SiPMSensor(prop)

xbins = np.geomspace(0.1, 100 * prop.signalLength(), 512)
histDelay = ROOT.TH1D("h1", "Delays Slow", xbins.size - 1, xbins)
profDelayAmpl = ROOT.TProfile("p1", "Delay - Amplitude", 64, 0, 150, -0.2, 1.2)

N = 1000000
dcrTimes = []
for i in trange(N):
    sensor.resetState()
    sensor.runEvent()

    for h in sensor.hits():
        if h.parent() is None:
            dcrTimes.append(i * prop.signalLength() + h.time())
        else:
            amplitude = h.amplitude()
            delay = h.time() - h.parent().time()
            histDelay.Fill(delay)
            profDelayAmpl.Fill(delay, amplitude)

[histDelay.Fill(d) for d in np.diff(np.sort(dcrTimes), prepend=0)]

c1 = ROOT.TCanvas()
fitRec.SetParameter("#tau_{r}", prop.recoveryTime())
fitRec.SetParameter("p0", 1)
profDelayAmpl.Fit(fitRec, "E", "")
fitRec.SetLineWidth(3)
ROOT.gPad.Update()
statBox = profDelayAmpl.FindObject("stats")
statBox.SetX1NDC(0.65)
statBox.SetX2NDC(0.85)
statBox.SetY1NDC(0.15)
statBox.SetY2NDC(0.30)
profDelayAmpl.GetXaxis().SetTitle("Delay [ns]")
profDelayAmpl.GetYaxis().SetTitle("Amplitude")

c2 = ROOT.TCanvas()
c2.Divide(1, 2, 0, 0)
p1 = c2.cd(1)
p1.SetLogx()
p1.SetLogy()

fitBkg.SetParameter("p0", 0.30)
fitBkg.SetParameter("DCR", prop.dcr() * 1e-9)
fitSig.SetParameter("#tau", 10)
fitSig.SetParameter("p1", 30)
histDelay.Fit(fitSig, "LE0", "", 0.1, 50)
histDelay.Fit(fitBkg, "LE0", "", 100, 100 * prop.signalLength())
fitTot.SetParameter("p0", fitBkg.GetParameter("p0"))
fitTot.SetParameter("p1", fitSig.GetParameter("p1"))
fitTot.SetParameter("#tau", fitSig.GetParameter("#tau"))
fitTot.SetParameter("DCR", fitBkg.GetParameter("DCR"))
histDelay.Fit(fitTot, "LE0", "", 0.1, 100 * prop.signalLength())
histDelay.Draw("E")
fitBkg.SetLineColor(ROOT.kRed)
fitSig.SetLineColor(ROOT.kRed)
fitTot.SetLineColor(ROOT.kMagenta)
fitTot.SetLineWidth(5)
fitBkg.DrawF1(0.1, 100 * prop.signalLength(), "same")
fitSig.DrawF1(0.1, 100 * prop.signalLength(), "same")
fitTot.DrawF1(0.1, 100 * prop.signalLength(), "same")
ROOT.gPad.Update()
statBox = histDelay.FindObject("stats")
statBox.SetX1NDC(0.125)
statBox.SetX2NDC(0.325)
histDelay.GetXaxis().SetTitle("Delay [ns]")
legend = ROOT.TLegend(0.15, 0.05, 0.30, 0.25)
legend.AddEntry(histDelay, "Data", "ple")
legend.AddEntry(fitBkg, "Fit DCR", "l")
legend.AddEntry(fitSig, "Fit AP", "l")
legend.AddEntry(fitTot, "Fit Combined", "l")
legend.Draw()

c2.cd(2)
histRes = ROOT.TH1D("h2", "Residuals", xbins.size - 1, xbins)
N_AP = 0
for i in range(histDelay.GetNbinsX() + 1):
    x = histDelay.GetBinCenter(i)
    y0 = fitBkg.Eval(x)
    y = histDelay.GetBinContent(i)
    if x < 40:
        N_AP += y - y0
    histRes.SetBinContent(i, y - y0)
N_AP = ufloat(N_AP, N_AP**0.5)
histRes.Sumw2()
p2 = c2.cd(2)
p2.SetLogx()
fitAp.SetParameter("p0", 1)
fitAp.SetParameter("#tau_{ap}", 10)
histRes.Fit(fitAp, "E0", "", 0.5, 40)
fitAp.SetLineColor(ROOT.kRed)
fitAp.SetLineWidth(5)
histRes.Draw("E")
fitAp.DrawF1(0.5, 40, "same")
histRes.GetXaxis().SetTitle("Delay [ns]")

c1.Modified()
c2.Modified()
c1.Update()
c2.Update()

AP_RATIO = N_AP / histDelay.GetEntries()
print(f"Afterpulse: {AP_RATIO}")


ROOT.gStyle.SetOptFit(10)
c1.SaveAs("ApAmplitude.pdf")
ROOT.gStyle.SetOptFit(1)
c2.SaveAs("ApDelays.pdf")
input()
