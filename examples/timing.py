import ROOT
import SiPM
import time
import numpy as np

ROOT.gROOT.SetStyle("ATLAS")

def runSim(npe):
    N = 1024
    prop = SiPM.SiPMProperties()
    sensor = SiPM.SiPMSensor(prop)
    rng = SiPM.SiPMRandom()
    t = 0
    for i in range(N):
        sensor.resetState()
        x = rng.randGaussian(10,0.1,npe)
        sensor.addPhotons(x)
        t0 = time.process_time_ns()
        sensor.runEvent()
        t += time.process_time_ns() - t0
    t/=1e3
    return t/N

def runSims(npe):
    times = []
    for i in range(64):
        time.sleep(np.random.rand()*1e-3)
        t = runSim(n)
        times.append(t)
    return np.mean(times), np.std(times)


npe = np.arange(1,1024,32)
canvas = ROOT.TCanvas()
canvas.Divide(1,2)
p1 = canvas.cd(1)
graph1 = ROOT.TGraphErrors(npe.size)
graph1.Draw("AP")
graph1.GetXaxis().SetTitle("Number of photons")
graph1.GetYaxis().SetTitle("Time per event $[\\mu s]$")
p2 = canvas.cd(2)
p2.SetLogy()
graph2 = ROOT.TGraphErrors(npe.size)
graph2.Draw("AP")
graph2.GetXaxis().SetTitle("Number of photons")
for i,n in enumerate(npe):
    t,e = runSims(n)
    graph1.SetPoint(i,n,t)
    graph1.SetPointError(i,0,e)
    p1.Modified()
    graph2.SetPoint(i,n,t/n)
    graph2.SetPointError(i,0,e/n)
    p2.Modified()
    canvas.Update()

input()
