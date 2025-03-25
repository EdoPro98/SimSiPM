import ROOT

ROOT.gStyle.SetLineScalePS(1)
ROOT.gStyle.SetOptFit(1)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetOptTitle(0)
ROOT.gStyle.SetTextFont(42)
ROOT.gStyle.SetImageScaling(3.0)
ROOT.EnableImplicitMT()

import numpy as np
import SiPM

prop = SiPM.SiPMProperties()
prop.setSignalLength(10e3)
prop.setDcr(300e3)
prop.setXt(0.3)
prop.setAp(0.2)
prop.setTauApFastComponent(50)
prop.setTauApSlowComponent(50)
prop.setRiseTime(1)
prop.setFallTimeFast(100)
prop.setSampling(0.1)
prop.setRecoveryTime(50)

sensor = SiPM.SiPMSensor(prop)
time = np.arange(0, prop.signalLength(), prop.sampling())

c = ROOT.TCanvas()
mg = ROOT.TMultiGraph()
gr = ROOT.TGraph()
grDCR = ROOT.TGraph()
grAP = ROOT.TGraph()
grXT = ROOT.TGraph()

grDCR.SetMarkerStyle(ROOT.kFullCircle)
grDCR.SetMarkerColor(ROOT.kRed)
grAP.SetMarkerStyle(ROOT.kFullCircle)
grAP.SetMarkerColor(ROOT.kGreen)
grXT.SetMarkerStyle(ROOT.kFullCircle)
grXT.SetMarkerColor(ROOT.kBlue)


mg.GetXaxis().SetTitle("Time [ns]")
mg.GetYaxis().SetTitle("Amplitude [A.U.]")


for t in time:
    gr.AddPoint(t, 0)
mg.Add(gr, "AL")
mg.Add(grDCR, "AP")
mg.Add(grXT, "AP")
mg.Add(grAP, "AP")
mg.Draw("A")

while True:
    print("==========")
    sensor.resetState()
    sensor.runEvent()

    grDCR.Set(0)
    grXT.Set(0)
    grAP.Set(0)

    waveform = sensor.signal().waveform()
    if np.max(waveform) > 2.5:
        continue

    hits = sensor.hits()
    for h in hits:
        print(h)
        if h.hitType() == SiPM.SiPMHit.HitType.kDarkCount:
            grDCR.AddPoint(h.time(), 0)
        if h.hitType() == SiPM.SiPMHit.HitType.kOpticalCrosstalk:
            grXT.AddPoint(h.time(), 0)
        if h.hitType() == SiPM.SiPMHit.HitType.kSlowAfterPulse:
            grAP.AddPoint(h.time(), 0)
        if h.hitType() == SiPM.SiPMHit.HitType.kFastAfterPulse:
            grAP.AddPoint(h.time(), 0)

    waveform = sensor.signal().waveform()
    for i, w in enumerate(waveform):
        gr.SetPointY(i, w)
    mg.GetXaxis().SetRangeUser(0, prop.signalLength())

    c.Modified()
    c.Update()
    input()
