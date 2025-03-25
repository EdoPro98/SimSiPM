import ROOT
import SiPM
import numpy as np
from tqdm import trange

ROOT.gStyle.SetLineScalePS(1)
ROOT.gStyle.SetOptFit(1)
ROOT.gStyle.SetOptStat(10)
ROOT.gStyle.SetOptTitle(0)

ROOT.gStyle.SetImageScaling(3.0)
ROOT.gROOT.SetBatch(True)


def signal(x, t, a, tr, tf):
    shape = np.exp(-(x - t) / tf) - np.exp(-(x - t) / tr)
    shape[x < t] = 0
    shape /= shape.max()
    return a * shape


prop = SiPM.SiPMProperties()
prop.setSampling(0.1)
prop.setXtOff()
prop.setDcrOff()
prop.setAp(0.2)
tr = prop.risingTime()
tf = prop.fallingTimeFast()
prop.setSignalLength(500)
t = np.arange(0, prop.signalLength(), prop.sampling())

sensor = SiPM.SiPMSensor(prop)

N = 1000
c1 = ROOT.TCanvas()
mg = ROOT.TMultiGraph()

waveform = np.zeros_like(t)

for i in trange(N):
    sensor.resetState()
    sensor.addPhoton(10)
    sensor.runEvent()

    for h in sensor.hits():
        if h.hitType() != SiPM.SiPMHit.HitType.kPhotoelectron:
            if h.parent().hitType() == SiPM.SiPMHit.HitType.kPhotoelectron:
                y = signal(t, h.time(), h.amplitude(), tr, tf)
                hitGraph = ROOT.TGraph(t.size, t, y)
                hitGraph.SetLineColor(ROOT.kGray+2)
                hitGraph.SetLineWidth(1)
                mg.Add(hitGraph, "c")

    waveform += np.array(sensor.signal().waveform())
waveform /= N

g = ROOT.TGraph(t.size, t, waveform)
g.SetLineWidth(5)
mg.Add(g, "l")

y = 1 - np.exp(-(t - 10) / prop.recoveryTime())
y[t < 10] = 0
gexp = ROOT.TGraph(t.size, t, y)
gexp.SetLineWidth(3)
gexp.SetLineColor(ROOT.kRed)
mg.Add(gexp, "c")

mg.Draw("a")
mg.GetXaxis().SetTitle("Time [ns]")
mg.GetYaxis().SetTitle("Amplitude")

legend = ROOT.TLegend(0.75, 0.8, 0.95, 0.95)
legend.AddEntry(g, "Average signal", "l")
legend.AddEntry(hitGraph, "Afterpulses", "l")
legend.AddEntry(gexp, "Cell Recovery", "l")
legend.Draw()

c1.Modified()
c1.Update()
c1.SaveAs("Afterpulses.pdf")
input()
