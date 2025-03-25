import SiPM
import ROOT
import numpy as np
from time import sleep

ROOT.gStyle.SetOptStat(0)

npe = 5
properties = SiPM.SiPMProperties()
properties.setSnr(20)

rng = SiPM.SiPMRandom()
sensor = SiPM.SiPMSensor(properties)
canvas = ROOT.TCanvas()
canvas.Divide(1, 2)

npt = properties.nSignalPoints()
siglen = properties.signalLength()

x = np.arange(0, siglen, properties.sampling())
graph = ROOT.TGraph(npt, x, np.zeros_like(x))
p1 = canvas.cd(1)
graph.GetXaxis().SetLimits(0, siglen)
graph.SetMaximum(5 * npe**0.5)
graph.SetMinimum(-0.5)
graph.Draw("AL")

p2 = canvas.cd(2)
hist = ROOT.TH2D("Signal", "Signal", npt, 0, siglen - 1, 256, -0.2, 3 * npe)
hist.Draw("COL2")
p2.SetLogz()

while True:
    sensor.resetState()
    npe = rng.randPoisson(5)
    if npe:
        t = rng.randGaussian(10, 0.1, npe)
        sensor.addPhotons(t)
    sensor.runEvent()
    signal = np.array(sensor.signal().waveform())
    for i, s in enumerate(signal):
        graph.SetPointY(i, s)
        hist.Fill(x[i], s)
    p1.Modified()
    p2.Modified()
    canvas.UpdateAsync()
    sleep(1e-3)
